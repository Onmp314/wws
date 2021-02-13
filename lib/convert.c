/*
 * convert.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- BITMAP conversion utility functions
 *
 * NOTES:
 *
 * *** These routines won't do color reduction (quantizing), they just
 * *** convert image data from one format to another.  For mono->color
 * *** conversion convertFunction() sets also the destination palette.
 * *** Dithering functions use static grayscale mapping and Matrixrow
 * *** variables.
 *
 * Unitsizes currently used by conversion routines:
 * - packed mono:  4	(M_UNIT define, 32 pixels)
 * - packed color: 2	(16 pixels interleaved on number planes: ATARI_SERVER)
 * - direct-8:     1	(one, but images are still long aligned!!!)
 * - direct-24:    3	(one pixel)
 *
 * Missing conversions:
 * - PACKEDCOLOR -> PACKEDCOLOR. Only server should use that type so
 *   conversion where only other format is PACKEDCOLOR should be enough.
 * - Conversion from DIRECT24 to another color type.  I assume that
 *   applications want to do that themselves, because it needs quite
 *   complicated (=slow) image / palette handling and then there's the
 *   question of dithering etc...
 *
 * mono->color conversion will use color index zero for a cleared bit and
 * index one for a set bit in the monochrome bitmap if the destination
 * bitmap format uses a palette. W server should always assign white and
 * black colors to these indeces.
 *
 * If you'll want PACKEDCOLOR->PACKEDMONO conversion to use dithering,
 * you'll have to convert image first to DIRECT8 format.
 *
 * DIRECT24 images are always FS-dithered to PACKEDMONO with
 * w_convertBitmap() before other conversions are done.
 * This is done whole bitmap at the time.
 *
 * If you want to use w_bitmapFunction() yourself, you might do the
 * conversions 'in place' when destination bitmap has less 'planes'
 * than source.
 *
 * CHANGES:
 *
 * ++eero, 10/97:
 * - Direct<->packed routines are moved here from TeSche's earlier block.c.
 * ++eero, 3/98:
 * - w_convertBitmap() function is now w_convertFunction which returns
 *   pointer to the line-conversion function between the formats of
 *   the two argument bitmaps.
 * - In order to make things byte order neutral, some conversion routines
 *   do things byte at the time instead of longs longs and shorts as
 *   earlier and some use netinet/in.h byte order conversion routines.
 * ++eero, 3/98:
 * - Atari/MiNT:  Noticed that DIRECT8 -> PACKEDCOLOR routines put the
 *   colors in reverse order.  Fixed.  What about the other way round?
 * ++eero, 7/98:
 * - optimized ordered dithering a little.
 *
 * TODO:
 * - Test with server and client(s) residing on different byte order using
 *   machines.  MOST LIKELY THIS NEEDS FIXING!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"
#include "dmatrix.h"


/* set what routines are compiled in */

/* conversion routines understand packed (interleaved) bitmaps */
#define ATARI_SERVER


/* monochrome unit size is long = 4 bytes, 32 pixels */
#define M_UNIT	4

/* sets other BITMAP values from BITMAP type/width/height(/colors)
 *
 * DIRECT8 has line lengths padded to a multiple of 4 so that they end on a
 * long boundary and PACKED ones have line lengths padded to a multiple of
 * 16 so that they end on a short boundary.  applications filling in data
 * must know and obey this (means just using `unitsize' and `upl' members).
 */
short
w_bmheader(BITMAP *bm)
{
	short colors;

	switch (bm->type) {

	case BM_PACKEDMONO:
		bm->upl = (bm->width + 31) / 32;
		bm->unitsize = M_UNIT;
		bm->planes = 1;
		bm->colors = 2;
		break;

	case BM_PACKEDCOLOR:
		bm->planes = _wserver.planes;
		bm->upl = (((bm->width + 15) & ~15) * bm->planes) / 16;
		colors = 1 << bm->planes;
		if (bm->colors < 2 || bm->colors > colors) {
			bm->colors = 2;
		}
		bm->unitsize = 2;
		break;

	case BM_DIRECT8:
		bm->upl = (bm->width + 3) & ~3;
		bm->unitsize = 1;
		bm->planes = 8;
		if (bm->colors < 2 || bm->colors > 256) {
			bm->colors = 2;
		}
		break;

	case BM_DIRECT24:
		bm->upl = bm->width;
		bm->unitsize = 3;
		bm->planes = 24;
		if (bm->colors < 2) {
			bm->colors = 2;
		}
		break;

	default:
		return -1;
	}
	/* other function rely on this */
	bm->palette = NULL;
	return 0;
}


