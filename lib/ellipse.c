/*
 * ellipse.c, a part of the W Window System
 *
 * Copyright (C) 1997 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- functions to draw ellipses
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short ellipse(WWIN *win, short x0, short y0, short rx, short ry,
			   short type, const char *fname)
{
  const char *cptr;
  ELLIPSEP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n", fname, win, x0, y0, rx, ry, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(ELLIPSEP));
  paket->type = htons(type);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->rx = htons(rx);
  paket->ry = htons(ry);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n", fname, win, x0, y0, rx, ry));
  TRACEEND();
  return 0;
}


short w_ellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  return ellipse(win, x0, y0, rx, ry, PAK_ELLIPSE, "w_ellipse");
}


short w_pellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  return ellipse(win, x0, y0, rx, ry, PAK_PELLIPSE, "w_pellipse");
}


short w_dellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  return ellipse(win, x0, y0, rx, ry, PAK_DELLIPSE, "w_dellipse");
}


short w_dpellipse(WWIN *win, short x0, short y0, short rx, short ry)
{
  return ellipse(win, x0, y0, rx, ry, PAK_DPELLIPSE, "w_dpellipse");
}
