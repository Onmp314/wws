/*
 * direct8.c, a part of the W Window System
 *
 * Copyright (C) 1994-1998 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- driver for direct 8 bit color displays (CG6, VGA)
 *
 * - only very straightforward routines, not very optimized...
 *
 * CHANGES
 * ++kay 1/96:
 * - removed the *box and *circle functions to use the generic ones instead.
 * - added dplot, dline and changed the other d* functions to use the new
 *   patterns.
 * - applied sign extension bug fixes to printchar/printstring.
 * - scale delta, dx, dy by 2 in line() and dline().
 * - added clipping (untested).
 * TeSche 01/96:
 * - lots of changes due to global context and clipping variables.
 * - optimized dhline & dvline.
 * - added real but slow clipping for printchar (untested).
 * Phx 02/96:
 * - included string.h for bcopy().
 * ++eero 11/96:
 * - added new normalc() / printc() character functions.
 * - added graphics modes to dvline() and dhline().
 * - added graphics modes to dplot() & dline() and fixed dhline().
 * ++eero 10/97:
 * - added createbm().
 * ++eero 5/98:
 * - changed INVERS mode to switch between fg and bg colors
 *   (ie. XOR with (fg XOR bg)).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "direct8.h"


/* this defines what the prints is perpended with */
#define FUNCTION(name) direct8_ ## name

/*
 * allocate a bitmap suitable for driver functions
 */
BITMAP *
direct8_createbm(bm, width, height, do_alloc)
	BITMAP *bm;
	short width;
	short height;
	short do_alloc;
{
  *bm = theScreen->bm;
  bm->width = width;
  bm->height = height;
  bm->upl = (width + 3) & ~3;	/* unitsize 1, but aligned to long */
  bm->type = BM_DIRECT8;

  if (do_alloc) {
    if (!(bm->data = malloc (height * bm->unitsize * bm->upl)))
      return NULL;
  } else {
    bm->data = NULL;
  }

  return bm;
}

/*
 * mouse stuff
 */

static uchar direct8_mbg[16*16];
static MOUSEPOINTER *painted;


void direct8_mouseShow (MOUSEPOINTER *pointer)
{
  register uchar fgCol, bgCol, *sptr, *bptr = direct8_mbg;
  register ushort *moptr = pointer->mask, *mpptr = pointer->icon;
  register short dx, dy;
  short w, h;

  register ushort startBit = 0x8000;

  w = h = 16;
  dx = glob_mouse.real.x0 + pointer->xDrawOffset;
  dy = glob_mouse.real.y0 + pointer->yDrawOffset;

  /* FIXME: what if this moves the pointer out of the lower right edge?
   * w or h would be negative then, and nobody checks this so far!!!!
   */

  if (dx < 0) {
    /* test for clipping at left edge? */
    w += dx;
    startBit >>= -dx;
    dx = 0;
  } else {
    /* test for clipping at right edge? */
    short tmp = theScreen->bm.width - dx;
    if (tmp < 16)
      w = tmp;
  }

  if (dy < 0) {
    /* test for clipping at upper edge */
    h += dy;
    moptr -= dy;
    mpptr -= dy;
    dy = 0;
  } else {
    /* test for clipping at lower edge */
    short tmp = theScreen->bm.height - dy;
    if (tmp < 16)
      h = tmp;
  }

  glob_mouse.drawn.x1 = (glob_mouse.drawn.x0 = dx) + (glob_mouse.drawn.w = w) -1;
  glob_mouse.drawn.y1 = (glob_mouse.drawn.y0 = dy) + (glob_mouse.drawn.h = h) -1;

  sptr = ((uchar *)theScreen->bm.data) + dy * theScreen->bm.upl + dx;

  fgCol = FGCOL_INDEX;
  bgCol = BGCOL_INDEX;

  /* dx and dy are no longer needed from now on
   */

  dy = h;
  while (--dy >= 0) {

    register ushort moval = *moptr++, mpval = *mpptr++, bit = startBit;
    register uchar *ptr = sptr;

    dx = w;
    while (--dx >= 0) {
      *bptr++ = *ptr;
      if (mpval & bit)
	*ptr = fgCol;
      else if (moval & bit)
	*ptr = bgCol;
      bit >>= 1;
      ptr++;
    }

    sptr += theScreen->bm.upl;
  }

  painted = pointer;
}


