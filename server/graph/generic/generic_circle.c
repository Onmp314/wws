/*
 * generic_circle.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines dealing with circles, ellipses, arcs and pies
 *
 * CHANGES
 * ++kay, 1/96:
 * - added patterned functions (untested).
 * - added comments on how to speed things up.
 * ++eero, 8/96:
 * - changed pattern functions to use solid ones.  One more function
 *   call shouldn't slow down that much?
 * - added ellipse functions.
 * ++eero, 1/98:
 * - added thick ellipses and circles (uses ellipse code), arc and pie
 *   functions.
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "generic.h"

static void (*PlotFn)(BITMAP *, long, long);
static void (*HlineFn)(BITMAP *, long, long, long);
static void (*VlineFn)(BITMAP *, long, long, long);
static void (*BoxFn)(BITMAP *, long, long, long, long);
static PIZZASLICE *Slice;	/* for arcs and pies */

/*
 *
 */

static void circle (bm, x0, y0, r)
	BITMAP *bm;
	register long x0;
	register long y0;
	long r;
{
	register void (*plot_fn) (BITMAP *, long, long);
	register long x, y, da;
	REC *oclip = glob_clip0;

	plot_fn = PlotFn;

	if (!r) {
		(*plot_fn)(bm, x0, y0);
		return;
	}

	if (!BOX_NEEDS_CLIPPING (x0-r-1, y0-r-1, r+r+2, r+r+2, glob_clip0)) {
		glob_clip0 = NULL;
	}
	if (gc0->drawmode == M_INVERS) {
		(*plot_fn)(bm, x0-r, y0);
		(*plot_fn)(bm, x0+r, y0);
		(*plot_fn)(bm, x0, y0-r);
		(*plot_fn)(bm, x0, y0+r);
	}

	x = 0;
	y = r;
	da = r - 1;

	do {
		if (da < 0) {
			y--;
			da += y + y;
		}

		(*plot_fn)(bm, x0+x, y0+y);
		(*plot_fn)(bm, x0-x, y0+y);
		(*plot_fn)(bm, x0+x, y0-y);
		(*plot_fn)(bm, x0-x, y0-y);
		(*plot_fn)(bm, x0+y, y0+x);
		(*plot_fn)(bm, x0-y, y0+x);
		(*plot_fn)(bm, x0+y, y0-x);
		(*plot_fn)(bm, x0-y, y0-x);

		da = da - x - x - 1;
		x++;

	} while (x < y);

	if ((gc0->drawmode == M_INVERS) && (x != y)) {
		(*plot_fn)(bm, x0-y, y0-y);
		(*plot_fn)(bm, x0+y, y0-y);
		(*plot_fn)(bm, x0+y, y0+y);
		(*plot_fn)(bm, x0-y, y0+y);
	}
	glob_clip0 = oclip;
}

static void pcircle (bm, x0, y0, r)
	BITMAP *bm;
	register long x0;
	register long y0;
	long r;
{
	register void (*hline_fn) (BITMAP *, long, long, long);
	register long dx, dy, rq = r * r;
	REC *oclip = glob_clip0;

	hline_fn = HlineFn;

	if (!r) {
		(*PlotFn)(bm, x0, y0);
		return;
	}

	if (!BOX_NEEDS_CLIPPING (x0-r-1, y0-r-1, r+r+2, r+r+2, glob_clip0)) {
		glob_clip0 = NULL;
	}
	(*hline_fn)(bm, x0-r, y0, x0+r);

	dy = 0;
	dx = r;
	while (dx) {

		dy++;
		while (dx*dx + dy*dy > rq) {
			dx--;
		}
		(*hline_fn)(bm, x0-dx, y0-dy, x0+dx);
		(*hline_fn)(bm, x0-dx, y0+dy, x0+dx);
	}
	glob_clip0 = oclip;
}

