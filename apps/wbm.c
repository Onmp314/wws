/*
 * wbm.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- read, scale and show Wlib supported PBM images (from file or pipe)
 *
 * You can use program like this:
 *	$ djpeg -grayscale color.jpeg | test_pbm -r -s &
 * to output color.jpeg so that it fills the W root window.  :)
 *
 * At least for now it's better to convert images first to grayscale as W
 * doesn't support colors very well yet and that takes lot less memory.
 *
 * CHANGES:
 * 09/97
 * - Added (DIRECT8) image scaling.
 * 10/97
 * - Added DIRECT24 image scaling and root window output.
 * 11/97
 * - Added scaling/tiling to window size.
 * 03/98
 * - Do tiling with bitblk instead of putblock.
 * 06/98
 * - Window palette setting for color servers.
 * - Show image information option.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Wlib.h>

/* DIRECT8 bitmap widths are long aligned, DIRECT24 pixel aligned */
#define DIRECT8_ALIGN	4

/* enable two debugging options */
#define DEBUG


/* functions for scaling given DIRECT bitmap to given size.  Operation is
 * done in-place if argument `in_place' is set (or new size is smaller than
 * old one).  Zero width or height are interpreted as current bitmap values.
 */
static BITMAP *
scale_bitmap(BITMAP *bm, register int to_width, int to_height, int in_place)
{
	register uchar *from, *to;
	register int x, xoff, tc;
	int y, yoff, to_bytes, from_bytes, from_width, from_height;
	uchar *start_from, *start_to;
	BITMAP *ret;

	/* checks */
	if ((bm->type != BM_DIRECT8 && bm->type != BM_DIRECT24)
	    || !(to_width || to_height)) {
		return bm;
	}
	if (to_width < 1) {
		to_width = bm->width;
	}
	if (to_height < 1) {
		to_height = bm->height;
	}

	from_width = bm->width;
	from_height = bm->height;

	if (to_width == from_width && to_height == from_height) {
		return bm;
	}

	from_bytes = bm->unitsize * bm->upl;
	tc = (bm->type == BM_DIRECT24);
	if (tc) {
		to_bytes = to_width * 3;
	} else {
		to_bytes = (to_width + (DIRECT8_ALIGN-1)) & ~(DIRECT8_ALIGN-1);
	}

	/* allocation */
	if (!in_place || from_bytes < to_bytes ||
	    from_bytes * from_height < to_bytes * to_height) {
		ret = w_allocbm(to_width, to_height, bm->type, bm->colors);
		if (!ret) {
			return bm;
		}
		if (to_bytes != ret->unitsize * ret->upl) {
			fprintf(stderr, "wbm::scale_bitmap(): to_bytes != bitmap width!\n");
			to_bytes = ret->unitsize * ret->upl;
		}
		memcpy(ret->palette, bm->palette, sizeof(rgb_t) * bm->colors);
	} else {
		ret = bm;
	}

	start_to = ret->data;
	start_from = bm->data;

	/* (copy with) shrink / enlarge */
	yoff = y = to_height;
	while (y > 0) {
		to = start_to;
		from = start_from;
		xoff = x = to_width;
		while (x > 0) {
			while (xoff > 0) {
				*to++ = *from;
				if (tc) {
					*to++ = *(from+1);
					*to++ = *(from+2);
				}
				xoff -= from_width;
				x--;
			}
			do {
				if (tc) {
					from += 3;
				} else {
					from++;
				}
				xoff += to_width;
			} while(xoff <= 0);
		}
		while (--y) {
			yoff -= from_height;
			start_to += to_bytes;
			if (yoff <= 0) {
				break;
			}
			memcpy(start_to, start_to - to_bytes, to_bytes);
		}
		do {
			yoff += to_height;
			start_from += from_bytes;
		} while (yoff <= 0);
	}

	ret->width = to_width;
	ret->height = to_height;
	ret->upl = to_bytes / bm->unitsize;
	return ret;
}


static void
help(char *name, char *option)
{
	fprintf(stderr, "\n%s: argument error '%s'\n", name, option);
	fprintf(stderr, "\nusage: %s [options] [PBM file]\n", name);
#ifdef DEBUG
	fprintf(stderr, "  -d    Switch Wlib debugging on\n");
	fprintf(stderr, "  -c #  Convert first to bitmap type number # (see Wlib.h)\n");
	fprintf(stderr, "  -e    Disable contrast expansion if type above is monochrome\n");
#endif
	fprintf(stderr, "  -g #  Set image (window) geometry (size/position)\n");
	fprintf(stderr, "  -r    Output image to the W root window\n");
	fprintf(stderr, "  -s    Scale image to root window size\n");
	fprintf(stderr, "  -t    Tile image to the window\n");
	fprintf(stderr, "  -i    Show image information\n\n");
	fprintf(stderr, "Image scaling works only for DIRECT (8 or 24 bit) images!\n");
	fprintf(stderr, "If no PBM filename is given, stdin will be used instead.\n");
}


