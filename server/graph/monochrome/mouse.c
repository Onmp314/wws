/*
 * mouse.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- mouse show/hide/move routines
 *
 * NOTES
 * - This is a straighforward conversion from packed driver
 */

#include <stdio.h>
#include <netinet/in.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "bmono.h"


/*
 * the background buffer (& other stuff)
 */

static MOUSEPOINTER *painted;
static ulong invMouseMask[16];
static ulong backGroundMask[16];


/*
 * the mouse functions
 */

void FUNCTION(mouseShow) (MOUSEPOINTER *pointer)
{
  ushort *ptr, *moptr = pointer->mask, *mpptr = pointer->icon;
  ulong *bptr = backGroundMask, *mptr = invMouseMask, mask, data;
  short bit, height;
  long wpl;
  short dx, dy;
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
  wpl = theScreen->bm.upl << 1;
  ptr = (ushort *)theScreen->bm.data + dy * wpl + (dx >> 4);

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

      /* be carefull, this is a long-access to a short-boundary!  luckily
       * this is no problem for Ataris, Amigas or Intel PCs.  however, if you
       * want to run it on a Sparc with BW2 graphics this will cause a
       * bus-error.  you would then have to use a construct like `(ptr[0] <<
       * 16) + ptr[1]'.
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
      *bptr++ = (data = htonl(*(ulong *)ptr)) & (mask = *moptr++ << bit);
      *(ulong *)ptr = ntohl((data & (*mptr++ = ~mask)) | (*mpptr++ << bit));

      ptr += wpl;
    }

  } else {

    /* accessing 'ptr' as long would mean to touch the next line here, so
     * we've got to deal with this special case seperately. we shift in the
     * opposite direction as above and use only 16 bit of the long values.
     * this yields no danger as all the values are unsigned.
     */
    while (--height >= 0) {

      *bptr++ = (data = htons(*ptr)) & (mask = *moptr++ >> bit);
      *ptr = ntohs((data & (*mptr++ = ~mask)) | (*mpptr++ >> bit));

      ptr += wpl;
    }
  }

  painted = pointer;
}


void FUNCTION(mouseHide) (void)
{
  ushort *ptr;
  ulong *bptr, *mptr;
  short height;
  long wpl;

  /* hiding the mouse means restore the background. thanks to the effort we've
   * spend in storing it, this can be done really fast (i.e. we don't need any
   * shifts anymore).
   */
  wpl = theScreen->bm.upl << 1;
  ptr = (ushort *)theScreen->bm.data + glob_mouse.drawn.y0 * wpl + (glob_mouse.drawn.x0 >> 4);

  bptr = backGroundMask;
  mptr = invMouseMask;

  height = glob_mouse.drawn.h;

  if (theScreen->bm.width - glob_mouse.drawn.x0 > 16) {

    while (--height >= 0) {

      /* this is what we'll do in long terms:
       *
       * data = *(ulong *)ptr;   "read data"
       * data &= *mptr++;        "clear the mouse mask"
       * data |= *bptr++;        "insert old background"
       * *(ulong *)ptr = data;   "write data"
       *
       * and in short terms:
       */
      *(ulong *)ptr = (*(ulong *)ptr & ntohl(*mptr++)) | ntohl(*bptr++);
      ptr += wpl;
    }

  } else {

    /* accessing 'ptr' as long would mean to touch the next line here, so
     * we've got to deal with this special case seperately. just like when
     * drawing the mouse we use only 16 bit, but still need no shifts.
     */
    while (--height >= 0) {
      *ptr = (*ptr & ntohs(*mptr++)) | ntohs(*bptr++);
      ptr += wpl;
    }
  }
}