void direct8_mouseHide (void)
{
  register uchar *sptr, *bptr = direct8_mbg;
  register short y;

  sptr = ((uchar *)theScreen->bm.data) +
    glob_mouse.drawn.y0 * theScreen->bm.upl +
      glob_mouse.drawn.x0;

  y = glob_mouse.drawn.h;
  while (--y >= 0 ) {
#if 1
    memcpy (sptr, bptr, glob_mouse.drawn.w);
    bptr += glob_mouse.drawn.w;
#else
    register uchar *ptr = sptr;
    register short x = glob_mouse.drawn.w;
    while (--x >= 0)
      *ptr++ = *bptr++;
#endif
    sptr += theScreen->bm.upl;
  }
}


/*
 *
 */

void direct8_plot (register BITMAP *bm,
		   register long x0,
		   register long y0)
{
  register uchar *dst = ((uchar *)bm->data) + y0 * bm->upl + x0;

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return;
  }

  switch (gc0->drawmode) {
    case M_CLEAR:
      *dst = gc0->bgCol;
      break;
    case M_DRAW:
    case M_TRANSP:
      *dst = gc0->fgCol;
      break;
    case M_INVERS:
      *dst ^= gc0->fgCol ^ gc0->bgCol;
      break;
  }
}


void direct8_dplot (register BITMAP *bm,
		    register long x0,
		    register long y0)
{
  register uchar *dst;
  ushort bit;

  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return;
  }

  bit = 0x8000 >> (x0 & 15);
  dst = ((uchar *)bm->data) + y0 * bm->upl + x0;
  if (gc0->pattern[y0 & 15] & bit) {
    switch (gc0->drawmode) {
      case M_CLEAR:
	*dst = gc0->bgCol;
	break;
      case M_DRAW:
      case M_TRANSP:
	*dst = gc0->fgCol;
	break;
      case M_INVERS:
	*dst ^= gc0->fgCol ^ gc0->bgCol;
	break;
    }
  } else {
    if (gc0->drawmode == M_DRAW) {
	*dst = gc0->bgCol;
    }
  }
}


long direct8_test (register BITMAP *bm,
		   register long x0,
		   register long y0)
{
  if (CLIP_POINT (x0, y0, glob_clip0)) {
    return -1;
  }

  return *(((uchar *)bm->data) + y0 * bm->upl + x0);
}


void direct8_line (register BITMAP *bm,
		   register long x0,
		   register long y0,
		   register long xe,
		   register long ye)
{
  register long dx, dy, delta, bpl, yi;
  register uchar *ptr, col = 0;   /* keep gcc happy */
  long swap, ox0, oy0;

  /* we always draw from left to right
   */
  if (xe < x0) {
    swap = x0;
    x0 = xe;
    xe = swap;
    swap = y0;
    y0 = ye;
    ye = swap;
  }

  dx = xe - x0;	/* this is guaranteed to be positive (or zero) */

  yi = 1;
  bpl = bm->upl;
  if ((dy = ye-y0) < 0) {
    yi = -1;
    dy = -dy;
    bpl = -bpl;
  }

  ox0 = x0;
  oy0 = y0;
  if (LINE_NEEDS_CLIPPING(x0, y0, xe, ye, glob_clip0)) {
    if (CLIP_LINE (x0, y0, xe, ye, glob_clip0)) {
      return;
    }
  }

  ptr = ((uchar *)bm->data) + y0 * bm->upl + x0;

  switch (gc0->drawmode) {
     case M_INVERS:
      col = gc0->fgCol ^ gc0->bgCol;
      break;
    case M_CLEAR:
      col = gc0->bgCol;
      break;
    default:
      col = gc0->fgCol;
  }

  if (FirstPoint) {
    if (gc0->drawmode == M_INVERS) {
      *ptr ^= col;
    } else {
      *ptr = col;
    }
  }

  /* scale delta, dx, dy by 2 to avoid inaccuracies when dividing by 2.
   * kay.
   */
  switch (gc0->drawmode) {
    case M_CLEAR: /* clear a line */
    case M_DRAW: /* draw a line */
    case M_TRANSP:
      if (dx < dy) {
	/* steep lines */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += bpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    ptr++;
	  }
	  *ptr = col;
	}
      } else {
	/* flat lines */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0++;
	  ptr++;
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += bpl;
	  }
	  *ptr = col;
	}
      }
      break;

    case M_INVERS: /* invert a line */
      if (dx < dy) {
	/* steep lines */
      delta = clipped_line_delta (dy, dx, y0-oy0);
      dx += dx;
      dy += dy;
      while (y0 != ye) {
	y0 += yi;
	ptr += bpl;
	delta -= dx;
	if (delta <= 0) {
	  delta += dy;
	  ptr++;
	}
	*ptr ^= col;
      }
    } else {
      /* flat lines */
      delta = clipped_line_delta (dx, dy, x0-ox0);
      dx += dx;
      dy += dy;
      while (x0 != xe) {
	x0++;
	ptr++;
	delta -= dy;
	if (delta <= 0) {
	  delta += dx;
	  ptr += bpl;
	}
	*ptr ^= col;
      }
    }
  }
}


