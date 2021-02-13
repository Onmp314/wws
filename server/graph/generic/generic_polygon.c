/*
 * generic_polygon.c, a part of the W Window System
 *
 * Copyright (C) 1996 Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines drawing polygons.
 *
 * NOTES
 * - See "Computer Graphics, 2nd edition" section 3.6 p.  92-99 for the
 *   basic idea.
 *
 * CHANGES
 * ++kay, 1/96:
 * - initial version. there is still code in the server/library missing to
 *   access it from clients.
 * ++kay, 2/96:
 * - some speedups.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <memory.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "generic.h"


typedef struct {
	long xmin, xmax;
} span_t;

typedef struct {
	long x, y;
} vertex_t;

/*
 * update the span-table for the edge ((x0,y0),(x1,y1))
 *
 * the information whether the edge is a left or a right one is needed
 * so we never draw a pixel whose center is outside the exact (analytic)
 * shape of the polygon.
 */
static void
process_edge (span_t *st, long x0, long y0, long x1, long y1,
	int is_left_edge)
{
	long x, y, num, den, inc;

	if (y0 == y1) {
		/*
		 * slope == 0
		 */
		if (is_left_edge) {
			if (x0 < st[y0].xmin)
				st[y0].xmin = x0;
			if (x1 < st[y0].xmin)
				st[y0].xmin = x1;
		} else {
			if (x0 > st[y0].xmax)
				st[y0].xmax = x0;
			if (x1 > st[y0].xmax)
				st[y0].xmax = x1;
		}
		return;
	}
	if (abs (x1 - x0) <= abs (y1 - y0)) {
		/*
		 * abs (slope) >= 1, iterate over `y'.
		 */
		if (x0 > x1) {
			long t;
			t = y1; y1 = y0; y0 = t;
			t = x1; x1 = x0; x0 = t;
		}
		x = x0;
		num = x1 - x0;
		den = y1 - y0;
		if (den >= 0) {
			/*
			 * slope >= 1
			 */
			if (is_left_edge) {
				inc = den;
				st = &st[y0];
				for (y = y0; y <= y1; ++y, ++st) {
					if (x < st->xmin)
						st->xmin = x;
					if ((inc += num) > den) {
						++x;
						inc -= den;
					}
				}
			} else {
				inc = 1;
				st = &st[y0];
				for (y = y0; y <= y1; ++y, ++st) {
					if (x > st->xmax)
						st->xmax = x;
					if ((inc += num) > den) {
						++x;
						inc -= den;
					}
				}
			}
		} else {
			/*
			 * slope <= -1
			 */
			den = -den;
			if (is_left_edge) {
				inc = den;
				st = &st[y0];
				for (y = y0; y >= y1; --y, --st) {
					if (x < st->xmin)
						st->xmin = x;
					if ((inc += num) > den) {
						++x;
						inc -= den;
					}
				}
			} else {
				inc = 1;
				st = &st[y0];
				for (y = y0; y >= y1; --y, --st) {
					if (x > st->xmax)
						st->xmax = x;
					if ((inc += num) > den) {
						++x;
						inc -= den;
					}
				}
			}
		}
	} else {
		/*
		 * abs (slope) < 1, iterate over `x'.
		 */
		if (y0 > y1) {
			long t;
			t = y1; y1 = y0; y0 = t;
			t = x1; x1 = x0; x0 = t;
		}
		y = y0;
		num = y1 - y0;
		den = x1 - x0;
		if (den >= 0) {
			/*
			 * 0 < slope < 1
			 */
			if (is_left_edge) {
				inc = den;
				st = &st[y];
				for (x = x0; x <= x1; ++x) {
					if (x < st->xmin)
						st->xmin = x;
					if ((inc += num) > den) {
						++y; ++st;
						inc -= den;
					}
				}
			} else {
				inc = 1;
				st = &st[y];
				for (x = x0; x <= x1; ++x) {
					if (x > st->xmax)
						st->xmax = x;
					if ((inc += num) > den) {
						++y; ++st;
						inc -= den;
					}
				}
			}
		} else {
			/*
			 * -1 < slope < 0
			 */
			den = -den;
			if (is_left_edge) {
				inc = den;
				st = &st[y];
				for (x = x0; x >= x1; --x) {
					if (x < st->xmin)
						st->xmin = x;
					if ((inc += num) > den) {
						++y; ++st;
						inc -= den;
					}
				}
			} else {
				inc = 1;
				st = &st[y];
				for (x = x0; x >= x1; --x) {
					if (x > st->xmax)
						st->xmax = x;
					if ((inc += num) > den) {
						++y; ++st;
						inc -= den;
					}
				}
			}
		}
	}
}

/*
 * the line (p1,p2) is an edge of the polygon, p3 is an arbitrary other vertex
 * != p1,p2 of the polygon. this function determines, whether (p1,p2) is a
 * "left" or "right" edge of the polygon.
 *
 * "left" edge means the whole polygon is on the right side of the edge, eg:
 *	        /
 *         vp2 +---+ vp3
 *            / P /
 *       vp1 +---+
 *          /
 * ------------------------> x axis
 *
 * "right" edge means the whole polygon is on the left side of the edge, eg:
 *	            /
 *             +---+ vp1
 *            / P /
 *       vp3 +---+ vp2
 *              /
 * ------------------------> x axis
 *
 * return values:
 *
 * <  0: (p1,p2) is a left edge
 * >  0: (p1,p2) is a right edge
 * == 0: p1, p2 and p3 are on one line, can't determine edge type
 */
