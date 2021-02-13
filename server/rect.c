/*
 * rect.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- generalized rectangle handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "config.h"
#include "types.h"
#include "rect.h"


inline REC *rect_create (int x, int y, int w, int h)
{
	REC *r = malloc (sizeof (REC));
	if (!r)
		return NULL;
	r->x0 = x;
	r->y0 = y;
	r->x1 = (r->w = w) - 1;
	r->y1 = (r->h = h) - 1;
	r->next = NULL;
	return r;
}

inline void rect_destroy (REC *r)
{
	free (r);
}

inline void rect_list_destroy (REC *r)
{
	while (r) {
		REC *freeme = r;
		r = r->next;
		free (freeme);
	}
}

/*
 * rect_compare (A, B) returns X??|Y??.
 */
#define XLL (5 << 3) /* Bx1 <  Bx2 <  Ax1               <  Ax2              */
#define XLM (4 << 3) /*        Bx1 <= Ax1 <= Bx2        <  Ax2              */
#define XLR (3 << 3) /*        Bx1 <= Ax1               <  Ax2 <= Bx2       */
#define XMM (2 << 3) /*               Ax1 <  Bx1 <  Bx2 <  Ax2              */
#define XMR (1 << 3) /*               Ax1 <  Bx1        <= Ax2 <= Bx2       */
#define XRR (0 << 3) /*               Ax1               <  Ax2 <  Bx1 < Bx2 */
#define YOO (5 << 0) /* By1 <  By2 <  Ay1               <  Ay2              */
#define YOM (4 << 0) /*        By1 <= Ay1 <= By2        <  Ay2              */
#define YOU (3 << 0) /*        By1 <= Ay1               <  Ay2 <= By2       */
#define YMM (2 << 0) /*               Ay1 <  By1 <  By2 <  Ay2              */
#define YMU (1 << 0) /*               Ay1 <  By1        <= Ay2 <= By2       */
#define YUU (0 << 0) /*               Ay1               <  Ay2 <  By1 < By2 */

static int
rect_compare (REC *A, REC *B)
{
	int flags;

	if (XL (B) <= XL (A)) {
		if (XR (B) - 1 < XL (A)) {
			flags = XLL;
		} else if (XR (B) - 1 < XR (A) - 1) {
			flags = XLM;
		} else {
			flags = XLR;
		}
	} else if (XL (B) <= XR (A) - 1) {
		if (XR (B) - 1 < XR (A) - 1) {
			flags = XMM;
		} else {
			flags = XMR;
		}
	} else {
		flags = XRR;
	}

	if (YO (B) <= YO (A)) {
		if (YU (B) - 1 < YO (A)) {
			flags |= YOO;
		} else if (YU (B) - 1 < YU (A) - 1) {
			flags |= YOM;
		} else {
			flags |= YOU;
		}
	} else if (YO (B) <= YU (A) - 1) {
		if (YU (B) - 1 < YU (A) - 1) {
			flags |= YMM;
		} else {
			flags |= YMU;
		}
	} else {
		flags |= YUU;
	}
	return flags;
}

static inline int rect_add (REC **rp, int x, int y, int w, int h)
{
	REC *r;

	if (!(r = rect_create (x, y, w, h)))
		return -1;
	r->next = *rp;
	*rp = r;
	return 0;
}

/*
 * subtracts rectangle B from rectangle A. The resulting
 * rectangles are prepended to the list R.
 */
