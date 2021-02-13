/*
 * WarZone game for W Window System and W Toolkit
 *
 * WarZone maintainence:
 * - creates and draws the 'mountain'.
 * - modifies the mountain array / window (explosion pits etc).
 * - draws objects on screen.
 *
 * TODO:
 * - Use flatbased bunkers instead of circular ones
 *   (dropping them down the mountain side is easier)
 * - Copy the range into screen from an image.
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include <unistd.h>
#include "hill.h"		/* global prototypes */
#include "sound.h"

/* ouput window size */
#define WIDTH		513	/* 2^n+1 */
#define HEIGHT		255	/* something that fits into Range[] */
#define WIN_HEIGHT	(HEIGHT + 65)


/* range output window */
static widget_t
  *Shell;

static WWIN
  *RangeWin;

/* array for the mountain range */
static unsigned char
  *Range;


/* local prototypes */
static void range_draw(void);
static void range_drawstart(widget_t *w, int x, int y, int wd, int ht);
static void range_do(int a, int b);
static void close_cb(widget_t *w);


/* -----------
 * initializations
 */

/* allocate range */
int range_alloc(void)
{
  if((Range = (unsigned char *)malloc(WIDTH)))
    return 1;

  fprintf(stderr, "range_alloc: not enough memory!\n");
  return 0;
}

void range_free(void)
{
  if(Range)
    free(Range);
}

void range_init(void)
{
  widget_t *range;
  long a, b;

  Shell = wt_create(wt_shell_class, Top);
  wt_setopt(Shell,
    WT_LABEL, " WarZone! ",
    WT_ACTION_CB, close_cb,
    WT_EOL);

  range = wt_create(wt_drawable_class, Shell);
  a = WIDTH;
  b = WIN_HEIGHT+1;
  wt_setopt(range,
    WT_WIDTH, &a,
    WT_HEIGHT, &b,
    WT_DRAW_FN, range_drawstart,
    WT_EOL);
}

/* draw the gaming range */
static void range_drawstart(widget_t *w, int x, int y, int wd, int ht)
{
  RangeWin = wt_widget2win(w);
}

static void close_cb(widget_t *w)
{
  round_over();
}

/* --------------
 * range creation / info functions
 */

void range_close(void)
{
  wt_close(Shell);
}

void range_open(void)
{
  if(!Shell)
  {
    range_init();
    wt_realize(Top);
  }

  /* range initilization */
  Range[0] = RND(HEIGHT);
  Range[(WIDTH-1)/2] = RND(HEIGHT);
  Range[WIDTH-1] = RND(HEIGHT);

  range_do(0, (WIDTH-1)/2);
  range_do((WIDTH-1)/2, WIDTH-1);
  range_draw();

  do_sound(SND_RANGE);

  wt_open(Shell);
}

static void range_draw(void)
{
  int x, y;

  w_setmode(RangeWin, M_CLEAR);
  w_pbox(RangeWin, 0, 0, RangeWin->width, RangeWin->height);
  w_setmode(RangeWin, M_DRAW);

  y = WIN_HEIGHT;
  for(x = 0; x < WIDTH; x++)
    w_vline(RangeWin, x, y, y - Range[x]);
}

/* iterate into smaller ranges until the whole range is filled */
static void range_do(int a, int b)
{
/* a scale limit for jagginess */
#define STRAIGHTS	8

  int d, h;

  /* heigth and distance midpoints */
  h = ((int)Range[b] + (int)Range[a]) / 2;
  d = (b - a) / 2;

  /* if not too small distance, add random to the height */
  if(d >= STRAIGHTS)
  {
    int z;
    z  = MIN(h, d);
    h += (2L * z * RND(HEIGHT) / HEIGHT - z) / 2;

    /* limit to the Range type */
    if(h < 0)
      h = 0;
    else if(h > HEIGHT)
      h = HEIGHT;
  }
  Range[a + d] = h;

  /* if range still divisable */
  if(d > 2)
  {
    /* iterate new ranges at both sides of midpoint */
    range_do(a, a + d);
    range_do(b - d, b);
  }
  else
  {
    /* calculate last ones */
    Range[a + 1] = ((int)Range[a] + (int)Range[a + 2]) / 2;
    Range[b - 1] = ((int)Range[b] + (int)Range[b - 2]) / 2;
  }
#undef STRAIGHTS
}

int range_width(void)
{
  return WIDTH;
}

/* range height at x */
int range_y(int x)
{
  return WIN_HEIGHT - Range[x];
}

/* level an area for bunker */
void range_area(int x, int r)
{
  int i, y;

  y = Range[x] - 1;
  if(y < 0)
    y = 0;

  for(i = x-r; i <= x+r; i++)
    if(i >= 0 && i < WIDTH && Range[i] > y)
      Range[i] = y;
}

