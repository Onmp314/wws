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
 * - bit blitting routines for the Atari packed graphics driver
 *
 * CHANGES
 * - major speedups in W0R8. ++kay, 10/94
 * - added clipping. ++kay, 1/96
 * - again a little speedup, TeSche 02/96
 */


/* this will disable bitfield instructions for color modes. they can't be
 * used because they access the *next* short when more bits are needed, they
 * don't obey the 'planes' value. telling them to do so would result in
 * copying only that much bits that no short boundaries are crossed, and that
 * would cause a major slowdown.
 */


#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "packed.h"


/*
 * note: if you're compiling this on any other processor than a motorolla 680x0
 * be aware that parts of this code make long-accesses on short-boundaries, and
 * may thus cause bus-errors if the processor can't do that. so far this hasn't
 * been a problem at all; in contrary the fast bitline2* functions are 4 times
 * faster than the slow ones working with the bfmasks.
 */


/*
 * this now really seems to be very close to the maximum speed achievable with
 * plain c-programming having in mind a 68000 cpu model, i.e. some idea how
 * many registers there're and what variables to put into them. further
 * optimization is *very* tricky, as already slight changes can lead gcc to
 * put other variables into registers than optimal and thus loose 20% of speed
 * without giving you very much of a chance to understand what went wrong.
 *
 * some things I've checked in particular and which are NOT faster than this:
 *
 * - make MONO version operate on long-boundaries and with bfmask32
 *   (the fast bitline versions operate on short-boundaries anyway and for the
 *   single slow bitline that's done per call it's not worth the trouble)
 *
 * some things I do have checked to be faster than the previous versions:
 *
 * - lots of range checks reduce to bittests rather than compares
 * - get rid of some code because of implicit information about the state of
 *   some variables (all fast bitline function try to put dptr on a short-
 *   boundary fist)
 * - get rid of a redundand variable (use variable sbit for two purposes)
 */



#ifdef MONO

/*
 * this bitline-copy-routine knows that the lines it works on are padded to
 * longs, and can therefore with an easy check do the job of the two seperate
 * functions that were needed up to W1R3. the 68000 version of this routine
 * is about 30% faster than W1R3, and the 68020 version about 100%. if you
 * are thinking about a simple loop with only 68020 bitfield instructions
 * in it: this routine is only about 70% faster than W1R3 (about the speed of
 * the 68000 version of this). if you doubt this, test this one:
 */

#if 0
static inline void bitline2right_mono(sptr, sbit, dptr, dbit, width)
     register ulong *sptr;
     register long sbit;
     register ulong *dptr;
     register long dbit;
     register short width;
{
  register ulong data;
  register short todo;
  
  while (width > 0) {
    todo = MIN(32, width);
    __asm__ __volatile__ ("bfextu %0@{%1:%2},%3\n"
			  "\tbfins %3,%4@{%5:%2}"
			  : /* no outputs */
			  : "a"(sptr), "d"(sbit), "d"(todo), "d"(data), "a"(dptr), "d"(dbit)
			  : "memory");
    sptr++;
    dptr++;
    width -= todo;
  }
}
#endif

static inline void FUNCTION(bitline2right)(sptr, sbit, dptr, dbit, width)
     register ulong *sptr;
     register long sbit;
     register ulong *dptr;
     register long dbit;
     register short width;
{
  register short todo;
  register ulong mask;

  if (dbit) {
    /* try to bring the destination bit to #0 of a long
     */
    if ((todo = 32 - dbit) > width) {
      todo = width;
    }
#if defined(__mc68020__)
    __asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
			  "\tbfins %0,%4@{%5:%3}"
			  : "=&d"(mask)
			  : "a"(sptr), "d"(sbit), "d"(todo), "a"(dptr), "d"(dbit)
			  : "memory");
    dptr++;
#else
    mask = bfmask32[dbit][todo-1];
    if (sbit) {
      if (sbit + todo > 32) {
	/* this will shift for positive arguments only when 'sbit > dbit',
	 * but 'sbit <= dbit' can't happen because then 'sbit + todo <= 32'
	 * and we wouldn't have come here. therefore this is the only case
	 * where we need to access two longs from the source.
	 */
	*dptr++ = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[1] >> (32 - sbit + dbit))) & mask);
      } else {
	/* these cases are all happy with only one long from the source. the
	 * case split only ensures positive arguments for the shifts.
	 *
	 * maybe another check for 'sbit == dbit' may improve this more?
	 */
	if (sbit >= dbit) {
	  *dptr++ = (*dptr & ~mask) | ((*sptr << (sbit - dbit)) & mask);
	} else {
	  *dptr++ = (*dptr & ~mask) | ((*sptr >> (dbit - sbit)) & mask);
	}
      }
    } else {
      /* source starts at bit #0? that's really easy...
       */
      *dptr++ = (*dptr & ~mask) | ((*sptr >> dbit) & mask);
    }
