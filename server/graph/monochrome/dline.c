/*
 * dline.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with pattern / dash lines for the BMONO graphics driver
 *
 * NOTES
 * - This is a straighforward conversion for packed driver
 */

#include <stdio.h>
#include <netinet/in.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "bmono.h"

/*
 * - M_CLEAR:  clear bit when bit is set in the pattern ('cut' pattern).
 * - M_DRAW:   clear or set bits according to the pattern (M_DRAW = REPLACE).
 * - M_TRANSP: set bit when bit is set in the pattern (transparent pattern).
 * - M_INVERS: invert bit when it's set in the pattern.
 */


/*
 * patterned line
 */

void FUNCTION(dline)(bm, x0, y0, xe, ye)
     BITMAP *bm;
     long x0;
     long y0;
     long xe;
     long ye;
{
  ushort *ptr, bit, *pattern = gc0->pattern;
  long	dx, yi, dy, delta, ywpl;
  long swap, ox0, oy0;

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

  oy0 = y0;
  ox0 = x0;
  if (LINE_NEEDS_CLIPPING (x0, y0, xe, ye, glob_clip0)) {
    if (CLIP_LINE (x0, y0, xe, ye, glob_clip0)) {
      return;
    }
  }

  ywpl = bm->upl << 1;
  ptr = ((ushort *)bm->data) + y0 * ywpl + (x0 >> 4);
  bit = ntohs(0x8000 >> (x0 & 15));

  yi = 1;
  if ((dy = ye - y0) < 0) {
    yi = -1;
    dy = -dy;
    ywpl = -ywpl;
  }

  if (FirstPoint) {
    switch(gc0->drawmode) {
      case M_CLEAR:
	*ptr &= ~(gc0->pattern[y0 & 15] & bit);
	break;
      case M_DRAW:
	*ptr &= ~bit;
      case M_TRANSP:
	*ptr |= gc0->pattern[y0 & 15] & bit;
	break;
      case M_INVERS:
	*ptr ^= gc0->pattern[y0 & 15] & bit;
	break;
    }
  }
  /*
   * scale delta, dy and dx by 2 so we avoid dividing and thus loosing
   * precision. kay.
   */

  bit = htons(bit);

  switch (gc0->drawmode) {
    case M_CLEAR:		/* clear a line */
      if (dx < dy) {
	/* abs (slope) > 1 */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += ywpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    if (!(bit >>= 1)) {
	      ptr++;
	      bit = 0x8000;
	    }
	  }
	  *ptr &= ~(pattern[y0 & 15] & ntohs(bit));
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
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
	  *ptr &= ~(pattern[y0 & 15] & ntohs(bit));
	}
      }
      break;

  case M_DRAW:
      if (dx < dy) {
	/* abs (slope) > 1 */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += ywpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    if (!(bit >>= 1)) {
	      ptr++;
	      bit = 0x8000;
	    }
	  }
	  *ptr &= ~ntohs(bit);
	  *ptr |= (pattern[y0 & 15] & ntohs(bit));
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
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
	  *ptr &= ~ntohs(bit);
	  *ptr |= (pattern[y0 & 15] & ntohs(bit));
	}
      }
      break;

    case M_TRANSP:
      if (dx < dy) {
	/* abs (slope) > 1 */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += ywpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    if (!(bit >>= 1)) {
	      ptr++;
	      bit = 0x8000;
	    }
	  }
	  *ptr |= (pattern[y0 & 15] & ntohs(bit));
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
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
	  *ptr |= (pattern[y0 & 15] & ntohs(bit));
	}
      }
      break;

    case M_INVERS:
      if (dx < dy) {
	/* abs (slope) > 1 */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += ywpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    if (!(bit >>= 1)) {
	      ptr++;
	      bit = 0x8000;
	    }
	  }
	  *ptr ^= (pattern[y0 & 15] & ntohs(bit));
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
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
	  *ptr ^= (pattern[y0 & 15] & ntohs(bit));
	}
      }
      break;
  }
}


/*
 * patterned vertical line
 */

