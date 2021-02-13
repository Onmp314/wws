/*
 * bitblit.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- bit blitting routines for endian neutral mono graphics driver
 *
 * CHANGES
 * - major speedups in W0R8. ++kay, 10/94
 * - added clipping. ++kay, 1/96
 * - again a little speedup, TeSche 02/96
 */


#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "bmono.h"


static inline void FUNCTION(bitline2right)(sptr, sbit, dptr, dbit, width)
     uchar *sptr;
     char sbit;
     uchar *dptr;
     char dbit;
     int width;
{
  int todo;
  uchar mask;

  if (dbit) {
    /* try to bring the destination bit to #0 of byte
     */
    if ((todo = 8 - dbit) > width) {
      todo = width;
    }
    mask = bfmask8[dbit][todo-1];
    if (sbit) {
      if (sbit + todo > 8) {
	/* this will shift for positive arguments only when 'sbit > dbit',
	 * but 'sbit <= dbit' can't happen because then 'sbit + todo <= 8'
	 * and we wouldn't have come here. therefore this is the only case
	 * where we need to access two bytes from the source.
	 */
	*dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[1] >> (8 - sbit + dbit))) & mask);
	dptr++;
      } else {
	/* these cases are all happy with only one byte from the source. the
	 * case split only ensures positive arguments for the shifts.
	 *
	 * maybe another check for 'sbit == dbit' may improve this more?
	 */
	if (sbit >= dbit) {
	  *dptr = (*dptr & ~mask) | ((*sptr << (sbit - dbit)) & mask);
	} else {
	  *dptr = (*dptr & ~mask) | ((*sptr >> (dbit - sbit)) & mask);
	}
	dptr++;
      }
    } else {
      /* source starts at bit #0? that's really easy...
       */
      *dptr = (*dptr & ~mask) | ((*sptr >> dbit) & mask);
      dptr++;
    }

    /* now see where we've landed in the source
     */
    if ((sbit += todo) & 8) {
      sbit &= 7;
      sptr++;
    }
    /* no need to check for dbit overrun here, as that was precicely our goal
     * and so we can assume that it happened without checking first. in fact
     * we can even move the incrementation of dptr into the upper calculation
     * and use the variable 'dbit' for other purposes from now on.
     */
    if ((width -= todo) <= 0)
      return;
  }

  /* dptr/dbit are now byte aligned
   */
  dbit = 8 - sbit;
  todo = width >> 3;
  if (sbit) {
    while (--todo >= 0) {
      *dptr++ = (sptr[0] << sbit) | (sptr[1] >> dbit);
      sptr++;
    }
  } else {
    while (--todo >= 0) {
      *dptr++ = *sptr++;
    }
  }
  if (!(width &= 7))
    return;

  /* dptr/dbit are still byte aligned, but there're less than 8 pixels to
   * copy left.
   */
  mask = bfmask8[0][width-1];
  if (sbit + width > 8) {
    *dptr = (*dptr & ~mask) | (((sptr[0] << sbit) | (sptr[1] >> dbit)) & mask);
  } else {
    *dptr = (*dptr & ~mask) | ((*sptr << sbit) & mask);
  }
}


