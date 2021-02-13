/*
 * dline.c, a part of the W Window System
 *
 * Copyright (C) 1994-1998 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with pattern / dash lines for the packed graphics driver
 *
 * CHANGES
 * ++eero, 8/96:
 * - Put these function into a file of their own (doing graphics
 *   modes added quite a bit of code...).
 * - Graphics modes to pattern functions.
 * ++eero, 10/96:
 * - Fixed bug in dhline().
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "packed.h"

/*
 * TeSche: the patterned functions copy pattern bits to screen directly. they
 * don't (yet) support drawing modes, except that you may call the way they
 * work M_DRAW.
 *
 * TeSche:  the COLOR driver implementes patterns as follows:  if there's a
 * bit set in the pattern it will write the foreground color, and if not the
 * back- ground color.  speaking in graphic modes this is a mixture of
 * M_DRAW/M_CLEAR (on most graphics programs referred as 'replace' mode).
 * The general function for this is (for all planes 'idx'):
 *
 * ptr[idx] = (ptr[idx] & ~bits) |
 *            (((fgColMask[idx] & patt) | (bgColMask[idx] & ~patt)) & bits);
 *
 * Eero: I added other graphics modes...
 * - M_CLEAR:  clear bit when bit is set in the pattern ('cut' pattern).
 * - M_DRAW:   clear or set bits according to the pattern (M_DRAW = REPLACE).
 * - M_TRANSP: set bit when bit is set in the pattern (transparent pattern).
 * - M_INVERS: invert bit when it's set in the pattern.
 *
 * With the COLOR driver M_CLEAR and M_TRANSP modes differ only by the color
 * used (background color for M_CLEAR and foreground one for M_TRANSP) on
 * the operation.
 */


/*
 * patterned line
 */

void FUNCTION(dline)(bm, x0, y0, xe, ye)
     BITMAP *bm;
     register long x0;
     register long y0;
     register long xe;
     register long ye;
{
  register ushort *ptr, bit, *pattern = gc0->pattern;
  register long	dx, yi, dy, delta, ywpl;
#ifdef COLOR
  register short idx;
  register ushort *mask, patt_bit;
  register short planes = bm->planes;
#endif
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

#ifdef MONO
  ywpl = bm->upl << 1;
  ptr = ((ushort *)bm->data) + y0 * ywpl + (x0 >> 4);
#else
  ywpl = bm->upl;
  ptr = ((ushort *)bm->data) + y0 * ywpl + (x0 >> 4) * bm->planes;
#endif
  bit = 0x8000 >> (x0 & 15);

  yi = 1;
  if ((dy = ye - y0) < 0) {
    yi = -1;
    dy = -dy;
    ywpl = -ywpl;
  }

  if (FirstPoint) {
    /* draw the first point, there always really is one */
#ifdef COLOR
    if (gc0->drawmode == M_INVERS) {
      idx = planes;
      while (--idx >= 0) {
	ptr[idx] ^= bit;
      }
    } else {
      switch(gc0->drawmode) {
	case M_CLEAR:
	  mask = gc0->bgColMask;
	  break;
	case M_TRANSP:
	  mask = gc0->bgColMask;
	  break;
	default:
	  if (pattern[y0 & 15] & bit) {
	    mask = gc0->fgColMask;
	  } else {
	    mask = gc0->bgColMask;
	  }
	  break;
      }
      idx = planes;
      while (--idx >= 0) {
	ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
      }
    }
#else
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
#endif
  }

  /*
   * scale delta, dy and dx by 2 so we avoid dividing and thus loosing
   * precision. kay.
   */

  switch (gc0->drawmode) {
    case M_CLEAR:		/* clear a line */
#ifdef COLOR
      mask = gc0->bgColMask;
#endif
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
#ifdef MONO
	      ptr++;
#else
	      ptr += bm->planes;
#endif
	      bit = 0x8000;
	    }
	  }
#ifdef COLOR
          patt_bit = pattern[y0 & 15] & bit;
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (ptr[idx] & ~patt_bit) | (mask[idx] & patt_bit);
	  }
#else
	  *ptr &= ~(pattern[y0 & 15] & bit);
#endif
	}
      } else {
	/* abs(slope) <= 1 */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0 += 1;
	  if (!(bit >>= 1)) {
#ifdef MONO
	    ptr++;
#else
	    ptr += bm->planes;
#endif
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
#ifdef COLOR
          patt_bit = pattern[y0 & 15] & bit;
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (ptr[idx] & ~patt_bit) | (mask[idx] & patt_bit);
	  }
#else
	  *ptr &= ~(pattern[y0 & 15] & bit);
#endif
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
#ifdef MONO
	      ptr++;
#else
	      ptr += bm->planes;
#endif
	      bit = 0x8000;
	    }
	  }
