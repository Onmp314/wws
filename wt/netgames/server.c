/*
 * A networked two player game interface, server version.
 *  - Implements game server <-> client message sending.
 *  - Takes care of changing turns.
 *
 * Implementation:
 * - the parent listens on Server.socket and when there's a connection
 *   (the child if server is forking one) listens on Server.client socket.
 *
 * Differencies to client version(s):
 * - Replaced show_info() with DEBUG_PRINT (no GUI...).
 * - Multiple clients if forking is supported by the OS and 'FORK' defined.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "server.h"		/* standard includes + messages */


#define VERSION	 "v0.8"

#ifdef DEBUG
#define SHOW_TURN(x)	fprintf(stderr, "move: %03d\n", x)
#else
#define SHOW_TURN(x)	(x)	/* for inc/decrements */
#endif

static GAME Game;		/* Game information */
static struct
{
  fd_set rfd;
  int  timeout;			/* minutes to think a move */
  int  socket;			/* game server socket */
  int  client;			/* socket for the opponent */
  int  toread;			/* bytes of message yet to arrive */
  int sending;			/* sent message not yet responded to */
  int handshake;		/* handshake in process */
  long alignement;
  uchar msg[MSG_SIZE];		/* buffer for the message (long aligned!) */
} Server;


/* function prototypes. returns 0 if ok */
static int check_events(int poll);
static void comp_move(void);
static int setup_server(void);
static void server_cb(void);
static void client_cb(void);
static const char *process_msg(short type, uchar x, uchar y);


#ifdef FORK
#if !defined(POSIX) && !defined(SYSV) && !defined(linux)
/* remove zombie children */
static void sigchild(int signal)
{
#ifdef SIGTSTP
#ifdef __MINT__
  union wait status;
  while (waitpid(-1, (__WP)&status, WNOHANG) > 0);
#else
  int status;
  while (waitpid(-1, &status, WNOHANG) > 0);
#endif
#else
  /* BSD: doesn't have SIGTSTP signal nor waitpid() */
#error "use wait3() instead of posixy waitpid()"
#endif
}
#endif
#endif

static void mysignal(int sig, void (*routine)(int))
{
#if defined(__MINT__) || defined(BSD)
  signal(sig, routine);
#else
  static struct sigaction handler;  /* too lazy to zero it... */
  handler.sa_handler = routine;
  sigaction(sig, &handler, NULL);
#endif
}


int main(int argc, char *argv[])
{
  get_configuration(&Game);

  if(argc > 1)
  {
    if(Game.args)
    {
      if(!Game.args(*argv, argc-1, argv+1))
	return -1;
    }
    else
    {
      fprintf(stderr, "Game server framework " VERSION "\n");
      fprintf(stderr, "Networking (w) 1996 by Eero Tamminen\n");
      return -1;
    }
  }

#ifdef FORK
  /* Disassociate from the process group (so that this won't be susceptiple
   * to signals sent to the entire process group) by forming an own process
   * group.  System V style...
   */
  setpgrp();

#if defined(POSIX) || defined(SYSV) || defined(linux)
  /* Posix */
  mysignal(SIGCHLD, SIG_IGN);
#else
  /* BSDish OS needs signal handler for getting rid of zombie children. */
  mysignal(SIGCHLD, sigchild);
#endif

  /* get onto background */
  if(fork())
    exit(0);
#endif

#ifdef SIGTTOU
  /* Allow background (error msg) writes to control (foreground) terminal
   * by ignoring terminal I/O signals.
   */
  mysignal(SIGTTOU, SIG_IGN);
#endif

  /* Don't 'hog' current directory. */
  chdir("/");

  /* setup server socket */
  if(!setup_server())
    return -1;

  for(;;)
    check_events(False);

  return 0;
}

/* return True if there was a client event <- poll should be used only to
 * clear of buffered events.
 */
static int check_events(int poll)
{
  struct timeval tv;
  fd_set rfd;

  FD_ZERO(&rfd);
  if(Server.socket)
    FD_SET(Server.socket, &rfd);
  if(Server.client)
    FD_SET(Server.client, &rfd);

  /* wait for next event */
  if(poll)
  {
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    select(FD_SETSIZE, &rfd, NULL, NULL, &tv);
  }
  else
    select(FD_SETSIZE, &rfd, NULL, NULL, NULL);

  /* connected / child */
  if(Server.client && FD_ISSET(Server.client, &rfd))
  {
    client_cb();
    return True;
  }
  /* connection / parent */
  if(Server.socket && FD_ISSET(Server.socket, &rfd))
    server_cb();
  return False;
}