void generic_circ (bm, x0, y0, r)
	BITMAP *bm;
	register long x0;
	register long y0;
	register long r;
{
	if (gc0->linewidth > 1) {
		generic_ellipse (bm, x0, y0, r, r);
	} else {
		PlotFn = theScreen->plot;
		circle (bm, x0, y0, r);
	}
}


void generic_pcirc (bm, x0, y0, r)
	BITMAP *bm;
	register long x0;
	register long y0;
	long r;
{
	PlotFn = theScreen->plot;
	HlineFn = theScreen->hline;
	pcircle (bm, x0, y0, r);
}

void generic_dcirc (bm, x0, y0, r)
	BITMAP *bm;
	register long x0;
	register long y0;
	register long r;
{
	if (gc0->linewidth > 1) {
		generic_dellipse (bm, x0, y0, r, r);
	} else {
		PlotFn = theScreen->plot;
		circle (bm, x0, y0, r);
	}
}


void generic_dpcirc (bm, x0, y0, r)
	BITMAP *bm;
	register long x0;
	register long y0;
	long r;
{
	PlotFn = theScreen->dplot;
	HlineFn = theScreen->dhline;
	pcircle (bm, x0, y0, r);
}

/* 
 * ellipses
 */

static void ellipse (bm, x0, y0, rx, ry)
	BITMAP *bm;
	register long x0;
	register long y0;
	long rx;
	long ry;
{
	register void (*plot_fn) (BITMAP *, long, long);
	register long dx, dy, dd, a, b, tmp;
	REC *oclip = glob_clip0;

	plot_fn = PlotFn;

	if (!rx) {
		(*VlineFn) (bm, x0, y0-ry, y0+ry);
		return;
	}
	if (!ry) {
		(*HlineFn) (bm, x0-rx, y0, x0+ry);
		return;
	}

	if (!BOX_NEEDS_CLIPPING (x0-rx-1, y0-ry-1, rx+rx+2, ry+ry+2, glob_clip0)) {
		glob_clip0 = NULL;
	}

	/* this is fastest when ry > rx */
	(*plot_fn) (bm, x0-rx, y0);
	(*plot_fn) (bm, x0+rx, y0);

	dy = 0;
	dx = rx;
	a = rx*rx;
	b = ry*ry;
	dd = a*b;
	for (;;) {

		dy++;
		tmp = dy*dy*a - dd;
		while (dx*dx*b + tmp > 0) {
			(*plot_fn) (bm, x0-dx, y0-dy);
			(*plot_fn) (bm, x0+dx, y0-dy);
			(*plot_fn) (bm, x0-dx, y0+dy);
			(*plot_fn) (bm, x0+dx, y0+dy);
			dx--;
		}
		if(!dx) {
			(*VlineFn) (bm, x0, y0-dy, y0-ry);
			(*VlineFn) (bm, x0, y0+dy, y0+ry);
			glob_clip0 = oclip;
			return;
		}
		(*plot_fn) (bm, x0-dx, y0-dy);
		(*plot_fn) (bm, x0+dx, y0-dy);
		(*plot_fn) (bm, x0-dx, y0+dy);
		(*plot_fn) (bm, x0+dx, y0+dy);
	}
}

static void pellipse (bm, x0, y0, rx, ry)
	BITMAP *bm;
	register long x0;
	register long y0;
	long rx;
	long ry;
{
	register void (*hline_fn) (BITMAP *, long, long, long);
	register long dx, dy, dd, a, b, tmp;
	REC *oclip = glob_clip0;

	hline_fn = HlineFn;

	if (!rx) {
		(*VlineFn) (bm, x0, y0-ry, y0+ry);
		return;
	}
	if (!ry) {
		(*hline_fn) (bm, x0-rx, y0, x0+rx);
		return;
	}

	if (!BOX_NEEDS_CLIPPING (x0-rx-1, y0-ry-1, rx+rx+2, ry+ry+2, glob_clip0)) {
		glob_clip0 = NULL;
	}
	(*hline_fn) (bm, x0-rx, y0, x0+rx);

	dy = 0;
	dx = rx;
	a = rx*rx;
	b = ry*ry;
	dd = a*b;
	while (dx) {

		dy++;
		tmp = dy*dy*a - dd;
		while (dx*dx*b + tmp > 0) {
			dx--;
		}
		(*hline_fn) (bm, x0-dx, y0-dy, x0+dx);
		(*hline_fn) (bm, x0-dx, y0+dy, x0+dx);
	}
	if (dy < ry) {
		dy++;
		/* ellipses that are only a couple of pixels wide... */
		(*VlineFn) (bm, x0, y0-dy, y0-ry);
		(*VlineFn) (bm, x0, y0+dy, y0+ry);
	}
	glob_clip0 = oclip;
}

