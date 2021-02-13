/*
  HOPALONG (w) 1994 (Gfa Basic), 1995 (Grafix-lib), 1996 (W toolkit)
  by Eero Tamminen

  Idea from the A. K. Dewney's book 'Armchair Universe'.

  Under MiNT this can be compiled with something like:
  	gcc -O2 hopalong.c -o whop -lWt -lW -lsocket -lm

  Because this uses slow floating point calculations, a faster machine
  (preferably with FPU) is recommmended.
*/

#include <math.h>		/* sqrt() */
#include <stdio.h>
#include <stdlib.h>		/* atoi(), atof() */
#include <Wlib.h>
#include <Wt.h>

#define VERSION "v0.3"

#define DEF_FIRSTCOL	2	/* skip window colors */
#define DEF_WIDTH	200
#define DEF_HEIGHT	200
#define DEF_POINTS	"200000"
#define DEF_ZOOM	"0.0"
#define DEF_A		"13.0"
#define DEF_B		"9.0"
#define DEF_C		"55.0"

#ifndef SGN
#define SGN(x)	((x) < 0 ? -1 : 1)
#endif
#ifndef ABS
#define ABS(x)	((x) < 0 ? -(x) : (x))
#endif

static struct
{
  WWIN *win;
  long points;			/* how many points left to plot */
  int change;			/* color change count */
  int color;			/* current color */
  int skip;			/* current count */
  int mx;			/* center point */
  int my;
  double x;			/* center offsets for the current dot */
  double y;
  double zoom;			/* magnification factor */
  double a;			/* parameters */
  double b;
  double c;
} Par;

#define POINTS	20		/* how many points at the time */
#define TIMEOUT	20		/* how many ms between them */

static widget_t
  *Top,
  *Draw,
  *String[5];

/* function prototypes */
static void quit_cb(widget_t *w);
static void close_cb(widget_t *w);
static void calc_cb(widget_t *w, int pressed);
static void draw_fn(widget_t *w, int x, int y, int wd, int ht);
static void timer_cb(long w);
static void hopalong(long points);


/* main program */
int main(int argc, char *argv[])
{
  widget_t *par, *pane, *hpane, *points, *zoom, *values, *calc, *quit;
  long a;
  int i;

  Top = wt_init();

  par  = wt_create(wt_shell_class, Top);
  pane = wt_create(wt_pane_class, par);

  points    = wt_create(wt_label_class, pane);
  String[0] = wt_create(wt_getstring_class, pane);
  zoom      = wt_create(wt_label_class, pane);
  String[1] = wt_create(wt_getstring_class, pane);
  values    = wt_create(wt_label_class, pane);
  String[2] = wt_create(wt_getstring_class, pane);
  String[3] = wt_create(wt_getstring_class, pane);
  String[4] = wt_create(wt_getstring_class, pane);

  hpane = wt_create(wt_pane_class, pane);
  calc  = wt_create(wt_button_class, hpane);
  quit  = wt_create(wt_button_class, hpane);

  wt_setopt(par, WT_LABEL, " W-Hopalong " VERSION " ", WT_EOL);

  wt_setopt(points, WT_LABEL, "Points to plot:", WT_EOL);
  wt_setopt(zoom,   WT_LABEL, "Magnification factor:", WT_EOL);
  wt_setopt(values, WT_LABEL, "Hopalong parameters:", WT_EOL);

  wt_setopt(calc,
    WT_LABEL, "Calculate",
    WT_ACTION_CB, calc_cb,
    WT_EOL);

  wt_setopt(quit,
    WT_LABEL, "Quit",
    WT_ACTION_CB, quit_cb,
    WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);

  a = 6;
  wt_setopt(String[0],
    WT_STRING_LENGTH, &a,
    WT_STRING_MASK, "0-9",
    WT_EOL);

  for(i = 1; i < 5; i++)
  {
    wt_setopt(String[i],
      WT_STRING_LENGTH, &a,
      WT_STRING_MASK, "0-9.",
      WT_EOL);
  }
  wt_setopt(String[0], WT_STRING_ADDRESS, DEF_POINTS, WT_EOL);
  wt_setopt(String[1], WT_STRING_ADDRESS, DEF_ZOOM, WT_EOL);
  wt_setopt(String[2], WT_STRING_ADDRESS, DEF_A, WT_EOL);
  wt_setopt(String[3], WT_STRING_ADDRESS, DEF_B, WT_EOL);
  wt_setopt(String[4], WT_STRING_ADDRESS, DEF_C, WT_EOL);

  wt_realize(Top);
  wt_run();
  return 0;
}

