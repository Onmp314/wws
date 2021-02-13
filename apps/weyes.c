/*
 * weyes.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- show eyes that follow the mouse pointer
 *
 * CHANGES
 * 11/97, ++eero:
 * - resizable
 * - may be quit with 'q' key
 * - uses ellipses instead of circles
 * - eye highlights
 * - may be put on root
 * 12/97, jps:
 * - adapted to the swallow feature of wlaunch
 * - added long options
 * 2/98, ++eero:
 * - replaced trigonometrics with integer square root
 */

#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>
#include <getopt.h>


static struct option long_options [] = {
  { "geometry", required_argument, NULL, 'g' },
  { "help", no_argument, NULL, 'h' },
  { "parent", required_argument, NULL, 'p'},
  { "root", no_argument, NULL, 'r' },
  { NULL, 0, NULL, 0}
};

#define MIN_XSIZE 48
#define MIN_YSIZE 48

typedef struct {
  short midx, midy;
  short pupx, pupy;
} EYE;


static WWIN *win;
static short rootw, rooth, winw, winh, rx, ry, rxpup, rypup;
static EYE lefteye, righteye;
static int use_root;


static void updateeye(EYE *eye)
{
  short dx, dy, scale;

  w_querymousepos(win, &dx, &dy);
  dx -= eye->midx;
  dy -= eye->midy;

  scale = isqrt((ulong)dx*dx + (ulong)dy*dy);
  if (scale > rx) {
    dx = eye->midx + (short)((long)dx * rx / scale);
  } else {
    dx += eye->midx;
  }
  if (scale > ry) {
    dy = eye->midy + (short)((long)dy * ry / scale);
  } else {
    dy += eye->midy;
  }

  /* move the pupille */
  if ((dx != eye->pupx) || (dy != eye->pupy)) {
    w_pbox(win, eye->pupx - rxpup/2, eye->pupy - rypup/2, rxpup/3, rypup/3);
    w_pellipse(win, eye->pupx, eye->pupy, rxpup, rypup);
    eye->pupx = dx;
    eye->pupy = dy;
    w_pellipse(win, dx, dy, rxpup, rypup);
    w_pbox(win, dx - rxpup/2, dy - rypup/2, rxpup/3, rypup/3);
  }
}

static void draw_eyes(int xp, int yp)
{
  if (!use_root) {
    w_setmode(win, M_DRAW);
    w_pbox(win, 0, 0, winw, winh);
  }

  rx = winw/4;
  ry = winh/2;
  lefteye.midx = lefteye.pupx = xp + rx;
  lefteye.midy = lefteye.pupy = yp + ry;
  righteye.midx = righteye.pupx = xp + 3*rx;
  righteye.midy = righteye.pupy = yp + ry;

  rx--;
  ry--;
  w_setmode(win, M_CLEAR);
  w_pellipse(win, lefteye.midx, lefteye.midy, rx, ry);
  w_pellipse(win, righteye.midx, righteye.midy, rx, ry);

  if (use_root) {
    w_setmode(win, M_DRAW);
    w_ellipse(win, lefteye.midx, lefteye.midy, rx, ry);
    w_ellipse(win, righteye.midx, righteye.midy, rx, ry);
  }

  rxpup = rx / 4;
  rypup = ry / 4;
  rx -= (rxpup + 2);
  ry -= (rypup + 2);

  w_setmode(win, M_INVERS);
  w_pellipse(win, lefteye.pupx, lefteye.pupy, rxpup, rypup);
  w_pellipse(win, righteye.pupx, righteye.pupy, rxpup, rypup);
  w_pbox(win, lefteye.pupx - rxpup/2, lefteye.pupy - rypup/2, rxpup/3, rypup/3);
  w_pbox(win, righteye.pupx - rxpup/2, righteye.pupy - rypup/2, rxpup/3, rypup/3);
}

static void check_size(short *winw, short *winh)
{
  if (*winw < MIN_XSIZE) {
    *winw = MIN_XSIZE;
  }
  if (*winh < MIN_YSIZE) {
    *winh = MIN_YSIZE;
  }
}



static void usage (char *name) {
  fprintf (stderr, "\
Usage: %s [OPTION]...\n\
A well-known program for a well-known purpose.\n\
\n\
  -g, --geometry             set size and position\n\
  -h, --help                 display this help and exit\n\
  -r, --root                 use root window for eyes\n\
\n\
", name);
}



int main(int argc, char **argv)
{
  short ewinw = MIN_XSIZE;
  short ewinh = MIN_YSIZE;
  short xp = UNDEF;
  short yp = UNDEF;
  short show_usage = 0;
  ulong parentid = 0;
  WWIN *pwin = NULL;
  WSERVER *wserver;
  WEVENT *ev;
  int c;

  while ((c = getopt_long (argc, argv, "g:hrp:", long_options, NULL)) != -1) {
    switch (c) {

      case 'g':
        scan_geometry (optarg, &winw, &winh, &xp, &yp);
        break;

      case 'p':
        sscanf (optarg, "%lx", &parentid);
        break;

      case 'r':
	use_root = 1;
	break;

      default:
        show_usage = 1;
	break;
    }
  }

  if (show_usage) {
    usage (argv[0]);
    return -1;
  }

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: %s can't connect to wserver\n", argv[0]);
    return -1;
  }
  rootw = wserver->width;
  rooth = wserver->height;

  if (parentid) {
    /* have to be connected to W server first */
    pwin = w_winFromID(parentid);
    winw = pwin->width;
    winh = pwin->height;
  }

  check_size(&winw, &winh);

  if (use_root) {
    win = WROOT;
    ewinw = winw + 2;
    ewinh = winh + 2;
    w_setlinewidth(win, 1);	/* just in case */
  } else {
    if (pwin) {
      if (!(win = w_createChild (pwin, winw, winh, W_NOBORDER))) {
        fprintf(stderr, "error: %s can't create child window\n", argv[0]);
        exit(-1);
      }
    }
    else { 
      if (!(win = w_create(winw, winh, W_MOVE | W_RESIZE | EV_KEYS))) {
        fprintf(stderr, "error: %s can't create window\n", argv[0]);
        exit(-1);
      }
    }
    w_querywinsize(win, 1, &ewinw, &ewinh);
  }

  if (xp < 0 && xp != UNDEF) {
    xp = rootw - ewinw - xp;
  }
  if (yp < 0 && yp != UNDEF) {
    yp = rooth - ewinh - yp;
  }

  if (use_root) {
    if (xp == UNDEF || yp == UNDEF) {
      xp = 0;
      yp = 0;
    }
    draw_eyes(xp, yp);
  } else {
    draw_eyes(0, 0);
    if (pwin) {
      xp = yp = 0;
    }
    if (w_open(win, xp, yp) < 0) {
      fprintf(stderr, "error: %s can't open window\n", argv[0]);
      w_exit();
      return -1;
    }
  }

  while (42) {

    updateeye(&lefteye);
    updateeye(&righteye);

    /* watch out for `exit' event */
    ev = w_queryevent(NULL, NULL, NULL, 200);
    if(!ev)
      continue;

    switch(ev->type)
    {
      case EVENT_GADGET:
      case EVENT_KEY:
        if (ev->key != GADGET_EXIT && ev->key != 'q')
	  break;
	w_exit();
	return 0;

      case EVENT_RESIZE:
	winw = ev->w;
	winh = ev->h;
	check_size(&winw, &winh);
	w_move (ev->win, ev->x, ev->y);
	w_resize (ev->win, winw, winh);

	/* updates global size variables too */
	draw_eyes(0, 0);
	break;
    }
  }
}
