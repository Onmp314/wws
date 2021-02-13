/*
 * A networked two player game interface.
 *  - Implements the general GUI interface. Engine needs just
 *    to implement required callbacks.
 *  - Takes care of changing turns.
 *
 * (w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include <unistd.h>
#include <arpa/inet.h>		/* byte order conversion (htonl) */
#include "w_game.h"		/* includes standard + W stuff */
#include "comms.h"

#define VERSION	 "v0.8"		/* interface version */

GAME Game;			/* Game information */

static long Local;		/* game server toolkit handle */
static long Remote;		/* toolkit handle for the opponent */

static widget_t
  *Top,			/* realizable */
  *Shell,		/* parent widget for confirm() */
  *First,		/* switch on which one starts the game */
  *Turn,		/* shows turn number */
  *Status,		/* show game status, error messages etc. */
  *Address;		/* game server address string */


/* function prototypes */
static int initialize_window(void);
static void set_side(void);
static WEVENT *event_cb(widget_t *w, WEVENT *ev);
static void start_cb(widget_t *w, int pressed);
static void cont_cb(widget_t *w, int pressed);
static void resign_cb(widget_t *w, int pressed);
static void pass_cb(widget_t *w, int pressed);
static void undo_cb(widget_t *w, int pressed);
static void network_cb(widget_t *w, int pressed);
static void connect_cb(widget_t *w, int pressed);
static void disconnect_client_cb(widget_t *w, int pressed);
static void connect_server_cb(widget_t *w, const char *addr);
static void setup_cb(widget_t *w, int pressed);
static int setup_server_w(void);
static void server_cb_w(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex);
static void client_cb_w(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex);


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
      fprintf(stderr, "Board game framework for W window system " VERSION "\n");
      fprintf(stderr, "GUI and networking (w) 1996 by Eero Tamminen\n");
      return -1;
    }
  }

  if(!initialize_window())
    return -1;

  show_info("Ready.");
  wt_realize(Top);
  wt_run();

  return 0;
}

/* only buttons that have some actions are output onto the screen */
static int initialize_window(void)
{
  widget_t *vpane, *hpane, *buttons, *options, *connect,
    *start, *cont, *resign, *undo, *pass, *board;
  long a;

  if(!(Top = wt_init()))
    return False;

  Shell   = wt_create(wt_shell_class, Top);
  vpane   = wt_create(wt_pane_class, Shell);
  hpane   = wt_create(wt_pane_class, vpane);
  Status  = wt_create(wt_label_class, vpane);

  buttons = wt_create(wt_pane_class, hpane);
  connect = wt_create(wt_button_class, buttons);
  First   = wt_create(wt_checkbutton_class, buttons);
  start   = wt_create(wt_button_class, buttons);

  if (!(hpane && Status && connect && First && start))
  {
    fprintf(stderr, "unable to initialize widgets!\n");
    return False;
  }

  if(Game.cont && (cont = wt_create(wt_button_class, buttons)))
    wt_setopt(cont, WT_LABEL, "Continue", WT_ACTION_CB, cont_cb, WT_EOL);

  resign  = wt_create(wt_button_class, buttons);

  if(Game.pass && (pass = wt_create(wt_button_class, buttons)))
    wt_setopt(pass, WT_LABEL, "Pass move", WT_ACTION_CB, pass_cb, WT_EOL);
  if(Game.undo && (undo = wt_create(wt_button_class, buttons)))
    wt_setopt(undo, WT_LABEL, "Undo last", WT_ACTION_CB, undo_cb, WT_EOL);

  /* add game specific options */
  if(!add_options(buttons))
    return False;

  Turn    = wt_create(wt_label_class, buttons);
  board   = wt_create(wt_drawable_class, hpane);

  if(!(resign && Turn && board))
    return False;

  /* add game specific pane right of the board? */
  if((options = add_rpane()))
    wt_add_after(hpane, board, options);


  wt_setopt(Shell, WT_LABEL, Game.name, WT_EOL);

  a = AlignFill;
  wt_setopt(vpane, WT_ALIGNMENT, &a, WT_EOL);
  wt_setopt(buttons, WT_ALIGNMENT, &a, WT_EOL);

  a = LabelModeWithBorder;
  wt_setopt(Status, WT_MODE, &a, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);

  a = ButtonStatePressed;
  wt_setopt(First,   WT_LABEL, "Move first", WT_STATE, &a, WT_EOL);
  wt_setopt(connect, WT_LABEL, "Connect", WT_ACTION_CB, network_cb, WT_EOL);
  wt_setopt(start,   WT_LABEL, "Start game", WT_ACTION_CB, start_cb, WT_EOL);
  wt_setopt(resign,  WT_LABEL, "Resign", WT_ACTION_CB, resign_cb, WT_EOL);
  wt_setopt(Turn,    WT_LABEL, "Turn: 000", WT_EOL);

  a = EV_MOUSE;
  wt_setopt(board,
    WT_WIDTH, &Game.width,
    WT_HEIGHT, &Game.height,
    WT_EVENT_MASK, &a,
    WT_DRAW_FN, initialize_board,
    WT_EVENT_CB, event_cb,
    WT_EOL);

  return True;
}

/* True if user confirmed operation */
int confirm(const char *operation)
{
  char *buf;
  int ret;

  if(!(buf = malloc(strlen(operation) + 3)))
    return False;

  sprintf(buf, "%s?", operation);
  ret = wt_dialog(Shell, buf, WT_DIAL_QUEST, Game.name, "Confirm", "Refuse", NULL);
  free(buf);

  if(ret == 1)
    return True;
  return False;
}

