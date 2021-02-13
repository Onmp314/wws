/*
 * misc.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- miscellaneous tool functions
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"


static GCONTEXT drawgc   = { M_DRAW, F_NORMAL, 1, &glob_font[TITLEFONT],
			     DefaultPattern, FGCOL_INDEX, BGCOL_INDEX };
static GCONTEXT cleargc  = { M_CLEAR, F_NORMAL, 1, &glob_font[TITLEFONT],
			     DefaultPattern, FGCOL_INDEX, BGCOL_INDEX };
static GCONTEXT inversgc = { M_INVERS, F_NORMAL, 1, &glob_font[TITLEFONT],
			     DefaultPattern, FGCOL_INDEX, BGCOL_INDEX };

/* these could be used on other functions.  Their contents shouldn't
 * be modified.
 */
GCONTEXT *glob_drawgc = &drawgc;
GCONTEXT *glob_cleargc = &cleargc;
GCONTEXT *glob_inversgc = &inversgc;


/*
 * needed for PACKED driver
 */

static ushort fgColMask[8], bgColMask[8];

void init_defmasks(void)
{
  colorGetMask(FGCOL_INDEX, fgColMask);
  colorGetMask(BGCOL_INDEX, bgColMask);
  memcpy (drawgc.bgColMask, bgColMask, sizeof(bgColMask));
  memcpy (drawgc.fgColMask, fgColMask, sizeof(fgColMask));
  memcpy (cleargc.bgColMask, bgColMask, sizeof(bgColMask));
  memcpy (cleargc.fgColMask, fgColMask, sizeof(fgColMask));
  memcpy (inversgc.bgColMask, bgColMask, sizeof(bgColMask));
  memcpy (inversgc.fgColMask, fgColMask, sizeof(fgColMask));
}

/*
 * set up the default GCONTEXT structure for this window
 */
void set_defaultgc(WINDOW *win)
{
  win->gc.drawmode = DEFAULT_GMODE;
  win->gc.linewidth = 1;
  win->gc.textstyle = F_NORMAL;
  win->gc.font = NULL;
  win->gc.bgCol = BGCOL_INDEX;
  win->gc.fgCol = FGCOL_INDEX;
  memcpy (win->gc.bgColMask, bgColMask, sizeof(bgColMask));
  memcpy (win->gc.fgColMask, fgColMask, sizeof(fgColMask));
  memcpy (win->patbuf, DefaultPattern, sizeof(win->patbuf));
  win->gc.pattern = win->patbuf;
}


/*
 * just say yes or no when asking for intersection of
 * two rectangles
 */

short intersect (short x0, short y0, short width0, short height0, short x1, short y1, short width1, short height1)
{
  /* #0 is the mouse, #1 the other rectangle */

  /* is the lower edge of #1 above the upper edge of #0? */
  if (y1+height1-1 < y0)
    return 0;

  /* is the left edge of #1 right from the right edge of #0? */
  if (x1 > x0+width0-1)
    return 0;

  /* is the upper edge of #1 under the lower edge of #0? */
  if (y1 > y0+height0-1)
    return 0;

  /* is the right edge of #1 left from the left edge of #0? */
  if (x1+width1-1 < x0)
    return 0;

  /* must intersect somehow */
  return 1;
}


/*
 * this one calculates an intersection if there's any
 */

REC *wrec_intersect(REC *rec1, REC *rec2)
{
  static REC rec;

  if (rec2->x0 <= rec1->x0) {
    rec.x0 = rec1->x0;
    if ((rec.w = MIN(rec2->x0+rec2->w, rec1->x0+rec1->w) - rec1->x0) <= 0)
      return 0;
  } else {
    rec.x0 = rec2->x0;
    if ((rec.w = MIN(rec2->x0+rec2->w, rec1->x0+rec1->w) - rec2->x0) <= 0)
      return 0;
  }

  if (rec2->y0 <= rec1->y0) {
    rec.y0 = rec1->y0;
    if ((rec.h = MIN(rec2->y0+rec2->h, rec1->y0+rec1->h) - rec1->y0) <= 0)
      return 0;
  } else {
    rec.y0 = rec2->y0;
    if ((rec.h = MIN(rec2->y0+rec2->h, rec1->y0+rec1->h) - rec2->y0) <= 0)
      return 0;
  }

  return &rec;
}