void direct8_hline (register BITMAP *bm,
		    register long x0,
		    register long y0,
		    register long xe)
{
  register long count;
  register uchar col, *dst;
  register ulong colcol;
  long swap;

  if (xe < x0) {
    swap = x0;
    x0 = xe;
    xe = swap;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

  count = xe - x0 + 1;
  dst = ((uchar *)bm->data) + y0 * bm->upl + x0;

  switch (gc0->drawmode) {

    case M_CLEAR:
    memset (dst, gc0->bgCol, count);
    break;

    case M_DRAW:
    case M_TRANSP:
    memset (dst, gc0->fgCol, count);
    break;

    case M_INVERS:
      col = gc0->fgCol ^ gc0->bgCol;
      colcol = col | col << 8;
      colcol |= colcol << 16;
      while (count && ((unsigned long)dst & 3)) {
	*dst++ ^= col;
	count--;
      }
      while (count & ~3) {
	*((unsigned long *)dst) ^= colcol;
        dst += 4;
	count -= 4;
      }
      while (count--) {
	*dst++ ^= col;
      }
  }
}


void direct8_vline (register BITMAP *bm,
		    register long x0,
		    register long y0,
		    register long ye)
{
  register long count, bpl = bm->upl;
  register uchar *dst, col;
  long swap;

  if (ye < y0) {
    swap = y0;
    y0 = ye;
    ye = swap;
  }

  if (CLIP_VLINE (x0, y0, ye, glob_clip0)) {
    return;
  }

  count = ye - y0 + 1;
  dst = (uchar *)bm->data + y0 * bpl + x0;

  switch (gc0->drawmode) {

    case M_CLEAR:
      col = gc0->bgCol;
      while (count--) {
	*dst = col;
	dst += bpl;
      }
      break;

    case M_DRAW:
    case M_TRANSP:
      col = gc0->fgCol;
      while (count--) {
	*dst = col;
	dst += bpl;
      }
      break;

    case M_INVERS:
      col = gc0->fgCol ^ gc0->bgCol;
      while (count--) {
	*dst ^= col;
	dst += bpl;
      }
  }
}


void direct8_dline(register BITMAP *bm,
		   register long x0,
		   register long y0,
		   register long xe,
		   register long ye)
{
  register long dx, dy, delta, bpl, yi;
  register uchar *ptr, bgCol, fgCol, Col;
  register ushort *pattern = gc0->pattern;
  long swap, ox0, oy0;

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

  bpl = bm->upl;
  yi = 1;
  if ((dy = ye-y0) < 0) {
    yi = -1;
    dy = -dy;
    bpl = -bpl;
  }

  oy0 = y0;
  ox0 = x0;
  if (LINE_NEEDS_CLIPPING (x0, y0, xe, ye, glob_clip0)) {
    if (CLIP_LINE (x0, y0, xe, ye, glob_clip0)) {
      return;
    }
  }

  bgCol = gc0->bgCol;
  fgCol = gc0->fgCol;
  ptr = ((uchar *)bm->data) + y0 * bm->upl + x0;

  if (FirstPoint) {
    if (pattern[y0 & 15] & (0x8000 >> (x0 & 15))) {
      switch (gc0->drawmode) {
	case M_CLEAR:
	  *ptr = bgCol;
	  break;
	case M_DRAW:
	case M_TRANSP:
	  *ptr = fgCol;
	  break;
	case M_INVERS:
	  *ptr ^= fgCol ^ bgCol;
	  break;
      }
    } else {
      if (gc0->drawmode == M_DRAW) {
	  *ptr = bgCol;
      }
    }
  }

  /*
   * scale delta, dx, dy by 2 to avoid inaccuracies when dividing by 2.
   * kay.
   */
  Col = fgCol;
  switch (gc0->drawmode) {
    case M_CLEAR:
      Col = bgCol;
    case M_TRANSP:
      if (dx < dy) {
	/* steep lines */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += bpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    ptr++;
	  }
	  if (pattern[y0 & 15] & (0x8000 >> (x0 & 15)))
	    *ptr = Col;
	}
      } else {
	/* flat lines */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0++;
	  ptr++;
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += bpl;
	  }
	  if (pattern[y0 & 15] & (0x8000 >> (x0 & 15)))
	    *ptr = Col;
	}
      }
      break;

    case M_DRAW:
      if (dx < dy) {
	/* steep lines */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += bpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    ptr++;
	  }
	  if (pattern[y0 & 15] & (0x8000 >> (x0 & 15)))
	    *ptr = fgCol;
	  else
	    *ptr = bgCol;
	}
      } else {
	/* flat lines */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0++;
	  ptr++;
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += bpl;
	  }
	  if (pattern[y0 & 15] & (0x8000 >> (x0 & 15)))
	    *ptr = fgCol;
	  else
	    *ptr = bgCol;
	}
      }
      break;

    case M_INVERS:
      Col = fgCol ^ bgCol;
      if (dx < dy) {
	/* steep lines */
	delta = clipped_line_delta (dy, dx, y0-oy0);
	dx += dx;
	dy += dy;
	while (y0 != ye) {
	  y0 += yi;
	  ptr += bpl;
	  delta -= dx;
	  if (delta <= 0) {
	    delta += dy;
	    ptr++;
	  }
	  if (pattern[y0 & 15] & (0x8000 >> (x0 & 15)))
	    *ptr ^= Col;
	}
      } else {
	/* flat lines */
	delta = clipped_line_delta (dx, dy, x0-ox0);
	dx += dx;
	dy += dy;
	while (x0 != xe) {
	  x0++;
	  ptr++;
	  delta -= dy;
	  if (delta <= 0) {
	    delta += dx;
	    ptr += bpl;
	  }
	  if (pattern[y0 & 15] & (0x8000 >> (x0 & 15)))
	    *ptr ^= Col;
	}
      }
      break;
  }
}


