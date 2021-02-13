/*
 * A networked two player game socket interface.
 *  - Implements game server / client and game message (which all
 *    do a round trip ie. need confirmation by other end) sending.
 *  - Takes care of changing turns.
 *  - Uses BSD sockets.
 *
 * TODO:
 * - Timing and timeouts.
 * - some kind of crude bulk transfer (with engine specified the lenght)
 *   for loaded boards etc.
 *
 * (w) 1996 by Eero Tamminen
 */

/* version: 0.8 */
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "comms.h"

SERVER Server;

static const char *process_msg(short type, uchar x, uchar y);


void mysignal(int sig, void (*routine)(int))
{
#if defined(__MINT__) || defined(BSD)
  signal(sig, routine);
#else
  static struct sigaction handler;  /* too lazy to zero it... */
  handler.sa_handler = routine;
  sigaction(sig, &handler, NULL);
#endif
}


int connect_server(const char *string)
{
  int sock;
  struct hostent *host;
  struct sockaddr_in addr;
  long id;

  if(Server.client)
  {
    show_info("Already connected.");
    return False;
  }

  if(string == NULL || string[0] == 0)
  {
    /* can't connect to myself */
    if(Server.socket)
      return False;
    string = "localhost";
  }

  /* resolv host address */
  if(!(host = gethostbyname(string)))
  {
    show_info("Unresolvable address.");
    return False;
  }

  /* create non-local stream socket */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_INET socket.\n");
    return False;
  }

  /* connect specified address */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(GAME_PORT);
  addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
  if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)))
  {
    show_info("Connection failed.");
    close(sock);
    return False;
  }

  if(Server.socket)
  {
    /* remove server */
    close(Server.socket);
    Server.socket = 0;
  }

  /* send handshake */
  id = htonl(Game.game_id);
  write(sock, &id, MSG_SIZE);
  Server.handshake = True;

  Server.client = sock;
  show_info("Connected.");

  /* initialize, if not yet done */
  if(!Game.playing)
  {
    Game.player = Player0;
    Game.side(Player0);
  }
  return True;
}

void disconnect_client(const char *info)
{
  if(Server.client)
  {
    long val;
    /* server needs to be rebindable to the same port */
    setsockopt(Server.client, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    close(Server.client);

    Server.sending = 0;
    Server.client = 0;
    Server.toread = 0;

    /* do GUI specific stuff */
    disconnect_cb();
  }
  show_info(info);
}

int setup_server(void)
{
  int sock;
  struct sockaddr_in addr;
  long val;

  if(Server.client)
  {
    show_info("Can't setup server while connected.");
    return False;
  }

  if(!Server.socket)
  {
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      fprintf(stderr, "can't create AF_INET socket.\n");
      return False;
    }

    /* server needs to be rebindable to the same port */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /* where we are listening */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(GAME_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      /* socket already in use, client stuff should still work though */
      show_info("Game server port already bound.");
      close(sock);
      return True;	/* failed, but that shouldn't be fatal :-) */
    }

    /* no queueing */
    if(listen(sock, 1) < 0)
    {
      fprintf(stderr, "unable to listen on server socket.\n");
      close(sock);
      return False;
    }
    Server.socket = sock;
  }
  show_info("Waiting connection...");
  return True;
}

int server_cb(void)
{
  int client;
  socklen_t addrlen;
  struct sockaddr_in addr;
  long id;

  /* create a connection to client */
  if((client = accept(Server.socket, (struct sockaddr *)&addr, &addrlen)) < 0)
  {
    client = 0;
    fprintf(stderr, "client connection accept failed.\n");
    return False;
  }

  /* remove server so that other games can use the port too */
  close(Server.socket);
  Server.socket = 0;

  /* send handshake */
  id = htonl(Game.game_id);
  write(client, &id, MSG_SIZE);
  Server.handshake = True;

  Server.client = client;
  show_info("Connected.");

  /* initialize, if not yet done */
  if(!Game.playing)
  {
    Game.player = Player1;
    Game.side(Player1);
  }
  return True;
}

