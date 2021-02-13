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
 * -- character output routines for BMONO graphics driver
 *
 * NOTES
 * - This is a straighforward conversion from packed driver
 */

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "bmono.h"


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
  ulong *cptr, cbit, cdata, fbit, prevbit;
  ulong *dptr, *dlptr, dlbit, dldata;
  ulong dbit, lighten = 0xffffffff;
  short lcwidth, reverse = 0;
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

  dbit = 0x80000000 >> (x0 & 31);
  dptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);

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

	/* shift lower part of character to left *on-screen* */
	if (dbit >= 0x80000000) {
	  dptr--;
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
      lighten |= (lighten << 16);
    }
    prevbit = 0;
    dlbit = dbit;
    dlptr = dptr;
    dldata = htonl(*dlptr);
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
	dlbit = 0x80000000;
	*dlptr++ = ntohl(dldata);
	dldata = htonl(*dlptr);
      }
    }
    *dlptr = ntohl(dldata);
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
  ulong *cptr, cbit, cdata, fbit, prevbit;
  ulong *dptr, *dlptr, dlbit, dldata;
  ulong dbit, lighten = 0xffffffff;
  short lcwidth, cheight, cwidth, reverse = 0;
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

  dbit = 0x80000000 >> (x0 & 31);
  dptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);

  cptr = font->data + font->offsets[c];
  cbit = 0x80000000;
  cdata = *cptr++;


  while (--cheight >= 0) {

    if (skew & 1) {
      skew |= 0x10000;
      /* shift lower part of character (co-ordinate) to left */
      if (dbit >= 0x80000000) {
	dptr--;
	dbit = 1;
      } else {
	dbit <<= 1;
      }
    }
    skew >>= 1;
    if (pattern) {
      lighten = pattern[cheight & 15];
      lighten |= (lighten << 16);
    }
    prevbit = 0;
    dlbit = dbit;
    dlptr = dptr;
    dldata = htonl(*dlptr);
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
	dlbit = 0x80000000;
	*dlptr++ = ntohl(dldata);
	dldata = htonl(*dlptr);
      }
    }
    *dlptr = ntohl(dldata);
    dptr += bm->upl;
  }
}

#if 0

/* do normal text, no clipping. Could be used for window widgets etc. */
static void FUNCTION(normalc)(bm, x0, y0, c)
     BITMAP *bm;
     long x0;
     long y0;
     ulong c;
{
  ulong *cptr, cbit, cdata;
  ulong *dptr, *dlptr, dbit, dlbit, dldata;
  short cheight, lcwidth;
  FONT *font = gc0->font;
  short cwidth;

  c &= 0xff;
  cheight = font->hdr.height;
  cwidth = font->widths[c];

  dbit = 0x80000000 >> (x0 & 31);
  dptr = (ulong *)bm->data + y0 * bm->upl + (x0 >> 5);

  cptr = font->data + font->offsets[c];
  cbit = 0x80000000;
  cdata = *cptr++;

  while (--cheight >= 0) {		/* TODO: graphics modes */
    dlbit = dbit;
    dlptr = dptr;
    dldata = htonl(*dlptr);
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
	dlbit = 0x80000000;
	*dlptr++ = ntohl(dldata);
	dldata = htonl(*dlptr);
      }
    }
    *dlptr = ntohl(dldata);
    dptr += bm->upl;
  }
}

#endif

/* do character strings with above two functions */
#include "../generic/generic_prints.h"
