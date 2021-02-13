/*
 * clip.h, a part of the W Window System
 *
 * Copyright (C) 1996 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- code for clipping lines, vlines, hlines, points and boxes and bitblits
 *
 * almost all of them are macros instead of inline functions to avoid taking
 * the address of the operands (which totally messes up the code gcc
 * generates for the function that "call" the inline). most of the arguments
 * are evaluated more the once, so be careful!
 *
 * Yes, this looks messy, but I can't help it...
 *
 * CHANGES:
 * ++tesche 1/96:
 * - only do clipping if clipping rec pointer != NULL
 * ++kay 2/96:
 * - fixed a bug in CLIP_BITBLIT()
 * - fixed a bug in CLIP_BOX()
 */

#ifndef _CLIP_H
#define _CLIP_H

#include <stdlib.h>
#ifdef __MINT__
#include <macros.h>
#endif


/*
 * all the macros that perform clipping return something != 0 if the
 * figure to draw is totally outside the clip rectangle.
 */

/*
 * calculate the initial "delta" value for the line drawing algorithm
 * for a clipped line. the arguments should be selected using the following
 * algorithm (it looks complicated, but the line drawing algorithm has those
 * if's already in it):
 *
 * if (dx >= dy), then
 *   // lines are drawn from left to right
 *   dx     = x1_before_clipping - x0_before_clipping
 *   dy     = y1_before_clipping - x0_before_clipping
 *   x_offs = x0_before_clipping - x0_after_clipping
 *   if (dx < 0)
 *     dx = -dx;
 *     dy = -dy;
 * else
 *   // lines are drawn from top to bottom
 *   dx     = y1_before_clipping - y0_before_clipping
 *   dy     = x1_before_clipping - x0_before_clipping
 *   x_offs = y0_before_clipping - y0_after_clipping
 *   if (dy < 0)
 *     dy = -dy;
 *     dx = -dx;
 *
 * clipped_line_delta () then returns an initial value for "delta"
 * (scaled by two) that makes the line algorithm draw exactly the same
 * pixels for the clipped line as for the unclipped line.
 */
static inline long
clipped_line_delta (long dx, long dy, long x_offs)
{
	int delta;

	if (x_offs == 0)
		return dx;
	delta = (dx - 2*abs(x_offs)*dy) % (2*dx);
	if (delta < 0)
		delta += 2*dx;
	return delta;
}

/*
 * perform a quick check whether a line is totally visible. returns 0 if
 * so.
 *
 * xa <= xe is required.
 */
#define LINE_NEEDS_CLIPPING(xa, ya, xe, ye, cl)			\
({								\
	ulong	_ret = 0;					\
	if (cl) _ret = ((cl)->x0 > (xa) || (cl)->x1 < (xe) ||	\
		(ulong)((ya) - (cl)->y0) >= (cl)->h ||		\
		(ulong)((ye) - (cl)->y0) >= (cl)->h);		\
	_ret;							\
})

/*
 * clip a line to a rectangle. the algorithm is not perfect, the generated
 * line may be too short if the line has a very large (near 1) or small
 * (near 0) slope. the reasons are to complicated to explain here, see
 * "Computer Graphics, 2nd Edition" section 3.2.3 page 79.
 *
 * returns something != 0 if the line is fully outside the clip rectangle.
 *
 * xa <= xe is required.
 */
