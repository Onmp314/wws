/*
 * lines.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- WSaver module: 'lines'
 *
 * CHANGES
 * 3/98, ++eero:
 * - changed into a module for the new wsaver.
 * - decreased number of lines, increased their distance and changed mode to
 *   M_INVERS.  I like screensavers which keep screen relatively dark.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Wlib.h>
#include "wsaver.h"


#define	MAXLINES 32
#define OFFSET	15	/* 2^x - 1 */

typedef struct {
  short	x0, y0, xe, ye;
} LINE;


static LINE line[MAXLINES];
static short idx;
static short x0, y0, xe, ye;
static short dx0, dy0, dxe, dye;


static void movelines(void)
{
  /* delete the old line */
  /* w_setmode(win, M_DRAW); */
  w_line(win, line[idx].x0, line[idx].y0, line[idx].xe, line[idx].ye);

  /* calculate new pos */
  x0 += dx0;
  if (x0 < 0) {
    x0 = -x0;
    dx0 = (random() & OFFSET) + 1;
  }
  if (x0 >= swidth) {
    x0 = 2 * swidth - x0 - 1;
    dx0 = - (random() & OFFSET) - 1;
  }

  y0 += dy0;
  if (y0 < 0) {
    y0 = -y0;
    dy0 = (random() & OFFSET) + 1;
  }
  if (y0 >= sheight) {
    y0 = 2 * sheight - y0 - 1;
    dy0 = - (random() & OFFSET) - 1;
  }

  xe += dxe;
  if (xe < 0) {
    xe = -xe;
    dxe = (random() & OFFSET) + 1;
  }
  if (xe >= swidth) {
    xe = 2 * swidth - xe - 1;
    dxe = - (random() & OFFSET) - 1;
  }

  ye += dye;
  if (ye < 0) {
    ye = -ye;
    dye = (random() & OFFSET) + 1;
  }
  if (ye >= sheight) {
    ye = 2 * sheight - ye - 1;
    dye = - (random() & OFFSET) - 1;
  }

  /* then draw new line */
  line[idx].x0 = x0;
  line[idx].y0 = y0;
  line[idx].xe = xe;
  line[idx].ye = ye;
  /* w_setmode(win, M_CLEAR); */
  w_line(win, line[idx].x0, line[idx].y0, line[idx].xe, line[idx].ye);

  if (++idx == MAXLINES) {
    idx = 0;
  }
}


void save_lines(void)
{
  WEVENT *ev;
  short i;

  idx = 0;
  x0 = (xe = (swidth >> 1));
  y0 = (ye = (sheight >> 1));

  /* w_setmode(win, M_CLEAR); */
  w_setmode(win, M_INVERS);
  for (i=0; i<MAXLINES; i++) {
    line[i].x0 = x0;
    line[i].y0 = y0;
    line[i].xe = xe;
    line[i].ye = ye;
    w_line(win, line[i].x0, line[i].y0, line[i].xe, line[i].ye);
  }

  dx0 = (random() & OFFSET) + 1;
  if (random() & 1) dx0 = -dx0;
  dy0 = (random() & OFFSET) + 1;
  if (random() & 1) dy0 = -dy0;
  dxe = (random() & OFFSET) + 1;
  if (random() & 1) dxe = -dxe;
  dye = (random() & OFFSET) + 1;
  if (random() & 1) dye = -dye;

  while (42) {

    if ((ev = w_queryevent(NULL, NULL, NULL, timeout))) switch (ev->type) {

    case EVENT_GADGET:
      if (ev->key == GADGET_EXIT) {
	w_exit();
	exit(0);
      }
      break;

    default:
      return;
    }

    movelines();
  }
}
