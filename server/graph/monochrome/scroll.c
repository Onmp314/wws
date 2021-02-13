/*
 * scroll.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- scrolling routines for the BMONO graphics driver
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
 * this one is a specialized version of the bitblk routine, mainly used for
 * scrolling in the same bitmap, it uses aligned copy where possible and is
 * therefore a *bit* :) faster...
 */

void FUNCTION(scroll)(bm, x0, y0, width, height, y1)
     BITMAP *bm;
     long x0;
     long y0;
     long width;
     long height;
     long y1;
{
  uchar *sptr, *dptr, lbit, lmask, rmask;
  int upl = bm->upl * bm->unitsize;
  int todo, count;
  int x1 = x0;

  if ((y0 == y1) || !width) {
    return;
  }

  if (CLIP_BITBLIT (x0, y0, width, height, x1, y1, glob_clip0, glob_clip0)) {
    return;
  }

  if (y1 < y0) {
    /* scroll up */
    sptr = (uchar *)bm->data + y0 * upl + (x0 >> 3);
    dptr = (uchar *)bm->data + y1 * upl + (x0 >> 3);
  } else {
    /* scroll down */
    sptr = (uchar *)bm->data + (y0 + height - 1) * upl + (x0 >> 3);
    dptr = (uchar *)bm->data + (y1 + height - 1) * upl + (x0 >> 3);
    upl = -upl;
  }

  rmask = lmask = 0;
  if ((lbit = x0 & 7)) {
    if ((todo = 8 - lbit) > width) {
      todo = width;
    }
    lmask = bfmask8[lbit][todo-1];
    width -= todo;
  }

  if ((todo = width & 7)) {
    rmask = bfmask8[0][todo-1];
  }

  count = width >> 3;
  upl -= count;

  if (lmask && rmask) {

    upl--;
    while (--height >= 0) {
      *dptr = (*dptr & ~lmask) | (*sptr & lmask);
      dptr++;
      sptr++;

      todo = count;
      while (--todo >= 0) {
	*dptr++ = *sptr++;
      }

      *dptr = (*dptr & ~rmask) | (*sptr & rmask);
      sptr += upl;
      dptr += upl;
    }

  } else {
    
    /* one the edges needs to be masked? */
    if (lmask || rmask) {

      if (lmask) {
	upl--;
      }
      while (--height >= 0) {

	if (lmask) {
	  *dptr = (*dptr & ~lmask) | (*sptr & lmask);
	  dptr++;
	  sptr++;
	}

	todo = count;
	while (--todo >= 0) {
	  *dptr++ = *sptr++;
	}

	if (rmask) {
	  *dptr = (*dptr & ~rmask) | (*sptr & rmask);
	}

	sptr += upl;
	dptr += upl;
      }
    } else {

      while (--height >= 0) {

	todo = count;
	while (--todo >= 0) {
	  *dptr++ = *sptr++;
	}

	sptr += upl;
	dptr += upl;
      }
    }
  }
}