#define CLIP_LINE(xa, ya, xe, ye, cl)					\
({									\
	long __clip;							\
	long __dx, __dy, __tmp;						\
	long __ox0, __ox1, __oy0, __oy1;				\
	long __ret = 0;							\
									\
	if (!cl) goto __leave;						\
									\
	__ret = -1;							\
	__ox0 = (xa);							\
	__ox1 = (xe);							\
	__oy0 = (ya);							\
	__oy1 = (ye);							\
									\
	__dx = __ox1 - __ox0;						\
	__dy = __oy1 - __oy0;						\
									\
	/*								\
	 * left edge of clip rectangle					\
	 */								\
	__clip = (cl)->x0;						\
	if (__clip <= (xa)) {						\
	    /*								\
	     * clip.x0 <= x0   &&   clip.x0 <= x1			\
	     *								\
	     * accept							\
	     */								\
	} else if (__clip <= (xe)) {					\
	    /*								\
	     * x0 <  clip.x0   &&   clip.x0 <= x1			\
	     *								\
	     * cut x0							\
	     *								\
	     * we want (if all args were floats):			\
	     *   y0 = round (y0 + (clip - x0)*dy/dx)			\
	     *								\
	     * here is how to get the same result with			\
	     * integer arithmetik:					\
	     */								\
	    (xa) = __clip;						\
	    (ya) = (2*(__oy0*__dx + __dy*(__clip - __ox0)) + __dx)	\
			/ (2*__dx);					\
	} else {							\
	    /*								\
	     * x0 <  clip.x0   &&   x1 <  clip.x0			\
	     *								\
	     * reject							\
	     */								\
	   goto __leave;						\
	}								\
									\
	/*								\
	 * right edge of clipping rectangle				\
	 */								\
	__clip = (cl)->x1;						\
	if ((xe) <= __clip) {						\
	    /*								\
	     * x0 <= clip.x1  &&  x1 <= clip.x1				\
	     *								\
	     * accept							\
	     */								\
	} else if ((xa) <= __clip) {					\
	    /*								\
	     * x0 <= clip.x1  &&  clip.x1 <  x1				\
	     *								\
	     * cut x1							\
	     *								\
	     * y1 = round (y1 + (clip - x1)*dy/dx)			\
	     */								\
	    (xe) = __clip;						\
	    (ye) = (2*(__oy1*__dx + __dy*(__clip - __ox1)) + __dx)	\
			/ (2*__dx);					\
	} else {							\
		/*							\
		 * clip.x1 <  x0  &&  clip.x1 <  x1			\
		 *							\
		 * reject						\
		 */							\
		goto __leave;						\
	}								\
									\
	/*								\
	 * top edge of clip rectangle					\
	 */								\
	__clip = (cl)->y0;						\
	if (__clip <= (ya)) {						\
	    if (__clip <= (ye)) {					\
		/*							\
		 * clip.y0 <= y0   &&   clip.y0 <= y1			\
		 *							\
		 * accept						\
		 */							\
	    } else {							\
		/*							\
		 * clip.y0 <= y0   &&   y1 <  clip.y0			\
		 *							\
		 * cut y1						\
		 *							\
		 * x1 = round (x1 + (clip - y1)*dx/dy)			\
		 */							\
		__tmp = (2*(__ox1*__dy + __dx*(__clip - __oy1)) + __dy)	\
			/ (2*__dy);					\
									\
		(ye) = __clip;						\
		(xe) = (__tmp < (cl)->x0)				\
			? (cl)->x0					\
			: ((__tmp > (cl)->x1)				\
				? (cl)->x1				\
				: __tmp);				\
	    }								\
	} else {							\
	    if (__clip <= (ye)) {					\
		/*							\
		 * y0 <  clip.y0   &&   clip.y0 <= y1			\
		 *							\
		 * cut y0						\
		 *							\
		 * x0 = round (x0 + (clip - y0)*dx/dy)			\
		 */							\
		__tmp = (2*(__ox0*__dy + __dx*(__clip - __oy0)) + __dy)	\
			/ (2*__dy);					\
									\
		(ya) = __clip;						\
		(xa) = (__tmp < (cl)->x0)				\
			? (cl)->x0					\
			: ((__tmp > (cl)->x1)				\
				? (cl)->x1				\
				: __tmp);				\
	    } else {							\
		/*							\
		 * y0 <  clip.y0   &&   y1 <  clip.y0			\
		 *							\
		 * reject						\
		 */							\
		goto __leave;						\
	    }								\
	}								\
									\
	/*								\
	 * bottom edge of clipping rectangle				\
	 */								\
	__clip = (cl)->y1;						\
	if ((ya) <= __clip) {						\
	    if ((ye) <= __clip) {					\
		/*							\
		 * y0 <= clip.y1  &&  y1 <= clip.y1			\
		 *							\
		 * accept						\
		 */							\
	    } else {							\
		/*							\
		 * y0 <= clip.y1  &&  clip.y1 <  y1			\
		 *							\
		 * cut y1						\
		 *							\
		 * x1 = round (x1 + (clip - y1)*dx/dy)			\
		 */							\
		__tmp = (2*(__ox1*__dy + __dx*(__clip - __oy1)) + __dy)	\
			/ (2*__dy);					\
									\
		(ye) = __clip;						\
		(xe) = (__tmp < (cl)->x0)				\
			? (cl)->x0					\
			: ((__tmp > (cl)->x1)				\
				? (cl)->x1				\
				: __tmp);				\
	    }								\
	} else {							\
	    if ((ye) <= __clip) {					\
		/*							\
		 * clip.y1 <  y0  &&  y1 <= clip.y1			\
		 *							\
		 * cut y0						\
		 *							\
		 * x0 = round (x0 + (clip - y0)*dx/dy)			\
		 */							\
		__tmp = (2*(__ox0*__dy + __dx*(__clip - __oy0)) + __dy)	\
			/ (2*__dy);					\
									\
		(ya) = __clip;						\
		(xa) = (__tmp < (cl)->x0)				\
			? (cl)->x0					\
			: ((__tmp > (cl)->x1)				\
				? (cl)->x1				\
				: __tmp);				\
	    } else {							\
		/*							\
		 * clip.y1 <  y0  &&  clip.y1 <  y1			\
		 *							\
		 * reject						\
		 */							\
		goto __leave;						\
	    }								\
	}								\
	__ret = 0;							\
									\
__leave:								\
	__ret;								\
})

