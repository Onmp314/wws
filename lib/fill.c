/*
 * fill.c, a part of the W Window System
 *
 * Copyright (C) 1997 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a very slow generic flood fill routine
 *
 * Warning!  This may take a long time, depending on the area that is
 * flooded.  Especially dithered areas are awful in this respect...
 *
 * Doing this on server side would be much faster (because pixel tests need
 * not then to go over socket and back and checks could be done several
 * pixels at the time) but 'malicious' programs could then badly disrupt
 * server responsiveness.
 */

#include <stdio.h>		/* printf() */
#include "Wlib.h"
#include "proto.h"

#define STACK	64
#define PUSH(x,y) { if(top < lid) { *top++ = (x); *top++ = (y); } \
		    else { printf("w_fill() stack overflow!\n"); } }


/* Fill all the pixels of color of pixel(x,y) with fg or bg color
 * depending on the gfx mode.
 */
short w_fill(WWIN *win, short x, short y)
{
	short stack[STACK], *top = stack, *lid = top + (STACK-1);
	short start, middle, wd = win->width, ht = win->height;
	int above, below, old_above, old_below;
	uchar change;
	const char *cptr;

	TRACESTART();

	if ((cptr = _check_window(win))) {
		TRACEPRINT(("w_fill(%p,%i,%i) -> %s\n", win, x, y, cptr));
		TRACEEND();
		return -1;
	}

	/* on server modify 'wd' and 'ht' according to clip rectangle and
	 * then disable clipping for the duration of this function.
	 */

	/* has to check for this as algorithm expects that */
	if (x < 0 || y < 0 || x >= wd || y >= ht) {
		TRACEPRINT(("w_fill(%p,%i,%i) -> 0\n", win, x, y));
		TRACEEND();
		return 0;
	}

	change = w_test(win, x, y);
	switch (win->drawmode) {
		case M_CLEAR:
			if (change == win->bg) {
				TRACEPRINT(("w_fill(%p,%i,%i) -> 0\n", win, x, y));
				TRACEEND();
				return 0;
			}
		case M_INVERS:
			break;
		default:
			if (change == win->fg) {
				TRACEPRINT(("w_fill(%p,%i,%i) -> 0\n", win, x, y));
				TRACEEND();
				return 0;
			}
	}

	PUSH(x, y);

	/* tests are expensive on client side so I go to both directions
	 * instead of eg. first just scanning for left edge and then
	 * checking onto the right.
	 */

	/* while something in stack, POP(x,y) */
	while(top > stack + 1) {

		/* last-in, first-out! */
		y = *(--top);
		x = *(--top);

		/* already filled? */
		if (w_test(win, x, y) != change) {
			continue;
		}

		/* check above and below here to get old_* vars */
		if (y && w_test(win, x, y-1) == change) {
			PUSH(x, y-1);
			above = 0;
		} else {
			above = 1;
		}
		y++;
		if (y < ht && w_test(win, x, y) == change) {
			PUSH(x, y);
			below = 0;
		} else {
			below = 1;
		}
		y--;

		old_above = above;
		old_below = below;
		middle = x;

		/* if you'll want to speed these, have separate loops
		 * for case were y > 0 and y+1 < win-size.
		 */

		/* go left */
		while(--x >= 0) {
			if (w_test(win, x, y) != change) {
			 	break;
			}

			if(y) {
				/* check above */
				if (above) {
					if (w_test(win, x, y-1) == change) {
						PUSH(x, y-1);
						above = 0;
					}
				} else {
					if (w_test(win, x, y-1) != change) {
						above = 1;
					}
				}
			}

			y++;
			if(y < ht) {
				/* check below */
				if (below) {
					if (w_test(win, x, y) == change) {
						PUSH(x, y);
						below = 0;
					}
				} else {
					if (w_test(win, x, y) != change) {
						below = 1;
					}
				}
			}
			y--;
		}

		start = x + 1;
		above = old_above;
		below = old_below;
		x = middle;

		/* go right (first check starting point) */
		while(++x < wd) {
			if (w_test(win, x, y) != change) {
			 	break;
			}

			if(y) {
				/* check above */
				if (above) {
					if (w_test(win, x, y-1) == change) {
						PUSH(x, y-1);
						above = 0;
					}
				} else {
					if (w_test(win, x, y-1) != change) {
						above = 1;
					}
				}
			}

			y++;
			if(y < ht) {
				/* check below */
				if (below) {
					if (w_test(win, x, y) == change) {
						PUSH(x, y);
						below = 0;
					}
				} else {
					if (w_test(win, x, y) != change) {
						below = 1;
					}
				}
			}
			y--;
		}

		/* draw the scanline */
		w_hline(win, start, y, x - 1);
	}

	TRACEPRINT(("w_fill(%p,%i,%i) -> 0\n", win, x, y));
	TRACEEND();
	return 0;
}
