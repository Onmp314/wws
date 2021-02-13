/*
 * text.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- character output routines for the Atari packed graphics driver
 *
 * CHANGES
 * ++eero 3/97
 * - Character functions are now driver specific and prints a
 *   generic function.  Prints implements underline effect.
 * ++eero 2/98
 * - Added clipping character drawing functions.
 *
 * TODO:
 * - Optimize the color and 68020 versions like TeSche's original printc()
 *   functions were.
 */

#include <stdio.h>
#include <string.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "packed.h"


#ifdef COLOR

/* Do all the text styles, with clipping. */
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
  register ulong *cptr, cbit, cdata, fbit, prevbit;
  register ushort *dptr, *dlptr, dlbit;
  register ushort *mask, lighten = 0xffff;
  register short lcwidth, idx;
  FONT *font = gc0->font;
  short planes = bm->planes;
  ushort dbit, *bgmask, *fgmask, *pattern = NULL;
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
    fgmask = gc0->bgColMask;
    bgmask = gc0->fgColMask;
  } else {
    fgmask = gc0->fgColMask;
    bgmask = gc0->bgColMask;
  }

  dbit = 0x8000 >> (x0 & 15);
  dptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * planes;

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
	/* shift lower part of character (co-ordinate) to left */
	if (dbit >= 0x8000) {
	  dptr -= planes;
	  dbit = 1;
	} else {
	  dbit <<= 1;
	}
      }

      skew |= 0x10000;	/* could also be rolled like `lighten' */
    }
    skew >>= 1;

    if (pattern) {
      lighten = pattern[cheight & 15];
    }
    prevbit = 0;
    dlbit = dbit;
    dlptr = dptr;
    lcwidth = cwidth;

    while (--lcwidth >= 0) {

      fbit = cdata & cbit;
      fbit |= bold & prevbit;
      prevbit = fbit & cbit;
      if (fbit) {
	mask = fgmask;
      } else {
	mask = bgmask;
      }
      idx = planes;
      while (--idx >= 0) {
	dlptr[idx] = (dlptr[idx] & ~dlbit) | (mask[idx] & (dlbit & lighten));
      }

      if (!(cbit >>= 1)) {
	cbit = 0x80000000;
	cdata = *cptr++;
      }
      if (!(dlbit >>= 1)) {
	dlbit = 0x8000;
	dlptr += planes;
      }
    }
    dptr += bm->upl;

    lcwidth = xoff;
    while (--lcwidth >= 0) {
      if (!(cbit >>= 1)) {
	cbit = 0x80000000;
	/* as with font[255] this might go over the mallocated area, I
	 * allocate one additional long for font data i font loading.
	 */
	cdata = *cptr++;
      }
    }
  }
}


/* Do all the text styles, no clipping. */
static inline void FUNCTION(stylec)(bm, x0, y0, c)
     BITMAP *bm;
     long x0;
     long y0;
     ulong c;
{
  register ulong *cptr, cbit, cdata, fbit, prevbit;
  register ushort *dptr, *dlptr, dlbit;
  register ushort *mask, lighten = 0xffff;
  register short lcwidth, idx;
  FONT *font = gc0->font;
  short cheight, cwidth, planes = bm->planes;
  ushort dbit, *bgmask, *fgmask, *pattern = NULL;
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
    fgmask = gc0->bgColMask;
    bgmask = gc0->fgColMask;
  } else {
    fgmask = gc0->fgColMask;
    bgmask = gc0->bgColMask;
  }

  dbit = 0x8000 >> (x0 & 15);
  dptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * planes;

  cptr = font->data + font->offsets[c];
  cbit = 0x80000000;
  cdata = *cptr++;

  while (--cheight >= 0) {

    if (skew & 1) {
      skew |= 0x10000;	/* could also be rolled like `lighten' */
      /* shift lower part of character (co-ordinate) to left */
      if (dbit >= 0x8000) {
	dptr -= planes;
	dbit = 1;
      } else {
	dbit <<= 1;
      }
    }
    skew >>= 1;
    if (pattern) {
      lighten = pattern[cheight & 15];
    }
    prevbit = 0;
    dlbit = dbit;
    dlptr = dptr;
    lcwidth = cwidth;

    while (--lcwidth >= 0) {

      fbit = cdata & cbit;
      fbit |= bold & prevbit;
      prevbit = fbit & cbit;
      if (fbit) {
	mask = fgmask;
      } else {
	mask = bgmask;
      }
      idx = planes;
      while (--idx >= 0) {
	dlptr[idx] = (dlptr[idx] & ~dlbit) | (mask[idx] & (dlbit & lighten));
      }

      if (!(cbit >>= 1)) {
	cbit = 0x80000000;
	cdata = *cptr++;
      }
      if (!(dlbit >>= 1)) {
	dlbit = 0x8000;
	dlptr += planes;
      }
    }
    dptr += bm->upl;
  }
}