#endif
    /* now see where we've landed in the source
     */
    if ((sbit += todo) & 32) {
      sbit &= 31;
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

  /* dptr/dbit are now long aligned
   */
  dbit = 32 - sbit;
  todo = width >> 5;
#if defined(__mc68020__)
  while (--todo >= 0) {
    __asm__ __volatile__ ("bfextu %1@{%2:#0},%0"
			  : "=&d"(mask)
			  : "a"(sptr), "d"(sbit)
			  : "memory");
    *dptr++ = mask;
    sptr++;
  }
#else
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
#endif
  if (!(width &= 31))
    return;

  /* dptr/dbit are still long aligned, but there're less than 32 pixels to
   * copy left.
   */
#if defined(__mc68020__)
  __asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
			"\tbfins %0,%4@{#0:%3}"
			: "=&d"(mask)
			: "a"(sptr), "d"(sbit), "d"(width), "a"(dptr)
			: "memory");
#else
  mask = bfmask32[0][width-1];
  if (sbit + width > 32) {
    *dptr = (*dptr & ~mask) | (((sptr[0] << sbit) | (sptr[1] >> dbit)) & mask);
  } else {
    *dptr = (*dptr & ~mask) | ((*sptr << sbit) & mask);
  }
#endif
}

#elif defined(COLORMONO)

/*
 * the same for emulated monochrome-on-color screens and short alignments
 */

static inline void FUNCTION(bitline2right)(sptr, sbit, splanes, dptr, dbit, dplanes, width)
     register ushort *sptr;
     register long sbit;
     register long splanes;
     register ushort *dptr;
     register long dbit;
     register long dplanes;
     register long width;
{
  register long mask, todo;

  if (dbit) {
    if ((todo = (16 - dbit)) > width) {
      todo = width;
    }
    mask = bfmask16[dbit][todo-1];
    if (sbit) {
      if (sbit + todo > 16) {
	*dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[splanes] >> (16 - sbit + dbit))) & mask);
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
    if ((sbit += todo) & 16) {
      sbit &= 15;
      sptr += splanes;
    }
    if ((width -= todo) <= 0)
      return;
    dptr += dplanes;
  }

  dbit = 16 - sbit;
  todo = width >> 4;
  if (sbit) {
    while (--todo >= 0) {
      *dptr = (sptr[0] << sbit) | (sptr[splanes] >> dbit);
      sptr += splanes;
      dptr += dplanes;
    }
  } else {
    while (--todo >= 0) {
      *dptr = *sptr;
      sptr += splanes;
      dptr += dplanes;
    }
  }
  if (!(width &= 15))
    return;

  mask = bfmask16[0][width-1];
  if (sbit + width > 16) {
    *dptr = (*dptr & ~mask) | (((sptr[0] << sbit) | (sptr[splanes] >> dbit)) & mask);
  } else {
    *dptr = (*dptr & ~mask) | ((*sptr << sbit) & mask);
  }
}

#else /* COLOR */

/*
 * the same for real color screens.
 */

