/*
 * mouse.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * mouse show/hide/move routines for the packed graphics driver
 *
 * CHANGES
 * ++TeSche 04-11/96:
 * - real color support + some enhancements.
 * - support for multiple mouse pointer icons.
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "packed.h"


/*
 * the background buffer (& other stuff)
 */

static MOUSEPOINTER *painted;
static ulong invMouseMask[16];
#ifdef COLOR
static ulong backGroundMask[16*8];   /* worst case: TT-low */
#else
static ulong backGroundMask[16];
#endif


/*
 * the mouse functions
 */

void FUNCTION(mouseShow) (MOUSEPOINTER *pointer)
{
  register ushort *ptr, *moptr = pointer->mask, *mpptr = pointer->icon;
  register ulong *bptr = backGroundMask, *mptr = invMouseMask, mask, data;
  register short bit, height;
  register long wpl;
#ifndef MONO
#ifdef COLOR
  register short idx;
#endif
  register short planes = theScreen->bm.planes;
#endif
  register short dx, dy;
  short w;
  int xStart = 0;

  dx = glob_mouse.real.x0 + pointer->xDrawOffset;
  dy = glob_mouse.real.y0 + pointer->yDrawOffset;

  /* FIXME: what if this moves the pointer out of the lower right edge?
   * w or h would be negative then, and nobody checks this so far!!!!
   */

  height = 16;
  if (dy < 0) {
    /* test for clipping at upper edge */
    height += dy;
    moptr -= dy;
    mpptr -= dy;
    dy = 0;
  } else {
    /* test for clipping at lower edge */
    short tmp = theScreen->bm.height - dy;
    if (tmp < 16)
      height = tmp;
  }

  w = 16;
  if (dx < 0) {
    /* test for clipping at left edge? */
    w += dx;
    xStart = -dx;
    dx = 0;
  } else {
    /* test for clipping at right edge? */
    short tmp = theScreen->bm.width - dx;
    if (tmp < 16)
      w = tmp;
  }

  glob_mouse.drawn.x1 = (glob_mouse.drawn.x0 = dx) + (glob_mouse.drawn.w = w) - 1;
  glob_mouse.drawn.y1 = (glob_mouse.drawn.y0 = dy) + (glob_mouse.drawn.h = height) - 1;

  /* draw the mouse, hmm, this merely are bit block operations on a block which
   * size is definitely known and which targets are definitely word aligned.
   */

  bit = dx & 15;
#ifdef MONO
  wpl = theScreen->bm.upl << 1;
  ptr = (ushort *)theScreen->bm.data + dy * wpl + (dx >> 4);
#else
  wpl = theScreen->bm.upl;
  ptr = (ushort *)theScreen->bm.data + dy * wpl + (dx >> 4) * planes;
#endif

  if (theScreen->bm.width - dx > 16) {

    /* not clipped at right edge. variable 'bit' is not needed in here, we only
     * need '16-bit'
     */
#if 1
    bit = 16 - bit + xStart;   /* compensate for clip at left edge */
#else
    bit = 16 - bit;
#endif

    while (--height >= 0) {

#ifdef MONO

      /* be carefull, this is a long-access to a short-boundary! luckily this
       * is no problem for a 68000'er, and nobody so far wanted to run this on
       * any other processor. however, if you want to run it on a Sparc with
       * BW2 graphics this will cause a bus-error. you would then have to use
       * a construct like `(ptr[0] << 16) + ptr[1]'.
       *
       * this is what we'll do in long terms:
       *
       * data = *(ulong *)ptr;      "read data"
       * mask = *moptr++ << bit;    "create mouse pointer mask"
       * *bptr++ = data & mask;     "save background under mask"
       * data &= ~mask;             "clear data in mask"
       * data |= *mpptr++ << bit;   "insert mouse data in mask"
       * *(ulong *)ptr = data;      "store data"
       *
       * and in short terms:
       */
      *bptr++ = (data = *(ulong *)ptr) & (mask = *moptr++ << bit);
      *(ulong *)ptr = (data & (*mptr++ = ~mask)) | (*mpptr++ << bit);

#else /* !MONO */

      /* this is very much the same like in MONO, except that it uses a
       * construct like 'ptr[0]<<16 | ptr[planes]' to get/put the data.
       */
      *mptr++ = ~(mask = *moptr++ << bit);

#ifdef COLOR
      /* this will do all the planes except the first one
       */
      idx = planes;
      while (--idx > 0) {
	*bptr++ = (data = (ptr[idx] << 16) | ptr[idx+planes]) & mask;
	data &= ~mask;
	ptr[idx] = data >> 16;
	ptr[idx+planes] = data;
      }
#endif
      /* this will do the first plane
       */
      *bptr++ = (data = (ptr[0] << 16) | ptr[planes]) & mask;
      data = (data & ~mask) | (*mpptr++ << bit);
      ptr[0] = data >> 16;
      ptr[planes] = data;

#endif /* !MONO */
      ptr += wpl;
    }

  } else {

    /* accessing 'ptr' as long would mean to touch the next line here, so
     * we've got to deal with this special case seperately. we shift in the
     * opposite direction as above and use only 16 bit of the long values.
     * this yields no danger as all the values are unsigned.
     */
    while (--height >= 0) {

#ifdef MONO

      *bptr++ = (data = *ptr) & (mask = *moptr++ >> bit);
      *ptr = (data & (*mptr++ = ~mask)) | (*mpptr++ >> bit);

#else /* !MONO */

      *mptr++ = ~(mask = *moptr++ >> bit);

#ifdef COLOR
      idx = planes;
      while (--idx > 0) {
	*bptr++ = (data = ptr[idx]) & mask;
	data = (data & ~mask);
	ptr[idx] = data;
      }
#endif
      *bptr++ = (data = ptr[0]) & mask;
      data = (data & ~mask) | (*mpptr++ >> bit);
      ptr[0] = data;

#endif /* !MONO */
      ptr += wpl;
    }
  }

  painted = pointer;
}