int
main(int argc, char *argv[])
{
#ifdef DEBUG
	int type = 0, debug = 0;
#endif
	int info, root, tile, scale, idx;
	short x, y, w, h;
	BITMAP *bm, *old;
	WSERVER *server;
	uchar *colmap;
	WWIN *win;

	w = h = 0;
	x = y = UNDEF;
	info = scale = tile = root = idx = 0;
	while (++idx < argc) {
		if (argv[idx][0] == '-' && argv[idx][2] == '\0') {
			switch (argv[idx][1]) {
#ifdef DEBUG
			case 'd':
				debug = 1;
				break;
			case 'e':
				w_ditherOptions(NULL, 0);
				break;
			case 'c':
				if (++idx < argc) {
					type = atoi(argv[idx]);
					if (type > 0)
						break;
				}
				help(*argv, argv[idx]);
				return -1;
#endif
			case 'g':
				if (++idx < argc) {
					scan_geometry(argv[idx], &w, &h, &x, &y);
					break;
				}
				help(*argv, argv[idx]);
				return -1;
			case 'r':
				root = 1;
				break;
			case 's':
				scale = 1;
				tile = 0;
				break;
			case 't':
				tile = 1;
				scale = 0;
				break;
			case 'i':
				info = 1;
				break;
			default:
				help(*argv, argv[idx]);
				return -1;
			}
		} else {
			break;
		}
	}

#ifdef DEBUG
	if (debug) {
		w_trace(1);
	}
#endif

	if (!(server = w_init())) {
		fprintf(stderr, "\n%s: W server connection failed\n", *argv);
		return -1;
	}

	if (!(bm = w_readpbm(argv[idx]))) {
		fprintf(stderr, "\n%s: Cannot read image `%s'\n", *argv, argv[idx]);
		return -1;
	}

	if (info) {
		const char *msg = "unknown";

		switch (bm->type) {
			case BM_PACKEDMONO:
				msg = "monochrome";
				break;
			case BM_PACKEDCOLOR:
				msg = "interleaved color";
				break;
			case BM_DIRECT8:
				msg = "palettized 8-bit color";
				break;
			case BM_DIRECT24:
				msg = "24-bit truecolor";
				break;
		}
		fprintf(stderr, "%s is a %dx%dx%d %s image\n", argv[idx],
			bm->width, bm->height, bm->colors, msg);
		return 0;
	}

	if (scale) {
		w = server->width;
		h = server->height;
		if (!root) {
			/* substract window border widths */
			w -= 8;
			h -= 20;
		}
	}

	old = bm;
	bm = scale_bitmap(bm, w, h, 1);
	if (bm != old) {
		w_freebm(old);
	}

#ifdef DEBUG
	if (type) {
		old = bm;
		if (!(bm = w_convertBitmap(bm, type, bm->colors))) {
			fprintf(stderr, "\n%s: type %d conversion failed\n", *argv, type);
			return -1;
		}
		if (old != bm) {
			w_freebm(old);
		}
	}
#endif

	if (root) {
		win = WROOT;
	} else {
		win = w_create(bm->width, bm->height, W_MOVE|EV_KEYS);
		if (!win) {
			fprintf(stderr, "\n%s: unable to open (%d,%d) window\n",
			        *argv, bm->width, bm->height);
			return -1;
		}
	}

	/* map colors if bm is DIRECT8 and server/window has enough colors
	 */
	colmap = w_allocMap(win, bm->colors, bm->palette, NULL);
	if (colmap) {
		w_mapData(bm, colmap);
		free(colmap);
	}

	if (tile) {
		if (w_putblock(bm, win, 0, 0)) {
			fprintf(stderr, "\n%s: image putblock failed\n", *argv);
			return -1;
		}
		w_freebm(bm);

		for (w = bm->width; w < win->width; w += bm->width) {
			w_bitblk(win, 0, 0, bm->width, bm->height, w, 0);
		}
		for (h = bm->height; h < win->height; h += bm->height) {
			w_bitblk(win, 0, 0, win->width, bm->height, 0, h);
		}
	} else {
		w = (win->width - bm->width) / 2;
		h = (win->height - bm->height) / 2;
		if (w_putblock(bm, win, w, h)) {
			fprintf(stderr, "\n%s: image putblock failed\n", *argv);
			return -1;
		}
		w_freebm(bm);
	}
	w_flush();

	if (!root) {
		if (w_open(win, x, y)) {
			fprintf(stderr, "\n%s: window open failed\n", *argv);
			return -1;
		}
		w_queryevent(NULL, NULL, NULL, -1);
	}
	w_exit();
	return 0;
}
