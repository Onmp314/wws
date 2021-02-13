/*
 * pbmread.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- read binary PBM formats and the new proprietory ASCII format(s)
 *
 * NOTES:
 * - PBM misses a format which would use a palette, ASCII color format used
 *   here fixes that upto 92 colors (see w_readpbm manual for more info) and
 *   the binary palette version does rest.
 * - Use w_freebm() to free read images.  It knows what has been allocated.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"

/* ASCII format */
#define MAX_PALETTE	64	/* maximum palette size */
#define PALETTE_ENTRY	3	/* bytes / palette (rgb) entry */


/* ----------------- PBM reading utility routines ---------------- */

#define IS_SPACE(ch) (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
#define IS_LINE_END(ch) (ch == EOF || ch == '\n' || ch == '\r')

/* read char from file ignoring commented line ends */
static int
pbm_getc(FILE *fp)
{
	int ch;
	ch = fgetc(fp);
	if (ch == '#') {
		/* pass comment */
		do {
			ch = fgetc(fp);
		} while (!IS_LINE_END(ch));
	}
	return ch;
}

/* next no white space nor comment character */
static int
pbm_next(FILE *fp)
{
	int ch;
	do {
		ch = pbm_getc(fp);
	} while(IS_SPACE(ch));
	return ch;
}

/* Read a number skipping comments and white space separating numbers.
 * Negative value means an error.
 */
static int
pbm_getint(FILE *fp)
{
	int ch, number = 0;

	ch = pbm_next(fp);
	while (ch >= '0' && ch <= '9') {
		number = number * 10 + ch - '0';
		ch = fgetc(fp);
	}
	if (!IS_SPACE(ch)) {
		number = -1;
	}
	return number;
}

/* next no white space character */
static int
next_char(FILE *fp)
{
	int ch;
	do {
		ch = fgetc(fp);
	} while(IS_SPACE(ch));
	return ch;
}

/* Read a hexadecimal number skipping white space (not comments) separating
 * numbers.  Negative value means an error.
 */
static long
pbm_gethex(FILE *fp)
{
	long number = 0;
	int ch;

	ch = toupper(next_char(fp));
	for (;;) {
		if (ch >= '0' && ch <= '9') {
			number = number * 16 + ch - '0';
		} else {
			if (ch >= 'A' && ch <= 'F') {
				number = number * 16 + ch - 'A' + 10;
			} else {
				break;
			}
		}
		ch = toupper(fgetc(fp));
	}
	if (!IS_SPACE(ch)) {
		number = -1;
	}
	return number;
}

/* ----------------- binary PBM reading ------------------- */

static BITMAP *
mono_loader(FILE *fp, int width, int height, const char **error)
{
	int sbytes, dbytes;
	uchar *datap;
	BITMAP *bm;

	bm = w_allocbm(width, height, BM_PACKEDMONO, 0);
	if (!bm) {
		*error = "alloc failed";
		return NULL;
	}
	sbytes = (width + 7) / 8;
	dbytes = bm->unitsize * bm->upl;
	datap = bm->data;
	while (--height >= 0) {
		if (fread(datap, sbytes, 1, fp) != 1) {
			w_freebm(bm);
			*error = "read failed";
			return NULL;
		}
		datap += dbytes;
	}
	return bm;
}

/* no scaling for color values even if they aren't for the full 8-bit range.
 * image might be intentionally dark instead of using less than 8 bits.
 */
static BITMAP *
gray_loader(FILE *fp, int width, int height, const char **error)
{
	int dbytes, maxval;
	uchar *datap;
	BITMAP *bm;
	rgb_t *rgb;

	maxval = pbm_getint(fp);
	if (maxval < 1 || maxval > 255) {
		*error = "illegal max. color value";
		return NULL;
	}
	bm = w_allocbm(width, height, BM_DIRECT8, maxval+1);
	if (!bm) {
		*error = "alloc failed";
		return NULL;
	}
	rgb = bm->palette + maxval;
	while (--maxval >= 0) {
		rgb--;
		rgb->red = rgb->green = rgb->blue = maxval;
	}
	dbytes = bm->unitsize * bm->upl;
	datap = bm->data;
	while (--height >= 0) {
		if (fread(datap, width, 1, fp) != 1) {
			w_freebm(bm);
			*error = "read failed";
			return NULL;
		}
		datap += dbytes;
	}
	return bm;
}