void FUNCTION(mouseHide) (void)
{
  register ushort *ptr;
  register ulong *bptr, *mptr;
  register short height;
#ifndef MONO
#ifdef COLOR
  register short idx;
#endif
  register ulong data, mask;
  register short planes = theScreen->bm.planes;
#endif
  register long wpl;

  /* hiding the mouse means restore the background. thanks to the effort we've
   * spend in storing it, this can be done really fast (i.e. we don't need any
   * shifts anymore).
   */
#ifdef MONO
  wpl = theScreen->bm.upl << 1;
  ptr = (ushort *)theScreen->bm.data + glob_mouse.drawn.y0 * wpl + (glob_mouse.drawn.x0 >> 4);
#else
  wpl = theScreen->bm.upl;
  ptr = (ushort *)theScreen->bm.data + glob_mouse.drawn.y0 * wpl + (glob_mouse.drawn.x0 >> 4) * planes;
#endif

  bptr = backGroundMask;
  mptr = invMouseMask;

  height = glob_mouse.drawn.h;

  if (theScreen->bm.width - glob_mouse.drawn.x0 > 16) {

    while (--height >= 0) {

#ifdef MONO
      /* this is what we'll do in long terms:
       *
       * data = *(ulong *)ptr;   "read data"
       * data &= *mptr++;        "clear the mouse mask"
       * data |= *bptr++;        "insert old background"
       * *(ulong *)ptr = data;   "write data"
       *
       * and in short terms:
       */
      *(ulong *)ptr = (*(ulong *)ptr & *mptr++) | *bptr++;

#else /* !MONO */

      /* this is very much the same like in MONO, except that it uses a
       * construct like 'ptr[0]<<16 | ptr[planes]' to get/put the data.
       */
      mask = *mptr++;

#ifdef COLORMONO
      data = (((ptr[0] << 16) | ptr[planes]) & mask) | *bptr++;
      ptr[0] = data >> 16;
      ptr[planes] = data;
#else
      idx = planes;
      while (--idx >= 0) {
	data = (((ptr[idx] << 16) | ptr[idx+planes]) & mask) | *bptr++;
	ptr[idx] = data >> 16;
	ptr[idx+planes] = data;
      }
#endif

#endif /* !MONO */
      ptr += wpl;
    }

  } else {

    /* accessing 'ptr' as long would mean to touch the next line here, so
     * we've got to deal with this special case seperately. just like when
     * drawing the mouse we use only 16 bit, but still need no shifts.
     */
    while (--height >= 0) {

#ifdef MONO

      *ptr = (*ptr & *mptr++) | *bptr++;

#else /* !MONO */

      mask = *mptr++;

#ifdef COLORMONO
      ptr[0] = (ptr[0] & mask) | *bptr++;
#else
      idx = planes;
      while (--idx >= 0) {
	ptr[idx] = (ptr[idx] & mask) | *bptr++;
      }
#endif

#endif /* !MONO */
      ptr += wpl;
    }
  }
}