static inline void FUNCTION(bitline2right)(sptr, sbit, dptr, dbit, width)
     register ushort *sptr;
     register long sbit;
     register ushort *dptr;
     register long dbit;
     register long width;
{
  register ushort mask;
  register long todo;
  register short idx;
  register short planes = theScreen->bm.planes;

  if (dbit) {
    if ((todo = (16 - dbit)) > width) {
      todo = width;
    }
    mask = bfmask16[dbit][todo-1];
    idx = planes;
    if (sbit) {
      if (sbit + todo > 16) {
	while (--idx >= 0) {
	  dptr[idx] = (dptr[idx] & ~mask) | (((sptr[idx] << (sbit - dbit)) | (sptr[idx+planes] >> (16 - sbit + dbit))) & mask);
	}
      } else {
	if (sbit >= dbit) {
	  while (--idx >= 0) {
	    dptr[idx] = (dptr[idx] & ~mask) | ((sptr[idx] << (sbit - dbit)) & mask);
	  }
	} else {
	  while (--idx >= 0) {
	    dptr[idx] = (dptr[idx] & ~mask) | ((sptr[idx] >> (dbit - sbit)) & mask);
	  }
	}
      }
    } else {
      while (--idx >= 0) {
	dptr[idx] = (dptr[idx] & ~mask) | ((sptr[idx] >> dbit) & mask);
      }
    }
    if ((sbit += todo) & 16) {
      sbit &= 15;
      sptr += planes;
    }
    if ((width -= todo) <= 0)
      return;
    dptr += planes;
  }

  dbit = 16 - sbit;
  todo = (width >> 4) * theScreen->bm.planes;
  if (sbit) {
    while (--todo >= 0) {
      *dptr++ = (sptr[0] << sbit) | (sptr[planes] >> dbit); sptr++;
    }
  } else {
    while (--todo >= 0) {
      *dptr++ = *sptr++;
    }
  }
  if (!(width &= 15))
    return;

  mask = bfmask16[0][width-1];
  idx = theScreen->bm.planes;
  if (sbit + width > 16) {
    while (--idx >= 0) {
      *dptr++ = (*dptr & ~mask) | (((sptr[0] << sbit) | (sptr[planes] >> dbit)) & mask);
      sptr++;
    }
  } else {
    while (--idx >= 0) {
      *dptr++ = (*dptr & ~mask) | ((*sptr++ << sbit) & mask);
    }
  }
}

#endif   /* COLOR */


#ifdef MONO

static inline void FUNCTION(bitline2left)(sptr, sbit, dptr, dbit, width)
     register ulong *sptr;
     register long sbit;
     register ulong *dptr;
     register long dbit;
     register long width;
{
  register ulong mask;
  register long todo;

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
      sbit += 32;
      sptr--;
    }
    /* now copy 'todo' bits from 'dptr/dbit' to 'sptr/sbit'. we can't be sure
     * dbit is 0, allthough that's what we want to achieve, but there may be
     * not enough pixels. this part is exactly like in bitline2right.
     */
#if defined(__mc68020__)
    __asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
			  "\tbfins %0,%4@{%5:%3}"
			  : "=&d"(mask)
			  : "a"(sptr), "d"(sbit), "d"(todo), "a"(dptr), "d"(dbit)
			  : "memory");
#else
    mask = bfmask32[dbit][todo-1];
    if (sbit) {
      if (sbit + todo > 32) {
	*dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[1] >> (32 - sbit + dbit))) & mask);
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
#endif
    if ((width -= todo) <= 0)
      return;
  }

  /* 'dptr/dbit' and 'sptr/sbit' still point to the first pixel behind what
   * we're going to copy, but the destination is short-aligned now, say
   * 'dbit == 0'. so copy complete longs now.
   */
  todo = width >> 5;
#if defined(__mc68020__)
  while (--todo >= 0) {
    sptr--;
    dptr--;
    __asm__ __volatile__ ("bfextu %1@{%2:#0},%0"
			  : "=&d"(mask)
			  : "a"(sptr), "d"(sbit)
			  : "memory");
    *dptr = mask;
  }
#else
  if (sbit) {
    dbit = 32 - sbit;
    while (--todo >= 0) {
      sptr--;
      *--dptr = (sptr[0] << sbit) | (sptr[1] >> dbit);
    }
  } else {
    while (--todo >= 0) {
      *--dptr = *--sptr;
    }
  }
#endif
  if (!(width &= 31))
    return;

  /* destination is no longer long aligned because there're less than 32
   * pixels left to copy.
   */
  dbit = 32 - width;
  dptr--;
  if ((sbit -= width) < 0) {
    sbit += 32;
    sptr--;
  }