static inline void FUNCTION(bitline2left)(sptr, sbit, dptr, dbit, width)
     uchar *sptr;
     char sbit;
     uchar *dptr;
     char dbit;
     int width;
{
  uchar mask;
  int todo;

  /* dbit/sbit point to the first pixel behind what we're going to copy. the
   * scheme is to first calculate how many pixels to copy, then decrement the
   * pointers to get both the actual bit to start copying at and keep the
   * 'one-bit-too-far' kriterion valid for the next stages.
   *
   * first we try to make 'dbit == 0'
   */
  if ((todo = dbit)) {
    if (todo > width) {
      todo = width;
    }
    /* now that we know how many pixels to copy correct the pointers to
     * actually point to the first pixel. this only involves changing 'sptr',
     * because 'dbit' can't become < 0 here, but 'sbit' can.
     */
    dbit -= todo;
    if ((sbit -= todo) < 0) {
      sbit += 8;
      sptr--;
    }
    /* now copy 'todo' bits from 'dptr/dbit' to 'sptr/sbit'. we can't be sure
     * dbit is 0, allthough that's what we want to achieve, but there may be
     * not enough pixels. this part is exactly like in bitline2right.
     */
    mask = bfmask8[dbit][todo-1];
    if (sbit) {
      if (sbit + todo > 8) {
	*dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[1] >> (8 - sbit + dbit))) & mask);
      } else {
	if (sbit >= dbit) {
	  *dptr = (*dptr & ~mask) | ((*sptr << (sbit - dbit)) & mask);
	} else {
	  *dptr = (*dptr & ~mask) | ((*sptr >> (dbit - sbit)) & mask);
	}
      }
    } else {
      *dptr = (*dptr & ~mask) | ((*sptr >> dbit) & mask);
    }
    if ((width -= todo) <= 0)
      return;
  }

  /* 'dptr/dbit' and 'sptr/sbit' still point to the first pixel behind what
   * we're going to copy, but the destination is short-aligned now, say
   * 'dbit == 0'. so copy complete byte now.
   */
  todo = width >> 3;
  if (sbit) {
    dbit = 8 - sbit;
    while (--todo >= 0) {
      sptr--;
      *--dptr = (sptr[0] << sbit) | (sptr[1] >> dbit);
    }
  } else {
    while (--todo >= 0) {
      *--dptr = *--sptr;
    }
  }
  if (!(width &= 7))
    return;

  /* destination is no longer byte aligned because there're less than 8
   * pixels left to copy.
   */
  dbit = 8 - width;
  dptr--;
  if ((sbit -= width) < 0) {
    sbit += 8;
    sptr--;
  }
  mask = bfmask8[dbit][width-1];
  if (sbit) {
    if (sbit + width > 8) {
      *dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[1] >> (8 - sbit + dbit))) & mask);
    } else {
      if (sbit >= dbit) {
	*dptr = (*dptr & ~mask) | ((*sptr << (sbit - dbit)) & mask);
      } else {
	*dptr = (*dptr & ~mask) | ((*sptr >> (dbit - sbit)) & mask);
      }
    }
  } else {
    *dptr = (*dptr & ~mask) | ((*sptr >> dbit) & mask);
  }
}


/*
 * finally the real bitblk function
 */

void FUNCTION(bitblk)(bm0, x0, y0, width, height, bm1, x1, y1)
     BITMAP *bm0;
     long x0;
     long y0;
     long width;
     long height;
     BITMAP *bm1;
     long x1;
     long y1;
{
  uchar *sptr, *dptr;
  int supl, dupl;
  char sbit, dbit;

  if (height <= 0 || width <= 0) {
    return;
  }
  if (CLIP_BITBLIT (x0, y0, width, height, x1, y1, glob_clip0, glob_clip1)) {
    return;
  }

  if (y1 >= y0) {
    y0 += (height - 1);
    y1 += (height - 1);
  }

  supl = bm0->upl * bm0->unitsize;
  dupl = bm1->upl * bm1->unitsize;

  /* the '2left' functions are a lot slower than the '2right' ones, so try to
   * avoid them whenever possible.
   */
  if ((x1 > x0) && (y1 == y0) && (bm1 == bm0) && (x1 < x0+width)) {

    /* this points to one pixel behind what we actually want to copy
     */
    x0 += width;
    x1 += width;
    sbit = x0 & 7;
    dbit = x1 & 7;
    sptr = (uchar *)bm0->data + y0 * supl + (x0 >> 3);
    dptr = (uchar *)bm1->data + y1 * dupl + (x1 >> 3);

    if (y1 >= y0) {
      supl = -supl;
      dupl = -dupl;
    }

    while (--height >= 0) {
      FUNCTION(bitline2left)(sptr, sbit, dptr, dbit, width);
      sptr += supl;
      dptr += dupl;
    }

  } else {
    sbit = x0 & 7;
    dbit = x1 & 7;
    sptr = (uchar *)bm0->data + y0 * supl + (x0 >> 3);
    dptr = (uchar *)bm1->data + y1 * dupl + (x1 >> 3);

    if (y1 >= y0) {
      supl = -supl;
      dupl = -dupl;
    }

    while (--height >= 0) {
      FUNCTION(bitline2right)(sptr, sbit, dptr, dbit, width);
      sptr += supl;
      dptr += dupl;
    }
  }
}
