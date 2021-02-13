/*  Wt-Ant
 *
 * This is 'want' with Wt GUI.  See the W-Ant manual for more info.
 *
 * Compile with something like (under MiNT):
 *   gcc -O2 -Wall -mbaserel ant.c -o want  -lbWt -lbW -lbsocket
 *
 *  (w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <Wlib.h>
#include <Wt.h>

#define VERSION		"v1"
#define WIN_NAME	"W-Ant"
#define BLK_SIZE	8		/* size for an ant 'position' */

#define MIN_COLS	12
#define MIN_ROWS	12
#define DEF_COLS	40
#define DEF_ROWS	40

#define MAX_STEP	0xffffffffUL
#define DEF_STEP	"2000"
#define DEF_RULE	"rllrrl"
#define DEF_SEED	38
#define DEF_LEN		6

/* max. sequence lenght (= bits in seed variable) */
#define MAX_SEQ		32

#ifndef MAX
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y)	((x) < (y) ? (x) : (y))
#endif

static struct
{
  WWIN *win;
  int antx;			/* ant position */
  int anty;
  int heading;			/* ant direction (0-3)		*/
  unsigned char len;		/* lenght of seq. (2->MAX_SEQ)	*/
  unsigned long seed;		/* move sequence (rllrrl)	*/
  unsigned char **grid;		/* 2-dim. array			*/
  unsigned long steps;		/* movement counter (down)	*/
  int colors;			/* whether to use / how many colors */
  int cols;			/* ant walking area size	*/
  int rows;
} Par;

#define STEPS	32		/* how many steps at the time when fast */
#define TIMEOUT	100		/* how many ms between them */

static unsigned long Fast = 1;	/* how many steps done in succession */
static long Timer = -1;		/* stepping timer handle */

static widget_t
  *Top,
  *Draw,			/* output shell */
  *Step,			/* Step amount */
  *Rule,			/* rule sequence */
  *Error;

/* function prototypes */
static int  parse_args(int argc, char *argv[], int *rows, int *cols);
static int  parse_rule(char *rule, unsigned long *seed, unsigned char *len);
static void quit_cb(widget_t *w);
static void close_cb(widget_t *w);
static void fast_cb(widget_t *w, int pressed);
static void clear_cb(widget_t *w, int pressed);
static void stop_cb(widget_t *w, int pressed);
static void walk_cb(widget_t *w, int pressed);
static void draw_fn(widget_t *w, int x, int y, int wd, int ht);
static void timer_cb(long w);
static void ant_walk(int steps);

/* ---------------------------- Main ---------------------------------- */

int main (int argc, char *argv[])
{
  widget_t *par, *pane, *hpane1, *hpane2, *step, *fast,
    *rule, *clr, *walk, *stop, *quit;
  long a, b;

  Par.cols = DEF_COLS;
  Par.rows = DEF_ROWS;
  if(!parse_args(argc, argv, &Par.rows, &Par.cols))
  {
    fprintf(stderr, WIN_NAME " " VERSION " (w) 1996 by Eero Tamminen.\n");
    fprintf(stderr, "usage: want [options]\n");
    fprintf(stderr, "  -w <width in blocks>\n");
    fprintf(stderr, "  -h <height in blocks>\n");
    return -1;
  }

  Top = wt_init();

  par  = wt_create(wt_shell_class, Top);
  pane = wt_create(wt_pane_class, par);

  step = wt_create(wt_label_class, pane);
  hpane1 = wt_create(wt_pane_class, pane);
  Step = wt_create(wt_getstring_class, hpane1);
  fast = wt_create(wt_checkbutton_class, hpane1);

  rule = wt_create(wt_label_class, pane);
  Rule = wt_create(wt_getstring_class, pane);
  Error = wt_create(wt_label_class, pane);

  hpane2 = wt_create(wt_pane_class, pane);
  clr  = wt_create(wt_button_class, hpane2);
  walk = wt_create(wt_button_class, hpane2);
  stop = wt_create(wt_button_class, hpane2);
  quit = wt_create(wt_button_class, hpane2);

  a = OrientHorz;
  b = 12;
  wt_setopt(hpane1,
    WT_ORIENTATION, &a,
    WT_HDIST, &b,
    WT_EOL);

  wt_setopt(hpane2, WT_ORIENTATION, &a, WT_EOL);

  wt_setopt(par, WT_LABEL, " " WIN_NAME " " VERSION " ", WT_EOL);
  wt_setopt(step, WT_LABEL, "Number of steps:", WT_EOL);
  wt_setopt(rule, WT_LABEL, "Rule sequence (use R/L):", WT_EOL);
  wt_setopt(Error, WT_LABEL, "                       ", WT_EOL);

  a = 8;
  wt_setopt(Step,
    WT_STRING_LENGTH, &a,
    WT_STRING_ADDRESS, DEF_STEP,
    WT_STRING_MASK, "0-9",
    WT_EOL);

  wt_setopt(fast,
    WT_LABEL, "Burst mode",
    WT_ACTION_CB, fast_cb,
    WT_EOL);

  a = 32;
  wt_setopt(Rule,
    WT_STRING_LENGTH, &a,
    WT_STRING_ADDRESS, DEF_RULE,
    WT_STRING_MASK, "rlRL",
    WT_EOL);

  wt_setopt(clr,
    WT_LABEL, "Clear",
    WT_ACTION_CB, clear_cb,
    WT_EOL);

  wt_setopt(walk,
    WT_LABEL, "Walk",
    WT_ACTION_CB, walk_cb,
    WT_EOL);

  wt_setopt(stop,
    WT_LABEL, "Stop",
    WT_ACTION_CB, stop_cb,
    WT_EOL);

  wt_setopt(quit,
    WT_LABEL, "Quit",
    WT_ACTION_CB, quit_cb,
    WT_EOL);

  wt_realize(Top);
  wt_run();
  return 0;
}