static void tellipse (bm, x0, y0, rx, ry)
	BITMAP *bm;
	register long x0;
	register long y0;
	long rx;
	long ry;
{
	/* variables for inner and outer ellipse borders */
	long dxi, dyi, ddi, ai, bi, dxo, dyo, ddo, ao, bo;

	register void (*hline_fn) (BITMAP *, long, long, long);
	short th = gc0->linewidth >> 1;
	REC *oclip = glob_clip0;

	dxo = rx + th;
	dyo = ry + th;

	if (!dxo || !dyo) {
		return;
	}

	if (!BOX_NEEDS_CLIPPING (x0-dxo-1, y0-dyo-1,
	    dxo+dxo+2, dyo+dyo+2, glob_clip0)) {
		glob_clip0 = NULL;
	}
	hline_fn = HlineFn;

	dxi = rx - th;
	dyi = ry - th;
	ai = dxi * dxi;
	bi = dyi * dyi;
	ddi = ai * bi;

	ao = dxo * dxo;
	bo = dyo * dyo;
	ddo = ao * bo;

	if (dxi > 0 && dyi > 0) {

		(*hline_fn) (bm, x0-dxo, y0, x0-dxi);
		(*hline_fn) (bm, x0+dxi, y0, x0+dxo);

		dyi = 0;
		while (dxi > 0) {

			dyi++;

			dyo = dyi*dyi*ai - ddi;
			while (dyo + dxi*dxi*bi > 0) {
				dxi--;
			}
			(*hline_fn) (bm, x0-dxo, y0-dyi, x0-dxi);
			(*hline_fn) (bm, x0+dxi, y0-dyi, x0+dxo);
			(*hline_fn) (bm, x0-dxo, y0+dyi, x0-dxi);
			(*hline_fn) (bm, x0+dxi, y0+dyi, x0+dxo);

			dyo = dyi*dyi*ao - ddo;
			while (dyo + dxo*dxo*bo > 0) {
				dxo--;
			}
		}
	} else {
		(*hline_fn) (bm, x0-dxo, y0, x0+dxo);
		dyi = 0;
	}
	while (dxo > 0) {

		dyi++;
		dyo = dyi*dyi*ao - ddo;
		while (dyo + dxo*dxo*bo > 0) {
			dxo--;
		}
		(*hline_fn) (bm, x0-dxo, y0-dyi, x0+dxo);
		(*hline_fn) (bm, x0-dxo, y0+dyi, x0+dxo);
	}
	glob_clip0 = oclip;
}

void generic_ellipse (bm, x0, y0, rx, ry)
	BITMAP *bm;
	register long x0;
	register long y0;
	long rx;
	long ry;
{
	HlineFn = theScreen->hline;
	if (gc0->linewidth > 1) {
		BoxFn = theScreen->box;
		tellipse (bm, x0, y0, rx, ry);
	} else {
		PlotFn = theScreen->plot;
		VlineFn = theScreen->vline;
		ellipse (bm, x0, y0, rx, ry);
	}
}

void generic_pellipse (bm, x0, y0, rx, ry)
	BITMAP *bm;
	register long x0;
	register long y0;
	long rx;
	long ry;
{
	HlineFn = theScreen->hline;
	VlineFn = theScreen->vline;
	pellipse (bm, x0, y0, rx, ry);
}