#ifdef COLOR
	  if (pattern[y0 & 15] & bit) {
	    mask = gc0->fgColMask;
	  } else {
	    mask = gc0->bgColMask;
	  }
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	  }
#else
	  *ptr &= ~bit;
	  *ptr |= (pattern[y0 & 15] & bit);
#endif
	}
      } else {
	/* abs(slope) <= 1 */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0 += 1;
	  if (!(bit >>= 1)) {
#ifdef MONO
	    ptr++;
#else
	    ptr += bm->planes;
#endif
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
#ifdef COLOR
	  if (pattern[y0 & 15] & bit) {
	    mask = gc0->fgColMask;
	  } else {
	    mask = gc0->bgColMask;
	  }
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	  }
#else
	  *ptr &= ~bit;
	  *ptr |= (pattern[y0 & 15] & bit);
#endif
	}
      }
      break;

    case M_TRANSP:
#ifdef COLOR
      mask = gc0->fgColMask;
#endif
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
#ifdef MONO
	      ptr++;
#else
	      ptr += bm->planes;
#endif
	      bit = 0x8000;
	    }
	  }
#ifdef COLOR
          patt_bit = pattern[y0 & 15] & bit;
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (ptr[idx] & ~patt_bit) | (mask[idx] & patt_bit);
	  }
#else
	  *ptr |= (pattern[y0 & 15] & bit);
#endif
	}
      } else {
	/* abs(slope) <= 1 */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0 += 1;
	  if (!(bit >>= 1)) {
#ifdef MONO
	    ptr++;
#else
	    ptr += bm->planes;
#endif
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
#ifdef COLOR
          patt_bit = pattern[y0 & 15] & bit;
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (ptr[idx] & ~patt_bit) | (mask[idx] & patt_bit);
	  }
#else
	  *ptr |= (pattern[y0 & 15] & bit);
#endif
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
#ifdef MONO
	      ptr++;
#else
	      ptr += bm->planes;
#endif
	      bit = 0x8000;
	    }
	  }
#ifdef COLOR
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (pattern[y0 & 15] & bit);
	  }
#else
	  *ptr ^= (pattern[y0 & 15] & bit);
#endif
	}
      } else {
	/* abs(slope) <= 1 */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0 += 1;
	  if (!(bit >>= 1)) {
#ifdef MONO
	    ptr++;
#else
	    ptr += bm->planes;
#endif
	    bit = 0x8000;
	  }
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += ywpl;
	  }
#ifdef COLOR
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] ^= (pattern[y0 & 15] & bit);
	  }
#else
	  *ptr ^= (pattern[y0 & 15] & bit);
#endif
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
     register long y0;
     register long ye;
{
  register ushort *ptr, set_bit, clr_bit, *pattern = gc0->pattern;
  register long wpl;
#ifdef COLOR
  register short idx;
  register ushort *mask;
  register short planes = bm->planes;
#endif
  long y;

  if (ye < y0) {
    y  = y0;
    y0 = ye;
    ye = y;
  }

  if (CLIP_VLINE (x0, y0, ye, glob_clip0)) {
    return;
  }

#ifdef MONO
  wpl = bm->upl << 1;
  ptr = ((ushort *)bm->data) + y0 * wpl + (x0 >> 4);
#else
  wpl = bm->upl;
  ptr = ((ushort *)bm->data) + y0 * wpl + (x0 >> 4) * bm->planes;
#endif

  set_bit = 0x8000 >> (x0 & 15);

  switch(gc0->drawmode) {
    case M_CLEAR:
      while(y0 <= ye) {
#ifdef COLOR
	clr_bit = pattern[y0 & 15] & set_bit;
	mask = gc0->bgColMask;
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~clr_bit) | (mask[idx] & clr_bit);
	}
#else
	*ptr &= ~(pattern[y0 & 15] & set_bit);
#endif
	ptr += wpl;
	y0++;
      }
      break;

    case M_DRAW:
      clr_bit = ~set_bit;
      while(y0 <= ye) {
#ifdef COLOR
	if (pattern[y0 & 15] & set_bit) {
	  mask = gc0->fgColMask;
	} else {
	  mask = gc0->bgColMask;
	}
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & clr_bit) | (mask[idx] & set_bit);
	}
#else
        *ptr = (*ptr & clr_bit) | (pattern[y0 & 15] & set_bit);
#endif
	ptr += wpl;
	y0++;
      }
      break;

    case M_TRANSP:
      break;
      while(y0 <= ye) {
#ifdef COLOR
	clr_bit = pattern[y0 & 15] & set_bit;
	mask = gc0->fgColMask;
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~clr_bit) | (mask[idx] & clr_bit);
	}
