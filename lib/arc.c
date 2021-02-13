/*
 * arc.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines for drawing arcs and pies
 *
 * NOTES
 * Uses qsin() and qcos() from qmath.c instead of sin() / cos() so that
 * linker doesn't require math library when linking programs with a dynamic
 * Wlib.
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"

/* 
 * Note that on screen X grows to right and Y grows down!
 * Therefore we'll flip the angle X-coordinate around Y-axis.
 *
 * Dir is positive when right side is filled and negative when
 * left side is to be filled.
 */
static inline short
do_pie(WWIN *win, short x0, short y0, short rx, short ry,
	float a, float b, short type, const char *fname)
{
	short ax, ay, bx, by, adir, bdir;
	const char *cptr;
	PIEP *paket;

	TRACESTART();

	if ((cptr = _check_window(win))) {
		TRACEPRINT(("%s(%p,%i,%i,%i,%i,%f,%f) -> %s\n", fname, win,\
			x0, y0, rx, ry, a, b, cptr));
		TRACEEND();
		return -1;
	}

	/* calculate pie edge offsets from center to the ellipse rim */
	ax = qcos(a) * rx;
	bx = qcos(b) * rx;

	a = -qsin(a);
	b = -qsin(b);

	/* for accuracy reasons it's better to use floating point */
	if (a >= 0.0) {
		adir = 1;
	} else {
		adir = -1;
	}
	if (b >= 0.0) {
		bdir = -1;
	} else {
		bdir = 1;
	}
	ay = a * ry;
	by = b * ry;

	/* swap rightmost edge first */
	if (bx > ax) {
		short swap;

		swap = ax;
		ax = bx;
		bx = swap;

		swap = ay;
		ay = by;
		by = swap;

		swap = adir;
		adir = bdir;
		bdir = swap;
	}

	paket = _wreservep(sizeof(PIEP));
	paket->type   = htons(type);
	paket->handle = htons(win->handle);

	paket->x0 = htons(x0);
	paket->y0 = htons(y0);
	paket->rx = htons(rx);
	paket->ry = htons(ry);
	paket->ax = htons(ax);
	paket->ay = htons(ay);
	paket->bx = htons(bx);
	paket->by = htons(by);
	paket->adir = htons(adir);
	paket->bdir = htons(bdir);

	TRACEPRINT(("%s(%p,%i,%i,%i,%i,%f,%f) -> 0\n", fname, win,\
			x0, y0, rx, ry, a, b));
	TRACEEND();
	return 0;
}


short
w_arc(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
	return do_pie(win, x0, y0, rx, ry, a, b, PAK_ARC, "w_arc");
}

short
w_darc(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
	return do_pie(win, x0, y0, rx, ry, a, b, PAK_DARC, "w_darc");
}

short
w_pie(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
	return do_pie(win, x0, y0, rx, ry, a, b, PAK_PIE, "w_pie");
}

short
w_dpie(WWIN *win, short x0, short y0, short rx, short ry, float a, float b)
{
	return do_pie(win, x0, y0, rx, ry, a, b, PAK_DPIE, "w_dpie");
}