void generic_dellipse (bm, x0, y0, rx, ry)
	BITMAP *bm;
	register long x0;
	register long y0;
	long rx;
	long ry;
{
	HlineFn = theScreen->dhline;
	if (gc0->linewidth > 1) {
		BoxFn = theScreen->dbox;
		tellipse (bm, x0, y0, rx, ry);
	} else {
		PlotFn = theScreen->dplot;
		VlineFn = theScreen->dvline;
		ellipse (bm, x0, y0, rx, ry);
	}
}

void generic_dpellipse (bm, x0, y0, rx, ry)
	BITMAP *bm;
	register long x0;
	register long y0;
	long rx;
	long ry;
{
	HlineFn = theScreen->dhline;
	VlineFn = theScreen->dvline;
	pellipse (bm, x0, y0, rx, ry);
}

/* 
 * arcs and pies
 */

/* returns 0 if line is clipped or on acceptable side, 1 if it's vertically
 * on other side, otherwise 3.
 */
static inline int clip_line(dx, dy, dir, y, x0, x1)
	long dx;
	long dy;
	long dir;
	long y;
	long *x0;
	long *x1;
{
	/* hline on the same vertical side with the given edge? */
	if ((y > 0 && dy > 0) || (y < 0 && dy < 0)) {

		long x = dx * y / dy;

		if (x >= *x0 && x <= *x1) {
			if (dir > 0) {
				*x0 = x;
			} else {
				*x1 = x;
			}
			return 0;
		} else {
			if (dir > 0) {
				if (x <= *x0) {
					return 0;
				}
			} else {
				if (x >= *x1) {
					return 0;
				}
			}
		}
		return 3;
	}
	return 1;
}

/* relative offsets, direction from left to right. */
static void draw_line(bm, x0, y, x1)
	BITMAP *bm;
	long x0;
	long y;
	long x1;
{
	int discard, ret, dbl = (Slice->adir > 0 && Slice->bdir < 0);
	long x2 = x0, x3 = x1;

	if (!y) {
		/* edges on different sides */
		if ((Slice->ay < 0 && Slice->by > 0) ||
		    (Slice->ay > 0 && Slice->by < 0)) {
			if (Slice->adir < 0) {
				if (x1 > 0) {
					x1 = 0;
				}
			} else {
				if (x0 < 0) {
					x0 = 0;
				}
			}
		} else {
			if (!dbl) {
				(*PlotFn)(bm, Slice->x0, Slice->y0);
				return;
			}
		}
		(*HlineFn) (bm, Slice->x0 + x0, Slice->y0, Slice->x0 + x1);
		return;
	}

	/* clip left edge / line */
	ret = clip_line(Slice->ax, Slice->ay, Slice->adir, y, &x0, &x1);

	if (dbl) {
		if (!ret) {
			/* edges separate line to two parts */
			(*HlineFn) (bm, Slice->x0 + x0, Slice->y0 + y, Slice->x0 + x1);
			x0 = x2;
			x1 = x3;
		}
	} else {
		if (ret > 1) {
			return;
		}
	}

	discard = ret;
	ret = clip_line(Slice->bx, Slice->by, Slice->bdir, y, &x0, &x1);

	discard += ret;
	if (discard > 2 && !(dbl && ret == 0 && discard == 3)) {
		return;
	}
	if (discard == 2) {
		/* line on other side than slice */
		if (Slice->adir < 0 || Slice->bdir > 0) {
			return;
		}
	}
	(*HlineFn) (bm, Slice->x0 + x0, Slice->y0 + y, Slice->x0 + x1);
}


