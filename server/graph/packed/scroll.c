/*
 * scroll.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- scrolling routines for the packed graphics driver
 *
 * CHANGES
 * ++kay, 10/94:
 * - major speedups in W0R8.
 * - added clipping.
 * ++TeSche 04/96:
 * - splitted into seperate functions, speedups, color support.
 * ++eero 10/96:
 * - Made away with two variables and moved some multiplies outside loop
 *   on COLOR scroll.
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "packed.h"


/*
 * this one is a specialized version of the bitblk routine, mainly used for
 * scrolling in the same bitmap, it uses aligned copy where possible and is
 * therefore a *bit* :) faster...
 *
 * TeSche 01/96: why didn't the MONO version use 32-bit access so far?
 * TeSche 03/96: why did I waste so many registers? where were the 68020 calls?
 */

#ifdef MONO

void FUNCTION(scroll)(bm, x0, y0, width, height, y1)
     BITMAP *bm;
     register long x0;
     register long y0;
     long width;
     register long height;
     register long y1;
{
  register ulong *sptr, *dptr;
  register short todo;
  register long x1 = x0;
  register long upl = bm->upl;

  if ((y0 == y1) || !width) {
    return;
  }

  if (CLIP_BITBLIT (x0, y0, width, height, x1, y1, glob_clip0, glob_clip0)) {
    return;
  }

  if (y1 < y0) {
    /* scroll up */
    sptr = (ulong *)bm->data + y0 * upl + (x0 >> 5);
    dptr = (ulong *)bm->data + y1 * upl + (x0 >> 5);
  } else {
    /* scroll down */
    sptr = (ulong *)bm->data + (y0 + height - 1) * upl + (x0 >> 5);
    dptr = (ulong *)bm->data + (y1 + height - 1) * upl + (x0 >> 5);
    upl = -upl;
  }

  /* 'y0', 'y1' and 'x1' are no longer needed now, they now take the purpose of
   * 'lbit', 'lmask' and 'rmask'
   */
  y1 = 0;
  if ((y0 = x0 & 31)) {
    if ((todo = 32 - y0) > width) {
      todo = width;
    }
#ifdef __mc68020__
    y1 = todo;
#else
    y1 = bfmask32[y0][todo-1];
#endif
    width -= todo;
  }

#ifdef __mc68020__
  x1 = width & 31;
#else
  if ((todo = width & 31)) {
    x1 = bfmask32[0][todo-1];
  }
#endif
  /* 'todo' is no longer needed now, it now takes the purpose of 'tmp'
   * (better this way than using x0, because todo is a short).
   */

  /* 'x0' is no longer needed now, it now takes the purpose of 'nunits' */
  x0 = width >> 5;
  upl -= x0;

  /* As scrolling is used (mainly?) for scrolling whole window content's
   * width, the left window border (ATM width 4) makes sure that leftmost
   * edge needs to be masked, and on average only one window out of 31
   * (MONO) / 15 (!MONO) aligns it's content's right edge along long/short
   * border.  So...  It's worth to test for a case where both of the edges
   * need to be masked.
   */

  if (y1 && x1) {

    upl--;
    while (--height >= 0) {

#ifdef __mc68020__
      __asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
			    "\tbfins %0,%4@{%2:%3}"
			    : "=&d"(todo)
			    : "a"(sptr), "d"(y0), "d"(y1), "a"(dptr)
			    : "memory");
#else
      *dptr = (*dptr & ~y1) | (*sptr & y1);
#endif
      dptr++;
      sptr++;

      todo = x0;
      while (--todo >= 0) {
	*dptr++ = *sptr++;
      }

#ifdef __mc68020__
      __asm__ __volatile__ ("bfextu %1@{#0:%2},%0\n"
			    "\tbfins %0,%3@{#0:%2}"
			    : "=&d"(todo)
			    : "a"(sptr), "d"(x1), "a"(dptr)
			    : "memory");
#else
      *dptr = (*dptr & ~x1) | (*sptr & x1);
#endif

      sptr += upl;
      dptr += upl;
    }

  } else {
    
    /* one the edges needs to be masked? */
    if (y1 || x1) {

      if (y1) {
	upl--;
      }
      while (--height >= 0) {

	if (y1) {
#ifdef __mc68020__
	  __asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
				"\tbfins %0,%4@{%2:%3}"
				: "=&d"(todo)
				: "a"(sptr), "d"(y0), "d"(y1), "a"(dptr)
				: "memory");
#else
	  *dptr = (*dptr & ~y1) | (*sptr & y1);
#endif
	  dptr++;
	  sptr++;
	}

	todo = x0;
	while (--todo >= 0) {
	  *dptr++ = *sptr++;
	}

	if (x1) {
#ifdef __mc68020__
	  __asm__ __volatile__ ("bfextu %1@{#0:%2},%0\n"
				"\tbfins %0,%3@{#0:%2}"
				: "=&d"(todo)
				: "a"(sptr), "d"(x1), "a"(dptr)
				: "memory");
#else
	  *dptr = (*dptr & ~x1) | (*sptr & x1);
#endif
	}

	sptr += upl;
	dptr += upl;
      }
    } else {

      /* If you need really fast scrolling, make a W_CONTAINER window
       * and open into that a subwindow that doesn't have borders. :-)))
       */

      while (--height >= 0) {

	todo = x0;
	while (--todo >= 0) {
	  *dptr++ = *sptr++;
	}

	sptr += upl;
	dptr += upl;
      }
    }
  }
}