static void comp_move(void)
{
  int turn;

  turn = Game.turn;

  /* compute next move */
  Game.comp();

  do
  {
    /* process all the events that were pending */
    while(check_events(True))
    {
      while(Server.toread)
        check_events(False);
    }
    /* undo/start/resign? */
    if(!(Game.playing && turn == Game.turn))
      return;

    /* make move. other moves? */
    if(!Game.move())
      return;

    /* wait for return code */
    do {
      check_events(False);
    } while(Server.toread);
  }
  /* check in case received opponent's undo/start/resign instead... */
  while(Game.playing && turn == Game.turn);
}

static void disconnect_client(const char *info)
{
#ifdef FORK
  /* remove socket and handler process */
  if(Server.client)
    shutdown(Server.client, 2);
  exit(0);
#else
  if(Server.client)
  {
    long val;

    /* server needs to be rebindable to the same port */
    setsockopt(Server.client, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    close(Server.client);
  }
  Server.sending = 0;
  Server.client = 0;
  Server.toread = 0;
  if(!setup_server())
    exit(0);

  DEBUG_PRINT(info);
#endif
}

static int setup_server(void)
{
  int sock;
  struct sockaddr_in addr;

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_INET socket.\n");
    return False;
  }

#ifndef FORK
  {
    long val;
    /* server needs to be rebindable to the same port */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  }
#endif

  /* where we are listening */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(GAME_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;
  if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    /* socket already in use, client stuff should still work though */
    fprintf(stderr, "Game server port already bound.\n");
    close(sock);
    return False;
  }

  if(listen(sock, 2) < 0)
  {
    fprintf(stderr, "unable to listen on server socket.\n");
    close(sock);
    return False;
  }
  Server.socket = sock;

  DEBUG_PRINT("Waiting connection...");
  return True;
}

static void server_cb(void)
{
  int client;
  socklen_t addrlen;
  struct sockaddr_in addr;
  long id;

  /* create a connection to client
   *
   * Seems that this has to be done before forking because on
   * linux doing it after fork() resulted in about 100 children!
   *
   * I'm guessing that after fork, parent returned to event loop
   * before accept here had a chance to clear server socket...
   */
  if((client = accept(Server.socket, (struct sockaddr *)&addr, &addrlen)) < 0)
  {
    fprintf(stderr, "client connection accept failed.\n");
    return;
  }

#ifdef FORK
  {
    int child;

    /* branch server and game client handler */
    if((child = fork()))
    {
      /* parent */
      if(child < 0)
	fprintf(stderr, "Client connection forking failed.\n");

      DEBUG_PRINT("forked another instance to deal with a client...");
      return;
    }
  }
#ifdef SIGTTOU
  mysignal(SIGTTOU, SIG_IGN);
#endif
#endif	/* FORK */

  /* remove so that other servers can use the port too. */
  close(Server.socket);
  Server.socket = 0;
  Server.client = client;

  /* send handshake */
  id = htonl(Game.game_id);
  write(client, &id, MSG_SIZE);
  Server.handshake = True;

  DEBUG_PRINT("Connected.");

  /* initialize, if not yet done */
  if(!Game.playing && Game.side)
  {
    Game.player = Player1;
    Game.side(Player1);
  }
}

static void client_cb(void)
{
  short type;
  const char *err;
  int len;

  if(!Server.toread)
    Server.toread = MSG_SIZE;

  /* try to read rest of the message */
  len = read(Server.client, &Server.msg[MSG_SIZE-Server.toread], Server.toread);
  if(len <= 0)
  {
    disconnect_client("Message receive failed!");
    return;
  }
  Server.toread -= len;
  /* need to wait for more? */
  if(Server.toread)
    return;

  if(Server.handshake)
  {
    /* end handshake */
    Server.handshake = False;
    if(ntohl(*(long*)Server.msg) != Game.game_id)
      disconnect_client("Handshake failed.");

    return;
  }

  type = ntohs(*(short*)Server.msg);

#ifdef DEBUG
  fprintf(stderr, "message %x (%d, %d)\n", type, Server.msg[2], Server.msg[3]);
#endif

  if(type < RETURN_CODE && Server.sending)
  {
    /* block other end's messages until mine are acknowledged */
    send_msg(RETURN_FAIL, 0, 0);
    return;
  }
  Server.sending = False;

  if((err = process_msg(type, Server.msg[2], Server.msg[3])))
    disconnect_client(err);
}

/*
 * To overcome possible network latency etc. troubles, all actions
 * go first to the other end and are done here only when the other
 * end returns the action. If unable to do what requested, return
 * caller RETURN_FAIL message. Other end's messages are blocked
 * until this end's one is answered.
 */
