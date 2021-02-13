/*
 * bezier.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- draw bezier curves
 */

#include <stdio.h>
#include <stddef.h>
#include "Wlib.h"
#include "proto.h"


static inline short bezier(WWIN *win, short *points, short type, const char *fname)
{
  const char *cptr;
  BEZIERP *paket;
  long i;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%p) -> %s\n", fname, win, points, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(BEZIERP));
  paket->type   = htons(type);
  paket->handle = htons(win->handle);

  i = 8;
  points += i;
  while (--i >= 0) {
    --points;
    paket->points[i] = htons(*points);
  }

  TRACEPRINT(("%s(%p,%p)\n", fname, win, points));
  TRACEEND();
  return 0;
}


short w_bezier(WWIN *win, short *points)
{
  return bezier(win, points, PAK_BEZIER, "w_bezier");
}


short w_dbezier(WWIN *win, short *points)
{
  return bezier(win, points, PAK_DBEZIER, "w_dbezier");
}

