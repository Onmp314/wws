/*
 * block.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- get/put an image from/to W screen
 *
 * CHANGES
 * ++eero, 5/97:
 * - Fixed get/putblock functions.
 * - Added color and trace stuff to w_alloc/freebm().
 * - moved bitmap alloc/free and convertion functions to convert.c file.
 * - Changed putblock to do image convertion/sending line-by-line.
 * ++eero, 2/98:
 * - Putblock converts as many lines at the time as will fit into the paket
 *   to be sent to the server.  Line-by-line wasn't very nice when
 *   outputting eg. icons one at the time...
 * ++eero, 6/98:
 * - To avoid extra malloc & memcpy, images are now converted straight to
 *   the socket buffer.  As image convertion is done line by line, image
 *   line cannot anymore be longer than fits into packet / Wlib buffer.  If
 *   this is an issue, buffer size doubling might help.  Current limit is
 *   4000 for DIRECT8 images.
 * ++eero, 8/09:
 * - workaround for uninitialized mono bitmap color count
 * - fix gcc 4.x warnings
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Wlib.h"
#include "proto.h"


/*****************************************************************************/

/* get a block
 */
BITMAP *
w_getblock(WWIN *win, short x0, short y0, short width, short height)
{
	char *data;
	BITMAP *ret;
	GETBLKREQP *paket;
	GETBLKDATAP *paket2;
	PAKET *rawdata;
	long size, todo;
	int datasize;
	const char *cptr;

	TRACESTART();

	if ((width < 1) || (height < 1)) {
		TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> illegal size\n",\
		            win, x0, y0, width, height));
		TRACEEND();
	}

	if ((cptr = _check_window(win))) {
		TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> %s\n",\
		            win, x0, y0, width, height, cptr));
		TRACEEND();
		return NULL;
	}

	ret = w_allocbm (width, height, _wserver.type, 1<<_wserver.planes);
	if (!ret) {
		TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> alloc failed\n",\
		            win, x0, y0, width, height));
		TRACEEND();
		return NULL;
	}
	size = ret->height * ret->unitsize * ret->upl;

	paket = _wreservep(sizeof(GETBLKREQP));
	paket->type   = htons(PAK_GETBLKREQ);
	paket->handle = htons(win->handle);
	paket->x0     = htons(x0);
	paket->y0     = htons(y0);
	paket->width  = htons(width);
	paket->height = htons(height);

	todo = ntohl(((LRETP *)_wait4paket(PAK_LRET))->ret);
	if (todo != size) {
		w_freebm (ret);
		TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> NULL (%ld!=%ld)\n",\
		            win, x0, y0, width, height, size, todo));
		TRACEEND();
		return NULL;
	}

	data = ret->data;
	while (todo > 0) {
		paket2 = _wreservep(sizeof(GETBLKDATAP));
		paket2->type = htons(PAK_GETBLKDATA);

		rawdata = _wait4paket(PAK_RAWDATA);
		datasize = rawdata->len - (sizeof(*rawdata)-sizeof(rawdata->data));
		if (todo - datasize < 0) {
			w_freebm (ret);
			TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> NULL\n",\
			            win, x0, y0, width, height));
			TRACEEND();
			return NULL;
		}
		memcpy(data, rawdata->data, datasize);
		data += datasize;
		todo -= datasize;
	}

	TRACEPRINT(("w_getblock(%p,%i,%i,%i,%i) -> %p\n",\
		    win, x0, y0, width, height, ret));
	TRACEEND();

	/* with a bogus palette... */
	return ret;
}


/* put a block
 */
short
w_putblock (BITMAP *bm, WWIN *win, short x1, short y1)
{
	BITMAP use;
	PAKET *dpaket;
	PUTBLKREQP *paket;
	uchar *src, *dst, *(*conv)(BITMAP *, BITMAP *, uchar *);
	int width, height, lines, count, size, todo, linesize;
	const char *cptr;

	TRACESTART();

	if (!bm) {
		cptr = "no BITMAP";
		goto error;
	}
	if ((cptr = _check_window(win))) {
		goto error;
	}

	width  = bm->width;
	height = bm->height;

	use.width = width;
	if (bm->planes > 1) {
		/* need correct colorcount for direct8/24 */
		use.colors = bm->colors;
	} else {
		/* but it's not often set for mono pics */
		use.colors = 2;
	}
	use.type  = _wserver.type;
	w_bmheader(&use);

	size = use.upl * use.unitsize;

	/* how many lines fits into server packet?
	 */
	lines = sizeof(dpaket->data) / size;
	if (!lines) {
		cptr = "Image line too long for Wlib socket buffer";
		goto error;
	}
	use.height = lines;

	if (!(conv = w_convertFunction(bm, &use, y1))) {
		cptr = "no bitmap conversion function";
		goto error;
	}

	paket = _wreservep(sizeof(PUTBLKREQP));
	paket->type   = htons(PAK_PUTBLKREQ);
	paket->width  = htons(width);
	paket->height = htons(height);
	paket->handle = htons(win->handle);
	paket->x1     = htons(x1);
	paket->y1     = htons(y1);
	paket->shmKey = htonl(0);

	todo = ntohl(((LRETP *)_wait4paket(PAK_LRET))->ret);
	size *= height;

	if (todo != size) {
		TRACEPRINT(("server and bm sizes differ (%d!=%d)!\n",\
		           todo, size));
		goto error;
	}

	/* I'll convert and send as many lines (or at least one) at the time
	 * as can be fit into the server packet.  IMHO that should balance
	 * fairly well between speed and memory usage.  ++eero 2/98
	 */
	size = sizeof(*dpaket) - sizeof(dpaket->data);
	linesize = use.upl * use.unitsize;
	src = bm->data;

	while (height > 0) {

		count = lines;
		height -= count;
		if (height < 0) {
			count += height;
		}

		dpaket = _wreservep(size + count * linesize);
		dpaket->type = htons(PAK_RAWDATA);
		dst = dpaket->data;

		while(--count >= 0) {
			use.data = dst;
			src = (*conv)(bm, &use, src);
			dst += linesize;
		}
	}

	TRACEPRINT(("w_putblock(%p,%p,%i,%i) -> 0\n", bm, win, x1, y1));
	TRACEEND();
	return 0;

error:
	TRACEPRINT(("w_putblock(%p,%p,%i,%i) -> %s\n",\
	           bm, win, x1, y1, cptr));
	TRACEEND();
	return -1;
}
