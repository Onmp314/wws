/* 
 * W window system specific checkered board callbacks
 * for the frontend and chess.c functions.
 *
 * 23.9.1996
 * - Promotion dialog and nicer initialize.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <time.h>
#include "w_game.h"				/* includes standard stuff */
#include "chess.h"

extern BITMAP *Bitmaps;

#define IMAGES		"chess.img"
#define BLK_W		38
#define BLK_H		38
#define BORDER		3

/* used by the frontend (defaults) */
int BoardWidth  = BOARD_SIZE*BLK_W + BORDER*2;
int BoardHeight = BOARD_SIZE*BLK_H + BORDER*2;

#define PROMOTES	4
#define DEF_PROMOTE	QUEEN
static int Promotion[PROMOTES] = { QUEEN, ROOK, BISHOP, KNIGHT };

static WWIN *Board, *Pieces;
static time_t StartTime;
static long Timer = -1;
static widget_t *Time;

/* callback function prototypes. */
int initialize_board(widget_t *w, int x, int y, int wd, int ht);
void process_mouse(WEVENT *ev);
void draw_piece(int x, int y, char id);
void draw_mark(int x, int y, int on);
static void timer_cb(long arg);


/* Load images and draw the initial board. */
int initialize_board(widget_t *w, int x, int y, int wd, int ht)
{
  int idx;

  if((Pieces = w_create(Bitmaps->width, Bitmaps->height, 0)))
    w_putblock(Bitmaps, Pieces, 0, 0);

  if(!Pieces)
    return False;

  Board = wt_widget2win(w);

  for(idx = 0; idx < BORDER; idx++)
    w_box(Board, idx, idx, wd - 2*idx, ht - 2*idx);

  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if((x+y) & 1)
        w_dpbox(Board, BORDER + x * BLK_W, BORDER + y * BLK_H, BLK_W, BLK_H);

  return True;
}

void draw_mark(int x, int y, int on)
{
  if(x < 0 || y < 0)
    return;

  if(on)
    w_setmode(Board, M_DRAW);
  else
  {
    if(((x+y) & 1))
    {
      w_setmode(Board, M_DRAW);
      w_dbox(Board, BORDER + x*BLK_W,   BORDER + y*BLK_H,   BLK_W, BLK_H);
      w_dbox(Board, BORDER + x*BLK_W+1, BORDER + y*BLK_H+1, BLK_W-2, BLK_H-2);
      return;
    }
    else
      w_setmode(Board, M_CLEAR);
  }

  w_box(Board, BORDER + x*BLK_W,   BORDER + y*BLK_H,   BLK_W, BLK_H);
  w_box(Board, BORDER + x*BLK_W+1, BORDER + y*BLK_H+1, BLK_W-2, BLK_H-2);
}


/* this three are the only functions that needs to know how the piece images
 * are arranged in the image canvas...
 */
static int piece2index(int piece)
{
  /* mask flags away */
  switch(PIECE(piece))
  {
    case ROOK:
      return 0;
    case KNIGHT:
      return 1;
    case BISHOP:
      return 2;
    case QUEEN:
      return 3;
    case KING:
      return 4;
    case PAWN:
    default:
      return 5;
  }
}

void draw_piece(int x, int y, char id)
{
  int color;

  if(id == EMPTY)
  {
    if((x+y) & 1)
    {
      w_setmode(Board, M_DRAW);
      w_dpbox(Board, BORDER + x*BLK_W, BORDER + y*BLK_H, BLK_W, BLK_H);
    }
    else
    {
      w_setmode(Board, M_CLEAR);
      w_pbox(Board, BORDER + x*BLK_W, BORDER + y*BLK_H, BLK_W, BLK_H);
    }
    return;
  }

  /* piece color */
  color = 0;
  if(COLOR(id) == WHITE)
    color++;

  /* background color */
  if((x+y) & 1)
    color += 2;

  id = piece2index(id);

  w_bitblk2(Pieces, id*BLK_W, color*BLK_H, BLK_W, BLK_H,
    Board, BORDER + x*BLK_W, BORDER + y*BLK_H);
}


