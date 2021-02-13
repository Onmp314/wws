/*
 * point.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with single points for the packed graphics driver
 *
 * CHANGES
 * ++kay, 1/96:
 * - added dplot().
 * - added clipping.
 * ++TeSche 04/96:
 * - bug in dplot() fixed, color support.
 * ++eero, 8/96:
 * - M_TRANSP mode.
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "packed.h"


/*
 *
 */

void FUNCTION(plot)(bm, x0, y0)
     register BITMAP *bm;
     register long x0;
     register long y0;
{
#ifdef MONO
  register ulong *ptr, bit;
#else
  register ushort *ptr, bit;
#if defined(COLOR)
  register short idx = bm->planes;
  register ushort *colMask;
#endif
#endif

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return;
  }

#ifdef MONO
  bit = 0x80000000 >> (x0 & 31);
  ptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);
#else
  bit = 0x8000 >> (x0 & 15);
  ptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * bm->planes;
#endif

#ifdef COLOR
  switch(gc0->drawmode) {
    case M_CLEAR:
      colMask = gc0->bgColMask;
      while (--idx >= 0) {
	*ptr++ = (*ptr & ~bit) | (*colMask++ & bit);
      }
      break;
    case M_DRAW:
    case M_TRANSP:
      colMask = gc0->fgColMask;
      while (--idx >= 0) {
	*ptr++ = (*ptr & ~bit) | (*colMask++ & bit);
      }
      break;
    case M_INVERS:
      while (--idx >= 0) {
	*ptr++ ^= bit;
      }
      break;
  }
#else
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
#endif
}


/*
 * Without M_TRANSP this could also call plot() and be a generic function...
 */

void FUNCTION(dplot)(bm, x0, y0)
     register BITMAP *bm;
     register long x0;
     register long y0;
{
#ifdef MONO
  register ulong *ptr, bit, set;
#else
#ifdef COLOR
  register short idx = bm->planes;
  register ushort *colMask;
#else
  register ushort set;
#endif
  register ushort *ptr, bit;
#endif

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return;
  }

#ifdef MONO
  bit = 0x80000000 >> (x0 & 31);
  ptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);
#else
  bit = 0x8000 >> (x0 & 15);
  ptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * bm->planes;
#endif

#ifdef COLOR
  switch(gc0->drawmode) {
    case M_CLEAR:
      bit &= gc0->pattern[y0 & 15];
      colMask = gc0->bgColMask;
      while (--idx >= 0) {
	*ptr++ = (*ptr & ~bit) | (*colMask++ & bit);
      }
      break;

    case M_DRAW:
      if(bit & gc0->pattern[y0 & 15]) {
	colMask = gc0->fgColMask;
      } else {
        colMask = gc0->bgColMask;
      }
      while (--idx >= 0) {
	*ptr++ = (*ptr & ~bit) | (*colMask++ & bit);
      }
      break;

    case M_TRANSP:
      bit &= gc0->pattern[y0 & 15];
      colMask = gc0->fgColMask;
      while (--idx >= 0) {
        *ptr++ = (*ptr & ~bit) | (*colMask++ & bit);
      }
      break;

    case M_INVERS:
      if(bit & gc0->pattern[y0 & 15]) {
	while (--idx >= 0)
	  *ptr++ ^= bit;
      }
      break;
  }
#else
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
#endif
}


long FUNCTION(test)(bm, x0, y0)
     register BITMAP *bm;
     register long x0;
     register long y0;
{
#ifdef MONO
  register ulong *ptr, bit;
#else
  register ushort *ptr, bit;
#ifdef COLOR
  register short idx = bm->planes;
  register long ret = 0, mask = 1;
#endif
#endif

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return -1;
  }

#ifdef MONO
  bit = 0x80000000 >> (x0 & 31);
  ptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);
#else
  bit = 0x8000 >> (x0 & 15);
  ptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * bm->planes;
#endif

#ifdef COLOR
  while (--idx >= 0) {
    if (*ptr++ & bit) {
      ret |= mask;
    }
    mask <<= 1;
  }
  return ret;
#else
  if (*ptr & bit) {
    return 1;
  }
  return 0;
#endif
}
