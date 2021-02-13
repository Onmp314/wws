/*
 * block.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib image put/get function mappings to Xlib
 */

#include <stdio.h>
#include <stdlib.h>
#include "Wlib.h"
#include "proto.h"



/*****************************************************************************/

/* change byte bit order for/from the X */
static void flip_bits(BITMAP *bm)
{
  uchar *start, *end, tmp;

  start = bm->data;
  end = start + bm->unitsize * bm->upl * bm->height;
  while(start < end)
  {
    tmp = *start;
    *start++ = (tmp << 7 & 0x80) | (tmp << 5 & 0x40) | (tmp << 3 & 0x20) |
               (tmp << 1 & 0x10) | (tmp >> 1 & 0x08) | (tmp >> 3 & 0x04) |
	       (tmp >> 5 & 0x02) | (tmp >> 7 & 0x01);
  }
}

/* get a block
 */
BITMAP *
w_getblock(WWIN *win, short x0, short y0, short width, short height)
{
	const char *cptr;
	BITMAP *bm = NULL;
	XImage *img = NULL;
	W2XWin *ptr = (W2XWin *)win;

	TRACESTART();

	if ((width < 1) || (height < 1)) {
		cptr = "illegal size";
		goto error;
	}

	if ((cptr = _check_window(win))) {
		goto error;
	}

	bm = calloc(1, sizeof(BITMAP));
	if (!bm) {
		cptr = "BITMAP alloc failed";
		goto error;
	}

	bm->width = width;
	bm->height = height;
	bm->type = BM_PACKEDMONO;
	w_bmheader(bm);

	/* unlike XPutImage, this needs XYPixmap instead of XYBitmap? */
	img = XGetImage(_Display, ptr->pixmap, x0, y0,
		(bm->upl * bm->unitsize) << 3, height,
		1UL, XYPixmap);

	if (!img) {
		cptr = "XImage get failed";
		goto error;
	}

	bm->data = img->data;
	flip_bits(bm);

	img->data = NULL;	/* It's *my* data now */
	XDestroyImage(img);

	TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> %p\n",\
		    win, x0, y0, width, height, bm));
	TRACEEND();
	return bm;

error:
	if (bm) {
		free (bm);
	}
	TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> %s\n",\
	            win, x0, y0, width, height, cptr));
	TRACEEND();
	return NULL;
}


/* put a block
 */
short
w_putblock (BITMAP *bm, WWIN *win, short x1, short y1)
{
	const char *cptr;
	XImage *img = NULL;
	BITMAP *use = NULL;
	W2XWin *ptr = (W2XWin *)win;
	uchar *line, *(*conv)(BITMAP *, BITMAP *, uchar *);
	int y2, width, height;

	TRACESTART();

	if ((cptr = _check_window(win))) {
		goto error;
	}

	width = bm->width;
	height = bm->height;

	if (!(use = w_allocbm(width, 1, _wserver.type, 1 << _wserver.planes))) {
		cptr = "work bitmap allocation failed";
		goto error;
	}

	if (!(conv = w_convertFunction(bm, use, y1))) {
		cptr = "no bitmap conversion function";
		goto error;
	}

	/* XImages are on Xlib side, Pixmaps on X server side */
	img = XCreateImage(_Display, NULL, 1, XYBitmap, 0, use->data,
		width, 1, use->unitsize * 8, use->unitsize * use->upl);

	if (!img) {
		cptr = "XImage creation failed";
		goto error;
	}

	y2 = y1;
	line = bm->data;
	while (--height >= 0) {

		/* convert line for the X server */
		line = (*conv)(bm, use, line);
		flip_bits(use);

		if (ptr->wid == _Rootwin) {
			XPutImage(_Display, _Rootwin, _Rootgc, img,
				0, 0, x1, y2, width, 1);
		} else {
			XPutImage(_Display, ptr->pixmap, _Rootgc, img,
				0, 0, x1, y2, width, 1);
		}
		y2++;
	}

	img->data = NULL;	/* It's 'use' BITMAP's data */
	XDestroyImage(img);
	w_freebm (use);
	use = NULL;

	if ((cptr = _flush_area(ptr, x1, y1, x1 + width, y2, 0))) {
		goto error;
	}

	TRACEPRINT(("w_putblock(%p,%p,%i,%i) -> 0\n", bm, win, x1, y1));
	TRACEEND();
	return 0;

error:
	if (use) {
		w_freebm (use);
	}
	TRACEPRINT(("w_putblock(%p,%p,%i,%i) -> %s\n",\
	           bm, win, x1, y1, cptr));
	TRACEEND();
	return -1;
}
