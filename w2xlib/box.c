/*
 * box.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib box function mappings to Xlib
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short box(W2XWin *ptr, short x0, short y0, short width,
			short height, GC gc, const char *fname)
{
  const char *cptr;

  if (!(width && height))
    return -1;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  /* XDrawRectangle doesn't seem to handle negative box size */
  if(width < 0)
  {
    x0 += width;
    width = -width;
  }
  if(height < 0)
  {
    y0 += height;
    height = -height;
  }
  width--;
  height--;
  if((cptr = _flush_area(ptr, x0, y0, x0+width, y0+height, ptr->win.linewidth)))
    goto error;

  XDrawRectangle(_Display, ptr->pixmap, gc, x0, y0, width, height);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n", fname, ptr, x0, y0, width, height));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n",\
    fname, ptr, x0, y0, width, height, cptr));
  TRACEEND();
  return -1;
}


short w_box(WWIN *win, short x0, short y0, short width, short height)
{
  W2XWin *ptr = (W2XWin *)win;
  return box(ptr, x0, y0, width, height, ptr->gc, "w_box");
}


short w_dbox(WWIN *win, short x0, short y0, short width, short height)
{
  W2XWin *ptr = (W2XWin *)win;
  return box(ptr, x0, y0, width, height, ptr->fillgc, "w_dbox");
}


static inline short pbox(W2XWin *ptr, short x0, short y0, short width,
			short height, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if (!(width && height))
    return -1;

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  if(width < 0)
  {
    x0 += width;
    width = -width;
  }
  if(height < 0)
  {
    y0 += height;
    height = -height;
  }
  if((cptr = _flush_area(ptr, x0, y0, x0+width, y0+height, 0)))
    goto error;

  XFillRectangle(_Display, ptr->pixmap, gc, x0, y0, width, height);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n", fname, ptr, x0, y0, width, height));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n",\
    fname, ptr, x0, y0, width, height, cptr));
  TRACEEND();
  return -1;
}


short w_pbox(WWIN *win, short x0, short y0, short width, short height)
{
  W2XWin *ptr = (W2XWin *)win;
  return pbox(ptr, x0, y0, width, height, ptr->gc, "w_pbox");
}


short w_dpbox(WWIN *win, short x0, short y0, short width, short height)
{
  W2XWin *ptr = (W2XWin *)win;
  return pbox(ptr, x0, y0, width, height, ptr->fillgc, "w_dpbox");
}
