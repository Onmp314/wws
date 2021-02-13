/*
 * wapfel.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a shallow (due to fixed point accuracy) mandelbrot set viewer
 * 
 * CHANGES
 * ++kay, 10/94:
 * - use fixed point aritmetik
 * - implemented solid guessing/recursive subdivision algorithm
 *   (on my poor MegaST these changes speed wapfel up by a factor
 *   of 10 or more :)
 * - TeSche: Not only on your MegaST, Kay. :)
 * ++TeSche 10/94:
 * - use new method to specify rectangle to zoom
 * ++TeSche, 1/96:
 * - use new button concept
 * ++Eero, 10/97:
 * - use w_buttonqueryevent() instead of w_queryevent()
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <Wlib.h>

#define	YOFFSET	16


/*
 * We are going to use recursion and MiNT has fixed size stack...
 */
#ifdef __MINT__
long _stksize = 8000;
#endif


/*
 * Initial solid guessing block size. With
 *	X(i)   = 3*2^i + 1,	for i >= 1
 * BX_HT = X(i) for arbitrary i >= 1.
 */

#define BX_HT 25


/*
 * Picture width and heigth. With
 *	Y(i)	= i*(BX_HT-1) + 1,	for i >= 1
 * SC_WD = Y(v) for arbitrary v >= 1,
 * SC_HT = Y(w) for arbitrary w >= 1.
 */

#define SC_WD 337
#define SC_HT 217


/*
 * Fixed point stuff
 */

#define FIXSHIFT 12
#define FLT2FIX(x) ((long)((x)*(1L << FIXSHIFT)))
#define FIXADD(a,b) ((a) + (b))
#define FIXSUB(a,b) ((a) - (b))
#define FIXMUL(a,b) (((a) * (b)) >> FIXSHIFT)
#define FIXDIV(a,b) (((a) << FIXSHIFT) / (b))
#define COL(x,y) (lrow[(y)][(x)])

static long xstart, xlen, ystart, ylen;
static short lrow[BX_HT][512];
static short glob_wwidth = SC_WD;
static short glob_wheight = SC_HT + YOFFSET;
static WWIN *glob_win, *buttonExit, *buttonCalc, *buttonAbort;
static WFONT *glob_font;


/*
 *
 */

static void getwindow(void)
{
  WSERVER *wserver;

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: can't connect to server\n");
    exit(-1);
  }

  if (!(glob_font = w_loadfont("fixed", 7, 0))) {
    fprintf(stderr, "error: can't load font\n");
    exit(-1);
  }

  if (!(glob_win = w_create(glob_wwidth, glob_wheight,
			    W_MOVE | W_TITLE | W_CLOSE | EV_MOUSE))) {
    fprintf(stderr, "error: can't create window\n");
    exit(-1);
  }

  w_settitle(glob_win, " wapfel ");

  if (w_open(glob_win, UNDEF, UNDEF)) {
    w_delete(glob_win);
    fprintf(stderr, "error: can't open window\n");
    exit(-1);
  }

  w_setfont(glob_win, glob_font);
}


static void mandel_init (x0, xe, y0, ye)
     double x0, xe, y0, ye;
{
  xstart = FLT2FIX (x0);
  ystart = FLT2FIX (y0);
  xlen = FLT2FIX (xe-x0);
  ylen = FLT2FIX (ye-y0);
}


static inline short mandel (short x, short y, short max)
{
  short iter;
  long four, cx, cy, y1, y2, x1, x2;

  iter = 0;
  four = FLT2FIX (4);
  y1 = y2 = x1 = x2 = 0;
  cx = FIXADD (xstart, xlen*x/SC_WD);
  cy = FIXADD (ystart, ylen*y/SC_HT);
  do {
    y1 = FIXADD (FIXMUL (x1, y1) << 1, cy);
    x1 = FIXADD (FIXSUB (x2, y2), cx);
    x2 = FIXMUL (x1, x1);
    y2 = FIXMUL (y1, y1);
  } while (++iter < max && FIXADD (x2, y2) < four);
  return iter;
}


