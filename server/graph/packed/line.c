/*
 * line.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with lines for the packed graphics driver
 *
 * CHANGES:
 * ++kay 1/96:
 * - in the *line() functions scale delta, dx and dy by 2 to avoid
 *   losing precision and thus drawing wrong lines.
 *   NOTE: x>>1 != x/2 if x<0 and x is odd.
 * - fixed a bug in line() at three places. if (delta < 0) should be
 *   if (delta <= 0).
 * - added clipping.
 * ++tesche, 1/96:
 * - fixed a nasty bug in dhline, `width' musn't be unsigned.
 * ++kay, 2/96:
 * - fixed a patterning bug in dvline.
 * - fixed a bug in dhline. short lines with non-aligned start were sometimes
 *   drawn one pixel to short.
 * ++tesche, 4/96:
 * - some speedups for hline, slight ones for vline.
 * - color support.
 * ++eero, 8/96:
 * - M_TRANSP mode, with solid graphics == M_DRAW.
 * - pattern functions into a separate file.
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "packed.h"


/*
 * arbitrary line
 */

void FUNCTION(line)(bm, x0, y0, xe, ye)
     BITMAP *bm;
     register long x0;
     register long y0;
     register long xe;
     register long ye;
{
  register ushort *ptr, bit;
  register long	dx, yi, dy, delta, ywpl;
  long swap, ox0, oy0;
#ifndef MONO
  register short planes = bm->planes;
#ifdef COLOR
  register short idx;
  register ushort *mask;
#endif
#endif

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

#ifdef MONO
  ywpl = bm->upl << 1;
  ptr = ((ushort *)bm->data) + y0 * ywpl + (x0 >> 4);
#else
  ywpl = bm->upl;
  ptr = ((ushort *)bm->data) + y0 * ywpl + (x0 >> 4) * planes;
#endif
  bit = 0x8000 >> (x0 & 15);

  yi = 1;
  if (dy  < 0) {
    yi = -1;
    dy = -dy;
    ywpl = -ywpl;
  }

  if (FirstPoint) {
    /* draw the first point, there always really is one
     */
#ifdef COLOR
    idx = planes;
    switch (gc0->drawmode) {
      case M_CLEAR:
	mask = gc0->bgColMask;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	}
	break;
      case M_DRAW:
      case M_TRANSP:
	mask  = gc0->fgColMask;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	}
	break;
      case M_INVERS:
	while (--idx >= 0) {
	  ptr[idx] ^= bit;
	}
    }
#else
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
#endif
  }

  /*
   * scale delta, dy and dx by 2 so we avoid dividing and thus loosing
   * precision. kay.
   */

  switch (gc0->drawmode) {
    case M_CLEAR: /* clear a line */
#ifdef COLOR
      mask  = gc0->bgColMask;
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
	      ptr += planes;
#endif
	      bit = 0x8000;
	    }
	  }
#ifdef COLOR
	  idx = planes;
	  while (--idx >= 0) {
	    ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	  }
#else
	  *ptr &= ~bit;
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
	    ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	  }
#else
	  *ptr &= ~bit;
#endif
	}
      }
      break;

    case M_DRAW: /* draw a line */
    case M_TRANSP:
#ifdef COLOR
      mask  = gc0->fgColMask;
#endif
      if (dx < dy) {
	/* abs(slope) > 1 */
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
	    ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	  }
#else
	  *ptr |= bit;
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
	    ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	  }
#else
	  *ptr |= bit;
#endif
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
	    ptr[idx] ^= bit;
	  }
#else
	  *ptr ^= bit;
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
	    ptr[idx] ^= bit;
	  }
#else
	  *ptr ^= bit;
#endif
	}
      }
  }
}


/*
 * fast horizontal line
 */

#ifdef MONO

