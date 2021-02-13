/*
 * A networked two player game interface.
 *  - Implements the general curses interface. Engine needs just
 *    to implement required callbacks.
 *  - Takes care of changing turns.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>		/* byte order conversion (htonl) */
#include "c_game.h"		/* standard and curses specific stuff */
#include "comms.h"

#define VERSION	 "v0.8"		/* interface version */

#define SCREENWIDTH	80	/* screen width (minimum) */
#define TEXTCOL		49	/* game information */
#define MSGLINE		1	/* program messages */
#define INFOLINE	4	/* game information */
#define TURNLINE	22	/* turn number */

GAME Game;			/* Game information */

/* place and flag for whether to move first when starting */
static int MoveX, MoveY, MoveFirst;

/* function prototypes */
static void exit_game(int sig);
static void show_help(void);
static void check_events(void);

int main(int argc, char *argv[])
{
  char *address = NULL;
  int ok = 0;

  get_configuration(&Game);

  if(argc > 1)
  {
    char *name = *argv;
    argc--;
    argv++;
    if(argc > 1 && argv[0][0] == '-' && argv[0][1] == 'c' && !argv[0][2])
    {
      /* -c <connect address> */
      address = *(++argv);
      argc -= 2;
      argv++;
    }
    if(argc && Game.args)
      if(!Game.args(name, argc, argv))
        return -1;
  }
  else
  {
    fprintf(stderr, "Board game framework for curses " VERSION "\n");
    fprintf(stderr, "GUI and networking (w) 1996 by Eero Tamminen\n");
    fprintf(stderr, "usage: %s [-c <server address>] [<options>]\n", argv[0]);
  }
  Game.player = Player0;
  MoveFirst = True;

  /* restore screen etc. */
  mysignal(SIGINT, exit_game);
  mysignal(SIGHUP, exit_game);
  mysignal(SIGTERM, exit_game);

  /* initialize curses screen */
  initscr();
  cbreak();
  noecho();

  if(address)
    ok = connect_server(address);
  else
    ok = setup_server();

  if(!ok)
  {
    endwin();
    return -1;
  }

  show_help();
  if(!initialize_board())
  {
    endwin();
    return -1;
  }

  for(;;)
    check_events();

  return 0;
}

static void set_side(void)
{
  if(MoveFirst)
    Game.player = Player0;
  else
    Game.player = Player1;
}


/* loop waiting input... */
static void check_events(void)
{
  fd_set rfd;
  int key;

  FD_ZERO(&rfd);
  FD_SET(0, &rfd);			/* stdin */

  if(Server.client)
    FD_SET(Server.client, &rfd);
  else
    FD_SET(Server.socket, &rfd);

  select(FD_SETSIZE, &rfd, NULL, NULL, NULL);

  /* process messages */
  if(Server.client && FD_ISSET(Server.client, &rfd))
      client_cb();

  if(Server.socket && FD_ISSET(Server.socket, &rfd))
      server_cb();

  if(FD_ISSET(0, &rfd))
  {
    key = getch();
    switch(key)
    {
      case 'M':
      case 'm':
        if((MoveFirst = !MoveFirst))
	  mvaddch(MoveY, MoveX, 'X');
	else
	  mvaddch(MoveY, MoveX, ' ');
	refresh();
	break;

      case 'S':
      case 's':
        set_side();
	send_msg(GAME_START, Game.player, Game.player);
	break;

      case 'C':
      case 'c':
	set_side();
	send_msg(GAME_CONT, Game.player, Game.player);
	break;

      case 'R':
      case 'r':
	if(Game.my_move)
	  send_msg(GAME_RESIGN, 0, 0);
	break;

      case 'U':
      case 'u':
	if(Game.playing && Game.undo && !Game.my_move && Game.turn > 1)
	  send_msg(MOVE_UNDO, 0, 0);
	break;

      case 'P':
      case 'p':
	if(Game.my_move && Game.pass)
	  send_msg(MOVE_PASS, 0, 0);
	break;

      case 'D':
      case 'd':
        disconnect_client("Disconnected.");
	break;

      case 'V':
      case 'v':
        setup_server();
	break;

      case 'Q':
      case 'q':
	exit_game(0);
	break;

      default:
	/* my turn or not playing and possibility to continue edited */
	if(Game.my_move || (!Game.playing && Game.cont))
	  process_key(key);
    }
  }
}

static void show_help(void)
{
 int y = INFOLINE;

  standout();
  mvaddstr(y++, TEXTCOL, Game.name);
  standend();
  y++;
  
  mvaddstr(y++, TEXTCOL, "S = start game");
  MoveY = y;
  MoveX = TEXTCOL + 16;
  mvaddstr(y++, TEXTCOL, "M = Move first [X]");
  mvaddstr(y++, TEXTCOL, "R = resign from the game");
  mvaddstr(y++, TEXTCOL, "C = continue a setup game");
  if(Game.undo)
    mvaddstr(y++, TEXTCOL, "U = undo your last move");
  if(Game.pass)
    mvaddstr(y++, TEXTCOL, "P = pass your move");
  y++;

  mvaddstr(y++, TEXTCOL, "D = disconnect client");
  mvaddstr(y++, TEXTCOL, "V = setup game server");
  mvaddstr(y++, TEXTCOL, "Q = quit program");

  add_text(TEXTCOL, ++y);
  refresh();
}

void disconnect_cb(void) { }

static void exit_game(int sig)
{
  disconnect_client("Exiting...");
  endwin();
  exit(0);
}

void game_over(void)
{
  Game.my_move = Game.playing = False;
}

void show_turn(int turn)
{
  char text[12];

  sprintf(text, "move: %03d", turn);
  mvaddstr(TURNLINE, TEXTCOL, text);
  refresh();
}

/* show a game state message */
void show_info(const char *msg)
{
  char text[SCREENWIDTH-TEXTCOL+1];
  sprintf(text, "%-*s", SCREENWIDTH-TEXTCOL, msg);
  mvaddstr(MSGLINE, TEXTCOL, text);
  refresh();
}

/* Blocks until user responds. True if user confirmed operation. */
int confirm(const char *operation)
{
  int idx;

  standout();
  mvaddstr(MSGLINE, TEXTCOL, operation);
  standend();
  addstr(" (y/n)?");
  idx = TEXTCOL + strlen(operation) + 7;
  while(idx++ < SCREENWIDTH)
    addch(' ');
  refresh();

  for(;;)
  {
    switch(getch())
    {
      case 'Y':
      case 'y':
        show_info("");
        return True;

      case 'N':
      case 'n':
        show_info("");
        return False;
    }
  }
}