#elif defined(COLORMONO)

/*
 * the colormono version
 */

void FUNCTION(scroll)(bm, x0, y0, width, height, y1)
     BITMAP *bm;
     register long x0;
     register long y0;
     long width;
     register long height;
     register long y1;
{
  register ushort *sptr, *dptr;
  register short todo;
  register long x1 = x0, planes = bm->planes, upl = bm->upl;

  if ((y0 == y1) || !width) {
    return;
  }

  if (CLIP_BITBLIT (x0, y0, width, height, x1, y1, glob_clip0, glob_clip0)) {
    return;
  }

  if (y1 < y0) {
    /* scroll up */
    sptr = (ushort *)bm->data + y0 * upl + (x0 >> 4) * planes;
    dptr = (ushort *)bm->data + y1 * upl + (x0 >> 4) * planes;
  } else {
    /* scroll down */
    sptr = (ushort *)bm->data + (y0 + height - 1) * upl + (x0 >> 4) * planes;
    dptr = (ushort *)bm->data + (y1 + height - 1) * upl + (x0 >> 4) * planes;
    upl = -upl;
  }

  /* 'y0', 'y1' and 'x1' are no longer needed now, they now take the purpose
   * of 'lbit', 'lmask' and 'rmask'
   */
  y1 = 0;
  if ((y0 = x0 & 15)) {
    if ((todo = 16 - y0) > width) {
      todo = width;
    }
#ifdef __mc68020__
    y1 = todo;
#else
    y1 = bfmask16[y0][todo-1];
#endif
    width -= todo;
  }

#ifdef __mc68020__
  x1 = width & 15;
#else
  if ((todo = width & 15)) {
    x1 = bfmask16[0][todo-1];
  }
#endif
  /* 'todo' is no longer needed now, it now takes the purpose of 'tmp' */

  /* 'x0' is no longer needed now, it now takes the purpose of 'nunits' */
  x0 = width >> 4;
  upl -= x0 * planes;
  if (y1) {
    upl -= planes;
  }

  while (--height >= 0) {

    if (y1) {
#ifdef __mc68020__
      __asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
			    "\tbfins %0,%4@{%2:%3}"
			    : "=&d"(todo)
			    : "a"(sptr), "d"(y0), "d"(y1), "a"(dptr)
			    : "memory");
#else
      *dptr = (*dptr & ~y1) | (*sptr & y1);
#endif
      dptr += planes;
      sptr += planes;
    }

    todo = x0;
    while (--todo >= 0) {
      *dptr = *sptr;
      sptr += planes;
      dptr += planes;
    }

    if (x1) {
#ifdef __mc68020__
      __asm__ __volatile__ ("bfextu %1@{#0:%2},%0\n"
			    "\tbfins %0,%3@{#0:%2}"
			    : "=&d"(todo)
			    : "a"(sptr), "d"(x1), "a"(dptr)
			    : "memory");
#else
      *dptr = (*dptr & ~x1) | (*sptr & x1);
#endif
    }
    sptr += upl;
    dptr += upl;
  }
}

