/*
 * pbm24to8.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- filter for converting 24-bit PPM in stdin to 8-bit P8M on stdout.
 *
 * NOTES
 * - as Wlib doesn't do color reductions (there are several ways to do them,
 *   all fairly expensive) this program can be used to convert PPM images
 *   with at most 256 colors into something which can be viewed more easily
 *   on color server using `wbm'.
 */

#include <stdio.h>
#include <Wlib.h>


static inline void
map_24to8 (uchar *src, uchar *dst, BITMAP *bm)
{
	int closest, rd, gd, bd, idx, colors = bm->colors;
	rgb_t *rgb = bm->palette;
	uchar red, green, blue;
	long dist, mindist;

	red = *src++;
	green = *src++;
	blue = *src;

	/* is color already in palette?
	 */
	for (idx = 0; idx < colors; idx++) {

		if (red == rgb->red && green == rgb->green && blue == rgb->blue) {
			*dst = idx;
			return;
		}
		rgb++;
	}

	/* do we have an entry for new color?
	 */
	if (colors < 256) {

		rgb->red = red;
		rgb->green = green;
		rgb->blue = blue;

		*dst = colors;
		bm->colors++;

		return;
	}

	/* hope that 256 first colors were distributed evenly enough and...
	 */
	fprintf(stderr, "More than 256 colors, matching color to previous one...\n");

	rgb = bm->palette;
	mindist = 0x7fffffff;
	closest = 0;

	/* ...find a closest (least square) match from them to new color.
	 */
	for (idx = 0; idx < colors; idx++) {

		rd = (int)red   - rgb->red;
		gd = (int)green - rgb->green;
		bd = (int)blue  - rgb->blue;
		dist = (long)rd*rd + gd*gd + bd*bd;

		if (dist < mindist) {
			mindist = dist;
			closest = idx;
		}
		rgb++;
	}

	*dst = closest;
}


int
main(int argc, char *argv[])
{
	BITMAP *dst_bm, *src_bm = w_readpbm(argv[1]);
	int src_off, dst_off, w, h;
	uchar *src, *dst;

	if (!src_bm) {
		fprintf(stderr, "PPM reading from stdin failed!\n");
		return -1;
	}

	if (src_bm->type != BM_DIRECT24) {
		fprintf(stderr, "Not a 24-bit PPM, passing along unmodified\n");
		w_writepbm(NULL, src_bm);
		return -1;
	}

	dst_bm = w_allocbm(src_bm->width, src_bm->height, BM_DIRECT8, 255);
	if (!dst_bm) {
		fprintf(stderr, "Destination P8M allocation failed!\n");
		return -1;
	}

	dst_bm->colors = 0;
	src = src_bm->data;
	dst = dst_bm->data;

	src_off = (src_bm->unitsize * src_bm->upl) - 3 * src_bm->width;
	dst_off = (dst_bm->unitsize * dst_bm->upl) - dst_bm->width;

	h = src_bm->height;
	while (--h >= 0) {

		w = src_bm->width;
		while (--w >= 0) {
			map_24to8 (src, dst, dst_bm);
			src += 3;
			dst++;
		}
		src += src_off;
		dst += dst_off;
	}

	if (w_writepbm(NULL, dst_bm)) {
		fprintf(stderr, "P8M writing to stdout failed!\n");
		return -1;
	}
	return 0;
}
