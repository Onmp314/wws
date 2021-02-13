/*
 * line.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with lines for the BMONO graphics driver
 *
 * NOTES
 * - This is a straighforward conversion from packed driver
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "bmono.h"


/*
 * arbitrary line
 */

void FUNCTION(line)(bm, x0, y0, xe, ye)
     BITMAP *bm;
     long x0;
     long y0;
     long xe;
     long ye;
{
  uchar *ptr, bit;
  int dx, yi, dy, delta, ybpl;
  int swap, ox0, oy0;

  /* we always draw from left to right */
  if (xe < x0) {
    swap = x0;
    x0 = xe;
    xe = swap;
    swap = y0;
    y0 = ye;
    ye = swap;
  }
  dx = xe - x0;	/* this is guaranteed to be positive, or zero */

  /*
   * dy must be calculated before clipping!
   */
  dy = ye - y0;

  ox0 = x0;
  oy0 = y0;
  if (LINE_NEEDS_CLIPPING(x0, y0, xe, ye, glob_clip0)) {
    if (CLIP_LINE (x0, y0, xe, ye, glob_clip0)) {
      return;
    }
  }

  ybpl = bm->upl * bm->unitsize;
  ptr = (uchar *)bm->data + y0 * ybpl + (x0 >> 3);
  bit = 0x80 >> (x0 & 7);

  yi = 1;
  if (dy  < 0) {
    yi = -1;
    dy = -dy;
    ybpl = -ybpl;
  }

  if (FirstPoint) {
    switch (gc0->drawmode) {
      case M_CLEAR:
	*ptr &= ~bit;
	break;
      case M_DRAW:
      case M_TRANSP:
	*ptr |= bit;
	break;
      case M_INVERS:
	*ptr ^= bit;
    }
  }
  /*
   * scale delta, dy and dx by 2 so we avoid dividing and thus loosing
   * precision. kay.
   */

  switch (gc0->drawmode) {
    case M_CLEAR: /* clear a line */
      if (dx < dy) {
	/* abs (slope) > 1 */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += ybpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    if (!(bit >>= 1)) {
	      ptr++;
	      bit = 0x80;
	    }
	  }
	  *ptr &= ~bit;
	}
      } else {
	/* abs(slope) <= 1 */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0 += 1;
	  if (!(bit >>= 1)) {
	    ptr++;
	    bit = 0x80;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ybpl;
	  }
	  *ptr &= ~bit;
	}
      }
      break;

    case M_DRAW: /* draw a line */
    case M_TRANSP:
      if (dx < dy) {
	/* abs(slope) > 1 */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += ybpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    if (!(bit >>= 1)) {
	      ptr++;
	      bit = 0x80;
	    }
	  }
	  *ptr |= bit;
	}
      } else {
	/* abs(slope) <= 1 */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0 += 1;
	  if (!(bit >>= 1)) {
	    ptr++;
	    bit = 0x80;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ybpl;
	  }
	  *ptr |= bit;
	}
      }
      break;

    case M_INVERS: /* invert a line */
      if (dx < dy) {
	/* abs(slope) > 1 */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += ybpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    if (!(bit >>= 1)) {
	      ptr++;
	      bit = 0x80;
	    }
	  }
	  *ptr ^= bit;
	}
      } else {
	/* abs(slope) <= 1 */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0 += 1;
	  if (!(bit >>= 1)) {
	    ptr++;
	    bit = 0x80;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ybpl;
	  }
	  *ptr ^= bit;
	}
      }
  }
}


/*
 * fast horizontal line
 */


void FUNCTION(hline)(bm, x0, y0, xe)
     BITMAP *bm;
     long x0;
     long y0;
     long xe;
{
  uchar *dptr, lbit, lmask, rmask;
  int width, todo;

  if (xe < x0) {
    width = x0;
    x0 = xe;
    xe = width;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

  width = xe - x0 + 1;
  dptr = (uchar *)bm->data + bm->upl * bm->unitsize * y0 + (x0 >> 3);

  lbit = x0;
  if ((lbit &= 7)) {
    if ((rmask = 8 - lbit) > width) {
      rmask = width;
    }
    lmask = bfmask8[lbit][rmask-1];
    width -= rmask;
  } else {
    lmask = 0;
  }
  todo = width >> 3;
  if ((rmask = width & 7)) {
    rmask = bfmask8[0][rmask-1];
  }

  switch (gc0->drawmode) {

    case M_CLEAR:
      if (lmask) {
	*dptr++ &= ~lmask;
      }
      while (--todo >= 0) {
	*dptr++ =0;
      }
      if (rmask) {
	*dptr &= ~rmask;
      }
      break;

    case M_DRAW:
    case M_TRANSP:
      if (lmask) {
	*dptr++ |= lmask;
      }
      while (--todo >= 0) {
	*dptr++ = 0xff;
      }
      if (rmask) {
	*dptr |= rmask;
      }
      break;

    case M_INVERS:
      if (lmask) {
	*dptr++ ^= lmask;
      }
      while (--todo >= 0) {
	*dptr++ ^= 0xff;
      }
      if (rmask) {
	*dptr ^= rmask;
      }
  }
}


/*
 * fast vertical line
 */

void FUNCTION(vline)(bm, x0, y0, ye)
     BITMAP *bm;
     long x0;
     long y0;
     long ye;
{
  uchar *ptr, bit;
  short upl, height;
  long y;

  if (ye < y0) {
    y = y0;
    y0 = ye;
    ye = y;
  }

  if (CLIP_VLINE (x0, y0, ye, glob_clip0)) {
    return;
  }

  upl = bm->upl * bm->unitsize;
  bit = 0x80 >> (x0 & 7);
  ptr = (uchar *)bm->data + y0 * upl + (x0 >> 3);
  height = ye - y0 + 1;

  switch (gc0->drawmode) {

    case M_CLEAR:
      while (--height >= 0) {
	*ptr &= ~bit;
	ptr += upl;
      }
      break;

    case M_DRAW:
    case M_TRANSP:
      while (--height >= 0) {
	*ptr |= bit;
	ptr += upl;
      }
      break;

    case M_INVERS:
      while (--height >= 0) {
	*ptr ^= bit;
	ptr += upl;
      }
  }
}

