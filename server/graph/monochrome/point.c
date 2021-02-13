/*
 * point.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with single points for the BMONO graphics driver
 *
 * NOTES
 * - This is a straighforward conversion from packed driver
 */

#include <stdio.h>
#include <netinet/in.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "bmono.h"


/*
 *
 */

void FUNCTION(plot)(bm, x0, y0)
     BITMAP *bm;
     long x0;
     long y0;
{
  uchar *ptr, bit;

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return;
  }

  bit = 0x80 >> (x0 & 7);
  ptr = (uchar *)bm->data + y0 * bm->upl * bm->unitsize + (x0 >> 3);

  switch(gc0->drawmode) {
    case M_CLEAR:
      *ptr &= ~bit;
      break;
    case M_DRAW:
    case M_TRANSP:
      *ptr |= bit;
      break;
    case M_INVERS:
      *ptr ^= bit;
      break;
  }
}


/*
 * Without M_TRANSP this could also call plot() and be a generic function...
 */

void FUNCTION(dplot)(bm, x0, y0)
     BITMAP *bm;
     long x0;
     long y0;
{
  ushort *ptr, bit, set;

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return;
  }

  bit = ntohs(0x8000 >> (x0 & 15));
  ptr = (ushort *)bm->data + y0 * (bm->upl << 1)  + (x0 >> 4);
  set = bit & gc0->pattern[y0 & 15];

  switch(gc0->drawmode) {
    case M_CLEAR:
      *ptr &= ~set;
      break;
    case M_DRAW:
      *ptr &= ~bit;
    case M_TRANSP:
      *ptr |= set;
      break;
    case M_INVERS:
      *ptr ^= set;
      break;
  }
}


long FUNCTION(test)(bm, x0, y0)
     BITMAP *bm;
     long x0;
     long y0;
{
  uchar *ptr, bit;

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return -1;
  }

  bit = 0x80 >> (x0 & 7);
  ptr = (uchar *)bm->data + y0 * bm->upl * bm->unitsize + (x0 >> 3);

  if (*ptr & bit) {
    return 1;
  }
  return 0;
}
