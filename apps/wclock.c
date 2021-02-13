/*
 * wclock.c, a part of the W Window System
 *
 * Copyright (C) 1994-1996 by Torsten Scherer and
 * Copyright (C) 1995-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- an analog clock
 *
 * CHANGES
 * 12/94, TeSche:
 * - Changed for W1R0
 * 11/95, Eero:
 * - Uses a lookup table instead of sin & cos
 * - calculations in fixed point instead of floats
 * - Clock face scales to the window size
 * - Shows only hours and minutes
 * 96, TeSche:
 * - adapted for W1R2
 * 11/97, Eero:
 * - resizable
 * 12/97, jps:
 * - adapted to the swallow feature of wlaunch
 * - added long options
 * 2/98, Eero:
 * - clock hand widths and middle 'stone' size according to clock size.
 * - root and bg pattern options. You can't have 'transparent' clock
 *   with root option because we have to use M_DRAW mode.
 * 6/98, Eero:
 * - background color can be changed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <Wlib.h>
#include <getopt.h>


static struct option long_options [] = {
  { "bg", required_argument, NULL, 'b' },
  { "color", required_argument, NULL, 'c' },
  { "geometry", required_argument, NULL, 'g' },
  { "parent", required_argument, NULL, 'p' },
  { "root", no_argument, NULL, 'r' },
  { "help", no_argument, NULL, 'h' },
  { NULL, 0, NULL, 0}
};

/* window minimum / default size */
#define MIN_SIZE	30
#define DEF_SIZE	80

#define PROPERTIES	(W_MOVE | W_RESIZE | EV_KEYS)


typedef struct {
  short	hxe, hye;
  short	mxe, mye;
} POINTERS;


static WWIN *window;
static short midx, midy;
static short maxxr, maxyr;
static short dot;


/* offset and draw clock pointers */
static void draw(POINTERS *p)
{
  w_pbox(window, midx - (dot>>1), midy - (dot>>1), dot+1, dot+1);
  w_line(window, midx - (p->hxe >> 2), midy - (p->hye >> 2),
         midx + p->hxe, midy + p->hye);
  w_line(window, midx - (p->mxe >> 3), midy - (p->mye >> 3),
         midx + p->mxe, midy + p->mye);
}

/* offset and clear the clock pointers */
static void clear(POINTERS *p)
{
  w_dline(window, midx - (p->hxe >> 2), midy - (p->hye >> 2),
          midx + p->hxe, midy + p->hye);
  w_dline(window, midx - (p->mxe >> 3), midy - (p->mye >> 3),
          midx + p->mxe, midy + p->mye);
}

/* calculate clock pointer offsets from the middle */
static void offset(short *x, short *y, short sixty)
{
  short sini[16] =			/* sin(2*PI / 60 * sixty) * (1<<14) */
  {
        0,  1713,  3406,  5063,  6664,  8192,  9630, 10963,
    12176, 13255, 14189, 14968, 15582, 16026, 16294, 16384
  };
  short xx, yy, index = sixty % 15;

  switch(sixty / 15)
  {
    case 3:
      xx = -sini[15-index];
      yy = -sini[index];
      break;
    case 2:
      xx = -sini[index];
      yy = sini[15-index];
      break;
    case 1:
      xx = sini[15-index];
      yy = sini[index];
      break;
    default:
      xx = sini[index];
      yy = -sini[15-index];
  }
  *x = ((long)xx * maxxr) >> 14;
  *y = ((long)yy * maxyr) >> 14;
}

/* update clock pointers */
static void update(struct tm *loc)
{
  static POINTERS p = {0, 0, 0, 0};

  /* clear old pointers */
  clear(&p);

  /* offset from the middle */
  offset(&p.hxe, &p.hye, (loc->tm_hour % 12) * 5 + (loc->tm_min + 6) / 12);
  offset(&p.mxe, &p.mye, loc->tm_min);

  /* shorten hour pointer */
  p.hxe = p.hxe * 2 / 3;
  p.hye = p.hye * 2 / 3;

  /* draw new pointers */
  draw(&p);
}

/* set globals and draw the clock 'numbers' */
static void draw_clock(short width, short height, short color, ushort pattern)
{
/* half_size / x offsets */
#define P_DIST   6
#define N_DIST  12	
  short i, xp, yp, size, skip;

  midx = width / 2;
  midy = height / 2;
  maxxr = midx - midx / N_DIST;
  maxyr = midy - midy / N_DIST;

  size = MIN(midx, midy) / 40;

  if (size > 1) {
    w_setlinewidth(window, (size<<1) + 1);
    skip = 1;
  } else {
    w_setlinewidth(window, 1);
    pattern = MAX_GRAYSCALES;	/* white bg */
    skip = 5;
  }

  if (color > 0) {
    w_setBackgroundColor(window, color);
  }
  w_setpattern(window, pattern);
  w_dpbox(window, 0, 0, width, height);

  for (i = 0; i < 60; i += skip)
  {
    offset(&xp, &yp, i);
    xp += midx;
    yp += midy;
    if(size)
    {
      if (i % 5) {
        /* minutes between... */
	w_pbox(window, xp - (size>>1), yp - (size>>1), size, size);
      } else {
        /* ...hours */
	w_pbox(window, xp - size, yp - size, (size<<1), (size<<1));
      }
    }
    else
      w_plot(window, xp, yp);
  }
  maxxr -= midx / P_DIST;
  maxyr -= midy / P_DIST;
  /* getting this comfortable with linewidth at all clock sizes really took
   * some experimenting...
   */
  dot = (size*size>>1) + (size<<1) + 2;
}