REC *
rect_subtract (REC *A, REC *B, REC *R, int *cut)
{
	REC *rp = R;
	int flags, tmpcut;

	flags = rect_compare (A, B);
	/*
	 * gcc generates a nice jump table for the following switch...
	 *
	 * the dotted lines in the drawings indicated how `A' is splitted
	 * into smaller rectangles.
	 */
	switch (flags) {
	case XMR|YMU:
		/*
		 * +---+
		 * | A |
		 * |.+-+-+
		 * | | | |
		 * +-+-+ |
		 *   | B |
		 *   +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      A->w,
		      YO (B) - YO (A)) ||
		    rect_add (&rp, XL (A), YO (B),
		      XL (B) - XL (A), 
		      YU (A) - YO (B)))
			goto bad;
		break;

	case XMR|YOM:
		/*
		 *   +---+
		 *   | B |
		 * +-+-+ |
		 * | | | |
		 * |.+-+-+
		 * | A |
		 * +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      XL (B) - XL (A),
		      YU (B) - YO(A)) ||
		    rect_add (&rp, XL (A), YU (B),
		      A->w,
		      YU (A) - YU (B)))
			goto bad;
		break;

	case XLM|YOM:
		/*
		 * +---+
		 * | B |
		 * | +-+-+
		 * | | | |
		 * +-+-+.|
		 *   | A |
		 *   +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XR (B), YO (A),
		      XR (A) - XR (B),
		      YU (B) - YO (A)) ||
		    rect_add (&rp, XL (A), YU (B),
		      A->w,
		      YU (A) - YU (B)))
			goto bad;
		break;

	case XLM|YMU:
		/*
		 *   +---+
		 *   | A |
		 * +-+-+.|
		 * | | | |
		 * | +-+-+
		 * | B |
		 * +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      A->w,
		      YO (B) - YO (A)) ||
		    rect_add (&rp, XR (B), YO (B),
		      XR (A) - XR (B),
		      YU (A) - YO (B)))
			goto bad;
		break;

	case XMR|YMM:
		/*
		 * +---+
		 * |.+-+---+
		 * |A| | B |
		 * |.+-+---+
		 * +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      A->w,
		      YO (B) - YO (A)) ||
		    rect_add (&rp, XL (A), YO (B),
		      XL (B) - XL (A),
		      B->h) ||
		    rect_add (&rp, XL (A), YU (B),
		      A->w,
		      YU (A) - YU (B)))
			goto bad;
		break;

	case XMM|YOM:
		/*
		 *   +---+
		 *   | B |
		 * +-+---+-+
		 * | +---+ |
		 * | : A : |
		 * +-------+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      XL (B) - XL (A),
		      A->h) ||
		    rect_add (&rp, XL (B), YU (B),
		      B->w,
		      YU (A) - YU (B)) ||
		    rect_add (&rp, XR (B), YO (A),
		      XR (A) - XR (B),
		      A->h))
			goto bad;
		break;

	case XLM|YMM:
		/*
		 *     +---+
		 * +---+-+.|
		 * | B | |A|
		 * +---+-+.|
		 *     +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      A->w,
		      YO (B) - YO (A)) ||
		    rect_add (&rp, XR (B), YO (B),
		      XR (A) - XR (B),
		      B->h) ||
		    rect_add (&rp, XL (A), YU (B),
		      A->w,
		      YU (A) - YU (B)))
			goto bad;
		break;

	case XMM|YMU:
		/*
		 * +-------+
		 * | : A : |
		 * | +---+ |
		 * +-+---+-+
		 *   | B |
		 *   +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      XL (B) - XL (A),
		      A->h) ||
		    rect_add (&rp, XL (B), YO (A),
		      B->w,
		      YO (B) - YO (A)) ||
		    rect_add (&rp, XR (B), YO (A),
		      XR (A) - XR (B),
		      A->h))
			goto bad;
		break;

	case XMR|YOU:
		/*
		 *     +---+
		 * +---+-+ |
		 * | A | |B|
		 * +---+-+ |
		 *     +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      XL (B) - XL (A),
		      A->h))
			goto bad;
		break;

	case XLR|YOM:
		/*
		 * +-------+
		 * |   B   |
		 * | +---+ |
		 * +-+---+-+
		 *   | A |
		 *   +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YU (B),
		      A->w,
		      YU (A) - YU (B)))
			goto bad;
		break;

	case XLM|YOU:
		/*
		 * +---+
		 * | +-+---+
		 * |B| | A |
		 * | +-+---+
		 * +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XR (B), YO (A),
		      XR (A) - XR (B),
		      A->h))
			goto bad;
		break;

	case XLR|YMU:
		/*
		 *   +---+
		 *   | A |
		 * +-+---+-+
		 * | +---+ |
		 * |   B   |
		 * +-------+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      A->w,
		      YO (B) - YO (A)))
			goto bad;
		break;

	case XLR|YMM:
		/*
		 *   +---+
		 * +-+---+---+
		 * | | A | B |
		 * +-+---+---+
		 *   +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      A->w,
		      YO (B) - YO (A)) ||
		    rect_add (&rp, XL (A), YU (B),
		      A->w,
		      YU (A) - YU (B)))
			goto bad;
		break;

	case XMM|YOU:
		/*
		 *   +---+
		 * +-+---+---+
		 * | | B | A |
		 * +-+---+---+
		 *   +---+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      XL (B) - XL (A),
		      A->h) ||
		    rect_add (&rp, XR (B), YO (A),
		      XR (A) - XR (B),
		      A->h))
			goto bad;
		break;

	case XLR|YOU:
		/*
		 * +---------+
		 * | +---+   |
		 * | | A | B |
		 * | +---+   |
		 * +---------+
		 */
		tmpcut = 1;
		break;

	case XMM|YMM:
		/*
		 * +---------+
		 * |.+---+...|
		 * | | B | A |
		 * |.+---+...|
		 * +---------+
		 */
		tmpcut = 1;
		if (rect_add (&rp, XL (A), YO (A),
		      A->w,
		      YO (B) - YO (A)) ||
		    rect_add (&rp, XL (A), YO (B),
		      XL (B) - XL (A),
		      B->h) ||
		    rect_add (&rp, XR (B), YO (B),
		      XR (A) - XR (B),
		      B->h) ||
		    rect_add (&rp, XL (A), YU (B),
		      A->w,
		      YU (A) - YU (B)))
			goto bad;
		break;

	default:
		/*
		 * non-overlapping
		 */
		tmpcut = 0;
		if (rect_add (&rp, XL (A), YO (A), A->w, A->h))
			goto bad;
		break;
	}
	if (cut)
		*cut = tmpcut;
	return rp;

bad:
	while (rp != R) {
		REC *freeme = rp;
		rp = rp->next;
		rect_destroy (freeme);
	}
	return RECT_ERROR;
}

