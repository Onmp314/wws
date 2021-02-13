/*
 * arc.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib arc and pie function mappings to Xlib
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short
arc(W2XWin *ptr, short x0, short y0, short rx, short ry,
    float a, float b, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  /* the arc bounding box co-ordinate & size */
  x0 -= rx;
  y0 -= ry;
  if((cptr = _flush_area(ptr, x0, y0, x0 + 2*rx, y0 + 2*ry, ptr->win.linewidth)))
    goto error;

  b -= a;
  if (b < 0) {
	  b += 360;
	  
  }
  XDrawArc(_Display, ptr->pixmap, gc, x0, y0, 2*rx, 2*ry, a*64, b*64);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i,%f,%f)\n", fname, ptr, x0, y0, rx, ry, a, b));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i,%i,%f,%f) -> %s\n",\
		fname, ptr, x0, y0, rx, ry, a, b, cptr));
  TRACEEND();
  return -1;
}


short
w_arc(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
  W2XWin *ptr = (W2XWin *)win;
  return arc(ptr, x0, y0, rx, ry, a, b, ptr->gc, "w_arc");
}


short
w_darc(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
  W2XWin *ptr = (W2XWin *)win;
  return arc(ptr, x0, y0, rx, ry, a, b, ptr->fillgc, "w_darc");
}


static inline short
pie(W2XWin *ptr, short x0, short y0, short rx, short ry,
    float a, float b, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  /* the arc bounding box co-ordinate & size */
  x0 -= rx;
  y0 -= ry;
  if((cptr = _flush_area(ptr, x0, y0, x0 + 2*rx, y0 + 2*ry, 0)))
    goto error;

  b -= a;
  if (b < 0) {
	  b += 360;
	  
  }
  XFillArc(_Display, ptr->pixmap, gc, x0, y0, 2*rx, 2*ry, a*64, b*64);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i,%f,%f)\n", fname, ptr, x0, y0, rx, ry, a, b));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i,%i,%f,%f) -> %s\n",\
		fname, ptr, x0, y0, rx, ry, a, b, cptr));
  TRACEEND();
  return -1;
}


short
w_pie(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
  W2XWin *ptr = (W2XWin *)win;
  return pie(ptr, x0, y0, rx, ry, a, b, ptr->gc, "w_pie");
}


short
w_dpie(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
  W2XWin *ptr = (W2XWin *)win;
  return pie(ptr, x0, y0, rx, ry, a, b, ptr->fillgc, "w_dpie");
}