/* -------------
 * drawing onto range
 */

/* draw a shot onto the range window */
extern void range_shot(int *px, int *py, int nx, int ny)
{
  if(*px != nx || *py != ny)
  {
    /* show only if moved */
    w_setmode(RangeWin, M_CLEAR);
    w_pbox(RangeWin, *px-BOMB_SIZE, *py-BOMB_SIZE, BOMB_SIZE*2+1, BOMB_SIZE*2+1);

    w_setmode(RangeWin, M_DRAW);
    w_pbox(RangeWin, nx-BOMB_SIZE, ny-BOMB_SIZE, BOMB_SIZE*2+1, BOMB_SIZE*2+1);

    *px = nx;
    *py = ny;
  }
}

extern void range_shotoff(int px, int py)
{
  w_setmode(RangeWin, M_CLEAR);
  w_pbox(RangeWin, px-BOMB_SIZE, py-BOMB_SIZE, BOMB_SIZE*2+1, BOMB_SIZE*2+1);
}

/* lower the the range from specified area (co-ordinates are supposed to
 * be in order) by shot size
 */
void range_lower(int x0, int x1)
{
  int idx, size = BOMB_SIZE*2+1;

  x0 -= BOMB_SIZE;
  x1 += BOMB_SIZE;
  if(x0 < 0)
    x0 = 0;
  if(x1 >= WIDTH)
    x1 = WIDTH-1;

  w_setmode(RangeWin, M_DRAW);
  for(idx = x0; idx <= x1; idx++)
  {
    Range[idx] -= size;
    w_vline(RangeWin, idx, WIN_HEIGHT, WIN_HEIGHT - Range[idx]);
  }

  w_setmode(RangeWin, M_CLEAR);
  for(idx = x0; idx <= x1; idx++)
    w_vline(RangeWin, idx, WIN_HEIGHT - Range[idx], WIN_HEIGHT - Range[idx] - size);

  w_flush();
}

/* draw a player bunker onto the range. Border values mean:
 * border < 0: some kind of shield,
 * border = 0: remove bunker,
 * border > 0: bunker border
 */
void range_bunker(int x, int y, int border)
{
  w_setmode(RangeWin, M_CLEAR);
  w_pcircle(RangeWin, x, y, BUNKER_SIZE);
  w_setmode(RangeWin, M_DRAW);

  if(border < 0)
  {
    w_dpcircle(RangeWin, x, y, BUNKER_SIZE-1);
    border = -border;
  }
  while(border--)
    w_circle(RangeWin, x, y, BUNKER_SIZE-1-border);
}


/* dig an explosion pit */
void range_pit(int x, int y, int r, int snd)
{
  long dx, dy, rq = r*r;
  int idx, ry;

  /* play the required sound effect on the background */
  do_sound(snd);

  /* flash the explosion a couple of times */
  w_setmode(RangeWin, M_INVERS);

  for(idx = r-1; idx > 0; idx -= r / 4)
    w_pcircle(RangeWin, x, y, idx);

  for(idx = 0; idx < 4; idx++)
  {
    w_flush();
    usleep(200000);
    w_pcircle(RangeWin, x, y, r-1);
  }

  for(idx = r-1; idx > 0; idx -= r / 4)
    w_pcircle(RangeWin, x, y, idx);

  w_setmode(RangeWin, M_CLEAR);


  /* change window co-ordinates to range co-ordinate */
  y = WIN_HEIGHT - y;

  /* modify the range and make the pit */
  ry = Range[x];
  if(ry >= y - r)
  {
    if(ry > y + r && ry > r + r)
      Range[x] -= r + r;
    else
    {
      if(y > r)
	Range[x] = y - r;
      else
	Range[x] = 0;
    }
    w_vline(RangeWin, x, WIN_HEIGHT - ry, WIN_HEIGHT - Range[x]);
  }

  dx = 0;
  dy = r;
  while (dy)
  {
    /* calculate new y offset for the x offset */
    dx++;
    while (dx*dx + dy*dy > rq)
      dy--;

    /* left and right sides of the pit at center offset dx */
    for(idx = x - dx; idx <= x + dx; idx += dx + dx)
    {
      if(idx >= 0 && idx < WIDTH)
      {
	ry = Range[idx];
	if(ry < y - dy)
	  continue;

	if(ry > y + dy && ry > dy + dy)
	  Range[idx] -= dy + dy;
	else
	{
	  if(y > dy)
	    Range[idx] = y - dy;
	  else
	    Range[idx] = 0;
	}
        /* lower the range top at position i */
        w_vline(RangeWin, idx, WIN_HEIGHT - ry, WIN_HEIGHT - Range[idx]);
      }
    }
  }

  w_setmode(RangeWin, M_DRAW);
  w_flush();
}

