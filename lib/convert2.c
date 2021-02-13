/*
 * convert2.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- additional conversion functions
 *
 * TODO
 * - Put 'in_place' option back to w_convertBitmap()?
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


/* ------------------------------------------------------------
 * FS-dither whole image.
 */

/*
 * FS-dither 8-bit BM_DIRECT8 bitmap `bm' (with any unitsize) into long
 * aligned monochrome BM_PACKEDMONO bitmap `mono_bm' of same size using
 * color index -> grayscale value mapping array.  `mono_bm' can be same as
 * `bm'.
 *
 * This could be faster but I'd rather not duplicate the whole loop.
 */
static void
FSditherDirect(BITMAP *bm, BITMAP *mono_bm,
             register uchar *mapping, short *FSerrors, int expand)
{
	short threshold;
	register uchar *color;
	register short mingray, maxgray;
	ulong *mono, result, last, bit, hibit;
	int x, y, longs, bits, unit;

	register short cur;		/* current error or pixel value */
	register short delta;
	register short *errorptr;	/* FSerrors at line before current */
	register short belowerr;	/* error for pixel below cur */
	register short bpreverr;	/* error for below/prev col */
	register short bnexterr;	/* error for below/next col */

	if (expand) {
		maxgray = 0;
		mingray = 255;
		cur = bm->colors;
		while (--cur >= 0) {
			if (mapping[cur] < mingray) {
				mingray = mapping[cur];
			}
			if (mapping[cur] > maxgray) {
				maxgray = mapping[cur];
			}
		}
		maxgray -= mingray;
		if (maxgray < 2) {
			threshold = 1;
		} else {
			threshold = maxgray / 2;
		}
	} else {
		mingray = 0;
		threshold = 127;
		maxgray = 255;
	}


	unit = mono_bm->unitsize << 3;

	/* longs in the source - 1 */
	longs = (bm->width + (unit-1)) / unit - 1;

	/* bit/pixel counter for the last long */
	last = bm->upl * bm->unitsize - longs * unit;

	hibit = 1UL << (unit-1);

	color = bm->data;
	mono = mono_bm->data;

	y = bm->height;
	while (--y >= 0) {
		errorptr = FSerrors;
		cur = belowerr = bpreverr = 0;

		for(x = longs; x >= 0; x--) {
			result = 0;
			if (x) {
				bits = unit;
			} else {
				bits = last;
			}

			/* go through one long of pixels */
			for(bit = hibit; --bits >= 0; bit >>= 1) {

				/* color + error to range */
				cur = (cur + errorptr[1] + 8) >> 4;
				cur += (mapping[*color++] - mingray);
				if(cur < 0) {
					cur = 0;
				} else {
					if(cur > maxgray) {
						cur = maxgray;
					}
				}
				/* resulting color & error */
				if(cur < threshold) {
					result |= bit;
				} else {
					cur -= maxgray;
				}
				/* propagate error fractions */
				bnexterr = cur;
				delta = (cur << 1);
				cur += delta;		/* form error * 3 */
				errorptr[0] = bpreverr + cur;
				cur += delta;		/* form error * 5 */
				bpreverr = belowerr + cur;
				belowerr = bnexterr;
				cur += delta;		/* form error * 7 */
				errorptr++;
			}
			*mono++ = htonl(result);
		}
		errorptr[0] = bpreverr;		/* unload prev err into array */
	}
}


/* FS-dither BM_DIRECT24 bitmap `bm' to a long aligned monochrome `mono_bm'.
 * `mono_bm' may be same as `bm'.  If DIRECT24 unitsize != 3, you'll have to
 * skip alignment bytes after every row when searching through bitmap for
 * min/maxgray values! If expand is specified, this will go through the
 * bitmap twice, first to determine min and max grayscale and second time
 * to dither.
 */