/*
 * clip a horizontal line to a rectangle. xa <= xe is required.
 *
 * returns something != 0 if the point is fully outside the clip rectangle.
 */
#define CLIP_HLINE(xa, ya, xe, cl)			\
({							\
	long __ret = 0;					\
							\
	if (!cl) goto __leave;				\
							\
	if ((ulong)((ya) - (cl)->y0) >= (cl)->h) {	\
		/*					\
		 * reject				\
		 */					\
		__ret = -1;				\
		goto __leave;				\
	}						\
	if ((xa) >= (cl)->x0 && (xe) <= (cl)->x1)	\
		/*					\
		 * accept				\
		 */					\
		goto __leave;				\
							\
	if ((xa) < (cl)->x0)				\
		(xa) = (cl)->x0;			\
	if ((xe) > (cl)->x1)				\
		(xe) = (cl)->x1;			\
	__ret = ((xa) > (xe));				\
							\
__leave:						\
	__ret;						\
})

/*
 * clip a vertical line to a rectangle. ya <= ye is required.
 *
 * returns something != 0 if the point is fully outside the clip rectangle.
 */
#define CLIP_VLINE(xa, ya, ye, cl)			\
({							\
	long __ret = 0;					\
							\
	if (!cl) goto __leave;				\
							\
	if ((ulong)((xa) - (cl)->x0) >= (cl)->w) {	\
		/*					\
		 * reject				\
		 */					\
		__ret = -1;				\
		goto __leave;				\
	}						\
	if ((ya) >= (cl)->y0 && (ye) <= (cl)->y1)	\
		/*					\
		 * accept				\
		 */					\
		goto __leave;				\
							\
	if ((ya) < (cl)->y0)				\
		(ya) = (cl)->y0;			\
	if ((ye) > (cl)->y1)				\
		(ye) = (cl)->y1;			\
	__ret = ((ya) > (ye));				\
							\
__leave:						\
	__ret;						\
})

/*
 * perform a quick check whether a box needs clipping or not. returns
 * != 0 if clipping is needed.
 */
#define BOX_NEEDS_CLIPPING(x, y, wd, ht, cl)			\
({								\
	ulong _ret = 0;						\
								\
	if (cl) _ret = ((ulong)((x) - (cl)->x0) >= (cl)->w ||	\
		(ulong)((x) + (wd) - 1 - (cl)->x0) >= (cl)->w ||\
		(ulong)((y) - (cl)->y0) >= (cl)->h ||		\
		(ulong)((y) + (ht) - 1 - (cl)->y0) >= (cl)->h);	\
								\
	_ret;							\
})

/*
 * clip a box to a rectangle.
 *
 * returns something != 0 if the box is fully outside the clip rectangle.
 */
#define CLIP_BOX(x, y, w, h, cl)			\
({							\
	long __x1, __x2, __y1, __y2;			\
	long __ret = 0;					\
							\
	if (cl) {					\
		__ret = -1;				\
		__x1 = MAX ((x), (cl)->x0);		\
		__x2 = MIN ((x) + (w), (cl)->x1+1);	\
		__y1 = MAX ((y), (cl)->y0);		\
		__y2 = MIN ((y) + (h), (cl)->y1+1);	\
							\
		if (__x1 < __x2 && __y1 < __y2) {	\
			(x) = __x1;			\
			(y) = __y1;			\
			(w) = __x2-__x1;		\
			(h) = __y2-__y1;		\
			__ret = 0;			\
		}					\
	}						\
	__ret;						\
})

/*
 * perform clipping for bitblits.
 */
#define CLIP_BITBLIT(xs, ys, w, h, xd, yd, cls, cld)	\
({							\
	long __ox, __oy, __ret = 0;			\
							\
	if (!cls || !cld) goto __leave;			\
							\
	__ret = -1;					\
	__ox = (xs);					\
	__oy = (ys);					\
	if (CLIP_BOX (xs, ys, w, h, cls))		\
		goto __leave;				\
	(xd) += ((xs) - __ox);				\
	(yd) += ((ys) - __oy);				\
	__ox = (xd);					\
	__oy = (yd);					\
	if (CLIP_BOX (xd, yd, w, h, cld))		\
		goto __leave;				\
	(xs) += ((xd) - __ox);				\
	(ys) += ((yd) - __oy);				\
	__ret = 0;					\
							\
__leave:						\
	__ret;						\
})

/*
 * clip a point to a rectangle.
 *
 * returns something != 0 if the point is fully outside the clip rectangle.
 */
#define CLIP_POINT(x,y,cl)					\
({								\
	ulong _ret = 0;						\
								\
	if (cl) _ret = ((ulong)((x) - (cl)->x0) >= (cl)->w ||	\
		(ulong)((y) - (cl)->y0) >= (cl)->h);		\
								\
	_ret;							\
})

#endif