#else  /* !COLOR */


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
  register ulong *cptr, cbit, cdata, fbit, prevbit;
#ifdef MONO
  register ulong *dptr, *dlptr, dlbit, dldata;
  ulong dbit, lighten = 0xffffffff;
#else
  register ushort *dptr, *dlptr, dlbit, dldata;
  ushort dbit, lighten = 0xffff;
#endif
  register short lcwidth;
  short reverse = 0;
  ushort *pattern = NULL;
  FONT *font = gc0->font;
  ulong bold = 0, skew = 0;

  /* set styles */
  reverse = font->effects & F_REVERSE;

  if (font->effects & F_BOLD) {
    bold = 0xffffffff;
   }

  if (font->effects & F_LIGHT) {
    pattern = gc0->pattern;
  }

  if (font->effects & F_ITALIC) {
    skew = font->hdr.skew;
  }

#ifdef MONO
  dbit = 0x80000000 >> (x0 & 31);
  dptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);
#else
  dbit = 0x8000 >> (x0 & 15);
  dptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * bm->planes;
#endif
  
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

	/* shift lower part of character (co-ordinate) to left */
#ifdef MONO
	if (dbit >= 0x80000000) {
	  dptr--;
#else
	if (dbit >= 0x8000) {
	  dptr -= bm->planes;
#endif
	  dbit = 1;
	} else {
	  dbit <<= 1;
	}
      }

      skew |= 0x10000;
    }
    skew >>= 1;

    if (pattern) {
      lighten = pattern[cheight & 15];
#ifdef MONO
      lighten |= (lighten << 16);
#endif
    }
    prevbit = 0;
    dlbit = dbit;
    dlptr = dptr;
    dldata = *dlptr;
    lcwidth = cwidth;

    while (--lcwidth >= 0) {

      fbit = cdata & cbit;		/* font bit */
      fbit |= bold & prevbit;		/* F_BOLD */
      prevbit = fbit & cbit;
      if (reverse) {			/* F_REVERSE */
        dldata |= dlbit;
        if (fbit) {
	  dldata &= ~(dlbit & lighten);	/* F_LIGHT */
        }
      } else {
        dldata &= ~dlbit;
        if (fbit) {
	  dldata |= (dlbit & lighten);	/* F_LIGHT */
        }
      }
      if (!(cbit >>= 1)) {		/* next font bit */
	cbit = 0x80000000;
	cdata = *cptr++;
      }
      if (!(dlbit >>= 1)) {
#ifdef MONO
	dlbit = 0x80000000;
	*dlptr++ = dldata;
#else
	dlbit = 0x8000;
	*dlptr = dldata;
	dlptr += bm->planes;
#endif
	dldata = *dlptr;
      }
    }
    *dlptr = dldata;
    dptr += bm->upl;

    lcwidth = xoff;
    while (--lcwidth >= 0) {
      if (!(cbit >>= 1)) {
	cbit = 0x80000000;
	/* as with font[255] this might go over the mallocated area, I
	 * allocate one additional long for font data i font loading.
	 */
	cdata = *cptr++;
      }
    }
  }
}


/* Do all the text styles, no clipping. */
static inline void FUNCTION(stylec)(bm, x0, y0, c)
     BITMAP *bm;
     long x0;
     long y0;
     ulong c;
{
  register ulong *cptr, cbit, cdata, fbit, prevbit;
#ifdef MONO
  register ulong *dptr, *dlptr, dlbit, dldata;
  ulong dbit, lighten = 0xffffffff;
#else
  register ushort *dptr, *dlptr, dlbit, dldata;
  ushort dbit, lighten = 0xffff;
#endif
  register short lcwidth;
  short cheight, cwidth, reverse = 0;
  ushort *pattern = NULL;
  FONT *font = gc0->font;
  ulong bold = 0, skew = 0;
  
  c &= 0xff;
  cheight = font->hdr.height;
  cwidth = font->widths[c];

  /* set styles */
  reverse = font->effects & F_REVERSE;

  if (font->effects & F_BOLD) {
    bold = 0xffffffff;
   }

  if (font->effects & F_LIGHT) {
    pattern = gc0->pattern;
  }

  if (font->effects & F_ITALIC) {
    skew = font->hdr.skew;
  }

#ifdef MONO
  dbit = 0x80000000 >> (x0 & 31);
  dptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);
#else
  dbit = 0x8000 >> (x0 & 15);
  dptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * bm->planes;