static void split (short x, short y, short y0, short w, short max)
{
  short i, j, col, w2;

  col = COL (x, y);
  for (i = 0; i < w-1; ++i) {
    if (COL (x+i, y) != col || COL (x+w-1, y+i) != col ||
	COL (x+i+1, y+w-1) != col || COL (x, y+i+1) != col)
      goto cantfill;
  }
  if (col & 1)
    w_pbox(glob_win, x+1, y0+y+1+YOFFSET, w-2, w-2);
  return;

 cantfill:
  if (w <= 4) {
    y0 += y;
    for (i = w-2; i >= 1; --i) {
      for (j = w-2; j >= 1; --j) {
	if (mandel (x+i, y0+j, max) & 1)
	  w_plot(glob_win, x+i, y0+j+YOFFSET);
      }
    }
    return;
  }
  w2 = w >> 1;
  for (i = w-2; i >= 1; --i) {
    COL (x+i, y+w2) = mandel (x+i, y0+y+w2, max);
    if (COL (x+i, y+w2) & 1)
      w_plot (glob_win, x+i, y0+y+w2+YOFFSET);
    if (i != w2) {
      COL (x+w2, y+i) = mandel (x+w2, y0+y+i, max);
      if (COL (x+w2, y+i) & 1)
	w_plot (glob_win, x+w2, y0+y+i+YOFFSET);
    }
  }
  split (x   , y   , y0, w2+1, max);
  split (x+w2, y   , y0, w2+1, max);
  split (x   , y+w2, y0, w2+1, max);
  split (x+w2, y+w2, y0, w2+1, max);
}


static WEVENT *getevent(long timeout)
{
  WEVENT *ev;

  if ((ev = w_querybuttonevent(NULL, NULL, NULL, timeout))) {
    if ((ev->type == EVENT_GADGET) &&
	((ev->key == GADGET_EXIT) || (ev->key == GADGET_CLOSE))) {
      w_delete(glob_win);
      exit(0);
    }
  }

  return ev;
}


/*
 * a fractal routine...
 */

static void apfel(double x0, double xe, double y0, double ye, short max)
{
  short i, x, y;
  char buf[80];
  WEVENT *wevent;

  w_hideButton(buttonExit);
  w_hideButton(buttonCalc);

  w_setmode(glob_win, M_CLEAR);
  w_pbox(glob_win, 0, 0, glob_wwidth, glob_wheight);
  w_setmode(glob_win, M_DRAW);

  w_showButton(buttonAbort);

  mandel_init (x0, xe, y0, ye);

  /*
   * first line
   */
  for (i = 0; i < SC_WD; i++) {
    COL (i, 0) = mandel (i, 0, max);
    if (COL (i, 0) & 1)
      w_plot(glob_win, i, YOFFSET);
  }
  for (y = 0; y <= SC_HT-BX_HT; y += BX_HT-1) {
    /*
     * leftmost column
     */
    for (i = 1; i < BX_HT-1; ++i) {
      COL (0, i) = mandel (0, y+i, max);
      if (COL (0, i) & 1)
	w_plot (glob_win, 0, y+i+YOFFSET);
    }
    /*
     * last line
     */
    for (i = 0; i < SC_WD; ++i) {
      COL (i, BX_HT-1) = mandel (i, y+BX_HT-1, max);
      if (COL (i, BX_HT-1) & 1)
	w_plot(glob_win, i, y+BX_HT-1+YOFFSET);
    }
    for (x = 0; x <= SC_WD-BX_HT; x += BX_HT-1) {
      /*
       * rightmost column
       */
      for (i = 1; i < BX_HT-1; ++i) {
	COL (x+BX_HT-1, i) = mandel (x+BX_HT-1, y+i, max);
	if (COL (x+BX_HT-1, i) & 1)
	  w_plot (glob_win, x+BX_HT-1, y+i+YOFFSET);
      }
      split (x, 0, y, BX_HT, max);
    }
    sprintf(buf, "calculating %i%%", 100*(y+BX_HT)/SC_HT);
    w_printstring(glob_win, 0, 0, buf);

    if ((wevent = getevent(0))) {
      if ((wevent->type == EVENT_BUTTON) && (wevent->win == buttonAbort)) {
	return;
      }
    }

    memcpy (&COL (0, 0), &COL (0, BX_HT-1), SC_WD*2);
  }
}


