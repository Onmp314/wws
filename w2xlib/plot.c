/*
 * plot.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib plot function mappings to Xlib
 *
 * TODO:
 * - w_test().
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short plot(const char *fname, GC gc, W2XWin *ptr, short x0, short y0)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
   goto error;

  if((cptr = _flush_area(ptr, x0, y0, x0, y0, 0)))
    goto error;

  XDrawPoint(_Display, ptr->pixmap, gc, x0, y0);

  TRACEPRINT(("%s(%p,%i,%i)\n", fname, ptr, x0, y0));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i) -> %s\n", fname, ptr, x0, y0, cptr));
  TRACEEND();
  return -1;
}

short w_plot(WWIN *win, short x0, short y0)
{
  W2XWin *ptr = (W2XWin *)win;
  return plot("w_plot", ptr->gc, ptr, x0, y0);
}

short w_dplot(WWIN *win, short x0, short y0)
{
  W2XWin *ptr = (W2XWin *)win;
  return plot("w_dplot", ptr->fillgc, ptr, x0, y0);
}


short w_test(WWIN *win, short x0, short y0)
{
  XImage *img = NULL;
  W2XWin *ptr = (W2XWin *)win;
  const char *cptr;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_test(%p,%i %i) -> %s\n", win, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  /* some overhead, huh? If you got a better method, mail me! */
  img = XGetImage(_Display, ptr->pixmap, x0, y0, 1, 1, 1UL, XYPixmap);
  if (!img) {
    TRACEPRINT(("w_test(%p,%i %i) -> failed\n", win, x0, y0));
    TRACEEND();
    return -1;
  }

  ret = *(img->data) & 1;
  XDestroyImage(img);

  TRACEPRINT(("w_test(%p,%i %i) -> %i\n", win, x0, y0, ret));
  TRACEEND();
  return ret;
}