#if defined(__mc68020__)
  __asm__ __volatile__ ("bfextu %1@{%2:%3},%0\n"
			"\tbfins %0,%4@{%5:%3}"
			: "=&d"(mask)
			: "a"(sptr), "d"(sbit), "d"(width), "a"(dptr), "d"(dbit)
			: "memory");
#else
  mask = bfmask32[dbit][width-1];
  if (sbit) {
    if (sbit + width > 32) {
      *dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[1] >> (32 - sbit + dbit))) & mask);
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
#endif
}

#elif defined (COLORMONO)

/*
 * the same for emulated monochrome-on-color screens and short alignments
 */

static inline void FUNCTION(bitline2left)(sptr, sbit, splanes, dptr, dbit, dplanes, width)
     register ushort *sptr;
     register long sbit;
     register long splanes;
     register ushort *dptr;
     register long dbit;
     register long dplanes;
     register long width;
{
  register ushort mask;
  register long todo;

  if ((todo = dbit)) {
    if (todo > width) {
      todo = width;
    }
    dbit -= todo;
    if ((sbit -= todo) < 0) {
      sbit += 16;
      sptr -= splanes;
    }
    mask = bfmask16[dbit][todo-1];
    if (sbit) {
      if (sbit + todo > 16) {
	*dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[splanes] >> (16 - sbit + dbit))) & mask);
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

  todo = width >> 4;
  dbit = 16 - sbit;
  if (sbit) {
    while (--todo >= 0) {
      dptr -= dplanes;
      sptr -= splanes;
      *dptr = (sptr[0] << sbit) | (sptr[splanes] >> dbit);
    }
  } else {
    while (--todo >= 0) {
      dptr -= dplanes;
      sptr -= splanes;
      *dptr = *sptr;
    }
  }
  if (!(width &= 15))
    return;

  dbit = 16 - width;
  dptr -= dplanes;
  if ((sbit -= width) < 0) {
    sbit += 16;
    sptr -= splanes;
  }
  mask = bfmask16[dbit][width-1];
  if (sbit) {
    if (sbit + width > 16) {
      *dptr = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[splanes] >> (16 - sbit + dbit))) & mask);
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

#else /* COLOR */

/*
 * the same for real color screens
 */

static inline void FUNCTION(bitline2left)(sptr, sbit, dptr, dbit, width)
     register ushort *sptr;
     register long sbit;
     register ushort *dptr;
     register long dbit;
     register long width;
{
  register ushort mask;
  register long todo;
  register short idx;
  register short planes = theScreen->bm.planes;

  if ((todo = dbit)) {
    if (todo > width) {
      todo = width;
    }
    dbit -= todo;
    if ((sbit -= todo) < 0) {
      sbit += 16;
      sptr -= planes;
    }
    mask = bfmask16[dbit][todo-1];
    idx = planes;
    if (sbit) {
      if (sbit + todo > 16) {
	while (--idx >= 0) {
	  dptr[idx] = (dptr[idx] & ~mask) | (((sptr[idx] << (sbit - dbit)) | (sptr[idx+planes] >> (16 - sbit + dbit))) & mask);
	}
      } else {
	if (sbit >= dbit) {
	  while (--idx >= 0) {
	    dptr[idx] = (dptr[idx] & ~mask) | ((sptr[idx] << (sbit - dbit)) & mask);
	  }
	} else {
	  while (--idx >= 0) {
	    dptr[idx] = (dptr[idx] & ~mask) | ((sptr[idx] >> (dbit - sbit)) & mask);
	  }
	}
      }
    } else {
      while (--idx >= 0) {
	dptr[idx] = (dptr[idx] & ~mask) | ((sptr[idx] >> dbit) & mask);
      }
    }
    if ((width -= todo) <= 0)
      return;
  }

  dbit = 16 - sbit;
  todo = (width >> 4) * theScreen->bm.planes;
  if (sbit) {
    while (--todo >= 0) {
      --sptr;
      *--dptr = (sptr[0] << sbit) | (sptr[planes] >> dbit);
    }
  } else {
    while (--todo >= 0) {
      *--dptr = *--sptr;
    }
  }
  if (!(width &= 15))
    return;

  dbit = 16 - width;
  dptr -= planes;
  if ((sbit -= width) < 0) {
    sbit += 16;
    sptr -= planes;
  }
  mask = bfmask16[dbit][width-1];
  idx = theScreen->bm.planes;
  if (sbit) {
    if (sbit + width > 16) {
      while (--idx >= 0) {
	*dptr++ = (*dptr & ~mask) | (((sptr[0] << (sbit - dbit)) | (sptr[planes] >> (16 - sbit + dbit))) & mask);
	sptr++;
      }
    } else {
      if (sbit >= dbit) {
	while (--idx >= 0) {
	  *dptr++ = (*dptr & ~mask) | ((*sptr++ << (sbit - dbit)) & mask);
	}
      } else {
	while (--idx >= 0) {
	  *dptr++ = (*dptr & ~mask) | ((*sptr++ >> (dbit - sbit)) & mask);
	}
      }
    }
  } else {
    while (--idx >= 0) {
      *dptr++ = (*dptr & ~mask) | ((*sptr++ >> dbit) & mask);
    }
  }
}

