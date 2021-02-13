/*
 * wsetbg.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- set W server background color and/or pattern
 *
 * NOTES
 * - originally this loaded and IMG to W background, but as 'wbm' can
 *   do that and more, I rewrote this completely and preserved just name :)
 *
 * CHANGES
 * ++eero, 2/98:
 * - pattern ID is now interpreted as hexadecimal.
 * - user can now use hatch parameters too.
 * ++eero, 6/98:
 * - On color servers sets the background color instead of pattern (well, a
 *   single color value is interpreted as pattern if it's greater than the
 *   number of shared colors).
 */

#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>
#include "../server/config.h"


static void usage(void)
{
	fprintf(stderr, "usage:\twsetbg\n\n");
	fprintf(stderr, "\twsetbg <pattern id>\t\t\t(fffe)\n");
	fprintf(stderr, "\twsetbg <hatch id> <line width> <times>\t(2 8 3)\n\n");
	fprintf(stderr, "\twsetbg <shared color index>\t\t(7)\n");
	fprintf(stderr, "\twsetbg <red> <green> <blue>\t\t(255 255 255)\n\n");
	fprintf(stderr, "First sets the default, next two work on monochrome and last\n");
	fprintf(stderr, "two on color W servers.  Maximum values are in parenthesis.\n");
}

/*
 * guess what...
 */

int main(int argc, char *argv[])
{
	ushort color = W_PATTERN, colors = 0;
	WSERVER *wserver;

	if ((argc > 1 && argv[1][0] == '-') || (argc > 2 && argc != 4)) {
		usage();
		return -1;
	}

	if (!(wserver = w_init())) {
		fprintf(stderr, "error: can't connect to W server\n");
		return -1;
	}

	if (wserver->planes > 1) {
		colors = 1;
		if (argc == 1) {
			/* sets later default background, so set default
			 * color here too
			 */
			w_setForegroundColor(WROOT, FGCOL_INDEX);
			w_setBackgroundColor(WROOT, BGCOL_INDEX);
		}
	}

	if (argc == 2) {
		sscanf(argv[1], "%hx", &color);
		if (colors && color >= wserver->sharedcolors) {
			colors = 0;
		}
	}

	if (argc == 4) {

		short id;
		uchar r, g, b;
		r = atoi(argv[1]);
		g = atoi(argv[2]);
		b = atoi(argv[3]);

		if (colors) {
			/* free first non-shared color just in case
			 * */
			w_freeColor(WROOT, wserver->sharedcolors);

			if ((id = w_allocColor(WROOT, r, g, b)) < 0) {
				usage();
				fprintf(stderr, "\nerror: color allocation failed!\n");
				return -1;
			}
			color = id;
		} else {
			if (!(color = w_hatch(r, g, b))) {
				usage();
				fprintf(stderr, "\nerror: illegal hatch parameters!\n");
				return -1;
			}
		}
	}

	w_setmode(WROOT, M_DRAW);

	if (colors) {

		w_setForegroundColor(WROOT, color);
		w_pbox(WROOT, 0, 0, wserver->width, wserver->height);
	} else {

		w_setpattern(WROOT, color);
		w_dpbox(WROOT, 0, 0, wserver->width, wserver->height);
	}
	w_flush();

	return 0;
}