static int Expand;
static int Matrixrow = 0;	/* ordered dithering row */
static uchar Grayscale[256];
static uchar *Graymap = Grayscale;


/* Using an own graymap for ordered dithering is faster if there are lot of
 * images with the same palette.  Graymap is of course needed only for
 * mapped (2-8bit) bitmaps.
 *
 * Animations which update only parts of the output view should at least
 * disable contrast expansion.
 */
void
w_ditherOptions(uchar *graymap, int expand)
{
	if (graymap) {
		/* use this mapping instead of mapping palette every time */
		Graymap = graymap;
	} else {
		Graymap = Grayscale;
	}

	/* do/not use contrast expansion */
	Expand = expand;
}

void
_wget_options(uchar **graymap, int *expand)
{
	if (expand) {
		*expand = Expand;
	}
	if (graymap) {
		*graymap = Grayscale;
	}
}


uchar *
_winit_mapping(BITMAP *bm, int row)
{
	rgb_t *color;
	uchar *gray;
	ulong value;
	int idx;

	Matrixrow = row & 15;

	if (Graymap != Grayscale) {
		return Graymap;
	}

	gray = Grayscale;
	if (bm->palette) {
		/* color mapping */
		color = bm->palette;

		idx = bm->colors;
		while (--idx >= 0) {
			value =  color->red   * 307UL;
			value += color->green * 599UL;
			value += color->blue  * 118UL;
			*gray++ = value >> 10;
			color++;
		}
	} else {
		/* no palette (DIRECT24) -> linear grayscale */
		idx = 256;
		gray += 256;
		while (--idx >= 0) {
			*--gray = idx;
		}
	}

	return Grayscale;
}


/* ------------------------------------------------------------
 * line-by-line conversion functions
 */

/* ordered dither DIRECT8 bitmap to monochrome, short at the time */
static uchar *
odither_direct(BITMAP *src, BITMAP *dst, uchar *line)
{
	register const uchar *gray, *map, *mat, *matrix;
	register ushort bit, result, *mono;
	register int lastbits, shorts;

	matrix = DMatrix[Matrixrow++];
	Matrixrow &= 15;
	mono = dst->data;
	map = Graymap;
	gray = line;
	line += src->upl * src->unitsize;

	shorts = src->width;
	lastbits = shorts & 15;
	shorts >>= 4;

	while (--shorts >= 0) {

		result = 0;
		mat = matrix;

		/* go through one pattern line (short) of pixels */
		for(bit = 0x8000u; bit; bit >>= 1) {

			/* color/matrix->black? */
			if(map[*gray++] < *mat++) {
				result |= bit;
			}
		}
		*mono++ = ntohs(result);
	}

	result = 0;
	mat = matrix;
	
	for (bit = 0x8000u; --lastbits >= 0; bit >>= 1) {

		if(map[*gray++] < *mat++) {
			result |= bit;
		}
	}
	*mono++ = ntohs(result);

	return line;
}