void direct8_dvline (BITMAP *bm,
		     long x0,
		     register long y0,
		     register long ye)
{
  register long bpl = bm->upl;
  register uchar *dst, bgCol, fgCol;
  register ushort bit, *pattern = gc0->pattern;
  long swap;

  if (ye < y0) {
    swap = y0;
    y0 = ye;
    ye = swap;
  }

  if (CLIP_VLINE (x0, y0, ye, glob_clip0)) {
    return;
  }

  bgCol = gc0->bgCol;
  fgCol = gc0->fgCol;
  dst = (uchar *)bm->data + y0 * bpl + x0;
  bit = 0x8000 >> (x0 & 15);

  switch(gc0->drawmode) {
    case M_CLEAR:
      for ( ; y0 <= ye; ++y0, dst += bpl) {
	if (pattern[y0 & 15] & bit)
	  *dst = bgCol;
      }
      break;

    case M_DRAW:
      for ( ; y0 <= ye; ++y0, dst += bpl) {
	if (pattern[y0 & 15] & bit)
	  *dst = fgCol;
	else
	  *dst = bgCol;
      }
      break;

    case M_TRANSP:
      for ( ; y0 <= ye; ++y0, dst += bpl) {
	if (pattern[y0 & 15] & bit)
	  *dst = fgCol;
      }
      break;

    case M_INVERS:
      bgCol ^= fgCol;
      for ( ; y0 <= ye; ++y0, dst += bpl) {
	if (pattern[y0 & 15] & bit)
	  *dst ^= bgCol;
      }
      break;
  }
}


