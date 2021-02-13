/*
 * circle.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib circle function mappings to Xlib
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short circle(W2XWin *ptr, short x0, short y0, short r, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  /* the circle bounding box co-ordinate & size */
  x0 -= r;
  y0 -= r;
  r *= 2;
  if((cptr = _flush_area(ptr, x0, y0, x0+r, y0+r, ptr->win.linewidth)))
    goto error;

  XDrawArc(_Display, ptr->pixmap, gc, x0, y0, r, r, 0, 360 * 64);

  TRACEPRINT(("%s(%p,%i,%i,%i)\n", fname, ptr, x0, y0, r));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i) -> %s\n", fname, ptr, x0, y0, r, cptr));
  TRACEEND();
  return -1;
}


short w_circle(WWIN *win, short x0, short y0, short r)
{
  W2XWin *ptr = (W2XWin *)win;
  return circle(ptr, x0, y0, r, ptr->gc, "w_circle");
}


short w_dcircle(WWIN *win, short x0, short y0, short r)
{
  W2XWin *ptr = (W2XWin *)win;
  return circle(ptr, x0, y0, r, ptr->fillgc, "w_dcircle");
}


static inline short pcircle(W2XWin *ptr, short x0, short y0, short r, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  /* the circle bounding box co-ordinate & size */
  x0 -= r;
  y0 -= r;
  r *= 2;
  if((cptr = _flush_area(ptr, x0, y0, x0+r, y0+r, 0)))
    goto error;

  XFillArc(_Display, ptr->pixmap, gc, x0, y0, r, r, 0, 360 * 64);

  TRACEPRINT(("%s(%p,%i,%i,%i)\n", fname, ptr, x0, y0, r));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i) -> %s\n", fname, ptr, x0, y0, r, cptr));
  TRACEEND();
  return -1;
}


short w_pcircle(WWIN *win, short x0, short y0, short r)
{
  W2XWin *ptr = (W2XWin *)win;
  return pcircle(ptr, x0, y0, r, ptr->gc, "w_pcircle");
}


short w_dpcircle(WWIN *win, short x0, short y0, short r)
{
  W2XWin *ptr = (W2XWin *)win;
  return pcircle(ptr, x0, y0, r, ptr->fillgc, "w_dpcircle");
}