static void quit_cb(widget_t *w)
{
   wt_break(1);
}

static void close_cb(widget_t *w)
{
  wt_close(w);  
}

static void calc_cb(widget_t *w, int pressed)
{
  widget_t *draw;
  double arg;
  char *str;
  long a, b;

  if(pressed)
    return;

  /* get / set the new parameters */

  wt_getopt(String[0], WT_STRING_ADDRESS, &str, WT_EOL);
  a = atoi(str);
  if(a > 0)
    Par.points = a;

  /* calculate how often color should be changed */
  if (wt_global.screen_shared > DEF_FIRSTCOL) {
    Par.color = DEF_FIRSTCOL;
    w_setForegroundColor(Par.win, Par.color);
    Par.change = Par.points / POINTS;
    Par.change /= wt_global.screen_shared - DEF_FIRSTCOL + 1;
  }
  else
    Par.change = 0;


  wt_getopt(String[1], WT_STRING_ADDRESS, &str, WT_EOL);
  Par.zoom = atof(str);

  wt_getopt(String[2], WT_STRING_ADDRESS, &str, WT_EOL);
  arg = atof(str);
  if(arg > 0.0)
    Par.a = arg;

  wt_getopt(String[3], WT_STRING_ADDRESS, &str, WT_EOL);
  arg = atof(str);
  if(arg > 0.0)
    Par.b = arg;

  wt_getopt(String[4], WT_STRING_ADDRESS, &str, WT_EOL);
  arg = atof(str);
  if(arg > 0.0)
    Par.c = arg;

  /* starting point */
  Par.x = 0.0;
  Par.y = 0.0;

  /* window size */
  a = DEF_WIDTH;
  b = DEF_HEIGHT;

  /* open the output window */

  if(!Draw)
  {
    Draw = wt_create(wt_shell_class, Top);
    draw = wt_create(wt_drawable_class, Draw);

    wt_setopt(Draw,
      WT_LABEL, "Hopalong!",
      WT_ACTION_CB, close_cb,
      WT_EOL);

    wt_setopt(draw,
      WT_WIDTH, &a,
      WT_HEIGHT, &b,
      WT_DRAW_FN, draw_fn,
      WT_EOL);

    wt_realize(Top);
  }
  else
  {
    w_setmode(Par.win, M_CLEAR);
    w_pbox(Par.win, 0, 0, a, b);
    w_setmode(Par.win, M_DRAW);
    wt_open(Draw);
  }

  wt_addtimeout(TIMEOUT, timer_cb, 0L);
}

static void draw_fn(widget_t *w, int x, int y, int wd, int ht)
{
  Par.win = wt_widget2win(w);
  w_setmode(Par.win, M_DRAW);

  Par.mx = wd / 2;
  Par.my = ht / 2;

  /* zoom factor */
  if(Par.zoom <= 0.0)
  {
    char buf[12];
    Par.zoom = (wd + ht) / 400.0;
    sprintf(buf, "%g", Par.zoom);
    wt_setopt(String[1],
      WT_STRING_ADDRESS, buf,
      WT_EOL);
  }
}

static void timer_cb(long w)
{
  if(Par.points > POINTS)
  {
    hopalong(POINTS);
    Par.points -= POINTS;
    wt_addtimeout(TIMEOUT, timer_cb, w);
    if (Par.change && --Par.skip < 0) {
      Par.skip = Par.change;
      w_setForegroundColor(Par.win, Par.color++);
    }
  }
  else
  {
    hopalong(Par.points);
    Par.points = 0;
  }
}

static void hopalong(long points)
{
  double xx, yy;

  while(points--)
  {
    /* plot the dot where we 'hopped' */
    w_plot(Par.win,
      Par.mx + (int)(Par.x * Par.zoom),
      Par.my + (int)(Par.y * Par.zoom));

    /* calculate new offset */
    xx = Par.b * Par.x - Par.c;
    xx = Par.y - SGN(Par.x) * sqrt(ABS(xx));
    yy = Par.a - Par.x;

    Par.x = xx;
    Par.y = yy;
  }
}

