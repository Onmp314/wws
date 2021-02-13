/*
 * poly.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1996 by Kay Römer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- draw polygons
 */

#include <stdio.h>
#include <stddef.h>
#include "Wlib.h"
#include "proto.h"


static inline short poly(WWIN *win, short numpoints, short *points,
			 short type, const char *fname)
{
  const char *cptr;
  POLYP *paket;
  long i;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%i,%p) -> %s\n", fname, win, numpoints, points, cptr));
    TRACEEND();
    return -1;
  }

  if (numpoints > MAXPOLYPOINTS) {
    TRACEPRINT(("%s(%p,%i,%p) -> too many points\n",\
		fname, win, numpoints, points));
    TRACEEND();
    return -1;
  }

  if (numpoints < 3) {
    TRACEPRINT(("%s(%p,%i,%p) -> too few points\n",\
		fname, win, numpoints, points));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(offsetof(POLYP, points) + (numpoints << 2));
  paket->type = htons(type);
  paket->handle = htons(win->handle);

  paket->numpoints = htons(numpoints);
  i = numpoints << 1;
  points += i;
  while (--i >= 0) {
    --points;
    paket->points[i] = htons(*points);
  }

  TRACEPRINT(("%s(%p,%i,%p)\n",\
	      fname, win, numpoints, points));

  TRACEEND();
  return 0;
}


short w_poly(WWIN *win, short numpoints, short *points)
{
  return poly(win, numpoints, points, PAK_POLY, "w_poly");
}


short w_ppoly(WWIN *win, short numpoints, short *points)
{
  return poly(win, numpoints, points, PAK_PPOLY, "w_ppoly");
}


short w_dpoly(WWIN *win, short numpoints, short *points)
{
  return poly(win, numpoints, points, PAK_DPOLY, "w_dpoly");
}


short w_dppoly(WWIN *win, short numpoints, short *points)
{
  return poly(win, numpoints, points, PAK_DPPOLY, "w_dppoly");
}