static void pie(bm)
	BITMAP *bm;
{
	long tmp, dx, dy, dd, a, b;
	REC *oclip = glob_clip0;

	dx = Slice->rx;
	dy = Slice->ry;

	if (dx <= 0 || dy <= 0) {
		/* ellipse draws a line in this case */
		return;
	}

	if (!BOX_NEEDS_CLIPPING (Slice->x0-dx-1, Slice->y0-dy-1,
	    dx+dx+2, dy+dy+2, glob_clip0)) {
		glob_clip0 = NULL;
	}
	draw_line (bm, -dx, 0, +dx);

	a = dx * dx;
	b = dy * dy;
	dd = a * b;
	dy = 0;

	while (dx > 0) {

		dy++;
		tmp = dy*dy*a - dd;
		while (tmp + dx*dx*b > 0) {
			dx--;
		}
		draw_line (bm, -dx, -dy, +dx);
		draw_line (bm, -dx, +dy, +dx);
	}
	glob_clip0 = oclip;
}

static void arc(bm)
	BITMAP *bm;
{
	/* variables for inner and outer ellipse borders */
	long dxi, dyi, ddi, ai, bi, dxo, dyo, ddo, ao, bo;
	short th = gc0->linewidth >> 1;
	REC *oclip = glob_clip0;

	if (th < 0) {
		return;
	}

	dxo = Slice->rx + th;
	dyo = Slice->ry + th;

	if (dxo <= 0 || dyo <= 0) {
		return;
	}

	if (!BOX_NEEDS_CLIPPING (Slice->x0-dxo-1, Slice->y0-dyo-1,
	    dxo+dxo+2, dyo+dyo+2, glob_clip0)) {
		glob_clip0 = NULL;
	}
	
	dxi = Slice->rx - th;
	dyi = Slice->ry - th;
	ai = dxi * dxi;
	bi = dyi * dyi;
	ddi = ai * bi;

	ao = dxo * dxo;
	bo = dyo * dyo;
	ddo = ao * bo;

	if (dxi > 0 && dyi > 0) {

		draw_line (bm, -dxo, 0, -dxi);
		draw_line (bm, +dxi, 0, +dxo);

		dyi = 0;
		while (dxi > 0) {

			dyi++;

			dyo = dyi*dyi*ai - ddi;
			while (dyo + dxi*dxi*bi > 0) {
				dxi--;
			}

			draw_line (bm, -dxo, -dyi, -dxi);
			draw_line (bm, +dxi, -dyi, +dxo);
			draw_line (bm, -dxo, +dyi, -dxi);
			draw_line (bm, +dxi, +dyi, +dxo);

			dyo = dyi*dyi*ao - ddo;
			while (dyo + dxo*dxo*bo > 0) {
				dxo--;
			}
		}
	} else {
		draw_line (bm, -dxo, 0, +dxo);
		dyi = 0;
	}
	while (dxo > 0) {

		dyi++;
		dyo = dyi*dyi*ao - ddo;
		while (dyo + dxo*dxo*bo > 0) {
			dxo--;
		}
		draw_line (bm, -dxo, -dyi, +dxo);
		draw_line (bm, -dxo, +dyi, +dxo);
	}
	glob_clip0 = oclip;
}


void generic_arc (bm, slice)
	BITMAP *bm;
	PIZZASLICE *slice;
{
	BoxFn = theScreen->box;
	PlotFn = theScreen->plot;
	HlineFn = theScreen->hline;
	Slice = slice;
	arc (bm);
}

void generic_pie (bm, slice)
	BITMAP *bm;
	PIZZASLICE *slice;
{
	PlotFn = theScreen->plot;
	HlineFn = theScreen->hline;
	Slice = slice;
	pie (bm);
}

void generic_darc (bm, slice)
	BITMAP *bm;
	PIZZASLICE *slice;
{
	BoxFn = theScreen->dbox;
	PlotFn = theScreen->dplot;
	HlineFn = theScreen->dhline;
	Slice = slice;
	arc (bm);
}

void generic_dpie (bm, slice)
	BITMAP *bm;
	PIZZASLICE *slice;
{
	PlotFn = theScreen->dplot;
	HlineFn = theScreen->dhline;
	Slice = slice;
	pie (bm);
}