#endif


/*
 * finally the real bitblk function
 */

void FUNCTION(bitblk)(bm0, x0, y0, width, height, bm1, x1, y1)
     BITMAP *bm0;
     long x0;
     long y0;
     register long width;
     register long height;
     BITMAP *bm1;
     long x1;
     long y1;
{
#ifdef MONO
  register ulong *sptr, *dptr;
#else
  register ushort *sptr, *dptr;
#endif
  long supl, dupl;
  long sbit, dbit;

  /* force gcc *not* to put these variables into registers - we need as much
   * registers as possible for the inline functions! TeSche 02/96
   */
  &bm0; &x0; &y0; &bm1; &x1; &y1; &sbit; &dbit;

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

  supl = bm0->upl;
  dupl = bm1->upl;

  /* the '2left' functions are a lot slower than the '2right' ones, so try to
   * avoid them whenever possible.
   */
  if ((x1 > x0) && (y1 == y0) && (bm1 == bm0) && (x1 < x0+width)) {

    /* this points to one pixel behind what we actually want to copy
     */
    x0 += width;
    x1 += width;
#ifdef MONO
    sbit = x0 & 31;
    dbit = x1 & 31;
    sptr = (ulong *)bm0->data + y0 * supl + (x0 >> 5);
    dptr = (ulong *)bm1->data + y1 * dupl + (x1 >> 5);
#else
    sbit = x0 & 15;
    dbit = x1 & 15;
    sptr = (ushort *)bm0->data + y0 * supl + (x0 >> 4) * bm0->planes;
    dptr = (ushort *)bm1->data + y1 * dupl + (x1 >> 4) * bm1->planes;
#endif

    if (y1 >= y0) {
      supl = -supl;
      dupl = -dupl;
    }

    while (--height >= 0) {
#ifdef COLORMONO
      FUNCTION(bitline2left)(sptr, sbit, bm0->planes, dptr, dbit, bm1->planes, width);
#else /* COLOR || MONO */
      FUNCTION(bitline2left)(sptr, sbit, dptr, dbit, width);
#endif
      sptr += supl;
      dptr += dupl;
    }

  } else {

#ifdef MONO
    sbit = x0 & 31;
    dbit = x1 & 31;
    sptr = (ulong *)bm0->data + y0 * supl + (x0 >> 5);
    dptr = (ulong *)bm1->data + y1 * dupl + (x1 >> 5);
#else
    sbit = x0 & 15;
    dbit = x1 & 15;
    sptr = (ushort *)bm0->data + y0 * supl + (x0 >> 4) * bm0->planes;
    dptr = (ushort *)bm1->data + y1 * dupl + (x1 >> 4) * bm1->planes;
#endif

    if (y1 >= y0) {
      supl = -supl;
      dupl = -dupl;
    }

    while (--height >= 0) {
#ifdef COLORMONO
      FUNCTION(bitline2right)(sptr, sbit, bm0->planes, dptr, dbit, bm1->planes, width);
#else
      FUNCTION(bitline2right)(sptr, sbit, dptr, dbit, width);
#endif
      sptr += supl;
      dptr += dupl;
    }
  }
}