static int parse_args(int argc, char *argv[], int *rows, int *cols)
{
  int idx = 0;

  while (++idx < argc)
  {
    if(argv[idx][0] == '-' && !argv[idx][2] && idx+1 < argc)
    {
      switch (argv[idx][1])
      {
	case 'W':				/* screen width		*/
	  *cols = MAX(MIN_COLS, atoi(argv[++idx]) & 0x1ff);
	  break;
	case 'H':				/* screen height	*/
	  *rows = MAX(MIN_ROWS, atoi(argv[++idx]) & 0x1ff);
	  break;
	default:				/* unrecogniced option	*/
	  return 0;
      }
    }
    else
      return 0;
  }
  return 1;
}

static int parse_rule(char *rule, unsigned long *seed, unsigned char *len)
{
  char token;
  int left = 0;

  *seed = 0; *len = 0;
  token = toupper(*rule);
  while (*len < MAX_SEQ && (token == 'R' || token == 'L'))
  {
    *seed <<= 1;
    if (token == 'R')
      *seed |= 1;
    else
      left = 1;

    token = toupper(*++rule);
    (*len)++;
  }
  /* not any rights or lefts? */
  if(!*seed || !left)
    return 0;

  return 1;
}

static void quit_cb(widget_t *w)
{
  int x;

  if(Par.grid)
  {
    /* free array alloc.	*/
    for (x = 0; x < Par.cols; x++)
      free (Par.grid[x]);
    free (Par.grid);
  }
  wt_break(1);
}

static void close_cb(widget_t *w)
{
  wt_close(w);  
}

static void fast_cb(widget_t *w, int pressed)
{
  if(Fast == 1UL)
    Fast = STEPS;
  else
    Fast = 1UL;
}

static void clear_cb(widget_t *w, int pressed)
{
  int x;

  if(pressed)
    return;

  if(Draw && Par.grid)
  {
    if (Par.colors)
    {
      w_setForegroundColor(Par.win, 1);
    }
    /* clear the window to black */
    w_pbox(Par.win, 0, 0, Par.win->width, Par.win->height);

    /* clear array */
    for (x = 0; x < Par.cols; x++)
      memset(Par.grid[x], 0, Par.rows);

    /* default values */
    Par.antx = Par.cols / 2;
    Par.anty = Par.rows / 2;
    Par.heading = 0;
  }
}

static void stop_cb(widget_t *w, int pressed)
{
  if(pressed)
    return;

  if(Timer >= 0)
    wt_deltimeout(Timer);
}