static int installButtons()
{
  buttonExit =  w_createButton(glob_win,          0, 0, 70, YOFFSET - 8);
  buttonCalc =  w_createButton(glob_win,         80, 0, 70, YOFFSET - 8);
  buttonAbort = w_createButton(glob_win, SC_WD - 78, 0, 70, YOFFSET - 8);

  if (!buttonExit || !buttonCalc || !buttonAbort) {
    return -1;
  }

  w_centerPrints(buttonExit, glob_font, "exit");
  w_centerPrints(buttonCalc, glob_font, "calculate");
  w_centerPrints(buttonAbort, glob_font, "abort");

  return 0;
}


/*
 * guess what...
 */

int main(void)
{
  short end, in_menu;
  WEVENT *wevent;
  double x0, y0, xe, ye, dx, dy;
  short cx0, cy0, cxe, cye, cflag, mx, my;
  short setul, setlr;
  short state;

  getwindow();

  if (installButtons() < 0) {
    return -1;
  }

  cx0 = 0;
  cxe = SC_WD-1;
  cy0 = 0;
  cye = SC_HT-1;

  x0 = -2.0;
  xe =  1.0;
  y0 = -1.0;
  ye =  1.0;

  end = 0;
  setul = 0;
  setlr = 0;

  do {
    dx = (xe - x0) / (SC_WD-1);
    dy = (ye - y0) / (SC_HT-1);

    xe = x0 + cxe * dx;
    x0 = x0 + cx0 * dx;
    ye = y0 + cye * dy;
    y0 = y0 + cy0 * dy;

    cx0 = 0;
    cxe = SC_WD-1;
    cy0 = 0;
    cye = SC_HT-1;

    apfel(x0, xe, y0, ye, 64);

    w_hideButton(buttonAbort);

    w_setmode(glob_win, M_CLEAR);
    w_pbox(glob_win, 0, 0, glob_wwidth, YOFFSET);
    w_setmode(glob_win, M_DRAW);

    w_showButton(buttonCalc);
    w_showButton(buttonExit);

    cflag = 0;
    in_menu = 1;
    w_setmode(glob_win, M_INVERS);

    state = 0;
    while (in_menu) {

      if ((wevent = getevent(100))) switch (wevent->type) {

        case EVENT_BUTTON:
	  if (wevent->win == buttonCalc) {
	    /* calculate this */
	    in_menu = 0;
	  } else if (wevent->win == buttonExit) {
	    /* exit program */
	    in_menu = 0;
	    end = 1;
	  }
	  break;

	case EVENT_MPRESS:
	  if (wevent->key & BUTTON_LEFT) {
	    if (state == 2) {
	      w_box(glob_win, cx0, cy0, cxe-cx0+1, cye-cy0+1);
	    }
	    state = 1;
	    cxe = cx0 = wevent->x;
	    cye = cy0 = wevent->y;
	    w_box(glob_win, cx0, cy0, cxe-cx0+1, cye-cy0+1);
	  }
	  break;

	case EVENT_MRELEASE:
	  if (wevent->key == BUTTON_LEFT) {
	    if ((wevent->x > 0) && (wevent->y > 0)) {
	      w_box(glob_win, cx0, cy0, cxe-cx0+1, cye-cy0+1);
	      cxe = wevent->x;
	      cye = wevent->y;
	      state = 2;
	    } else {
	      state = 0;
	    }
	    w_box(glob_win, cx0, cy0, cxe-cx0+1, cye-cy0+1);
	  }
	  break;
      }

      if (state == 1) if (!w_querymousepos(glob_win, &mx, &my)) {

	if ((mx != cxe) || (my != cye)) {
	  w_box(glob_win, cx0, cy0, cxe-cx0+1, cye-cy0+1);
	  cxe = mx;
	  cye = my;
	  w_box(glob_win, cx0, cy0, cxe-cx0+1, cye-cy0+1);
	}
      }

    } while (in_menu);

  } while (!end);

  w_delete(buttonAbort);
  w_delete(buttonCalc);
  w_delete(buttonExit);
  w_delete(glob_win);

  return 0;
}