#endif

  cptr = font->data + font->offsets[c];
  cbit = 0x80000000;
  cdata = *cptr++;


  while (--cheight >= 0) {

    if (skew & 1) {

	/* shift lower part of character (co-ordinate) to left */
#ifdef MONO
	if (dbit >= 0x80000000) {
	  dptr--;
#else
	if (dbit >= 0x8000) {
	  dptr -= bm->planes;
#endif
	  dbit = 1;
	} else {
	  dbit <<= 1;
	}

      skew |= 0x10000;
    }
    skew >>= 1;

    if (pattern) {
      lighten = pattern[cheight & 15];
#ifdef MONO
      lighten |= (lighten << 16);
#endif
    }
    prevbit = 0;
    dlbit = dbit;
    dlptr = dptr;
    dldata = *dlptr;
    lcwidth = cwidth;

    while (--lcwidth >= 0) {

      fbit = cdata & cbit;		/* font bit */
      fbit |= bold & prevbit;		/* F_BOLD */
      prevbit = fbit & cbit;
      if (reverse) {			/* F_REVERSE */
        dldata |= dlbit;
        if (fbit) {
	  dldata &= ~(dlbit & lighten);	/* F_LIGHT */
        }
      } else {
        dldata &= ~dlbit;
        if (fbit) {
	  dldata |= (dlbit & lighten);	/* F_LIGHT */
        }
      }
      if (!(cbit >>= 1)) {		/* next font bit */
	cbit = 0x80000000;
	cdata = *cptr++;
      }
      if (!(dlbit >>= 1)) {
#ifdef MONO
	dlbit = 0x80000000;
	*dlptr++ = dldata;
#else
	dlbit = 0x8000;
	*dlptr = dldata;
	dlptr += bm->planes;
#endif
	dldata = *dlptr;
      }
    }
    *dlptr = dldata;
    dptr += bm->upl;
  }
}

#endif /* !COLOR */


#if 0

#ifdef COLOR

/* do normal text, no clipping */
static inline void FUNCTION(normalc)(bm, x0, y0, c)
     BITMAP *bm;
     long x0;
     long y0;
     ulong c;
{
  register ulong *cptr, cbit, cdata;
  register ushort *dptr, *dlptr, dbit, dlbit;
  register short cheight, lcwidth;
  register FONT *font = gc0->font;
  register short idx, planes = bm->planes;
  register ushort *mask;
  short cwidth;
  
  c &= 0xff;
  cheight = font->hdr.height;
  cwidth = font->widths[c];

  dbit = 0x8000 >> (x0 & 15);
  dptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * planes;

  cptr = font->data + font->offsets[c];
  cbit = 0x80000000;
  cdata = *cptr++;

  while (--cheight >= 0) {			/* TODO: graphics modes */
    dlbit = dbit;
    dlptr = dptr;
    lcwidth = cwidth;
    while (--lcwidth >= 0) {
      if (cdata & cbit) {
	mask = gc0->fgColMask;
      } else {
	mask = gc0->bgColMask;
      }
      idx = planes;
      while (--idx >= 0) {
	dlptr[idx] = (dlptr[idx] & ~dlbit) | (mask[idx] & dlbit);
      }
      if (!(cbit >>= 1)) {
	cbit = 0x80000000;
	cdata = *cptr++;
      }
      if (!(dlbit >>= 1)) {
	dlbit = 0x8000;
	dlptr += planes;
      }
    }
    dptr += bm->upl;
  }
}

#else /* !COLOR */

/* do normal text, no clipping. */
static inline void FUNCTION(normalc)(bm, x0, y0, c)
     BITMAP *bm;
     long x0;
     long y0;
     register ulong c;
{
  register ulong *cptr, cbit, cdata;
#ifdef MONO
  register ulong *dptr, *dlptr, dbit, dlbit, dldata;
#else
  register ushort *dptr, *dlptr, dbit, dlbit, dldata;
#endif
  register short cheight, lcwidth;
  register FONT *font = gc0->font;
  short cwidth;

  c &= 0xff;
  cheight = font->hdr.height;
  cwidth = font->widths[c];

#ifdef MONO
  dbit = 0x80000000 >> (x0 & 31);
  dptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);
#else
  dbit = 0x8000 >> (x0 & 15);
  dptr = (ushort *)bm->data + y0 * bm->upl + (x0 >> 4) * bm->planes;
#endif

  cptr = font->data + font->offsets[c];
  cbit = 0x80000000;
  cdata = *cptr++;

  while (--cheight >= 0) {		/* TODO: graphics modes */
    dlbit = dbit;
    dlptr = dptr;
    dldata = *dlptr;
    lcwidth = cwidth;
    while (--lcwidth >= 0) {
      dldata &= ~dlbit;
      if (cdata & cbit) {
	dldata |= dlbit;
      }
      if (!(cbit >>= 1)) {
	cbit = 0x80000000;
	cdata = *cptr++;
      }
      if (!(dlbit >>= 1)) {
#ifdef MONO
	dlbit = 0x80000000;
	*dlptr++ = dldata;
#else
	dlbit = 0x8000;
	*dlptr = dldata;
	dlptr += bm->planes;
#endif
	dldata = *dlptr;
      }
    }
    *dlptr = dldata;
    dptr += bm->upl;
  }
}

#endif	/* !COLOR */

#endif	/* 0 */


/* do character strings with above functions */
#include "../generic/generic_prints.h"
