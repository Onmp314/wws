/*
 * pbm8to24.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- filter for converting 8-bit P8M in stdin to 24-bit PPM on stdout.
 *
 * NOTES
 * - You can use this to convert images produced by `ppm24to8' back to
 *   the standard PPM image format.
 */

#include <stdio.h>
#include <Wlib.h>


int main(int argc, char *argv[])
{
	BITMAP *dst_bm, *src_bm = w_readpbm(argv[1]);
	int src_off, dst_off, w, h;
	rgb_t *rgb, *palette;
	uchar *src, *dst;

	if (!src_bm) {
		fprintf(stderr, "P8M reading from stdin failed!\n");
		return -1;
	}

	if (src_bm->type != BM_DIRECT8) {
		fprintf(stderr, "Not a P8M image, passing along unmodified\n");
		w_writepbm(NULL, src_bm);
		return -1;
	}

	dst_bm = w_allocbm(src_bm->width, src_bm->height, BM_DIRECT24, src_bm->colors);
	if (!dst_bm) {
		fprintf(stderr, "Destination PPM allocation failed!\n");
		return -1;
	}

	src = src_bm->data;
	dst = dst_bm->data;

	src_off = (src_bm->unitsize * src_bm->upl) - src_bm->width;
	dst_off = (dst_bm->unitsize * dst_bm->upl) - 3 * dst_bm->width;

	palette = src_bm->palette;

	h = src_bm->height;
	while (--h >= 0) {

		w = src_bm->width;
		while (--w >= 0) {

			rgb = &palette[*src++];
			*dst++ = rgb->red;
			*dst++ = rgb->green;
			*dst++ = rgb->blue;
		}

		src += src_off;
		dst += dst_off;
	}

	if (w_writepbm(NULL, dst_bm)) {
		fprintf(stderr, "PPM writing to stdout failed!\n");
		return -1;
	}
	return 0;
}
