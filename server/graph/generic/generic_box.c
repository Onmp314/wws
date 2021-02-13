/*
 * generic_box.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer, Kay Roemer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with boxes
 *
 * CHANGES
 *  ++eero, 1/96:
 * - added proper patterned versions of box and pbox.
 * ++kay, 1/96:
 * - changed eero's pbox optimizations into something more portable and
 *   equally fast.
 * - added comments on how to speed up things.
 * ++TeSche, 01/96:
 * - bug fixed and clipping optimized in (d)pbox().
 * ++eero, 2/98:
 * - added wide lined boxes.
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "generic.h"


/*
 *
 */


static void thickbox (bm, x0, y0, width, height, hline_fn, vline_fn)
     register BITMAP *bm;
     register long x0;
     register long y0;
     register long width;
     register long height;
     register void (*hline_fn) (BITMAP*,long,long,long);
     register void (*vline_fn) (BITMAP*,long,long,long);
{
  long i, lwd = gc0->linewidth >> 1;
  REC *oclip = glob_clip0;

  x0 -= lwd;
  y0 -= lwd;
  lwd = lwd << 1;
  if (lwd >= width || lwd >= height) {
    width += x0 + lwd - 1;		/* the other endpoint */
    height += lwd;
    while (--height >= 0) {
      (*hline_fn) (bm, x0, y0++, width);
    }
    return;
  }

  width += lwd;
  height += lwd;
  if (!BOX_NEEDS_CLIPPING (x0, y0, width, height, glob_clip0)) {
    glob_clip0 = NULL;
  }

  width += x0 - 1;			/* the other endpoint */
  i = ++lwd;
  while (--i >= 0) {
	  (*hline_fn) (bm, x0, y0++, width);
  }

  height += y0 - 2*lwd - 1;		/* the other endpoint */
  i = lwd;
  while (--i >= 0) {
	  (*vline_fn) (bm, x0+i, y0, height);
	  (*vline_fn) (bm, width-i, y0, height);
  }

  while (--lwd >= 0) {
	  (*hline_fn) (bm, x0, ++height, width);
  }

  glob_clip0 = oclip;
}


void generic_box (bm, x0, y0, width, height)
     register BITMAP *bm;
     register long x0;
     register long y0;
     register long width;
     register long height;
{
  if (gc0->linewidth > 1) {
    thickbox(bm, x0, y0, width, height, theScreen->hline, theScreen->vline);
    return;
  }

  if (!width || !height) {
    return;
  }

  if (height == 1) {
    (*theScreen->hline) (bm, x0, y0, x0+width-1);
    return;
  }

  if (width == 1) {
    (*theScreen->vline) (bm, x0, y0, y0+height-1);
    return;
  }

  (*theScreen->hline) (bm, x0, y0, x0+width-2);
  (*theScreen->vline) (bm, x0+width-1, y0, y0+height-2);
  (*theScreen->hline) (bm, x0+width-1, y0+height-1, x0+1);
  (*theScreen->vline) (bm, x0, y0+height-1, y0+1);
}


void generic_pbox (bm, x0, y0, width, height)
     register BITMAP *bm;
     register long x0;
     register long y0;
     register long width;
     register long height;
{
  register void (*line_fn) (BITMAP *, long, long, long);
  REC *oclip = glob_clip0;

  if (!width || !height) {
    return;
  }

  if (height == 1) {
    (*theScreen->hline) (bm, x0, y0, x0+width-1);
    return;
  }

  if (width == 1) {
    (*theScreen->vline) (bm, x0, y0, y0+height-1);
    return;
  }

  if (CLIP_BOX (x0, y0, width, height, glob_clip0)) {
    return;
  }

  /* now tell vline/hline not to perform clipping, we did it already
   * ourself.
   */
  glob_clip0 = NULL;

  if (width+width >= height) {
    line_fn = theScreen->hline;
    width = x0 + width - 1;	/* the other endpoint */
    while (--height >= 0)
      (*line_fn) (bm, x0, y0++, width);
  } else {
    line_fn = theScreen->vline;
    height = y0 + height - 1;
    while (--width >= 0)
      (*line_fn) (bm, x0++, y0, height);
  }
  glob_clip0 = oclip;
}


/*
 * patterned versions
 */

void generic_dbox (bm, x0, y0, width, height)
     register BITMAP *bm;
     register long x0;
     register long y0;
     register long width;
     register long height;
{
  if (gc0->linewidth > 1) {
    thickbox(bm, x0, y0, width, height, theScreen->dhline, theScreen->dvline);
    return;
  }

  if (!width || !height)
    return;

  if (height == 1) {
    (*theScreen->dhline) (bm, x0, y0, x0 + width - 1);
    return;
  }

  if (width == 1) {
    (*theScreen->dvline) (bm, x0, y0, y0 + height - 1);
    return;
  }

  (*theScreen->dhline) (bm, x0, y0, x0 + width - 2);
  (*theScreen->dvline) (bm, x0 + width - 1, y0, y0 + height - 2);
  (*theScreen->dhline) (bm, x0 + width - 1, y0 + height - 1, x0 + 1);
  (*theScreen->dvline) (bm, x0, y0 + height - 1, y0 + 1);
}


void generic_dpbox (bm, x0, y0, width, height)
     register BITMAP *bm;
     register long x0;
     register long y0;
     register long width;
     register long height;
{
  register void (*dline_fn) (BITMAP *, long, long, long);
  REC *oclip = glob_clip0;

  if (!width || !height)
    return;

  if (height == 1) {
    (*theScreen->dhline) (bm, x0, y0, x0 + width - 1);
    return;
  }

  if (width == 1) {
    (*theScreen->dvline) (bm, x0, y0, y0 + height - 1);
    return;
  }

  if (CLIP_BOX (x0, y0, width, height, glob_clip0)) {
    return;
  }

  /* now tell vline/hline not to perform clipping, we did it already
   * ourself.
   */
  glob_clip0 = NULL;

  /*
   * hlines's are about twice as fast as vlines.
   */
  if (width+width >= height) {
    dline_fn = theScreen->dhline;
    width = x0 + width - 1;	/* the other endpoint */
    while (--height >= 0)
      (*dline_fn) (bm, x0, y0++, width);
  } else {
    dline_fn = theScreen->dvline;
    height = y0 + height - 1;
    while (--width >= 0)
      (*dline_fn) (bm, x0++, y0, height);
  }
  glob_clip0 = oclip;
}
