/*
 * ellipse.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib ellipse function mappings to Xlib
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short ellipse(W2XWin *ptr, short x0, short y0, short rx, short ry, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  /* the ellipse bounding box co-ordinate & size */
  x0 -= rx;
  y0 -= ry;
  if((cptr = _flush_area(ptr, x0, y0, x0 + 2*rx, y0 + 2*ry, ptr->win.linewidth)))
    goto error;

  XDrawArc(_Display, ptr->pixmap, gc, x0, y0, 2*rx, 2*ry, 0, 360 * 64);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n", fname, ptr, x0, y0, rx, ry));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n", fname, ptr, x0, y0, rx, ry, cptr));
  TRACEEND();
  return -1;
}


short w_ellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  W2XWin *ptr = (W2XWin *)win;
  return ellipse(ptr, x0, y0, rx, ry, ptr->gc, "w_ellipse");
}


short w_dellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  W2XWin *ptr = (W2XWin *)win;
  return ellipse(ptr, x0, y0, rx, ry, ptr->fillgc, "w_dellipse");
}


static inline short pellipse(W2XWin *ptr, short x0, short y0, short rx, short ry, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  /* the ellipse bounding box co-ordinate & size */
  x0 -= rx;
  y0 -= ry;
  if((cptr = _flush_area(ptr, x0, y0, x0 + 2*rx, y0 + 2*ry, 0)))
    goto error;

  XFillArc(_Display, ptr->pixmap, gc, x0, y0, 2*rx, 2*ry, 0, 360 * 64);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n", fname, ptr, x0, y0, rx, ry));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n", fname, ptr, x0, y0, rx, ry, cptr));
  TRACEEND();
  return -1;
}


short w_pellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  W2XWin *ptr = (W2XWin *)win;
  return pellipse(ptr, x0, y0, rx, ry, ptr->gc, "w_pellipse");
}


short w_dpellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  W2XWin *ptr = (W2XWin *)win;
  return pellipse(ptr, x0, y0, rx, ry, ptr->fillgc, "w_dpellipse");
}
