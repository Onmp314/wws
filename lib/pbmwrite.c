/*
 * pbmwrite.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- write binary PBM formats
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


static short
mono_saver(FILE *fp, BITMAP *bm)
{
	int index, dbytes, sbytes;
	uchar *datap;

	sbytes = bm->upl * bm->unitsize;
	dbytes = (bm->width + 7) / 8;
	datap = bm->data;
	index = bm->height;
	while (--index >= 0) {
		if (fwrite(datap, dbytes, 1, fp) != 1) {
			return -1;
		}
		datap += sbytes;
	}
	return 0;
}

static short
color_saver(FILE *fp, BITMAP *bm)
{
	int index, sbytes;
	uchar *datap, buf[3];
	rgb_t *rgb;

	index = bm->colors;
	fprintf(fp, "%d\n", index);

	rgb = bm->palette;
	while (--index >= 0) {
		buf[0] = rgb->red;
		buf[1] = rgb->green;
		buf[2] = rgb->blue;
		if (fwrite(buf, 3, 1, fp) != 1) {
			return -1;
		}
		rgb++;
	}
	sbytes = bm->upl * bm->unitsize;
	datap = bm->data;
	index = bm->height;
	while (--index >= 0) {
		if (fwrite(datap, bm->width, 1, fp) != 1) {
			return -1;
		}
		datap += sbytes;
	}
	return 0;
}

static short
trueColor_saver(FILE *fp, BITMAP *bm)
{
	fprintf(fp, "%d\n", 255);	/* max color channel value */
	if (fwrite(bm->data, (long)bm->width * 3 * bm->height, 1, fp) != 1) {
		return -1;
	}
	return 0;
}

short
w_writepbm(const char *path, BITMAP *bm)
{
	short ret, (*saver) (FILE *, BITMAP *);
	BITMAP *old = bm;
	char type;
	FILE *fp;

	TRACESTART();

	if (!bm) {
		TRACEPRINT(("w_writepbm(`%s',%p) -> no bitmap\n", path, bm));
		TRACEEND();
		return -1;
	}
	type = 0;
	saver = NULL;
	switch (bm->type) {
		case BM_PACKEDMONO:
			type = '4';
			saver = mono_saver;
			break;
		case BM_PACKEDCOLOR:
			bm = w_convertBitmap(bm, BM_DIRECT8, bm->colors);
			if (!bm) {
				break;
			}
		case BM_DIRECT8:
			type = '8';
			saver = color_saver;
			break;
		case BM_DIRECT24:
			type = '6';
			saver = trueColor_saver;
			break;
	}
	if (!saver) {
		TRACEPRINT(("w_writepbm(`%s',%p) -> no bitmap type handler\n", path, bm));
		TRACEEND();
		return -1;
	}
	if (!path) {
		fp = stdout;
	} else {
		if (!(fp = fopen(path, "wb"))) {
			if (bm != old) {
				w_freebm(bm);
			}
			TRACEPRINT(("w_writepbm(`%s',%p) -> unable to open file\n", path, bm));
			TRACEEND();
			return -1;
		}
	}
	/* write image */
	fprintf(fp, "P%c\n%d %d\n", type, bm->width, bm->height);
	ret = (*saver) (fp, bm);
	if (path) {
		fclose(fp);
	}
	if (bm != old) {
		w_freebm(bm);
	}
	TRACEPRINT(("w_writepbm(`%s',%p) -> %d\n", path, bm, ret));
	TRACEEND();
	return ret;
}