static void
FSditherTruecolor(BITMAP *bm, BITMAP *mono_bm, short *FSerrors, int expand)
{
	register ulong value;
	register uchar *color;
	register short mingray, maxgray;
	ulong *mono, result, last, bit, hibit;
	int x, y, longs, bits, unit;
	short threshold;

	register short cur;		/* current error or pixel value */
	register short delta;
	register short *errorptr;	/* FSerrors at line before current */
	short belowerr;			/* error for pixel below cur */
	short bpreverr;			/* error for below/prev col */
	short bnexterr;			/* error for below/next col */

	if (expand) {
		maxgray = 0;
		mingray = 255;
		color = bm->data;
		y = bm->height;
		while (--y >= 0) {
			for (cur = bm->width; cur > 0; cur--) {
				value =  (*color++ * 307UL);
				value += (*color++ * 599UL);
				value += (*color++ * 118UL);
				value >>= 10;
				if (value < mingray) {
					mingray = value;
				}
				if (value > maxgray) {
					maxgray = value;
				}
			}
		}
		maxgray -= mingray;
		if (maxgray < 2) {
			threshold = 1;
		} else {
			threshold = maxgray / 2;
		}
	} else {
		maxgray = 255;
		threshold = 127;
		mingray = 0;
	}


	unit = mono_bm->unitsize << 3;

	/* longs in the source - 1 */
	longs = (bm->width + (unit-1)) / unit - 1;

	/* bit/pixel counter for the last long, expects bm->upl to be pixels
	 * (ie. unitsize = pixel `size', ATM three bytes)!
	 */
	last = bm->upl - longs * unit;

	hibit = 1UL << (unit-1);

	color = bm->data;
	mono = mono_bm->data;

	y = bm->height;
	while (--y >= 0) {
		errorptr = FSerrors;
		cur = belowerr = bpreverr = 0;

		for(x = longs; x >= 0; x--) {
			result = 0;
			if (x) {
				bits = unit;
			} else {
				bits = last;
			}
			/* go through one long of pixels */
			for(bit = hibit; --bits >= 0; bit >>= 1) {

				/* color + error to range */
				cur = (cur + errorptr[1] + 8) >> 4;
				value =  (*color++ * 307UL);
				value += (*color++ * 599UL);
				value += (*color++ * 118UL);
				value >>= 10;
				cur += value - mingray;
				if(cur < 0) {
					cur = 0;
				} else {
					if(cur > maxgray) {
						cur = maxgray;
					}
				}
				/* resulting color & error */
				if(cur < threshold) {
					result |= bit;
				} else {
					cur -= maxgray;
				}
				/* propagate error fractions */
				bnexterr = cur;
				delta = (cur << 1);
				cur += delta;		/* form error * 3 */
				errorptr[0] = bpreverr + cur;
				cur += delta;		/* form error * 5 */
				bpreverr = belowerr + cur;
				belowerr = bnexterr;
				cur += delta;		/* form error * 7 */
				errorptr++;
			}
			*mono++ = htonl(result);
		}
		errorptr[0] = bpreverr;	/* unload prev err into array */
	}
}


/* convert DIRECT color bitmap to monochrome.  If `in_place' is set,
 * conversion is done to the source bitmap, otherwise a new one is
 * allocated.
 */