#else
	*ptr |= pattern[y0 & 15] & set_bit;
#endif
	ptr += wpl;
	y0++;
      }
      break;

    case M_INVERS:
      while(y0 <= ye) {
#ifdef COLOR
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] ^= pattern[y0 & 15] & set_bit;
	}
#else
        *ptr ^= pattern[y0 & 15] & set_bit;
#endif
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
  register ushort *ptr, bits, patt;
  register short width, words;
#ifndef MONO
  register short planes = bm->planes;
#ifdef COLOR
  register short idx;
  register ushort *bgColMask = gc0->bgColMask;
  register ushort *fgColMask = gc0->fgColMask;
#endif
#endif
  long x;

  if (xe < x0) {
    x = x0;
    x0 = xe;
    xe = x;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

#ifdef MONO
  ptr = ((ushort *)bm->data) + y0 * (bm->upl << 1) + (x0 >> 4);
#else
  ptr = ((ushort *)bm->data) + y0 * bm->upl + (x0 >> 4) * planes;
#endif

  width = xe - x0 + 1;		/* how much left to 'draw' */
  patt  = gc0->pattern[y0 & 15];
  bits  = (ushort)x0 & 15;

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
#if defined(MONO)
	*ptr++ &= ~(patt & bits);
#elif defined(COLORMONO)
	*ptr &= ~(patt & bits);
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bits) |
	    (((fgColMask[idx] & ~patt) | (bgColMask[idx] & patt)) & bits);
	}
	ptr += planes;
#endif
      }

      words = width >> 4;
      while (--words >= 0) {
#if defined(MONO)
	*ptr++ &= ~patt;
#elif defined(COLORMONO)
	*ptr &= ~patt;
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (fgColMask[idx] & ~patt) | (bgColMask[idx] & patt);
	}
	ptr += planes;
#endif
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = 0xFFFF << (16 - width);
#ifdef COLOR
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bits) |
	    (((fgColMask[idx] & ~patt) | (bgColMask[idx] & patt)) & bits);
	}
#else
	*ptr &= ~(patt & bits);
#endif
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
#if defined(MONO)
	*ptr = (*ptr & ~bits) | (patt & bits);
	ptr++;
#elif defined(COLORMONO)
	*ptr = (*ptr & ~bits) | (patt & bits);
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bits) |
	    (((fgColMask[idx] & patt) | (bgColMask[idx] & ~patt)) & bits);
	}
	ptr += planes;
#endif
      }

      words = width >> 4;
      while (--words >= 0) {
#if defined(MONO)
	*ptr++ = patt;
#elif defined(COLORMONO)
	*ptr = patt;
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (fgColMask[idx] & patt) | (bgColMask[idx] & ~patt);
	}
	ptr += planes;
#endif
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = 0xFFFF << (16 - width);
#ifdef COLOR
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bits) |
	    (((fgColMask[idx] & patt) | (bgColMask[idx] & ~patt)) & bits);
	}
#else
	*ptr = (*ptr & ~bits) | (patt & bits);
#endif
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
#if defined(MONO)
	*ptr++ |= (patt & bits);
#elif defined(COLORMONO)
	*ptr |= (patt & bits);
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] |= (((fgColMask[idx] & patt) | (bgColMask[idx] & ~patt)) & bits);
	}
	ptr += planes;
#endif
      }

      words = width >> 4;
      while (--words >= 0) {
#if defined(MONO)
	*ptr++ |= patt;
#elif defined(COLORMONO)
	*ptr |= patt;
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] |= (fgColMask[idx] & patt) | (bgColMask[idx] & ~patt);
	}
	ptr += planes;
#endif
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = 0xFFFF << (16 - width);
#ifdef COLOR
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] |= (((fgColMask[idx] & patt) | (bgColMask[idx] & ~patt)) & bits);
	}
#else
	*ptr |= (patt & bits);
#endif
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
#if defined(MONO)
	*ptr++ ^= (patt & bits);
#elif defined(COLORMONO)
	*ptr ^= (patt & bits);
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] ^= (patt & bits);
	}
	ptr += planes;
#endif
      }

      words = width >> 4;
      while (--words >= 0) {
#if defined(MONO)
	*ptr++ ^= patt;
#elif defined(COLORMONO)
	*ptr ^= patt;
	ptr += planes;
#else
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] ^= patt;
	}
	ptr += planes;
#endif
      }

      /* non-aligned end */
      if ((width &= 15) > 0) {
	bits = 0xFFFF << (16 - width);
#ifdef COLOR
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] ^= (patt & bits);
	}
#else
	*ptr ^= (patt & bits);
#endif
      }
      break;
  }
}