void direct8_dhline (BITMAP *bm,
		     register long x0,
		     long y0,
		     register long xe)
{
  register uchar *ptr, bgCol, fgCol;
  register ushort bit, pat;
  long	x;

  if (xe < x0) {
    x = x0;
    x0 = xe;
    xe = x;
  }

  if (CLIP_HLINE (x0, y0, xe, glob_clip0)) {
    return;
  }

  bgCol = gc0->bgCol;
  fgCol = gc0->fgCol;
  ptr = ((uchar *)bm->data) + y0 * bm->upl + x0;
  bit = 0x8000 >> (x0 & 15);
  pat = gc0->pattern[y0 & 15];

  switch(gc0->drawmode) {
    case M_CLEAR:
      for ( ; x0 <= xe; ++x0) {
	if (pat & bit) {
	  *ptr = bgCol;
	}
	ptr++;
	if (!(bit >>= 1)) {
	  bit = 0x8000;
	}
      }
      break;

    case M_DRAW:
      for ( ; x0 <= xe; ++x0) {
	if (pat & bit) {
	  *ptr = fgCol;
	} else {
	  *ptr = bgCol;
	}
	ptr++;
	if (!(bit >>= 1)) {
	  bit = 0x8000;
	}
      }
      break;

    case M_TRANSP:
      for ( ; x0 <= xe; ++x0) {
	if (pat & bit) {
	  *ptr = fgCol;
	}
	ptr++;
	if (!(bit >>= 1)) {
	  bit = 0x8000;
	}
      }
      break;

    case M_INVERS:
      bgCol ^= fgCol;
      for ( ; x0 <= xe; ++x0) {
	if (pat & bit) {
	  *ptr ^= bgCol;
	}
	ptr++;
	if (!(bit >>= 1)) {
	  bit = 0x8000;
	}
      }
      break;
  }
}


void direct8_bitblk (register BITMAP *bm,
		     register long x0,
		     register long y0,
		     register long width,
		     register long height,
		     register BITMAP *bm1,
		     register long x1,
		     register long y1)
{
  register uchar *sptr, *dptr;
  register long sbpl, dbpl;

  if (CLIP_BITBLIT (x0, y0, width, height, x1, y1, glob_clip0, glob_clip1)) {
    return;
  }

  sbpl = bm->upl;
  dbpl = bm1->upl;

  /* a by far not optimized check... */

  if (y1 < y0) {

    sptr = ((uchar *)bm->data) + y0 * sbpl + x0;
    dptr = ((uchar *)bm1->data) + y1 * dbpl + x1;

    while (height--) {
      bcopy (sptr, dptr, width);
      sptr += sbpl;
      dptr += dbpl;
    }

  } else {

    sptr = ((uchar *)bm->data) + (y0 + height - 1) * sbpl + x0;
    dptr = ((uchar *)bm1->data) + (y1 + height - 1) * dbpl + x1;

    while (height--) {
      bcopy (sptr, dptr, width);
      sptr -= sbpl;
      dptr -= dbpl;
    }
  }
}