/* DIRECT24 should be RGB-triple-byte aligned, with *no* padding */
static uchar *
odither_truecolor(BITMAP *src, BITMAP *dst, uchar *line)
{
	register const uchar *gray, *map, *mat, *matrix;
	register ushort bit, result, *mono;
	register int lastbits, shorts;
	register ulong value;

	matrix = DMatrix[Matrixrow++];
	Matrixrow &= 15;
	mono = dst->data;
	map = Graymap;
	gray = line;
	line += src->upl * src->unitsize;

	shorts = src->width;
	lastbits = shorts & 15;
	shorts >>= 4;

	while (--shorts >= 0) {

		result = 0;
		mat = matrix;

		/* go through one pattern line (short) of pixels */
		for(bit = 0x8000u; bit; bit >>= 1) {

			value =  (*gray++ * 307UL);
			value += (*gray++ * 599UL);
			value += (*gray++ * 118UL);
			if(map[value >> 10] < *mat++) {
				result |= bit;
			}
		}
		*mono++ = ntohs(result);
	}

	result = 0;
	mat = matrix;
	
	for (bit = 0x8000u; --lastbits >= 0; bit >>= 1) {

		value =  (*gray++ * 307UL);
		value += (*gray++ * 599UL);
		value += (*gray++ * 118UL);
		if(map[value >> 10] < *mat++) {
			result |= bit;
		}
	}
	*mono++ = ntohs(result);

	return line;
}


#ifdef ATARI_SERVER

/* ------------------------------------------------------------
 * Atari specific packed<->direct8 conversions
 */

/* convert a packed bitmap into a monochrome one, all colors besides
 * one with index zero, will be treated as `black' ie. there's no
 * dithering.
 */
static uchar *
packed2mono (BITMAP *src, BITMAP *dst, uchar *line)
{
	short height, count, planes, plane, shorts, skip;
	register ushort *dptr, *sptr;

	planes = src->planes;
	shorts = src->upl / planes;
	skip = (src->width + (M_UNIT*8-1)) / 16 - shorts;
	height = src->height;

	sptr = (ushort *)line;
	dptr = dst->data;

	planes--;
	while (--height >= 0) {
		count = shorts;
		while (--count >= 0) {
			*dptr = *sptr++;
			plane = planes;
			while (--plane >= 0) {
				*dptr |= *sptr++;
			}
			dptr++;
		}
		dptr += skip;
	}
	return line + src->unitsize * src->upl;
}


/*
 * these functions convert single lines of DIRECT8 data to PACKED data and
 * vice versa (either 8, 4, 2 or 1 target plane).
 *
 * 'dptr' is long so that we can convert 16 pixels on two planes
 * at the time.
 */

static uchar *
direct8packed (BITMAP *src, BITMAP *dst, uchar *line)
{
	uchar *sptr = (uchar *)line;
	ulong *dptr = (ulong *)dst->data;
	short count = dst->width;
	ulong d0 = 0, d1 = 0, d2 = 0, d3 = 0;
	register ulong dbitlo = 0x80000000, dbithi = 0x00008000;
	register uchar sval;

	while (--count >= 0) {

		if ((sval = *sptr++) & 0x01)
			d0 |= dbitlo;
		if (sval & 0x02)
			d0 |= dbithi;
		if (sval & 0x04)
			d1 |= dbitlo;
		if (sval & 0x08)
			d1 |= dbithi;
		if (sval & 0x10)
			d2 |= dbitlo;
		if (sval & 0x20)
			d2 |= dbithi;
		if (sval & 0x40)
			d3 |= dbitlo;
		if (sval & 0x80)
			d3 |= dbithi;

		dbitlo >>= 1;
		if (!(dbithi >>= 1)) {
			dbitlo = 0x80000000;
			dbithi = 0x00008000;

			*dptr++ = htonl(d0);
			*dptr++ = htonl(d1);
			*dptr++ = htonl(d2);
			*dptr++ = htonl(d3);
			d0 = d1 = d2 = d3 = 0;
		}
	}

	if (dbithi != 0x00008000) {
		*dptr++ = htonl(d0);
		*dptr++ = htonl(d1);
		*dptr++ = htonl(d2);
		*dptr = htonl(d3);
	}
	return line + src->unitsize * src->upl;
}


