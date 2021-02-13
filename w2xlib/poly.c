/*
 * poly.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib polygon function mappings to Xlib
 *
 * NOTES
 * - Differs from W so that these obey line width (W will hopefully
 *   get someday wide polylines too)!
 */

#include <stdio.h>
#include <stddef.h>
#include "Wlib.h"
#include "proto.h"


/* find the bounding rectangle for the polygon for refresh */
static short *PolyBound(W2XWin *ptr, int points, short *array)
{
  static short bound[4];
  int i;

  points *= 2;

  /* search polygon bounds */
  bound[0] = 0;
  bound[1] = 0;
  bound[2] = ptr->win.width-1;
  bound[3] = ptr->win.height-1;
  for(i = 0; i < points;)
  {
    if(array[i] < bound[0])
      bound[0] = array[i];
    if(array[i] > bound[2])
      bound[2] = array[i];
    i++;
    if(array[i] < bound[1])
      bound[1] = array[i];
    if(array[i] > bound[3])
      bound[3] = array[i];
    i++;
  }
  return bound;
}


static inline short poly(W2XWin *ptr, short numpoints, short *points,
			 GC gc, const char *fname)
{
  short *bound;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  bound = PolyBound(ptr, numpoints, points);
  if((cptr = _flush_area(ptr, bound[0], bound[1], bound[2], bound[3], ptr->win.linewidth)))
    goto error;

  XDrawLines(_Display, ptr->pixmap, gc, (XPoint *)points, numpoints, CoordModeOrigin);

  TRACEPRINT(("%s(%p,%i,%p)\n", fname, ptr, numpoints, points));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%p) -> container\n", fname, ptr, numpoints, points));
  TRACEEND();
  return -1;
}


short w_poly(WWIN *win, short numpoints, short *points)
{
  W2XWin *ptr = (W2XWin *)win;
  return poly(ptr, numpoints, points, ptr->gc, "w_poly");
}


short w_dpoly(WWIN *win, short numpoints, short *points)
{
  W2XWin *ptr = (W2XWin *)win;
  return poly(ptr, numpoints, points, ptr->fillgc, "w_dpoly");
}


static inline short ppoly(W2XWin *ptr, short numpoints, short *points,
			 GC gc, const char *fname)
{
  short *bound;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  bound = PolyBound(ptr, numpoints, points);
  if((cptr = _flush_area(ptr, bound[0], bound[1], bound[2], bound[3], 0)))
    goto error;

  XFillPolygon(_Display, ptr->pixmap, gc, (XPoint *)points, numpoints, Complex, CoordModeOrigin);

  TRACEPRINT(("%s(%p,%i,%p)\n", fname, ptr, numpoints, points));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%p) -> container\n", fname, ptr, numpoints, points));
  TRACEEND();
  return -1;
}


short w_ppoly(WWIN *win, short numpoints, short *points)
{
  W2XWin *ptr = (W2XWin *)win;
  return ppoly(ptr, numpoints, points, ptr->gc, "w_ppoly");
}


short w_dppoly(WWIN *win, short numpoints, short *points)
{
  W2XWin *ptr = (W2XWin *)win;
  return ppoly(ptr, numpoints, points, ptr->fillgc, "w_dppoly");
}