#else /* COLOR */

/*
 * the color version
 */

void FUNCTION(scroll)(bm, x0, y0, width, height, y1)
     BITMAP *bm;
     register long x0;
     register long y0;
     long width;
     register long height;
     register long y1;
{
  register ushort *sptr, *dptr;
  register short idx, todo;
  register long x1 = x0, planes = bm->planes, upl = bm->upl;

  if ((y0 == y1) || !width) {
    return;
  }

  if (CLIP_BITBLIT (x0, y0, width, height, x1, y1, glob_clip0, glob_clip0)) {
    return;
  }

  if (y1 < y0) {
    /* scroll up */
    sptr = (ushort *)bm->data + y0 * upl + (x0 >> 4) * planes;
    dptr = (ushort *)bm->data + y1 * upl + (x0 >> 4) * planes;
  } else {
    /* scroll down */
    sptr = (ushort *)bm->data + (y0 + height - 1) * upl + (x0 >> 4) * planes;
    dptr = (ushort *)bm->data + (y1 + height - 1) * upl + (x0 >> 4) * planes;
    upl = -upl;
  }

  /* 'y0', 'y1' and 'x1' are no longer needed now, they now take the purpose
   * of 'lbit', 'lmask' and 'rmask'
   */
  y1 = 0;
  if ((y0 = x0 & 15)) {
    if ((todo = 16 - y0) > width) {
      todo = width;
    }
#ifdef __mc68020__
    y1 = todo;
#else
    y1 = bfmask16[y0][todo-1];
#endif
    width -= todo;
  }

#ifdef __mc68020__
  x1 = width & 15;
#else
  if ((todo = width & 15)) {
    x1 = bfmask16[0][todo-1];
  }
#endif
  /* 'todo' is no longer needed now, it now takes the purpose of 'tmp' */

  /* 'x0' is no longer needed now, it now takes the purpose of 'nunits' */
  x0 = (width >> 4) * planes;
  upl -= x0;
  if (y1) {
    upl -= planes;
  }
  if (x1) {
    upl -= planes;
  }

  while (--height >= 0) {

    if (y1) {
      idx = planes;
      while (--idx >= 0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
			      "\tbfins %0,%4@{%2:%3}"
			      : "=&d"(todo)
			      : "a"(sptr), "d"(y0), "d"(y1), "a"(dptr)
			      : "memory");
	dptr++;
	sptr++;
#else
	*dptr++ = (*dptr & ~y1) | (*sptr++ & y1);
#endif
      }
    }

    todo = x0;
    while (--todo >= 0) {
      *dptr++ = *sptr++;
    }

    if (x1) {
      idx = planes;
      while (--idx >= 0) {
#ifdef __mc68020__
	__asm__ __volatile__ ("bfextu %1@{#0:%2},%0\n"
			      "\tbfins %0,%3@{#0:%2}"
			      : "=&d"(todo)
			      : "a"(sptr), "d"(x1), "a"(dptr)
			      : "memory");
	dptr++;
	sptr++;
#else
	*dptr++ = (*dptr & ~x1) | (*sptr++ & x1);
#endif
      }
    }

    sptr += upl;
    dptr += upl;
  }
}

#endif
