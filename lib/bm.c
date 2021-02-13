/*
 * bm.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- functions to allocate, copy, free and convert BITMAPs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


/* this one allocates a bitmap which is perfectly aligned for directly
 * sending it to the server.
 *
 * NOTE: PACKEDCOLOR will always have as many planes as W server
 * and hence palette size is same too (fixed if differs).
 */
BITMAP *
w_allocbm (short width, short height, short type, short colors)
{
	BITMAP *bm;

	TRACESTART();
	if (type != BM_PACKEDMONO &&
	    type != BM_PACKEDCOLOR &&
	    type != BM_DIRECT8 &&
	    type != BM_DIRECT24) {
		TRACEPRINT(("w_allocbm(%d,%d,%d,%d) -> illegal bm type\n",\
		           width, height, type, colors));
		TRACEEND();
		return NULL;
	}
	if (width < 1 || height < 1) {
		TRACEPRINT(("w_allocbm(%d,%d,%d,%d) -> illegal bm size\n",\
		           width, height, type, colors));
		TRACEEND();
		return NULL;
	}
	/* bogus color count (too large is fixed to suitable value) */
	if ((type == BM_PACKEDCOLOR || type ==BM_DIRECT8) && colors < 2) {
		TRACEPRINT(("w_allocbm(%d,%d,%d,%d) -> illegal colors\n",\
		           width, height, type, colors));
		TRACEEND();
		return NULL;
	}

	/* alloc and _zero_ BITMAP variables */
	if (!(bm = calloc(1, sizeof(BITMAP)))) {
		TRACEPRINT(("w_allocbm(%d,%d,%d,%d) -> bm alloc failed\n",\
		           width, height, type, colors));
		TRACEEND();
		return NULL;
	}
	bm->type = type;
	bm->width = width;
	bm->height = height;
	bm->colors = colors;

	/* get the rest */
	w_bmheader(bm);

	if (!(bm->data = malloc(bm->unitsize * bm->upl * height))) {
		free(bm);
		TRACEPRINT(("w_allocbm(%d,%d,%d,%d) -> data alloc failed\n",\
		           width, height, type, colors));
		TRACEEND();
		return NULL;
	}

	if (type == BM_DIRECT8 || type == BM_PACKEDCOLOR) {
		if (!(bm->palette = malloc(colors * sizeof(rgb_t)))) {
			free(bm->data);
			free(bm);
			TRACEPRINT(("w_allocbm(%d,%d,%d,%d) -> palette alloc failed\n",\
			           width, height, type, colors));
			TRACEEND();
			return NULL;
		}
	}
	TRACEPRINT(("w_allocbm(%d,%d,%d,%d) -> %p\n",\
	           width, height, type, colors, bm));
	TRACEEND();
	return bm;
}


BITMAP *
w_copybm(BITMAP *bm)
{
	BITMAP *new;

	TRACESTART();

	if (!bm) {
		TRACEPRINT(("w_copybm(NULL) -> no BITMAP\n"));
		TRACEEND();
		return NULL;
	}

	if (!(new = w_allocbm(bm->width, bm->height, bm->type, bm->colors))) {
		TRACEPRINT(("w_copybm(%p) -> bitmap allocation failed\n",\
		           bm));
		TRACEEND();
		return NULL;
	}
	memcpy(new->data, bm->data, bm->unitsize * bm->upl * bm->height);
	if (new->palette) {
		memcpy(new->palette, bm->palette, bm->colors * sizeof(rgb_t));
	}
	return new;
}


void
w_freebm(BITMAP *bm)
{
	TRACESTART();
	if (bm) {
		if (bm->palette) {
			free(bm->palette);
			bm->palette = NULL;	/* just to be sure... */
		}
		if (bm->data) {
			free(bm->data);
			bm->data = NULL;
		}
		free(bm);
		TRACEPRINT(("w_freebm(%p)\n", bm));
	} else {
		TRACEPRINT(("w_freebm(%p) -> no BITMAP\n", bm));
	}
	TRACEEND();
}