static void walk_cb(widget_t *w, int pressed)
{
  unsigned long seed;
  unsigned char len;
  widget_t *draw;
  char *str;
  int x;

  if(pressed)
    return;

  /* get / set the new parameters */

  wt_getopt(Step, WT_STRING_ADDRESS, &str, WT_EOL);
  Par.steps = atol(str);
  if(!Par.steps)
    Par.steps = MAX_STEP;

  wt_getopt(Rule, WT_STRING_ADDRESS, &str, WT_EOL);
  if(!parse_rule(str, &seed, &len))
  {
    wt_setopt(Error, WT_LABEL, "Invalid rule!", WT_EOL);
    return;
  }
  Par.seed = seed;
  Par.colors = 0;
  Par.len = len;

  if (wt_global.screen_shared >= Par.len)
  {
    Par.colors = wt_global.screen_shared;
  }

  /* create and initialize the grid array */

  if(!Par.grid)
  {
    if(!(Par.grid = (unsigned char **)malloc(sizeof(int) * Par.cols)))
      return;

    for (x = 0; x < Par.cols; x++)
      {
	if(!(Par.grid[x] = (unsigned char *)malloc(sizeof(int) * Par.rows)))
	{
	  for(--x; x >= 0; x--)
	    free(Par.grid[x]);
	  free(Par.grid);
	  Par.grid = NULL;
          wt_setopt(Error, WT_LABEL, "Not enough memory.", WT_EOL);
	  return;
	}
	memset(Par.grid[x], 0, Par.rows);
      }
  }

  /* open the output window */

  if(!Draw)
  {
    long a, b;

    /* window size */
    a = Par.cols * BLK_SIZE;
    b = Par.rows * BLK_SIZE;

    Draw = wt_create(wt_shell_class, Top);
    draw = wt_create(wt_drawable_class, Draw);

    wt_setopt(Draw,
      WT_LABEL, "AntWalk!",
      WT_ACTION_CB, close_cb,
      WT_EOL);

    wt_setopt(draw,
      WT_WIDTH, &a,
      WT_HEIGHT, &b,
      WT_DRAW_FN, draw_fn,
      WT_EOL);

    if(!(draw && wt_realize(Top)))
    {
      wt_setopt(Error, WT_LABEL, "Widget/window open failed.", WT_EOL);
      return;
    }
  }
  else
    wt_open(Draw);

  wt_setopt(Error, WT_LABEL, " ", WT_EOL);
  Timer = wt_addtimeout(TIMEOUT, timer_cb, 0L);
}

static void draw_fn(widget_t *w, int x, int y, int wd, int ht)
{
  Par.win = wt_widget2win(w);
  w_setmode(Par.win, M_DRAW);
  w_pbox(Par.win, x, y, wd, ht);

  Par.antx = Par.cols / 2;
  Par.anty = Par.rows / 2;
  Par.heading = 0;
}

static void timer_cb(long w)
{
  if(Par.steps > Fast)
  {
    ant_walk(Fast);
    Par.steps -= Fast;
    Timer = wt_addtimeout(TIMEOUT, timer_cb, w);
  }
  else
  {
    ant_walk(Par.steps);
    Par.steps = 0;
  }
}

static void ant_walk(int steps)
{
  unsigned char value, prev;

  while(steps--)
  {
    prev = Par.grid[Par.antx][Par.anty];	/* previous grid value	*/
    value = (prev + 1) % Par.len;		/* new grid value	*/
    Par.grid[Par.antx][Par.anty] = value;	/* store		*/

    /* display ant at (antx,anty) with 'color' according to value */
    if (Par.colors)
    {
      w_setForegroundColor(Par.win, Par.colors - 1 - value);
      w_pbox(Par.win,
	     Par.antx * BLK_SIZE+1, Par.anty * BLK_SIZE+1,
	     BLK_SIZE-2, BLK_SIZE-2);
    }
    else
    {
      w_setpattern(Par.win, MAX_GRAYSCALES * value / (Par.len-1));
      w_dpbox(Par.win,
	      Par.antx * BLK_SIZE+1, Par.anty * BLK_SIZE+1,
	      BLK_SIZE-2, BLK_SIZE-2);
    }

    /* turn according to seed sequence & (previous) grid value */
    if (Par.seed & (1 << prev))
      Par.heading++;
    else
      Par.heading--;

    /* new ant position */
    switch (Par.heading)
    {
      case 4:
	Par.heading = 0;
      case 0:
	Par.anty++;		/* down		*/
	break;
      case 1:
	Par.antx--;		/* left		*/
	break;
      case 2:
	Par.anty--;		/* up		*/
	break;
      case -1:
	Par.heading = 3;
      case 3:
	Par.antx++;		/* right	*/
	break;
    }

    /* check boundaries */
    if (Par.antx >= Par.cols)
      Par.antx = 0;
    if (Par.anty >= Par.rows)
      Par.anty = 0;
    if (Par.antx < 0)
      Par.antx = Par.cols - 1;
    if (Par.anty < 0)
      Par.anty = Par.rows - 1;
  }
}