void direct8_scroll (register BITMAP *bm,
		     register long x0,
		     register long y0,
		     register long width,
		     register long height,
		     register long y1)
{
  register uchar *sptr, *dptr;
  register long bpl;
  long x1 = x0;

  if (y0 == y1) {
    return;
  }

  if (CLIP_BITBLIT (x0, y0, width, height, x1, y1, glob_clip0, glob_clip0)) {
    return;
  }

  bpl = bm->upl;

  if (y1 < y0) {

    /* scrollup == top-down copy */

    sptr = (uchar *)bm->data + y0 * bpl + x0;
    dptr = (uchar *)bm->data + y1 * bpl + x0;

    while (height--) {
      bcopy (sptr, dptr, width);
      sptr += bpl;
      dptr += bpl;
    }

  } else {

    /* scrolldown == bottom-up copy */

    sptr = (uchar *)bm->data + (y0 + height - 1) * bpl + x0;
    dptr = (uchar *)bm->data + (y1 + height - 1) * bpl + x0;

    while (height--) {
      bcopy (sptr, dptr, width);
      sptr -= bpl;
      dptr -= bpl;
    }
  }
}


/* Do all the text styles with 'clipping'. */
static void FUNCTION(clipc)(bm, x0, y0, xoff, yoff, sskip, cwidth, cheight, c)
     BITMAP *bm;
     long x0;
     long y0;
     long xoff;
     long yoff;
     long sskip;
     long cwidth;
     long cheight;
     ulong c;
{
  register short lcwidth;
  register uchar *dptr, *dlptr;
  register ulong *cptr, cdata, cbit, fbit, prevbit;
  register ushort light, lighten = 0;	/* zero, bits indicate clearing here */
  register long dbpl;
  FONT *font = gc0->font;
  ushort *pattern = NULL;
  uchar bgCol = gc0->bgCol, fgCol = gc0->fgCol;
  ulong bold = 0, skew = 0;

  /* set styles */

  if (font->effects & F_BOLD) {
    bold = 0xffffffff;
   }

  if (font->effects & F_LIGHT) {
    pattern = gc0->pattern;
  }

  if (font->effects & F_ITALIC) {
    skew = font->hdr.skew;
  }

  if (font->effects & F_REVERSE) {
    fgCol = gc0->bgCol;
    bgCol = gc0->fgCol;
  } else {
    fgCol = gc0->fgCol;
    bgCol = gc0->bgCol;
  }

  dbpl = bm->upl;
  dptr = (uchar *)bm->data + y0 * dbpl + x0;

  c &= 0xff;

  /* jump over clipped bits */
  lcwidth = yoff * font->widths[c] + xoff;
  xoff = font->widths[c] - cwidth;
  cptr = font->data + font->offsets[c] + (lcwidth >> 5);
  cbit = 0x80000000 >> (lcwidth & 31);
  cdata = *cptr++;

  while (--cheight >= 0) {

    if (skew & 1) {

      if (sskip == 1) {

	/* skip leftmost chracter pixel and increase skip */
	if (!(cbit >>= 1)) {
	  cbit = 0x80000000;
	  cdata = *cptr++;
	}
	cwidth--;
	xoff++;

      } else {

	if (sskip < 0) {	/* right side clipping on, decrease */
	  sskip++;
	  cwidth++;
	  xoff--;
	} else {
	  if (sskip > 1) {		/* left side clipping */
	    sskip--;
	  }
	}
        dptr--;
      }

      skew |= 0x10000;
    }
    skew >>= 1;

    if (pattern) {
      /* `not' because later used to _clear_ a pixel */
      lighten = ~pattern[cheight & 15];
    }
    light = 0x8000;
    prevbit = 0;
    dlptr = dptr;
    lcwidth = cwidth;

    while (--lcwidth >= 0) {

      fbit = (cdata & cbit) | (bold & prevbit);
      prevbit = fbit & cbit;
      /* can't just AND fbit & lighten because chardata is a bitstream */
      if(light & lighten) {
	fbit = 0;
      }
      if (fbit) {
	*dlptr++ = fgCol;
      } else {
	*dlptr++ = bgCol;
      }
      if(!(light >>= 1)) {	/* asm: rol.w */
	light = 0x8000;
      }
      if (!(cbit >>= 1)) {
	cbit = 0x80000000L;
	cdata = *cptr++;
      }
    }
    dptr += dbpl;

    lcwidth = xoff;
    while (--lcwidth >= 0) {
      if (!(cbit >>= 1)) {
	cbit = 0x80000000;
	/* as with font[255] this might go over the mallocated area, I
	 * allocate one additional long for font data in font loading.
	 */
	cdata = *cptr++;
      }
    }
  }
}