void FUNCTION(hline)(bm, x0, y0, xe)
     BITMAP *bm;
     register long x0;
     register long y0;
     register long xe;
{
  register ulong *dptr;
  register long width;
  register short todo;

  if (xe < x0) {
    width = x0;
    x0 = xe;
    xe = width;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

  width = xe - x0 + 1;
  dptr = (ulong *)bm->data + bm->upl * y0 + (x0 >> 5);

  /* x0', 'y0' and 'xe' are no longer needed now, they now take the purpose of
   * 'lbit', 'lmask' and 'rmask'
   */
  if ((x0 &= 31)) {
    if ((xe = 32 - x0) > width) {
      xe = width;
    }
#ifdef __mc68020__
    y0 = xe;
#else
    y0 = bfmask32[x0][xe-1];
#endif
    width -= xe;
  } else {
    y0 = 0;
  }
  todo = width >> 5;
#ifdef __mc68020__
  xe = width & 31;
#else
  if ((xe = width & 31)) {
    xe = bfmask32[0][xe-1];
  }
#endif

  switch (gc0->drawmode) {

    case M_CLEAR:
      if (y0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfclr %0@{%1:%2}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(x0), "d"(y0)
			      : "memory");
	dptr++;
#else
	*dptr++ &= ~y0;
#endif
      }
      while (--todo >= 0) {
	*dptr++ =0;
      }
      if (xe) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfclr %0@{#0:%1}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(xe)
			      : "memory");
#else
	*dptr &= ~xe;
#endif
      }
      break;

    case M_DRAW:
    case M_TRANSP:
      if (y0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfset %0@{%1:%2}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(x0), "d"(y0)
			      : "memory");
	dptr++;
#else
	*dptr++ |= y0;
#endif
      }
      while (--todo >= 0) {
	*dptr++ = 0xffffffff;
      }
      if (xe) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfset %0@{#0:%1}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(xe)
			      : "memory");
#else
	*dptr |= xe;
#endif
      }
      break;

    case M_INVERS:
      if (y0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfchg %0@{%1:%2}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(x0), "d"(y0)
			      : "memory");
	dptr++;
#else
	*dptr++ ^= y0;
#endif
      }
      while (--todo >= 0) {
	*dptr++ ^= 0xffffffff;
      }
      if (xe) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfchg %0@{#0:%1}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(xe)
			      : "memory");
#else
	*dptr ^= xe;
#endif
      }
  }
}

#elif defined(COLORMONO)

void FUNCTION(hline)(bm, x0, y0, xe)
     BITMAP *bm;
     register long x0;
     register long y0;
     register long xe;
{
  register ushort *dptr;
  register long width;
  register short todo;
  register short planes = bm->planes;

  if (xe < x0) {
    width = x0;
    x0 = xe;
    xe = width;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

  width = xe - x0 + 1;
  dptr = (ushort *)bm->data + bm->upl * y0 + (x0 >> 4) * planes;

  /* x0', 'y0' and 'xe' are no longer needed now, they now take the purpose of
   * 'lbit', 'lmask' and 'rmask'
   */
  if ((x0 &= 15)) {
    if ((xe = 16 - x0) > width) {
      xe = width;
    }
#ifdef __mc68020__
    y0 = xe;
#else
    y0 = bfmask16[x0][xe-1];
#endif
    width -= xe;
  } else {
    y0 = 0;
  }
  todo = width >> 4;
#ifdef __mc68020__
  xe = width & 15;
#else
  if ((xe = width & 15)) {
    xe = bfmask16[0][xe-1];
  }
#endif

  switch (gc0->drawmode) {

    case M_CLEAR:
      if (y0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfclr %0@{%1:%2}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(x0), "d"(y0)
			      : "memory");
	dptr += planes;
#else
	*dptr &= ~y0;
	dptr += planes;
#endif
      }
      while (--todo >= 0) {
	*dptr =0;
	dptr += planes;
      }
      if (xe) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfclr %0@{#0:%1}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(xe)
			      : "memory");
#else
	*dptr &= ~xe;
#endif
      }
      break;

    case M_DRAW:
    case M_TRANSP:
      if (y0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfset %0@{%1:%2}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(x0), "d"(y0)
			      : "memory");
	dptr += planes;
#else
	*dptr |= y0;
	dptr += planes;
#endif
      }
      while (--todo >= 0) {
	*dptr = 0xffff;
	dptr += planes;
      }
      if (xe) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfset %0@{#0:%1}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(xe)
			      : "memory");
#else
	*dptr |= xe;
#endif
      }
      break;

    case M_INVERS:
      if (y0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfchg %0@{%1:%2}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(x0), "d"(y0)
			      : "memory");
	dptr += planes;
#else
	*dptr ^= y0;
	dptr += planes;
#endif
      }
      while (--todo >= 0) {
	*dptr ^= 0xffff;
	dptr += planes;
      }
      if (xe) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfchg %0@{#0:%1}\n"
			      : /* no outputs */
			      : "a"(dptr), "d"(xe)
			      : "memory");
#else
	*dptr ^= xe;
#endif
      }
  }
}

#else /* COLOR */