static uchar *
direct4packed (BITMAP *src, BITMAP *dst, uchar *line)
{
	uchar *sptr = (uchar *)line;
	ulong *dptr = (ulong *)dst->data;
	short count = dst->width;
	ulong d0 = 0, d1 = 0;
	register ulong dbitlo = 0x80000000, dbithi = 0x00008000;
	register uchar sval;

	while (--count >= 0) {

		if ((sval = *sptr++) & 0x01)
			d0 |= dbitlo;
		if (sval & 0x02)
			d0 |= dbithi;
		if (sval & 0x04)
			d1 |= dbitlo;
		if (sval & 0x08)
			d1 |= dbithi;

		dbitlo >>= 1;
		if (!(dbithi >>= 1)) {
			dbitlo = 0x80000000;
			dbithi = 0x00008000;
			*dptr++ = htonl(d0);
			*dptr++ = htonl(d1);
			d0 = d1 = 0;
		}
	}

	if (dbithi != 0x00008000) {
		*dptr++ = htonl(d0);
		*dptr = htonl(d1);
	}
	return line + src->unitsize * src->upl;
}


static uchar *
direct2packed (BITMAP *src, BITMAP *dst, uchar *line)
{
	uchar *sptr = (uchar *)line;
	ulong *dptr = (ulong *)dst->data;
	short count = dst->width;
	ulong d0 = 0;
	register ulong dbitlo = 0x80000000, dbithi = 0x00008000;
	register uchar sval;

	while (--count >= 0) {

		if ((sval = *sptr++) & 0x01)
			d0 |= dbitlo;
		if (sval & 0x02)
			d0 |= dbithi;

		dbitlo >>= 1;
		if (!(dbithi >>= 1)) {
			dbitlo = 0x80000000;
			dbithi = 0x00008000;
			*dptr++ = htonl(d0);
			d0 = 0;
		}
	}

	if (dbithi != 0x00008000) {
		*dptr = htonl(d0);
	}
	return line + src->unitsize * src->upl;
}

/* 'sptr' is long so that we can convert 16 pixels on two planes
 * at the time.
 */
static uchar *
packed8direct (BITMAP *src, BITMAP *dst, uchar *line)
{
	ulong *sptr = (ulong *)line;
	uchar *dptr = (uchar *)dst->data;
	short count = dst->width;
	ulong d0 = 0, d1 = 0, d2 = 0, d3 = 0;
	register ulong himask = 0, lomask = 0;
	register uchar d;

	while (--count >= 0) {
		if (!lomask) {
			d0 = ntohl(*sptr++);
			d1 = ntohl(*sptr++);
			d2 = ntohl(*sptr++);
			d3 = ntohl(*sptr++);
			himask = 0x80000000;
			lomask = 0x00008000;
		}
		d = 0;
		if (d0 & himask)
			d |= 0x80;
		if (d0 & lomask)
			d |= 0x40;
		if (d1 & himask)
			d |= 0x20;
		if (d1 & lomask)
			d |= 0x10;
		if (d2 & himask)
			d |= 0x08;
		if (d2 & lomask)
			d |= 0x04;
		if (d3 & himask)
			d |= 0x02;
		if (d3 & lomask)
			d |= 0x01;
		*dptr++ = d;
		himask >>= 1;
		lomask >>= 1;
	}
	return line + src->unitsize * src->upl;
}


static uchar *
packed4direct (BITMAP *src, BITMAP *dst, uchar *line)
{
	ulong *sptr = (ulong *)line;
	uchar *dptr = (uchar *)dst->data;
	short count = dst->width;
	ulong d0 = 0, d1 = 0;
	register ulong himask = 0, lomask = 0;
	register uchar d;

	while (--count >= 0) {
		if (!lomask) {
			d0 = ntohl(*sptr++);
			d1 = ntohl(*sptr++);
			himask = 0x80000000;
			lomask = 0x00008000;
		}
		d = 0;
		if (d0 & himask)
			d |= 0x08;
		if (d0 & lomask)
			d |= 0x04;
		if (d1 & himask)
			d |= 0x02;
		if (d1 & lomask)
			d |= 0x01;
		*dptr++ = d;
		himask >>= 1;
		lomask >>= 1;
	}
	return line + src->unitsize * src->upl;
}