static void check_size(short *width, short *height)
{
  /* limit size */
  if (*width < MIN_SIZE) {
    if (*width <= 0)
      *width = DEF_SIZE;
    else
      *width = MIN_SIZE;
  }
  if (*height < MIN_SIZE) {
    if(*height <= 0)
      *height = DEF_SIZE;
    else
      *height = MIN_SIZE;
  }
}

/* open window */
static int open_clock (char *name, short width, short height,
                       short xp, short yp, WWIN *parent)
{
  if (parent) {
    if (!(window = w_createChild (parent, width, height, W_NOBORDER))) {
      fprintf(stderr, "%s: can't create child window\n", name);
      return -1;
    }
  } else {
    if (!(window = w_create(width, height, PROPERTIES))) {
      fprintf(stderr, "%s: can't create window\n", name);
      return -1;
    }
    limit2screen(window, &xp, &yp);
  }

  if (w_open(window, xp, yp) < 0)
  {
    fprintf(stderr, "%s: can't open window\n", name);
    w_exit();
    return -1;
  }

  return 0;
}

static void terminate()
{
  w_exit();
  exit(0);
}



static void usage (char *name) {
  fprintf (stderr, "\
Usage: %s [OPTION]...\n\
The program that lets you know what time it is.\n\
\n\
  -b, --bg         background pattern id (hex)\n\
  -c, --color      background color index\n\
  -g, --geometry   set size and position\n\
  -r, --root       draw clock to the root window\n\
  -h, --help       display this help and exit\n\
\n\
", name);
}


int main(int argc, char *argv[])
{
  short	width = 0;
  short height = 0;
  short xp = UNDEF;
  short yp = UNDEF;
  short color = 0;			/* default bg color */
  ushort pattern = MAX_GRAYSCALES;		/* white bg */
  short use_root = 0, show_usage = 0;
  ulong parentid = 0;
  WWIN *parent = NULL;
  WEVENT *ev;
  struct tm *loc;
  long t;
  int c;

  while ((c = getopt_long (argc, argv, "b:c:g:p:rh", long_options, NULL)) != -1) {
    switch (c) {

      case 'g':
        scan_geometry (optarg, &width, &height, &xp, &yp);
        break;

      case 'b':
        sscanf (optarg, "%hx", &pattern);
        break;

      case 'p':
        sscanf (optarg, "%lx", &parentid);
        break;

      case 'c':
        color = atoi(optarg);
	break;

      case 'r':
        use_root = 1;
	break;

      default:
        show_usage = 1;
    }
  }

  if (show_usage) {
    usage (argv[0]);
    exit (0);
  }

  if (!w_init()) {
    fprintf (stderr, "%s: can't connect to wserver\n", argv[0]);
    exit (1);
  }

  if (use_root) {
    window = WROOT;
    width = window->width;
    height = window->height;
  } else {

    if (parentid) {
      xp = yp = 0;
      parent = w_winFromID(parentid);
      height = parent->height;
      width = parent->width;
    } else {
      check_size(&width, &height);
    }

    if (open_clock (argv[0], width, height, xp, yp, parent)) {
      return -1;
    }
  }

  w_setmode(window, M_DRAW);
  draw_clock(width, height, color, pattern);

  signal(SIGTTOU, SIG_IGN);

  /* set exit handlers (one time only, so won't need 'reliable' ones) */
  signal(SIGHUP,  terminate);
  signal(SIGINT,  terminate);
  signal(SIGQUIT, terminate);
  signal(SIGABRT, terminate);
  signal(SIGTERM, terminate);

  while(42)
  {
    t = time(0);
    loc = localtime(&t);
    update(loc);

    /* watch out for 'q' / `exit' event while idling a min */
    ev = w_queryevent(NULL, NULL, NULL, 60000);
    if(!ev)
      continue;

    switch(ev->type)
    {
      case EVENT_GADGET:
        if (ev->key == GADGET_EXIT)
	  terminate();
	break;

      case EVENT_KEY:
        if (ev->key == 'q')
	  terminate();
	break;

      case EVENT_RESIZE:
	width = ev->w;
	height = ev->h;
	check_size(&width, &height);
	w_move (ev->win, ev->x, ev->y);
	w_resize (ev->win, width, height);

	/* updates global size variables too */
	draw_clock(width, height, color, pattern);
	break;
    }
  }
  return 0;
}