static inline int
edge_type (vertex_t *p1, vertex_t *p2, vertex_t *p3)
{
	int i;

	if (p1->y == p2->y) {
		/*
		 * horizontal line
		 */
		i = p1->y - p3->y;
	} else if (p1->x == p2->x) {
		/*
		 * vertical line
		 */
		i = p1->x - p3->x;
	} else {
		/*
		 * (p1,p2) is a left edge iff:
		 *
		 *         p2->x - p1->x
		 * p1->x + -------------(p3->y - p1->y) < p3->x
		 *         p2->y - p1->y
		 */
		i = (p2->x - p1->x)*(p3->y - p1->y) -
		    (p3->x - p1->x)*(p2->y - p1->y);
		if (p2->y < p1->y)
			i = -i;
	}
	return i;
}

/*
 * given an array of vertices of the polygon make a span table, which is
 * an array with one horizontal line for every scanline the polygon covers
 * on the screen.
 */
static span_t *
make_span_table (long nv, vertex_t *v, int *ymin_ret, int *ymax_ret)
{
	vertex_t *vp1, *vp2, *vp3;
	span_t *st;
	long i, ymin, ymax;

	if (nv < 2)
		return NULL;

	/*
	 * determine ymin and ymax of the polygon
	 */
	ymin = LONG_MAX;
	ymax = LONG_MIN;
	for (i = 0; i < nv; ++i) {
		if (v[i].y < ymin)
			ymin = v[i].y;
		if (v[i].y > ymax)
			ymax = v[i].y;
	}

	/*
	 * allocate and initialize a span-table for the scanlines
	 * ymin, ..., ymax.
	 */
	st = malloc (sizeof (span_t) * (ymax - ymin + 1));
	if (!st)
		return NULL;
	for (i = ymax - ymin + 1; --i >= 0; ) {
		st[i].xmin = LONG_MAX;
		st[i].xmax = LONG_MIN;
	}

	/*
	 * build the span-table
	 */
	for (vp1 = v, vp2 = &v[nv-1]; vp2 >= v; vp1 = vp2, --vp2) {
		vp3 = vp2 - 1;
		if (vp3 < v)
			vp3 += nv;
		for (i = 0; vp3 != vp1; ) {
			i = edge_type (vp1, vp2, vp3);
			if (i != 0)
				break;
			/*
			 * vp3 is on the line (vp1,vp2).
			 */
			if (--vp3 < v)
				vp3 += nv;
		}
		if (0 == i &&
		    0 == (i = vp2->x - vp1->x) &&
		    0 == (i = vp2->y - vp1->y)) {
			/*
			 * vp1 and vp2 are the same. make sure this
			 * zero edge is processed once as a left edge
			 * and once as a right edge.
			 */
			i = vp2 - vp1;
		}
		process_edge (st - ymin, vp1->x, vp1->y, vp2->x, vp2->y, i<0);
	}
	*ymin_ret = ymin;
	*ymax_ret = ymax;
	return st;
}

/*
 * draw a filled polygon.
 */
void generic_ppoly (BITMAP *bm, long nv, long *__v)
{
	int i, ymax, ymin;
	span_t *st;
	vertex_t *v = (vertex_t *)__v;

	/*
	 * make the span table
	 */
	st = make_span_table (nv, v, &ymin, &ymax);
	if (!st)
		return;
	/*
	 * and output all the scanlines
	 */
	for (i = ymax - ymin + 1; --i >= 0; ) {
		if (st[i].xmin > st[i].xmax)
			/*
			 * strictly speaking this should be "continue".
			 * but then small "slivers" will have missing
			 * pixels.
			 */
			st[i].xmax = st[i].xmin;
	    (*theScreen->hline)(bm, st[i].xmin, ymin+i, st[i].xmax);
	}
	free (st);
}

void generic_dppoly (BITMAP *bm, long nv, long *__v)
{
	int i, ymax, ymin;
	span_t *st;
	vertex_t *v = (vertex_t *)__v;

	/*
	 * make the span table
	 */
	st = make_span_table (nv, v, &ymin, &ymax);
	if (!st)
		return;
	/*
	 * and output all the scanlines
	 */
	for (i = ymax - ymin + 1; --i >= 0; ) {
		if (st[i].xmin > st[i].xmax)
			/*
			 * strictly speaking this should be "continue".
			 * but then small "slivers" will have missing
			 * pixels.
			 */
			st[i].xmax = st[i].xmin;
	    (*theScreen->dhline)(bm, st[i].xmin, ymin+i, st[i].xmax);
	}
	free (st);
}

/*
 * draw a polygon. the algorithm is wrong, some pixel's are drawn twice
 * or more (matters for M_INVERS).
 */
void generic_poly (BITMAP *bm, long nv, long *__v)
{
	vertex_t *vp1, *vp2;
	vertex_t *v = (vertex_t *)__v;

	if (nv < 2)
		return;

	FirstPoint = 0;
	for (vp1 = v, vp2 = &v[nv-1]; vp2 >= v; vp1 = vp2, --vp2)
		(*theScreen->line)(bm, vp1->x, vp1->y, vp2->x, vp2->y);
	FirstPoint = 1;
}

void generic_dpoly (BITMAP *bm, long nv, long *__v)
{
	vertex_t *vp1, *vp2;
	vertex_t *v = (vertex_t *)__v;

	if (nv < 2)
		return;

	FirstPoint = 0;
	for (vp1 = v, vp2 = &v[nv-1]; vp2 >= v; vp1 = vp2, --vp2)
	  (*theScreen->dline)(bm, vp1->x, vp1->y, vp2->x, vp2->y);
	FirstPoint = 1;
}