static uchar *
packed2direct (BITMAP *src, BITMAP *dst, uchar *line)
{
	ulong *sptr = (ulong *)line;
	uchar *dptr = (uchar *)dst->data;
	short count = dst->width;
	register ulong d0 = 0;
	register ulong himask = 0, lomask = 0;
	register uchar d;

	while (--count >= 0) {
		if (!lomask) {
			d0 = ntohl(*sptr++);
			himask = 0x80000000;
			lomask = 0x00008000;
		}
		d = 0;
		if (d0 & himask) {
			d |= 0x02;
		}
		if (d0 & lomask) {
			d |= 0x01;
		}
		*dptr++ = d;
		himask >>= 1;
		lomask >>= 1;
	}
	return line + src->unitsize * src->upl;
}


/* ------------------------------------------------------------
 * mono->color conversions
 */

/* on types with palette, bits in mono bitmaps are set to color index 1.
 * this expects both bitmaps to be at least short aligned.
 */
static uchar *
mono2packed (BITMAP *src, BITMAP *dst, uchar *line)
{
	ushort *sptr = (ushort *)line;
	register ushort *dptr = dst->data;
	register short plane;
	short planes = dst->planes;
	short count = (dst->width + 15) >> 4;

	while (--count >= 0) {
		*dptr++ = *sptr++;
		plane = planes;
		while (--plane > 0) {
			*dptr++ = 0;
		}
	}
	return line + src->unitsize * src->upl;
}

#endif /* ATARI_SERVER */

static uchar *
mono2direct (BITMAP *src, BITMAP *dst, uchar *line)
{
	uchar *sptr = line;
	uchar *dptr = dst->data;
	short count = dst->width;
	uchar d0 = *sptr++;
	register uchar mask = 0x80;

	while (--count >= 0) {
		if (!mask) {
			d0 = *sptr++;
			mask = 0x80;
		}
		if (d0 & mask) {
			*dptr++ = FGCOL_INDEX;
		} else {
			*dptr++ = BGCOL_INDEX;
		}
		mask >>= 1;
	}
	return line + src->unitsize * src->upl;
}


static uchar *
mono2truecolor (BITMAP *src, BITMAP *dst, uchar *line)
{
	uchar *sptr = line;
	register uchar *dptr = dst->data;
	register uchar mask = 0x80;
	short count = dst->width;
	uchar d0 = *sptr++;

	while (--count >= 0) {
		if (!mask) {
			d0 = *sptr++;
			mask = 0x80;
		}
		if (d0 & mask) {
			*dptr++ = 0xff;
			*dptr++ = 0xff;
			*dptr++ = 0xff;
		} else {
			*dptr++ = 0x00;
			*dptr++ = 0x00;
			*dptr++ = 0x00;
		}
		mask >>= 1;
	}
	return line + src->unitsize * src->upl;
}

/* ------------------------------------------------------------
 * other 'conversions'
 */

static uchar *
direct2truecolor (BITMAP *src, BITMAP *dst, uchar *line)
{
	uchar *sptr = line;
	register uchar *dptr = dst->data;
	register uchar *rgb;
	short count = dst->width;
	rgb_t *color = src->palette;

	while (--count >= 0) {
		rgb = &color[*sptr++].red;
		*dptr++ = *rgb++;
		*dptr++ = *rgb++;
		*dptr++ = *rgb;
	}
	return line + src->unitsize * src->upl;
}


static uchar *
copy_bm (BITMAP *src, BITMAP *dst, uchar *line)
{
	size_t count = src->unitsize * src->upl;
	memcpy(dst->data, line, count);
	return line + count;
}


/* change destination palette to monochrome */
static void
set_monopal (BITMAP *dst)
{
	rgb_t *rgb;

	if (dst->palette) {
		rgb = dst->palette;

		/* background color is white */
		rgb[BGCOL_INDEX].red   = 0xff;
		rgb[BGCOL_INDEX].green = 0xff;
		rgb[BGCOL_INDEX].blue  = 0xff;

		/* foreground color is black */
		rgb[FGCOL_INDEX].red   = 0;
		rgb[FGCOL_INDEX].green = 0;
		rgb[FGCOL_INDEX].blue  = 0;

		dst->colors = 2;
	}
}

