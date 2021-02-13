/*
 * line.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- draw lines
 *
 * CHANGES
 * 1/98, ++eero: **HACK ALERT**
 * - When line width > 1 map sloped lines into convex polygons.
 *   That doesn't work very well for thin lines, but who cares? <g>
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


/*
 * normal lines
 */

static short line(WWIN *win, short x0, short y0, short xe, short ye, short type, const char *fname)
{
  const char *cptr;
  LINEP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n", fname, win, x0, y0, xe, ye, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(LINEP));
  paket->type = htons(type);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->xe = htons(xe);
  paket->ye = htons(ye);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n", fname, win, x0, y0, xe, ye));
  TRACEEND();
  return 0;
}


static short tline(WWIN *win, short x0, short y0, short xe, short ye,
	short (*box)(WWIN*,short,short,short,short),
	short (*poly)(WWIN*,short,short*))
{
	short scale, r = win->linewidth >> 1;
	short point[8];
	long dx, dy;

	if (y0 == ye) {
		return (*box)(win, x0, y0-r, xe-x0+1, ye-y0+1 + r+r);
	}
	if (x0 == xe) {
		return (*box)(win, x0-r, y0, xe-x0+1 + r+r, ye-y0+1);
	}

	/* we'll be going perpendically away from the line endpoint middles,
	 * hence we'll swap x and y co-ordinates to get the delta.
	 */
	dx = ye - y0;
	dy = xe - x0;
	if (dx < 0 && dy > 0) {
		dx = -dx;
	} else {
		dy = -dy;
	}
	scale = isqrt(dx*dx + dy*dy);
	dx = dx * r / scale;
	dy = dy * r / scale;

	point[0] = x0 + dx;
	point[1] = y0 + dy;
	point[2] = x0 - dx;
	point[3] = y0 - dy;
	point[4] = xe - dx;
	point[5] = ye - dy;
	point[6] = xe + dx;
	point[7] = ye + dy;

	/* draw line (a convex polygon) */
	return (*poly)(win, 4, point);
}


short w_line(WWIN *win, short x0, short y0, short xe, short ye)
{
  if (win && win->linewidth > 2) {
    return tline(win, x0, y0, xe, ye, w_pbox, w_ppoly);
  } else {
    return line(win, x0, y0, xe, ye, PAK_LINE, "w_line");
  }
}


short w_dline(WWIN *win, short x0, short y0, short xe, short ye)
{
  if (win && win->linewidth > 2) {
    return tline(win, x0, y0, xe, ye, w_dpbox, w_dppoly);
  } else {
    return line(win, x0, y0, xe, ye, PAK_DLINE, "w_dline");
  }
}

