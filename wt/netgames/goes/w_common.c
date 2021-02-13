/* 
 * W window system specific Go board callbacks for the frontend functions.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "w_game.h"				/* includes standard stuff */
#include "common.h"

#define BLOCK_SIZE	16			/* default size for GUIs */
#define PIECE_HALF	(BLOCK_SIZE/2-1)	/* piece radius */
#define POINT_HALF	3			/* point radius */

/* used by the frontend (defaults) */
int BoardWidth  = BOARD_SIZE * BLOCK_SIZE;
int BoardHeight = BOARD_SIZE * BLOCK_SIZE;

static WWIN *Win;
static widget_t *Black, *White;		/* how many prisoners shown */


/* GUI specific functions */

/* Draw the initial board.
 * Could also load and transmit play piecesto W server.
 */
int initialize_board(widget_t *w, int x, int y, int wd, int ht)
{
  Win = wt_widget2win(w);
  w_setmode(Win, M_DRAW);

  y = BLOCK_SIZE/2;
  /* draw board grid */
  for(x = 0; x < BOARD_SIZE; x++)
  {
    w_vline(Win, y, BLOCK_SIZE/2, BOARD_SIZE*BLOCK_SIZE - BLOCK_SIZE/2);
    w_hline(Win, BLOCK_SIZE/2, y, BOARD_SIZE*BLOCK_SIZE - BLOCK_SIZE/2);
    y += BLOCK_SIZE;
  }

  /* draw points */
  for(y = 3; y < BOARD_SIZE-1; y += 6)
    for(x = 3; x < BOARD_SIZE-1; x += 6)
      w_pcircle(Win,
	x*BLOCK_SIZE + BLOCK_SIZE/2,
 	y*BLOCK_SIZE + BLOCK_SIZE/2,
	POINT_HALF);

  return True;
}

void remove_mark(int x, int y, color_t color)
{
  switch(color)
  {
    case BLACK:
      draw_black(x, y);
      break;
    case WHITE:
      draw_white(x, y);
      break;
    default:
      draw_empty(x, y);
      break;
  }
}

void draw_mark(int x, int y, color_t color)
{
  w_setmode(Win, M_INVERS);
  w_pbox(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF/2 + 1,
    y*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF/2 + 1,
    PIECE_HALF-2, PIECE_HALF-2);
  w_setmode(Win, M_DRAW);
}

void draw_empty(int x, int y)
{
  w_setmode(Win, M_CLEAR);
  w_pbox(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF,
    y*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF,
    PIECE_HALF*2 + 1, PIECE_HALF*2 + 1);

  w_setmode(Win, M_DRAW);
  w_hline(Win,
    (x > 0 ? x*BLOCK_SIZE : BLOCK_SIZE/2), y*BLOCK_SIZE + BLOCK_SIZE/2,
    (x < BOARD_SIZE-1 ? (x+1)*BLOCK_SIZE : x*BLOCK_SIZE + BLOCK_SIZE/2));
  w_vline(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2, (y > 0 ? y*BLOCK_SIZE : BLOCK_SIZE/2),
    (y < BOARD_SIZE-1 ? (y+1)*BLOCK_SIZE : y*BLOCK_SIZE + BLOCK_SIZE/2));

  if(!((x-3) % 6 | (y-3) % 6))
    w_pcircle(Win,
      x*BLOCK_SIZE + BLOCK_SIZE/2,
      y*BLOCK_SIZE + BLOCK_SIZE/2,
      POINT_HALF);
}

void draw_white(int x, int y)
{
  w_pcircle(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2,
    y*BLOCK_SIZE + BLOCK_SIZE/2,
    PIECE_HALF);
  w_setmode(Win, M_CLEAR);
  w_pcircle(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2 - 1,
    y*BLOCK_SIZE + BLOCK_SIZE/2 - 1,
    PIECE_HALF - 1);
  w_setmode(Win, M_DRAW);
  w_circle(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2,
    y*BLOCK_SIZE + BLOCK_SIZE/2,
    PIECE_HALF);
}

void draw_black(int x, int y)
{
  w_pcircle(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2,
    y*BLOCK_SIZE + BLOCK_SIZE/2,
    PIECE_HALF);
  w_setmode(Win, M_CLEAR);
  w_line(Win,
    x*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF*2/3,
    y*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF*1/3,
    x*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF*1/3,
    y*BLOCK_SIZE + BLOCK_SIZE/2 - PIECE_HALF*2/3);
  w_setmode(Win, M_DRAW);
}

/* --------------------- */

widget_t *add_rpane(void)
{
  return NULL;
}

/* Handicap, color, captures, possessed area etc.  */
int add_options(widget_t *pane)
{
  widget_t *label;

  label = wt_create(wt_label_class, pane);
  Black = wt_create(wt_label_class, pane);
  White = wt_create(wt_label_class, pane);

  if(!(label && White && Black))
    return False;

  wt_setopt(label, WT_LABEL, "Captures:", WT_EOL);
  wt_setopt(Black, WT_LABEL, "Black 0", WT_EOL);
  wt_setopt(White, WT_LABEL, "White 0", WT_EOL);

  return True;
}

void draw_captures(color_t color, int captures)
{
  widget_t *label;
  char number[12];
  long a;

  if(color == BLACK)
  {
    sprintf(number, "Black %d", captures);
    label = Black;
  }
  else
  {
    sprintf(number, "White %d", captures);
    label = White;
  }

  a = ButtonStateReleased;
  wt_setopt(label, WT_LABEL, number, WT_STATE, &a, WT_EOL);
}

void mark_captures(color_t color)
{
  long a = ButtonStatePressed;

  if(color == BLACK)
    wt_setopt(Black, WT_STATE, &a, WT_EOL);
  else
    wt_setopt(White, WT_STATE, &a, WT_EOL);
}

/* return True when turn is over */
void process_mouse(WEVENT *ev)
{
  if(ev->type == EVENT_MRELEASE)
    my_move(ev->x / BLOCK_SIZE, ev->y / BLOCK_SIZE);
}