static WEVENT * event_cb(widget_t *w, WEVENT *ev)
{
  /* my turn or not playing and possibility to continue edited */
  if(Game.my_move || (!Game.playing && Game.cont))
    process_mouse(ev);

  return NULL;
}

static void set_side(void)
{
  long val;

  wt_getopt(First, WT_STATE, &val, WT_EOL);
  if(val == ButtonStatePressed)
    Game.player = Player0;
  else
    Game.player = Player1;
  
}

static void start_cb(widget_t *w, int pressed)
{
  if(!pressed)
  {
    set_side();
    send_msg(GAME_START, Game.player, Game.player);
  }
}

static void cont_cb(widget_t *w, int pressed)
{
  if(!pressed)
  {
    set_side();
    send_msg(GAME_CONT, Game.player, Game.player);
  }
}

static void resign_cb(widget_t *w, int pressed)
{
  if(!pressed && Game.my_move)
    send_msg(GAME_RESIGN, 0, 0);
}

static void undo_cb(widget_t *w, int pressed)
{
  if(!pressed && Game.playing && Game.undo && !Game.my_move && Game.turn > 1)
    send_msg(MOVE_UNDO, 0, 0);
}

static void pass_cb(widget_t *w, int pressed)
{
  if(!pressed && Game.my_move && Game.pass)
    send_msg(MOVE_PASS, 0, 0);
}

static void close_win(widget_t *w)
{
  wt_close(w);
}

static void network_cb(widget_t *w, int pressed)
{
  static widget_t *network = 0;
  widget_t *hpane, *vpane, *label, *connect, *discon, *server;
  long a, b;

  if(pressed)
    return;

  if(network)
  {
    wt_open(network);
    return;
  }

  network = wt_create(wt_shell_class, Top);
  vpane   = wt_create(wt_pane_class, network);
  label   = wt_create(wt_label_class, vpane);
  Address = wt_create(wt_getstring_class, vpane);
  hpane   = wt_create(wt_pane_class, vpane);
  server  = wt_create(wt_button_class, hpane);
  connect = wt_create(wt_button_class, hpane);
  discon  = wt_create(wt_button_class, hpane);

  if(!(label && Address && connect && discon && server))
  {
    fprintf(stderr, "unable to create network dialog\n");
    return;
  }

  wt_setopt(network, WT_LABEL, Game.name, WT_ACTION_CB, close_win, WT_EOL);
  wt_setopt(label, WT_LABEL, "Host address:", WT_EOL);

  a = AlignFill;
  wt_setopt(vpane, WT_ALIGNMENT, &a, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);

  a = 16;
  b = 64;
  wt_setopt(Address,
    WT_STRING_WIDTH, &a,
    WT_STRING_LENGTH, &b,
    WT_ACTION_CB, connect_server_cb,
    WT_EOL);

  wt_setopt(server,
    WT_LABEL, "Setup server",
    WT_ACTION_CB, setup_cb,
    WT_EOL);

  wt_setopt(connect,
    WT_LABEL, "Connect",
    WT_ACTION_CB, connect_cb,
    WT_EOL);

  wt_setopt(discon,
    WT_LABEL, "Disconnect",
    WT_ACTION_CB, disconnect_client_cb,
    WT_EOL);

  wt_realize(Top);
}

static void connect_cb(widget_t *w, int pressed)
{
  char *addr;

  if(!pressed)
  {
    wt_getopt(Address, WT_STRING_ADDRESS, &addr, WT_EOL);
    connect_server_cb(Address, addr);
  }
}

static void setup_cb(widget_t *w, int pressed)
{
  if(!pressed)
    setup_server_w();
}

static void connect_server_cb(widget_t *w, const char *string)
{
  fd_set rfd;

  if(connect_server(string))
  {
    /* add client to events and remove server */
    FD_ZERO(&rfd);
    FD_SET(Server.client, &rfd);
    wt_delinput(Local);
    Remote = wt_addinput(&rfd, NULL, NULL, client_cb_w, 0L);
  }
}

static void disconnect_client_cb(widget_t *w, int pressed)
{
  if(!pressed)
    disconnect_client("Disconnected.");
}

static int setup_server_w(void)
{
  fd_set rfd;

  if(setup_server())
  {
    if(Server.socket)
    {
      FD_ZERO(&rfd);
      FD_SET(Server.socket, &rfd);
      Local = wt_addinput(&rfd, NULL, NULL, server_cb_w, 0L);
    }
    return True;
  }
  return False;
}


static void server_cb_w(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex)
{
  fd_set new;

  if(server_cb())
  {
    FD_ZERO(&new);
    FD_SET(Server.client, &new);
    wt_delinput(Local);
    Remote = wt_addinput(&new, NULL, NULL, client_cb_w, 0L);
  }
}

static void client_cb_w(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex)
{
  client_cb();
}

void disconnect_cb(void)
{
  wt_delinput(Remote);
}

void game_over(void)
{
  Game.my_move = Game.playing = False;
}

void show_turn(int turn)
{
  char text[12];

  sprintf(text, "move: %03d", turn);
  wt_setopt(Turn, WT_LABEL, text, WT_EOL);
}

/* show a game state message */
void show_info(const char *msg)
{
  wt_setopt(Status, WT_LABEL, msg, WT_EOL);
}