void FUNCTION(hline)(bm, x0, y0, xe)
     BITMAP *bm;
     register long x0;
     register long y0;
     register long xe;
{
  register ushort *dptr;
  register long width;
  register short todo;
  register short idx, planes = bm->planes;
  register ushort *mask;

  if (xe < x0) {
    width = x0;
    x0 = xe;
    xe = width;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

  width = xe - x0 + 1;
  dptr = (ushort *)bm->data + bm->upl * y0 + (x0 >> 4) * planes;

  /* x0', 'y0' and 'xe' are no longer needed now, they now take the purpose of
   * 'lbit', 'lmask' and 'rmask'
   */
  if ((x0 &= 15)) {
    if ((xe = 16 - x0) > width) {
      xe = width;
    }
    y0 = bfmask16[x0][xe-1];
    width -= xe;
  } else {
    y0 = 0;
  }
  todo = width >> 4;
  if ((xe = width & 15)) {
    xe = bfmask16[0][xe-1];
  }

  switch (gc0->drawmode) {

    case M_CLEAR:
      mask = gc0->bgColMask;
      if (y0) {
	idx = planes;
	while (--idx >= 0) {
	  dptr[idx] = (dptr[idx] & ~y0) | (mask[idx] & y0);
	}
	dptr += planes;
      }
      while (--todo >= 0) {
	idx = planes;
	while (--idx >= 0) {
	  dptr[idx] = mask[idx];
	}
	dptr += planes;
      }
      if (xe) {
	idx = planes;
	while (--idx >= 0) {
	  dptr[idx] = (dptr[idx] & ~xe) | (mask[idx] & xe);
	}
      }
      break;

    case M_DRAW:
    case M_TRANSP:
      mask = gc0->fgColMask;
      if (y0) {
	idx = planes;
	while (--idx >= 0) {
	  dptr[idx] = (dptr[idx] & ~y0) | (mask[idx] & y0);
	}
	dptr += planes;
      }
      while (--todo >= 0) {
	idx = planes;
	while (--idx >= 0) {
	  dptr[idx] = mask[idx];
	}
	dptr += planes;
      }
      if (xe) {
	idx = planes;
	while (--idx >= 0) {
	  dptr[idx] = (dptr[idx] & ~xe) | (mask[idx] & xe);
	}
      }
      break;

    case M_INVERS:
      if (y0) {
	idx = planes;
	while (--idx >= 0) {
	  *dptr++ ^= y0;
	}
      }
      todo *= planes;
      while (--todo >= 0) {
	*dptr++ ^= 0xffff;
      }
      if (xe) {
	idx = planes;
	while (--idx >= 0) {
	  *dptr++ ^= xe;
	}
      }
  }
}

#endif


/*
 * fast vertical line
 */

void FUNCTION(vline)(bm, x0, y0, ye)
     BITMAP *bm;
     long x0;
     long y0;
     long ye;
{
#ifdef MONO
  register ulong *ptr, bit;
#else
  register ushort *ptr, bit;
#endif
  register long upl;
  register short height;
#ifdef COLOR
  register ushort *mask;
  register short idx;
  register short planes = bm->planes;
#endif
  long y;

  if (ye < y0) {
    y = y0;
    y0 = ye;
    ye = y;
  }

  if (CLIP_VLINE (x0, y0, ye, glob_clip0)) {
    return;
  }

  upl = bm->upl;
#ifdef MONO
  bit = 0x80000000 >> (x0 & 31);
  ptr = (ulong *)bm->data + y0 * upl + (x0 >> 5);
#else
  bit = 0x8000 >> (x0 & 15);
  ptr = (ushort *)bm->data + y0 * upl + (x0 >> 4) * bm->planes;
#endif
  height = ye - y0 + 1;

  switch (gc0->drawmode) {

    case M_CLEAR:
#ifdef COLOR
      mask = gc0->bgColMask;
      while (--height >= 0) {
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	}
	ptr += upl;
      }
#else
      while (--height >= 0) {
	*ptr &= ~bit;
	ptr += upl;
      }
#endif
      break;

    case M_DRAW:
    case M_TRANSP:
#ifdef COLOR
      mask = gc0->fgColMask;
      while (--height >= 0) {
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] = (ptr[idx] & ~bit) | (mask[idx] & bit);
	}
	ptr += upl;
      }
#else
      while (--height >= 0) {
	*ptr |= bit;
	ptr += upl;
      }
#endif
      break;

    case M_INVERS:
#ifdef COLOR
      while (--height >= 0) {
	idx = planes;
	while (--idx >= 0) {
	  ptr[idx] ^= bit;
	}
	ptr += upl;
      }
#else
      while (--height >= 0) {
	*ptr ^= bit;
	ptr += upl;
      }
#endif
  }
}

