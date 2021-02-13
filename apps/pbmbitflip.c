/*
 * pbmbitflip.c, a part of the W Window System
 *
 * Copyright (C) 1999 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- filter for flipping bits (mirror, not invert) in pbm image data
 */

#include <stdio.h>
#include <Wlib.h>


/* change byte bit order for the whole bitmap data */
static void
flip_bits(BITMAP *bm)
{
	uchar *start, *end, tmp;

	start = bm->data;
	end = start + bm->unitsize * bm->upl * bm->height;
	while(start < end)
	{
		tmp = *start;
		*start++ =
		(tmp << 7 & 0x80) | (tmp << 5 & 0x40) | (tmp << 3 & 0x20) |
		(tmp << 1 & 0x10) | (tmp >> 1 & 0x08) | (tmp >> 3 & 0x04) |
		(tmp >> 5 & 0x02) | (tmp >> 7 & 0x01);
	}
}


int
main(int argc, char *argv[])
{
	BITMAP *src_bm = w_readpbm(argv[1]);

	if (!src_bm) {
		fprintf(stderr, "PBM reading from stdin failed!\n");
		return -1;
	}

	if (src_bm->type == BM_PACKEDMONO) {
		flip_bits(src_bm);
	} else {
		fprintf(stderr, "Not a PBM, passing along unmodified\n");
	}

	if (w_writepbm(NULL, src_bm)) {
		fprintf(stderr, "PBM writing to stdout failed!\n");
		return -1;
	}
	return 0;
}