static const char *process_msg(short type, uchar x, uchar y)
{
  switch(type)
  {
    /* Game start/continue/quit.
     * As servers don't start games, start returns can be ignored.
     */
    case GAME_START:
    case GAME_CONT:
      if(x == Player0)
        Game.player = Player1;
      else
        Game.player = Player0;
      if(type == GAME_START)
      {
        send_msg(RETURN_START, x, y);
        if(Game.side)
	  Game.side(Game.player);
	Game.start();
      }
      else
      {
        if(Game.cont)
	{
	  if(Game.cont(Game.player))
            send_msg(RETURN_CONT, x, y);
	  else
	  {
	    send_msg(RETURN_FAIL, 0, 0);
	    return NULL;
	  }
	}
	else
	{
	  send_msg(RETURN_ILLEGAL, 0, 0);
	  return NULL;
	}
      }
      Game.turn = 1;
      Game.playing = True;
      if(Game.player == Player1)
      {
        Game.my_move = False;
        DEBUG_PRINT("Game started.");
      }
      else 
      {
        Game.my_move = True;
        DEBUG_PRINT("Your turn.");
        comp_move();
      }
      break;


    case GAME_RESIGN:
      send_msg(RETURN_RESIGN, 0, 0);
      DEBUG_PRINT("Opponent resigned.");
      Game.my_move = Game.playing = False;
      if(Game.over)
        Game.over();
      break;

    case RETURN_RESIGN:
      DEBUG_PRINT("Resigned.");
      Game.my_move = Game.playing = False;
      if(Game.over)
        Game.over();
      break;


    /* turn change */

    case MOVE_NEXT:
      send_msg(RETURN_NEXT, 0, 0);
      DEBUG_PRINT("Your turn.");
      SHOW_TURN(++Game.turn);
      Game.my_move = True;
      comp_move();
      break;

    case RETURN_NEXT:
      DEBUG_PRINT("Opponent's turn.");
      SHOW_TURN(++Game.turn);
      Game.my_move = False;
      break;

    case MOVE_PASS:
      if(Game.pass)
      {
        player_t player = (Game.player == Player0 ? Player1 : Player0);
	if(Game.pass(player))
	{
	  send_msg(RETURN_PASS, 0, 0);
	  DEBUG_PRINT("Opponent passed.");
	  SHOW_TURN(++Game.turn);
	  Game.my_move = True;
          comp_move();
	}
	else
	  send_msg(RETURN_FAIL, 0, 0);
      }
      else
	send_msg(RETURN_ILLEGAL, 0, 0);
      break;

    case RETURN_PASS:
      if(Game.pass(Game.player))
      {
	DEBUG_PRINT("Pass.");
	SHOW_TURN(++Game.turn);
	Game.my_move = False;
      }
      else
        return "Incompatible 'pass' operations.";
      break;


    case MOVE_UNDO:
      if(Game.undo && (Game.my_move || !Game.playing))
      {
	if(Game.undo())
	{
	  send_msg(RETURN_UNDO, 0, 0);
	  DEBUG_PRINT("Undo by opponent.");
	  SHOW_TURN(--Game.turn);
	  Game.my_move = False;
	}
	else
	  send_msg(RETURN_FAIL, 0, 0);
      }
      else
        send_msg(RETURN_ILLEGAL, 0, 0);
      break;

    case RETURN_UNDO:
      if(Game.undo())		/* checked when roundtrip msg sent */
      {
	SHOW_TURN(--Game.turn);
	DEBUG_PRINT("Your turn.");
	Game.my_move = True;
      }
      else
        return "Incompatible undo operations.";
      break;


    case GAME_LEVEL:
      if(Game.level)
      {
        if(Game.level(x))
          send_msg(RETURN_LEVEL, 0, 0);
        else
          send_msg(RETURN_FAIL, 0, 0);
      }
      else
        send_msg(RETURN_ILLEGAL, 0, 0);
      break;


    case RETURN_ILLEGAL:
      /* other ends doesn't implement the requested feature */
      DEBUG_PRINT("Illegal message.");
      break;

    case RETURN_FAIL:
      /* failed operation (may succeed later) or illegal message argument */
      DEBUG_PRINT("Failed.");
      break;

    default:
      /* engine message? */
      if(!Game.message(type, x, y))
      {
        send_msg(RETURN_ILLEGAL, 0, 0);
        return "Incompatible game messages!";
      }
  }
  return NULL;
}

void send_msg(short type, uchar x, uchar y)
{
  uchar msg[MSG_SIZE];

  if(!Server.client)
  {
    DEBUG_PRINT("Not connected.");
    return;
  }

  if(type < RETURN_CODE)
  {
    /* previous message not yet acknowledged? */
    if(Server.sending)
      return;
    Server.sending = True;
    DEBUG_PRINT("Sending message...");
  }

  /* compose message */
  *(short*)msg = htons(type);
  msg[2] = x;
  msg[3] = y;
  if(write(Server.client, &msg[0], sizeof(msg)) != sizeof(msg))
    disconnect_client("Message send failed!");

#ifdef DEBUG
  fprintf(stderr, "message %x (%d, %d)\n", type, msg[2], msg[3]);
#endif
}

void game_over(void)
{
  Game.my_move = Game.playing = False;
}