static BITMAP *
trueColor_loader(FILE *fp, int width, int height, const char **error)
{
	int maxval;
	long dbytes;
	BITMAP *bm;

	maxval = pbm_getint(fp);
	if (maxval < 1 || maxval > 255) {
		*error = "illegal max. color value";
		return NULL;
	}
	bm = w_allocbm(width, height, BM_DIRECT24, 0);
	if (!bm) {
		*error = "alloc failed";
		return NULL;
	}
	dbytes = (long)bm->unitsize * bm->upl * bm->height;
	if (fread(bm->data, dbytes, 1, fp) != 1) {
		*error = "read failed";
		w_freebm(bm);
		return NULL;
	}
	return bm;
}

/* ----------------- binary format with a palette -------------- */

static BITMAP *
color_loader(FILE *fp, int width, int height, const char **error)
{
	int dbytes, colors;
	uchar *datap, buf[3];
	BITMAP *bm;
	rgb_t *rgb;

	colors = pbm_getint(fp);
	if (colors < 2 || colors > 256) {
		*error = "illegal max. color value";
		return NULL;
	}
	bm = w_allocbm(width, height, BM_DIRECT8, colors);
	if (!bm) {
		*error = "alloc failed";
		return NULL;
	}
	rgb = bm->palette;
	while (--colors >= 0) {
		if (fread(buf, 3, 1, fp) != 1) {
			*error = "palette read failed";
			w_freebm(bm);
			return NULL;
		}
		rgb->red   = buf[0];
		rgb->green = buf[1];
		rgb->blue  = buf[2];
		rgb++;
	}
	dbytes = bm->unitsize * bm->upl;
	datap = bm->data;
	while (--height >= 0) {
		if (fread(datap, width, 1, fp) != 1) {
			*error = "read failed";
			w_freebm(bm);
			return NULL;
		}
		datap += dbytes;
	}
	return bm;
}

/* -------------- monochrome ASCII format reading ---------------- */

static BITMAP *
monoascii_loader(FILE *fp, int width, int height, const char **error)
{
	int x, black, white, next;
	ulong *datap, bit;
	BITMAP *bm;

	white = next_char(fp);
	black = fgetc(fp);
	if (white <= ' ' || black <= ' ') {
		*error = "illegal color mapping";
		return NULL;
	}
	bm = w_allocbm(width, height, BM_PACKEDMONO, 0);
	if (!bm) {
		*error = "alloc failed";
		return NULL;
	}
	datap = bm->data;
	memset(datap, 0, bm->unitsize * bm->upl * bm->height);
	while (--height >= 0) {
		next = next_char(fp);
		bit = 0x80000000UL;
		x = width;
		while (--x >= 0) {
			if (next == black) {
				*datap |= bit;
			} else {
				if (next != white) {
					if (next == EOF) {
						*error = "file too short";
					} else {
						*error = "unmapped character";
					}
					w_freebm(bm);
					return NULL;
				}
			}
			bit >>= 1;
			if (!bit) {
				bit = 0x80000000UL;
				datap++;
			}
			next = fgetc(fp);
		}
		if (bit != 0x80000000UL) {
			datap++;
		}
	}
	return bm;
}

/* -------------- color ASCII format reading ---------------- */

/* this returns either color image with palette (ATM impossible) or
 * to monochrome dithered image.
 */
