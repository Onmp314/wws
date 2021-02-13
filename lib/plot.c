/*
 * plot.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- draw and check points
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short plot(WWIN *win, short x0, short y0,
			 short type, const char *fname)
{
  const char *cptr;
  PLOTP	*paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%i,%i) -> %s\n", fname, win, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(PLOTP));
  paket->type = htons(type);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);

  TRACEPRINT(("%s(%p,%i,%i)\n", fname, win, x0, y0));
  TRACEEND();
  return 0;
}

short w_plot(WWIN *win, short x0, short y0)
{
  return plot(win, x0, y0, PAK_PLOT, "w_plot");
}


short w_dplot(WWIN *win, short x0, short y0)
{
  return plot(win, x0, y0, PAK_DPLOT, "w_dplot");
}


short w_test(WWIN *win, short x0, short y0)
{
  const char *cptr;
  TESTP	*paket;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_test(%p,%i %i) -> %s\n", win, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(TESTP));
  paket->type = htons(PAK_TEST);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);

  TRACEPRINT(("w_test(%p,%i %i) -> %i\n", win, x0, y0, ret));
  TRACEEND();
  return ret;
}