void client_cb(void)
{
  short type;
  const char *err;
  int len;

#ifdef DEBUG
  printf("receiving ");
#endif

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
  printf("message %x (%d, %d)\n", type, Server.msg[2], Server.msg[3]);
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
 *
 * return string indicates an error.
 */
static const char *process_msg(short type, uchar x, uchar y)
{
  switch(type)
  {
    /* game start/continue/quit */

    case GAME_START:
    case GAME_CONT:
      if(x == Player0)
        Game.player = Player1;
      else
        Game.player = Player0;

      if(type == GAME_START)
      {
        send_msg(RETURN_START, x, y);
        Game.side(Game.player);
	Game.start();
      }
      else
      {
        if(Game.cont)
	{
	  if(confirm("Continue game from current situation")
	  && Game.cont(Game.player))
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
    case RETURN_START:
    case RETURN_CONT:
      Game.turn = 1;
      show_turn(Game.turn);
      Game.playing = True;
      if(type == RETURN_START || type == RETURN_CONT)
      {
	Game.player = x;
	if(type == RETURN_START)
	{
	  Game.side(Game.player);
	  Game.start();
	}
	else
	  Game.cont(Game.player);
      }
      if(Game.player == Player1)
      {
        Game.my_move = False;
        show_info("Game started.");
      }
      else 
      {
        Game.my_move = True;
        show_info("Your turn.");
      }
      break;


    case GAME_RESIGN:
      send_msg(RETURN_RESIGN, 0, 0);
      show_info("Opponent resigned.");
      Game.my_move = Game.playing = False;
      if(Game.over)
        Game.over();
      break;

    case RETURN_RESIGN:
      show_info("Resigned.");
      Game.my_move = Game.playing = False;
      if(Game.over)
        Game.over();
      break;


    /* turn change */

    case MOVE_NEXT:
      send_msg(RETURN_NEXT, 0, 0);
      if(Game.i_info)
        show_info(Game.i_info);
      else
        show_info("Your turn.");
      show_turn(++Game.turn);
      Game.my_move = True;
      break;

    case RETURN_NEXT:
      if(Game.o_info)
        show_info(Game.o_info);
      else
        show_info("Opponent's turn.");
      show_turn(++Game.turn);
      Game.my_move = False;
      break;


    case MOVE_UNDO:
      if(Game.undo && (Game.my_move || !Game.playing))
      {
	if(Game.undo())
	{
	  send_msg(RETURN_UNDO, 0, 0);
	  show_info("Undo by opponent.");
	  show_turn(--Game.turn);
	  Game.my_move = False;
	}
	else
	  send_msg(RETURN_FAIL, 0, 0);
      }
      else
        send_msg(RETURN_ILLEGAL, 0, 0);
      break;

    case RETURN_UNDO:
      if(Game.undo())
      {
	show_turn(--Game.turn);
	show_info("Your turn.");
	Game.my_move = True;
      }
      else
        /* other end shouldn't have returned this */
        return "Incompatible undo.";
      break;


    case MOVE_PASS:
      if(Game.pass)
      {
        player_t player = (Game.player == Player0 ? Player1 : Player0);
	if(Game.pass(player))
	{
	  send_msg(RETURN_PASS, 0, 0);
          if(Game.i_info)
            show_info(Game.i_info);
          else
	    show_info("Opponent passed.");
	  show_turn(++Game.turn);
	  Game.my_move = True;
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
	if(Game.o_info)
	  show_info(Game.o_info);
	else
	  show_info("Pass.");
	show_turn(++Game.turn);
	Game.my_move = False;
      }
      else
        /* other end shouldn't have returned this */
        return "Incompatible pass.";
      break;


    case GAME_LEVEL:
      send_msg(RETURN_ILLEGAL, 0, 0);	/* not a server */
      break;

    case RETURN_LEVEL:
      show_info("Level set.");
      break;

    case RETURN_ILLEGAL:
      /* other ends doesn't implement the requested feature */
      show_info("Illegal message.");
      break;

    case RETURN_FAIL:
      /* failed operation (may succeed later) or illegal message argument */
      show_info("Failed.");
      break;

    default:
      /* engine message? */
      if(!Game.message(type, x, y))
      {
        send_msg(RETURN_ILLEGAL, 0, 0);
        return "Incompatible games!";
      }
  }
  return NULL;
}

void send_msg(short type, uchar x, uchar y)
{
  uchar msg[MSG_SIZE];

  if(!Server.client)
  {
    show_info("Not connected.");
    return;
  }
#ifdef DEBUG
  printf("sending ");
#endif

  if(type < RETURN_CODE)
  {
    /* previous message not yet acknowledged? */
    if(Server.sending)
      return;
    Server.sending = True;
    show_info("Sending message...");
  }

  /* compose message */
  *(short*)msg = htons(type);
  msg[2] = x;
  msg[3] = y;
  if(write(Server.client, &msg[0], sizeof(msg)) != sizeof(msg))
    disconnect_client("Message send failed!");

#ifdef DEBUG
  printf("message %x (%d, %d)\n", type, msg[2], msg[3]);
#endif
}