static BITMAP *
colorascii_loader(FILE *fp, int width, int height, const char **error)
{
	int x, index, next, dbytes, colors, red, green, blue;
	uchar *datap, map[MAX_PALETTE];
	rgb_t *rgb;
	BITMAP *bm;

	colors = pbm_getint(fp);
	if (colors < 2 || colors > MAX_PALETTE) {
		*error = "illegal number of colors";
		return NULL;
	}
	bm = w_allocbm(width, height, BM_DIRECT8, colors);
	if (!bm) {
		*error = "alloc failed";
		return NULL;
	}
	rgb = bm->palette;
	for (index = 0; index < colors; index++) {
		next = next_char(fp);
		if (next <= ' ') {
			*error = "illegal color mapping";
			w_freebm(bm);
			return NULL;
		}
		map[index] = next;

		red   = pbm_gethex(fp);
		green = pbm_gethex(fp);
		blue  = pbm_gethex(fp);
		if (red < 0   || green < 0   || blue < 0 ||
		    red > 255 || green > 255 || blue > 255) {
			*error = "illegal palette entry";
			w_freebm(bm);
			return NULL;
		}
		rgb->red   = red;
		rgb->green = green;
		rgb->blue  = blue;
		rgb++;
	}

	dbytes = bm->unitsize * bm->upl;
	datap = bm->data;
	while (--height >= 0) {
		next = next_char(fp);
		for (x = 0; x < width; x++) {
			index = 0;
			while (index < colors && next != map[index]) {
				index++;
			}
			if (index >= colors) {
				if (next == EOF) {
					*error = "file too short";
				} else {
					*error = "unmapped character";
				}
				w_freebm(bm);
				return NULL;
			}
			datap[x] = index;
			next = fgetc(fp);
		}
		datap += dbytes;
	}
	return bm;
}

/* ----------------- the read function itself ---------------- */

BITMAP *
w_readpbm(const char *path)
{
	BITMAP *(*loader) (FILE *, int, int, const char **);
	const char *error, *type;
	int width, height;
	BITMAP *bm;
	FILE *fp;

	TRACESTART();

	if (!path) {
		fp = stdin;
	} else {
		if (!(fp = fopen(path, "rb"))) {
			TRACEPRINT(("w_readpbm(`%s') -> unable to open file\n", path));
			TRACEEND();
			return NULL;
		}
	}

	/* check file format */
	if (fgetc(fp) != 'P') {
		TRACEPRINT(("w_readpbm(`%s') -> not PBM file\n", path));
		TRACEEND();
		if (path) {
			fclose(fp);
		}
		return NULL;
	}
	type = NULL;
	loader = NULL;
	switch(fgetc(fp)) {
		/* binary PBM */
		case '4':
			loader = mono_loader;
			type = "mono";
			break;
		case '5':
			loader = gray_loader;
			type = "gray";
			break;
		case '6':
			loader = trueColor_loader;
			type = "truecolor";
			break;

		/*  my own binary format with palette */
		case '8':
			loader = color_loader;
			type = "color";
			break;

		/* my own ASCII formats */
		case 'A':
			loader = monoascii_loader;
			type = "mono";
			break;
		case 'C':
			loader = colorascii_loader;
			type = "color";
			break;
	}
	if (!loader) {
		TRACEPRINT(("w_readpbm(`%s') -> unrecognized PBM type\n", path));
		TRACEEND();
		if (path) {
			fclose(fp);
		}
		return NULL;
	}

	/* get image size */
	width  = pbm_getint(fp);
	height = pbm_getint(fp);
	if(width < 1 || height < 1) {
		TRACEPRINT(("w_readpbm(`%s') -> %s PBM size error\n", type, path));
		TRACEEND();
		if (path) {
			fclose(fp);
		}
		return NULL;
	}

	error = NULL;
	bm = (*loader) (fp, width, height, &error);
	if (path) {
		fclose(fp);
	}

#ifdef TRACE
	if (error) {
		TRACEPRINT(("w_readpbm(`%s') -> %s PBM: %s\n", path, type, error));
	} else {
		TRACEPRINT(("w_readpbm(`%s') -> %p (%s)\n", path, bm, type));
	}
#endif
	TRACEEND();
	return bm;
}