void FUNCTION(dvline)(bm, x0, y0, ye)
     BITMAP *bm;
     long x0;
     long y0;
     long ye;
{
  ushort *ptr, set_bit, clr_bit, *pattern = gc0->pattern;
  long wpl;
  long y;

  if (ye < y0) {
    y  = y0;
    y0 = ye;
    ye = y;
  }

  if (CLIP_VLINE (x0, y0, ye, glob_clip0)) {
    return;
  }

  wpl = bm->upl << 1;
  ptr = ((ushort *)bm->data) + y0 * wpl + (x0 >> 4);

  set_bit = ntohs(0x8000 >> (x0 & 15));

  switch(gc0->drawmode) {
    case M_CLEAR:
      while(y0 <= ye) {
	*ptr &= ~(pattern[y0 & 15] & set_bit);
	ptr += wpl;
	y0++;
      }
      break;

    case M_DRAW:
      clr_bit = ~set_bit;
      while(y0 <= ye) {
        *ptr = (*ptr & clr_bit) | (pattern[y0 & 15] & set_bit);
	ptr += wpl;
	y0++;
      }
      break;

    case M_TRANSP:
      break;
      while(y0 <= ye) {
	*ptr |= pattern[y0 & 15] & set_bit;
	ptr += wpl;
	y0++;
      }
      break;

    case M_INVERS:
      while(y0 <= ye) {
        *ptr ^= pattern[y0 & 15] & set_bit;
	ptr += wpl;
	y0++;
      }
      break;
  }
}


/*
 * patterned horizontal line
 *
 * TeSche: if pattern were 32 bits wide this could again be speeded up
 */

void FUNCTION(dhline)(bm, x0, y0, xe)
     BITMAP *bm;
     long x0;
     long y0;
     long xe;
{
  ushort *ptr, bits, patt;
  short width, words;
  long x;

  if (xe < x0) {
    x = x0;
    x0 = xe;
    xe = x;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

  ptr = ((ushort *)bm->data) + y0 * (bm->upl << 1) + (x0 >> 4);

  width = xe - x0 + 1;		/* how much left to 'draw' */
  patt  = gc0->pattern[y0 & 15];
  bits  = x0 & 15;

  switch(gc0->drawmode) {
    case M_CLEAR:
      /* non-aligned beginning */
      if (bits) {
	width -= (16 - bits);
	bits = 0xFFFF >> bits;
	/* whole line inside a short? */
	if (width < 0) {
	  bits &= 0xFFFF << (-width);
	  width = 0;
        }
	*ptr++ &= ~(patt & ntohs(bits));
      }

      words = width >> 4;
      while (--words >= 0) {
	*ptr++ &= ~patt;
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = 0xFFFF << (16 - width);
	*ptr &= ~(patt & ntohs(bits));
      }
      break;

    case M_DRAW:
      /* non-aligned beginning */
      if (bits) {
	width -= (16 - bits);
	bits = 0xFFFF >> bits;
	/* whole line inside a short? */
	if (width < 0) {
	  bits &= 0xFFFF << (-width);
	  width = 0;
        }
	bits = ntohs(bits);
	*ptr = (*ptr & ~bits) | (patt & bits);
	ptr++;
      }

      words = width >> 4;
      while (--words >= 0) {
	*ptr++ = patt;
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = ntohs(0xFFFF << (16 - width));
	*ptr = (*ptr & ~bits) | (patt & bits);
      }
      break;

    case M_TRANSP:
      /* non-aligned beginning */
      if (bits) {
	width -= (16 - bits);
	bits = 0xFFFF >> bits;
	/* whole line inside a short? */
	if (width < 0) {
	  bits &= 0xFFFF << (-width);
	  width = 0;
        }
	*ptr++ |= (patt & ntohs(bits));
      }

      words = width >> 4;
      while (--words >= 0) {
	*ptr++ |= patt;
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = 0xFFFF << (16 - width);
	*ptr |= (patt & ntohs(bits));
      }
      break;

    case M_INVERS:
      /* non-aligned beginning */
      if (bits) {
	width -= (16 - bits);
	bits = 0xFFFF >> bits;
	/* whole line inside a short? */
	if (width < 0) {
	  bits &= 0xFFFF << (-width);
	  width = 0;
        }
	*ptr++ ^= (patt & ntohs(bits));
      }

      words = width >> 4;
      while (--words >= 0) {
	*ptr++ ^= patt;
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = 0xFFFF << (16 - width);
	*ptr ^= (patt & ntohs(bits));
      }
      break;
  }
}