static void event_cb(widget_t *w, WEVENT *ev)
{
  WWIN *win = wt_widget2win(w);
  static long prev;
  long i = 0;

  if(ev->type == EVENT_MPRESS)
  {
    prev = ev->x / BLK_W;
    w_pbox(win, prev * BLK_W, 0, BLK_W, BLK_H);
    return;
  }
  
  if(ev->type == EVENT_MRELEASE)
  {
    i = ev->x / BLK_W;
    w_pbox(win, prev * BLK_W, 0, BLK_W, BLK_H);
    if(i != prev)
      return;

    if(i >= 0 && i < PROMOTES)
      i = Promotion[i];
    else
      i = DEF_PROMOTE;

    /* set the usrval of the toplevel widget to the selected piece */
    for(w = w; w->parent; w = w->parent)
      ;
    wt_setopt(w, WT_USRVAL, &i, WT_EOL);
  }
}

static void draw_fn(widget_t *w, long x, long y, long wd, long ht)
{
  WWIN *win = wt_widget2win(w);
  int row, i, idx;
  long a;

  wt_getopt(w, WT_USRVAL, &a, WT_EOL);
  if(a == WHITE)
    row = BLK_H;
  else
    row = 0;

  for(i = 0; i < PROMOTES; i++)
  {
    idx = piece2index(Promotion[i]);
    w_bitblk2(Pieces, idx * BLK_W, row, BLK_W, BLK_H, win, i*BLK_W, 0);
  }
  w_setmode(win, M_INVERS);
}

static void close_cb(widget_t *w)
{
  long i = DEF_PROMOTE;

  for(w = w; w->parent; w = w->parent)
    ;
  /* 'return' the default */
  wt_setopt(w, WT_USRVAL, &i, WT_EOL);
}

int promote_to(color_t color)
{
  widget_t *top, *shell, *pieces;
  long a, b, c, d;
  short x, y;

  top = wt_create(wt_top_class, NULL);

  a = -1;
  wt_setopt(top, WT_USRVAL, &a, WT_EOL);

  shell = wt_create(wt_shell_class, top);

  w_querywindowpos(Board, 1, &x, &y);
  a = x - 10;
  b = y + 30;

  wt_setopt(shell,
    WT_ACTION_CB, close_cb,
    WT_LABEL, " Promote pawn to: ",
    WT_XPOS, &a,
    WT_YPOS, &b,
    WT_EOL);

  pieces = wt_create(wt_drawable_class, shell);

  a = BLK_W * PROMOTES;
  b = BLK_H;
  c = EV_MOUSE;
  d = color;

  wt_setopt(pieces,
    WT_WIDTH, &a,
    WT_HEIGHT, &b,
    WT_DRAW_FN, draw_fn,
    WT_EVENT_MASK, &c,
    WT_EVENT_CB, event_cb,
    WT_USRVAL, &d,
    WT_EOL);

  wt_realize(top);

  while(!wt_do_event())
  {
    wt_getopt(top, WT_USRVAL, &a, WT_EOL);
    if(a >= 0)
      break;
  }
  wt_getopt(top, WT_USRVAL, &a, WT_EOL);
  wt_delete(top);

  return a;
}


/* return 0 when turn is over */
void process_mouse(WEVENT *ev)
{
  if(ev->type == EVENT_MRELEASE)
    my_move(ev->x / BLK_W, ev->y / BLK_H);
}

static void timer_cb(long arg)
{
  time_t passed = time(NULL) - StartTime;
  static char string[10] = "00:00:00";

  /* igonore date and year */
  strncpy(string, asctime(gmtime(&passed)) + 11, 8);
  wt_setopt(Time, WT_LABEL, string, WT_EOL);

  /* called every two seconds */
  Timer = wt_addtimeout(2000L, timer_cb, arg);
}

void start_timer(void)
{
  StartTime = time(NULL);
  timer_cb(0L);
}

void stop_timer(void)
{
  if(Timer >= 0)
    wt_deltimeout(Timer);
}

/* no additional options for Chess */
int add_options(widget_t *pane)
{
  widget_t *label;

  label = wt_create(wt_label_class, pane);
  Time  = wt_create(wt_label_class, pane);

  wt_setopt(label, WT_LABEL, "Game time:", WT_EOL);
  wt_setopt(Time,  WT_LABEL, "00:00:00", WT_EOL);

  return True;
}

/* no game specific panel */
widget_t *add_rpane(void)
{
  return NULL;
}