BITMAP *
fs_direct2mono(BITMAP *bm, int in_place, const char **error)
{
	int expand;
	BITMAP *dest;
	short *FS;

	/* first checks */
	if (bm->type != BM_DIRECT8 && bm->type != BM_DIRECT24) {
		*error = "source bitmap type error";
		return NULL;
	}
	if (bm->type == BM_DIRECT8 && !bm->palette) {
		*error = "bitmap palette missing";
		return NULL;
	}

	/* then allocations */
	FS = calloc(1, (bm->upl * bm->unitsize + 1) * sizeof(*FS));
	if (!FS) {
		*error = "FS-array alloc failed";
		return NULL;
	}

	if (in_place) {
		static BITMAP tmp;
		dest = &tmp;
		*dest = *bm;

		dest->type = BM_PACKEDMONO;
		w_bmheader(dest);

		if (bm->upl * bm->unitsize < dest->unitsize) {
			/* doesn't fit into same bitmap */
			in_place = 0;
		}
	}

	if (!in_place) {
		dest = w_allocbm(bm->width, bm->height, BM_PACKEDMONO, 2);
		if (!dest) {
			free(FS);
			*error = "destination bitmap alloc failed";
			return NULL;
		}
	}

	_wget_options(NULL, &expand);

	/* do the conversion */
	if (bm->type == BM_DIRECT24) {
		FSditherTruecolor(bm, dest, FS, expand);
	} else {
		/* we use row 0 as with the error propagation you should
		 * be doing whole images...
		 */
		FSditherDirect(bm, dest, _winit_mapping(bm, 0), FS, expand);
	}
	free(FS);

	/* in-place conversion? */
	if (in_place) {
		*bm = *dest;
	}
	return dest;
}


/* ------------------------------------------------------------ */

/* DIRECT24 BITMAP conversions goes through PACKEDMONO and takes therefore
 * a bit more memory...
 */
BITMAP *
w_convertBitmap (BITMAP *bm, short type, short colors)
{
	const char *cptr;
	BITMAP *use = NULL, *old = bm;
	uchar *start, *src, *dst, *(*conv)(BITMAP *, BITMAP *, uchar *);
	int width, height, linesize;

	TRACESTART();

	if (!bm) {
		cptr = "no BITMAP";
		goto error;
	}

	if (bm->type == type && bm->colors == colors) {
		return bm;
	}

	if (bm->type == BM_DIRECT24 ||
	    (bm->type == BM_DIRECT8 && type == BM_PACKEDMONO)) {

		/* line-by-line conversion uses inferior ordered
		 * dithering as that's independent of other pixels,
		 * but here we can use fs-dither.
		 */
		if (!(bm = fs_direct2mono(bm, 0, &cptr))) {
			goto error;
		}
		if (type == BM_PACKEDMONO) {
			TRACEPRINT(("w_convertBitmap(%p,%i,%i) -> %p\n",\
				bm, type, colors, bm));
			TRACEEND();
			return bm;
		}
	}

	width = bm->width;
	height = bm->height;

	if (!colors) {
		/* set explicitly */
		if (bm->type == BM_PACKEDMONO) {
			colors = 2;
		} else {
			if (type == BM_DIRECT8) {
				colors = 256;
			} else if (type == BM_PACKEDCOLOR) {
				colors = 1 << _wserver.planes;
			}
		}
	}

	if (!(use = w_allocbm(width, height, type, colors))) {
		cptr = "work bitmap allocation failed";
		goto error;
	}

	if (!(conv = w_convertFunction(bm, use, 0))) {
		cptr = "no bitmap conversion function";
		goto error;
	}

	/* transfer as much of the palette as possible, a real conversion
	 * would really not belong here...
	 */
	if (bm->palette && use->palette) {
		colors = (bm->colors < use->colors ? bm->colors : use->colors);
		memcpy(use->palette, bm->palette, colors * sizeof(rgb_t));
		use->colors = colors;
	}

	src = bm->data;
	dst = use->data;
	linesize = use->unitsize * use->upl;

	start = dst;
	while (--height >= 0) {
		use->data = dst;
		src = (*conv)(bm, use, src);
		dst += linesize;
	}
	use->data = start;

	if (bm && bm != old) {
		/* free temporary monochrome bitmap */
		w_freebm(bm);
	}

	TRACEPRINT(("w_convertBitmap(%p,%i,%i) -> %p\n",
		bm, type, colors, use));
	TRACEEND();
	return use;

error:
	if (use) {
		w_freebm(use);
	}
	if (bm && bm != old) {
		w_freebm(bm);
	}
	TRACEPRINT(("w_convertBitmap(%p,%i,%i) -> %s\n",\
		bm, type, colors, cptr));
	TRACEEND();
	return NULL;
}

