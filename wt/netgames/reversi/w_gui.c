/* 
 * W window system specific Reversi board callbacks for the frontend functions.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "w_game.h"				/* includes standard stuff */
#include "common.h"

#define BORDER		2
#define BLOCK_SIZE	27			/* default size for GUIs */
#define PIECE_HALF	(BLOCK_SIZE/2-BORDER)	/* piece radius */
#define OFFSET		(BORDER/2)

/* used by the frontend (defaults) */
int BoardWidth  = BOARD_SIZE * BLOCK_SIZE + BORDER;
int BoardHeight = BOARD_SIZE * BLOCK_SIZE + BORDER;

static WWIN *Win;
static widget_t *Black, *White;		/* show how many pieces */


/* GUI specific functions */

/* Draw the initial board.
 * Could also load and transmit play pieces to W server.
 */
int initialize_board(widget_t *w, int x, int y, int wd, int ht)
{
  int i;

  Win = wt_widget2win(w);
  w_setmode(Win, M_DRAW);

  w_dpbox(Win, x, y, wd, ht);

  /* draw board grid */
  for(x = 0; x <= BOARD_SIZE; x++)
  {
    for(i = 0; i < BORDER; i++)
    {
      w_vline(Win, y + i, 0, BOARD_SIZE * BLOCK_SIZE);
      w_hline(Win, 0, y + i, BOARD_SIZE * BLOCK_SIZE);
    }
    y += BLOCK_SIZE;
  }
  return True;
}

void draw_empty(int x, int y)
{
  w_dpbox(Win,
    x*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - PIECE_HALF,
    y*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - PIECE_HALF,
    PIECE_HALF*2 + 1, PIECE_HALF*2 + 1);
}

void draw_white(int x, int y)
{
  w_pcircle(Win,
    x*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2,
    y*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2,
    PIECE_HALF);
  w_setmode(Win, M_CLEAR);
  w_pcircle(Win,
    x*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - 1,
    y*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - 1,
    PIECE_HALF - 1);
  w_setmode(Win, M_DRAW);
  w_circle(Win,
    x*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2,
    y*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2,
    PIECE_HALF);
}

void draw_black(int x, int y)
{
  w_pcircle(Win,
    x*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2,
    y*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2,
    PIECE_HALF);
  w_setmode(Win, M_CLEAR);
  w_line(Win,
    x*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - PIECE_HALF*2/3,
    y*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - PIECE_HALF*1/3,
    x*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - PIECE_HALF*1/3,
    y*BLOCK_SIZE + OFFSET + BLOCK_SIZE/2 - PIECE_HALF*2/3);
  w_setmode(Win, M_DRAW);
}

/* --------------------- */

widget_t *add_rpane(void)
{
  return NULL;
}

int add_options(widget_t *pane)
{
  widget_t *label;

  label = wt_create(wt_label_class, pane);
  Black = wt_create(wt_label_class, pane);
  White = wt_create(wt_label_class, pane);

  if(!(label && White && Black))
    return False;

  wt_setopt(label, WT_LABEL, "Pieces:", WT_EOL);
  wt_setopt(Black, WT_LABEL, "Black 0", WT_EOL);
  wt_setopt(White, WT_LABEL, "White 0", WT_EOL);

  return True;
}

void draw_pieces(color_t color, int pieces)
{
  widget_t *label;
  char number[12];
  long a;

  if(color == BLACK)
  {
    sprintf(number, "Black %d", pieces);
    label = Black;
  }
  else
  {
    sprintf(number, "White %d", pieces);
    label = White;
  }

  a = ButtonStateReleased;
  wt_setopt(label, WT_LABEL, number, WT_STATE, &a, WT_EOL);
}

void mark_pieces(color_t color)
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

void flush_out(void)
{
  w_flush();
}
