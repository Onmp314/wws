/*
 * palette.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- map BITMAP to (changed/allocated) private window palette
 *
 * NOTES
 * - maps BM_DIRECT8 bitmaps only (it's only user format with palette).
 * - BITMAP pixel color indeces won't correspond to palette entries
 *   after w_mapData() and reversing it might not produce the original.
 * - doesn't do color conversions/reductions so works only if server/window
 *   has at least as many colors as the given bitmap.
 * - this relies win->colors to be correct.
 */

#include <stdio.h>
#include <stdlib.h>
#include "Wlib.h"
#include "proto.h"


uchar *w_allocMap(WWIN *win, short mapcolors, rgb_t *rgb, uchar *colmap)
{
	short colors = 1 << _wserver.planes;
	uchar *mapping;
	int last, id;
	const char *cptr;

	TRACESTART();

	if ((cptr = _check_window(win))) {
		TRACEPRINT(("w_allocMap(%p,%i,%p,%p) -> %s\n",
			win, mapcolors, rgb, colmap, cptr));
		TRACEEND();
		return NULL;
	}

	/* no palette to map?
	 */
	if (!(rgb && mapcolors)) {
		TRACEPRINT(("w_allocMap(%p,%i,%p,%p) -> palette error\n",
			win, mapcolors, rgb, colmap));
		TRACEEND();
		return NULL;
	}

	/* has server enough colors?
	 */
	if (mapcolors > colors) {
		TRACEPRINT(("w_allocMap(%p,%i,%p,%p) -> not enough colors\n",
			win, mapcolors, rgb, colmap));
		TRACEEND();
		return NULL;
	}

	/* make room for new colors if necessary
	 */
	if (win == WROOT) {

		/* free all colors except the shared ones */
		w_freeColor(win, -1);

	} else {

		short shared = _wserver.sharedcolors;
		colors = mapcolors - colors;

		/* need to free inherited colors?
		 */
		if (!colmap && colors + win->colors > 0) {

			/* free all colors besides shared.  w_allocColor()
			 * will take care that new ones will be the least
			 * used ones.
			 */
			w_freeColor(win, -1);
		}

		/* need to free inherited colors?
		 */
		if ((colors += shared) > 0) {

			while (--colors >= 0) {
				w_freeColor(win, --shared);
			}
		}
	}

	colors = last = 0;

	if (colmap) {

		/* re-map already allocated colors
		 */
		mapping = colmap;
		while (colors++ < win->colors) {

			id = w_changeColor(win, *colmap,
					   rgb->red, rgb->green, rgb->blue);

			if (id >= 0) {
				*colmap = id;
				last = id;
			} else {
				/* a win->colors error */
				*colmap = last;
			}
			colmap++;
			rgb++;
		}		
	} else {
		/* no previously allocated colors, so alloc new map
		 */
		if (!(colmap = malloc (256))) {
			TRACEPRINT(("w_allocMap(%p,%i,%p,%p) -> color map alloc failed\n",
				win, mapcolors, rgb, colmap));
			TRACEEND();
			return NULL;
		}
		mapping = colmap;
	}

	while (colors++ < mapcolors) {

		id = w_allocColor(win, rgb->red, rgb->green, rgb->blue);

		if (id >= 0) {
			*colmap = id;
			last = id;
		} else {
			/* works best for (grayscale) images where
			 * colors are sorted, like with djpeg...
			 *
			 * We could also do closest match like in ppm24to8,
			 * but images should already be quantized, so this
			 * is only the easiest possible fallback.
			 */
			*colmap = last;
		}
		colmap++;
		rgb++;
	}

	TRACEPRINT(("w_allocMap(%p,%i,%p,%p) -> %p\n",
		win, mapcolors, rgb, colmap, mapping));
	TRACEEND();

	return mapping;
}


short w_mapData(BITMAP *bm, uchar *colmap)
{
	uchar *start, *data;

	TRACESTART();

	/* currently only (chunky) 8-bit images supported
	 */
	if (!bm || bm->type != BM_DIRECT8) {
		TRACEPRINT(("w_mapData(%p,%p) -> BITMAP error\n", bm, colmap));
		TRACEEND();
		return -1;
	}
	if (!colmap) {
		TRACEPRINT(("w_mapData(%p,%p) -> missing map\n", bm, colmap));
		TRACEEND();
		return -1;
	}

	start = (uchar *)bm->data;
	data = start + bm->upl * bm->unitsize * bm->height;

	/* map BITMAP color indeces to window color indeces
	 */
	while (--data >= start) {
		*data = colmap[*data];
	}

	TRACEPRINT(("w_mapData(%p,%p) -> 0\n", bm, colmap));
	TRACEEND();

	return 0;
}