/* Do all the text styles, no clipping. */
static void FUNCTION(stylec) (BITMAP *bm,
	long x0,
	long y0,
	ulong c)
{
  register short lcwidth, cwidth;
  register short cheight;
  register uchar *dptr, *dlptr;
  register ulong *cptr, cdata, cbit, fbit, prevbit;
  register ushort light, lighten = 0;	/* zero, bits indicate clearing here */
  register long dbpl;
  FONT *font = gc0->font;
  ushort *pattern = NULL;
  uchar bgCol = gc0->bgCol, fgCol = gc0->fgCol;
  ulong bold = 0, skew = 0;

  c &= 0xff;
  cheight = font->hdr.height;
  cwidth = font->widths[c];

  /* set styles */

  if (font->effects & F_BOLD) {
    bold = 0xffffffff;
   }

  if (font->effects & F_LIGHT) {
    pattern = gc0->pattern;
  }

  if (font->effects & F_ITALIC) {
    skew = font->hdr.skew;
  }

  if (font->effects & F_REVERSE) {
    fgCol = gc0->bgCol;
    bgCol = gc0->fgCol;
  } else {
    fgCol = gc0->fgCol;
    bgCol = gc0->bgCol;
  }

  dbpl = bm->upl;
  dptr = (uchar *)bm->data + y0 * dbpl + x0;
  cbit = 0x80000000L;
  cptr = font->data + font->offsets[c];
  cdata = *cptr++;

  while (--cheight >= 0) {

    if (skew & 1) {
      skew |= 0x10000;
      dptr--;
    }
    skew >>= 1;

    if (pattern) {
      /* `not' because later used to _clear_ a pixel */
      lighten = ~pattern[cheight & 15];
    }
    light = 0x8000;
    prevbit = 0;
    dlptr = dptr;
    lcwidth = cwidth;

    while (--lcwidth >= 0) {

      fbit = (cdata & cbit) | (bold & prevbit);
      prevbit = fbit & cbit;
      /* can't just AND fbit & lighten because chardata is a bitstream */
      if(light & lighten) {
	fbit = 0;
      }
      if (fbit) {
	*dlptr++ = fgCol;
      } else {
	*dlptr++ = bgCol;
      }
      if(!(light >>= 1)) {	/* asm: rol.w */
	light = 0x8000;
      }
      if (!(cbit >>= 1)) {
	cbit = 0x80000000L;
	cdata = *cptr++;
      }
    }
    dptr += dbpl;
  }
}


# if 0

/* do normal text, no clipping */
static inline void FUNCTION(normalc) (BITMAP *bm,
	long x0,
	long y0,
	ulong c)
{
  register short lcwidth, cwidth;
  register short cheight;
  register uchar *dptr, *dlptr;
  register ulong *cptr, cdata, cbit;
  register long dbpl;
  FONT *font = gc0->font;
  uchar bgCol = gc0->bgCol, fgCol = gc0->fgCol;

  c &= 0xff;
  cheight = font->hdr.height;
  cwidth = font->widths[c];

  dbpl = bm->upl;
  dptr = (uchar *)bm->data + y0 * dbpl + x0;
  cbit = 0x80000000L;
  cptr = font->data + font->offsets[c];
  cdata = *cptr++;

  while (--cheight >= 0) {
    dlptr = dptr;
    lcwidth = cwidth;
    while (--lcwidth >= 0) {
      if (cdata & cbit)
	*dlptr++ = fgCol;
      else
	*dlptr++ = bgCol;
      if (!(cbit >>= 1)) {
	cbit = 0x80000000L;
	cdata = *cptr++;
      }
    }
    dptr += dbpl;
  }
}

#endif

/* do character strings with above two functions */
#include "../generic/generic_prints.h"