/*
 * Subtract rectangle B from every rectangle in list A.
 * Returns a list of the resulting rectangles or RECT_ERROR if something
 * goes wrong.
 */
REC *
rect_list_subtract (REC *A, REC *B, int dofree, int *cut)
{
	REC *freeme, *nR, *R = NULL;
	int tmpcut1, tmpcut2 = 0;

	while (A) {
		nR = rect_subtract (A, B, R, &tmpcut1);
		freeme = A;
		A = A->next;
		if (dofree)
			rect_destroy (freeme);	
		if (nR == RECT_ERROR) {
			rect_list_destroy (R);
			return RECT_ERROR;
		}
		tmpcut2 |= tmpcut1;
		R = nR;
	}
	if (cut)
		*cut = tmpcut2;
	return R;
}

/*
 * clip rectangle A to the interior of rectangle B.
 */
REC *
rect_clip (REC *A, REC *B, REC *R, int *cut)
{
	int x1, y1, x2, y2;
	REC *rp;

	x1 = MAX (XL (A), XL (B));
	y1 = MAX (YO (A), YO (B));
	x2 = MIN (XR (A), XR (B));
	y2 = MIN (YU (A), YU (B));

	if (x1 >= x2 || y1 >= y2) {
		if (cut)
			*cut = 1;
		return R;
	}
	if (!(rp = rect_create (x1, y1, x2-x1, y2-y1)))
		return RECT_ERROR;

	if (cut)
		*cut = (A->x0 != rp->x0 || A->y0 != rp->y0 ||
			A->w != rp->w || A->h != rp->h);

	rp->next = R;
	return rp;
}

/*
 * clip every rectanlge in list A to the interior of rectangle B.
 */
REC *
rect_list_clip (REC *A, REC *B, int dofree, int *cut)
{
	REC *freeme, *nR, *R = NULL;
	int tmpcut1, tmpcut2 = 0;

	while (A) {
		nR = rect_clip (A, B, R, &tmpcut1);
		freeme = A;
		A = A->next;
		if (dofree)
			rect_destroy (freeme);	
		if (nR == RECT_ERROR) {
			rect_list_destroy (R);
			return RECT_ERROR;
		}
		tmpcut2 |= tmpcut1;
		R = nR;
	}
	if (cut)
		*cut = tmpcut2;
	return R;
}

/*
 * return nonzero if rectangles A and B intersect. If R != NULL return
 * the intersection in R.
 */
int
rect_intersect (REC *A, REC *B, REC *R)
{
	int x1, x2, y1, y2;

	x1 = MAX (XL (A), XL (B));
	y1 = MAX (YO (A), YO (B));
	x2 = MIN (XR (A), XR (B));
	y2 = MIN (YU (A), YU (B));

	if (x1 >= x2 || y1 >= y2)
		return 0;

	if (R) {
		R->x0 = x1;
		R->y0 = y1;
		R->w = x2-x1;
		R->h = y2-y1;
		R->x1 = R->x0 + R->w - 1;
		R->y1 = R->y0 + R->h - 1;
	}
	return 1;
}

void
rect_union (REC *A, REC *B, REC *R)
{
	int x1, x2, y1, y2;

	x1 = MIN (XL (A), XL (B));
	y1 = MIN (YO (A), YO (B));
	x2 = MAX (XR (A), XR (B));
	y2 = MAX (YU (A), YU (B));

	R->x0 = x1;
	R->y0 = y1;
	R->w = x2-x1;
	R->h = y2-y1;
	R->x1 = R->w - 1;
	R->y1 = R->h - 1;
}

void
rectUpdateDirty (REC *R, int x, int y, int w, int h)
{
  int x1, x2, y1, y2;

  if (R->w) {
    x1 = MIN (XL (R), x);
    y1 = MIN (YO (R), y);
    x2 = MAX (XR (R), x + w);
    y2 = MAX (YU (R), y + h);
    R->x0 = x1;
    R->y0 = y1;
    R->w = x2-x1;
    R->h = y2-y1;
  } else {
    R->x0 = x;
    R->y0 = y;
    R->w = w;
    R->h = h;
  }
  R->x1 = R->w - 1;
  R->y1 = R->h - 1;
}
