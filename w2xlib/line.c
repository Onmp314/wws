/*
 * line.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib line function mappings to Xlib
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


/*
 * fast lines
 */

static inline short hline(W2XWin *ptr, short x0, short y0, short e, GC gc, const char *fname)
{
  const char *cptr;
  int wd;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  wd = ptr->win.linewidth >> 1;
  if((cptr = _flush_area(ptr, x0, y0 - wd, e, y0 + wd, 0)))
    goto error;

  XDrawLine(_Display, ptr->pixmap, gc, x0, y0, e, y0);

  TRACEPRINT(("%s(%p,%i,%i,%i)\n", fname, ptr, x0, y0, e));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i) -> %s\n", fname, ptr, x0, y0, e, cptr));
  TRACEEND();
  return -1;
}


short w_hline(WWIN *win, short x0, short y0, short xe)
{
  W2XWin *ptr = (W2XWin *)win;
  return hline(ptr, x0, y0, xe, ptr->gc, "w_hline");
}

short w_dhline(WWIN *win, short x0, short y0, short xe)
{
  W2XWin *ptr = (W2XWin *)win;
  return hline(ptr, x0, y0, xe, ptr->fillgc, "w_dhline");
}


static inline short vline(W2XWin *ptr, short x0, short y0, short e, GC gc, const char *fname)
{
  const char *cptr;
  int wd;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  wd = ptr->win.linewidth >> 1;
  if((cptr = _flush_area(ptr, x0 - wd, y0, x0 + wd, e, 1)))
    goto error;

  XDrawLine(_Display, ptr->pixmap, gc, x0, y0, x0, e);

  TRACEPRINT(("%s(%p,%i,%i,%i)\n", fname, ptr, x0, y0, e));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i) -> %s\n", fname, ptr, x0, y0, e, cptr));
  TRACEEND();
  return -1;
}


short w_vline(WWIN *win, short x0, short y0, short ye)
{
  W2XWin *ptr = (W2XWin *)win;
  return vline(ptr, x0, y0, ye, ptr->gc, "w_vline");
}


short w_dvline(WWIN *win, short x0, short y0, short ye)
{
  W2XWin *ptr = (W2XWin *)win;
  return vline(ptr, x0, y0, ye, ptr->fillgc, "w_dvline");
}


/*
 * normal lines
 */

static short line(W2XWin *ptr, short x0, short y0, short xe, short ye, GC gc, const char *fname)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(&ptr->win)))
    goto error;

  if((cptr = _flush_area(ptr, x0, y0, xe, ye, ptr->win.linewidth)))
    goto error;

  XDrawLine(_Display, ptr->pixmap, gc, x0, y0, xe, ye);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n", fname, ptr, x0, y0, xe, ye));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n", fname, ptr, x0, y0, xe, ye, cptr));
  TRACEEND();
  return -1;
}


short w_line(WWIN *win, short x0, short y0, short xe, short ye)
{
  W2XWin *ptr = (W2XWin *)win;
  return line(ptr, x0, y0, xe, ye, ptr->gc, "w_line");
}


short w_dline(WWIN *win, short x0, short y0, short xe, short ye)
{
  W2XWin *ptr = (W2XWin *)win;
  return line(ptr, x0, y0, xe, ye, ptr->fillgc, "w_dline");
}
