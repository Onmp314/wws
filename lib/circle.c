/*
 * circle.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- functions for drawing circles
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short circle(WWIN *win, short x0, short y0, short r,
			   short type, const char *fname)
{
  const char *cptr;
  CIRCLEP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%i,%i,%i) -> %s\n", fname, win, x0, y0, r, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(CIRCLEP));
  paket->type = htons(type);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->r = htons(r);

  TRACEPRINT(("%s(%p,%i,%i,%i)\n", fname, win, x0, y0, r));
  TRACEEND();
  return 0;
}


short w_circle(WWIN *win, short x0, short y0, short r)
{
  return circle(win, x0, y0, r, PAK_CIRCLE, "w_circle");
}


short w_pcircle(WWIN *win, short x0, short y0, short r)
{
  return circle(win, x0, y0, r, PAK_PCIRCLE, "w_pcircle");
}


short w_dcircle(WWIN *win, short x0, short y0, short r)
{
  return circle(win, x0, y0, r, PAK_DCIRCLE, "w_dcircle");
}


short w_dpcircle(WWIN *win, short x0, short y0, short r)
{
  return circle(win, x0, y0, r, PAK_DPCIRCLE, "w_dpcircle");
}