/* ------------------------------------------------------------
 * conversion initialization
 */



/* generic conversion routine */
uchar *
(*w_convertFunction (BITMAP *src, BITMAP *dst, int row)) (BITMAP *, BITMAP *, uchar *)
{
	const char *error = "request for unknown BITMAP type";

	/* can't check for data member as client might supply it later
	 */
	TRACESTART();
	if (!(src && dst)) {
		TRACEPRINT(("w_convertFunction(%p,%p,%i) -> no bitmap\n",src,dst,row));
		TRACEEND();
		return NULL;
	}
	TRACEPRINT(("w_convertFunction(%p,%p,%i)\n",src,dst,row));
	TRACEEND();

	switch (dst->type) {

	    case BM_PACKEDMONO:
		switch (src->type) {
		    case BM_PACKEDMONO:
			return copy_bm;

		    case BM_DIRECT8:
			_winit_mapping(src, row);
			return odither_direct;

		    case BM_DIRECT24:
			_winit_mapping(src, row);
			return odither_truecolor;

#ifdef ATARI_SERVER
		    case BM_PACKEDCOLOR:
			/* non-background color -> black */
			return packed2mono;
#endif /* ATARI_SERVER */
		}
		break;

#ifdef ATARI_SERVER
	    case BM_PACKEDCOLOR:
		switch (src->type) {
		    case BM_PACKEDMONO:
			set_monopal(dst);
			return mono2packed;

		    case BM_PACKEDCOLOR:
			if (src->planes == dst->planes) {
				return copy_bm;
			}
			/* Only server can be packed color so packed bitmaps
			 * should always have the same number of planes!
			 */
			 error = "illegal conversion: PACKEDCOLOR -> PACKEDCOLOR";
			break;

		    case BM_DIRECT8:
			switch(dst->planes) {
			    case 2:
				return direct2packed;
			    case 4:
				return direct4packed;
			    case 8:
				return direct8packed;
			}
			error = "unsupported number of DIRECT8 -> PACKEDCOLOR planes";
			break;

		    case BM_DIRECT24:
			/* where the image data (=colors) -> palette
			 * conversion would be done?
			 */
			error = "DIRECT24 -> PACKEDCOLOR not implemented";
			break;
		}
		break;
#endif /* ATARI_SERVER */

	    case BM_DIRECT8:
		switch (src->type) {
		    case BM_PACKEDMONO:
			set_monopal(dst);
			return mono2direct;

#ifdef ATARI_SERVER
		    case BM_PACKEDCOLOR:
			switch(src->planes) {
			    case 2:
				return packed2direct;
			    case 4:
				return packed4direct;
			    case 8:
				return packed8direct;
			}
			error = "unsupported number of PACKEDCOLOR -> DIRECT8 planes";
			break;
#endif /* ATARI_SERVER */

		    case BM_DIRECT8:
			return copy_bm;

		    case BM_DIRECT24:
			/* where the image data (=colors) -> palette
			 * conversion would be done? Graycscale conversion
			 * would be easy...
			 */
			error = "DIRECT24 -> DIRECT8 not yet implemented";
			break;
		}
		break;

	    case BM_DIRECT24:
		/* these would be fairly easy to implement, but I don't see
		 * much point in it.  We haven't got a TC server and apps
		 * manipulating TC images surely do graphics operations
		 * themselves, instead of getting bitmaps from the server...
		 */
		switch (src->type) {
		    case BM_PACKEDMONO:
			return mono2truecolor;

		    case BM_DIRECT8:
			return direct2truecolor;

		    case BM_DIRECT24:
			return copy_bm;

		    default:
			error = "DIRECT24 destination type not yet implemented";
			break;
		}
		break;
	}

	/* this should be known regardless of tracing */
	fprintf(stderr, "w_convertFunction(%p,%p,%i): %s", src, dst, row, error);
	return NULL;
}

