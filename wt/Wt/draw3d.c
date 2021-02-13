/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * routines for drawing primitives with 3d-look
 *
 * $Id: draw3d.c,v 1.2 2008-08-29 19:47:09 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

void
wt_box3d (WWIN *win, int x, int y, int wd, int ht)
{
	w_setmode (win, M_DRAW);
	w_box (win, x, y, wd, ht);
	w_hline (win, x+2, y+ht-2, x+wd-2);
	w_vline (win, x+wd-2, y+2, y+ht-3);
}

void
wt_box3d_press (WWIN *win, int x, int y, int wd, int ht, int solid)
{
	if (solid)
		w_bitblk (win, x+2, y+2, wd-5, ht-5, x+3, y+3);

	w_setmode (win, M_CLEAR);
	w_hline (win, x+2, y+ht-2, x+wd-2);
	w_vline (win, x+wd-2, y+2, y+ht-3);

	w_setmode (win, M_DRAW);
	w_hline (win, x+1, y+1, x+wd-3);
	w_vline (win, x+1, y+2, y+ht-3);
}

void
wt_box3d_release (WWIN *win, int x, int y, int wd, int ht, int solid)
{
	if (solid)
		w_bitblk (win, x+3, y+3, wd-5, ht-5, x+2, y+2);

	w_setmode (win, M_CLEAR);
	w_hline (win, x+1, y+1, x+wd-3);
	w_vline (win, x+1, y+2, y+ht-3);

	w_setmode (win, M_DRAW);
	w_hline (win, x+2, y+ht-2, x+wd-2);
	w_vline (win, x+wd-2, y+2, y+ht-3);
}

void
wt_box3d_mark (WWIN *win, int x, int y, int wd, int ht)
{
	if (wd > 8 && ht > 8) {
		w_setmode (win, M_DRAW);
		w_pbox (win, x+4, y+4, wd-7, ht-7);
	}
}

void
wt_box3d_unmark (WWIN *win, int x, int y, int wd, int ht)
{
	if (wd > 8 && ht > 8) {
		w_setmode (win, M_CLEAR);
		w_pbox (win, x+4, y+4, wd-7, ht-7);
	}
}

void
wt_circle3d (WWIN *win, int x, int y, int r)
{
	w_setmode (win, M_DRAW);
	w_pcircle (win, x, y, r);

	w_setmode (win, M_CLEAR);
	w_pcircle (win, x-1, y-1, r-1);

	w_setmode (win, M_DRAW);
	w_circle (win, x, y, r);
}

void
wt_circle3d_press (WWIN *win, int x, int y, int r)
{
	w_setmode (win, M_DRAW);
	w_pcircle (win, x, y, r);

	w_setmode (win, M_CLEAR);
	w_pcircle (win, x+1, y+1, r-1);

	w_setmode (win, M_DRAW);
	w_circle (win, x, y, r);
}

void
wt_circle3d_release (WWIN *win, int x, int y, int r)
{
	return wt_circle3d (win, x, y, r);
}

void
wt_circle3d_mark (WWIN *win, int x, int y, int r)
{
	if (r > 3) {
		w_setmode (win, M_DRAW);
		w_pcircle (win, x+1, y+1, r-3);
	}
}

void
wt_circle3d_unmark (WWIN *win, int x, int y, int r)
{
	if (r > 3) {
		w_setmode (win, M_CLEAR);
		w_pcircle (win, x+1, y+1, r-3);
	}
}

void
wt_arrow3d (WWIN *win, int x, int y, int wd, int ht, int dir)
{
	w_setmode (win, M_CLEAR);
	w_pbox (win, x, y, wd, ht);
	w_setmode (win, M_DRAW);

	switch (dir) {
	case 0: /* UP */
		w_line (win, x,      y+ht-1, x+wd-1,   y+ht-1);
		w_line (win, x,      y+ht-1, x+wd/2,   y);
		w_line (win, x+wd-1, y+ht-1, x+wd/2,   y);

		w_line (win, x+wd-2, y+ht-1, x+wd/2-1, y);
		w_line (win, x+1,    y+ht-2, x+wd-2,   y+ht-2);

		w_setmode (win, M_CLEAR);
		w_plot (win, x+wd/2-1, y);
		if (wd < ht) {
			w_plot (win, x+wd/2-1, y+1);
		}
		break;

	case 1: /* DOWN */
		w_line (win, x,      y, x+wd-1,   y);
		w_line (win, x,      y, x+wd/2,   y+ht-1);
		w_line (win, x+wd-1, y, x+wd/2,   y+ht-1);

		w_line (win, x+wd-2, y, x+wd/2-1, y+ht-1);

		w_setmode (win, M_CLEAR);
		w_plot (win, x+wd/2-1, y+ht-1);
		if (wd < ht) {
			w_plot (win, x+wd/2-1, y+ht-2);
		}
		break;

	case 2: /* LEFT */
		w_line (win, x+wd-1, y,      x+wd-1, y+ht-1);
		w_line (win, x+wd-1, y,      x,      y+ht/2);
		w_line (win, x+wd-1, y+ht-1, x,      y+ht/2);

		w_line (win, x+wd-1, y+ht-2, x,      y+ht/2-1);
		w_line (win, x+wd-2, y+1,    x+wd-2, y+ht-2);

		w_setmode (win, M_CLEAR);
		w_plot (win, x, y+ht/2-1);
		if (wd > ht) {
			w_plot (win, x+1, y+ht/2-1);
		}
		break;

	case 3: /* RIGHT */
		w_line (win, x, y,      x,      y+ht-1);
		w_line (win, x, y,      x+wd-1, y+ht/2);
		w_line (win, x, y+ht-1, x+wd-1, y+ht/2);

		w_line (win, x, y+ht-2, x+wd-1, y+ht/2-1);

		w_setmode (win, M_CLEAR);
		w_plot (win, x+wd-1, y+ht/2-1);
		if (wd > ht) {
			w_plot (win, x+wd-2, y+ht/2-1);
		}
		break;
	}
}

void
wt_arrow3d_press (WWIN *win, int x, int y, int wd, int ht, int dir)
{
	w_setmode (win, M_CLEAR);
	w_pbox (win, x, y, wd, ht);
	w_setmode (win, M_DRAW);

	switch (dir) {
	case 0: /* UP */
		w_line (win, x,      y+ht-1, x+wd-1,   y+ht-1);
		w_line (win, x,      y+ht-1, x+wd/2,   y);
		w_line (win, x+wd-1, y+ht-1, x+wd/2,   y);

		w_line (win, x+1,    y+ht-1, x+wd/2+1, y);

		w_setmode (win, M_CLEAR);
		w_plot (win, x+wd/2+1, y);
		if (wd < ht) {
			w_plot (win, x+wd/2+1, y+1);
		}
		break;

	case 1: /* DOWN */
		w_line (win, x,      y,   x+wd-1,   y);
		w_line (win, x,      y,   x+wd/2,   y+ht-1);
		w_line (win, x+wd-1, y,   x+wd/2,   y+ht-1);

		w_line (win, x+1,    y,   x+wd/2+1, y+ht-1);
		w_line (win, x+1,    y+1, x+wd-2,   y+1);

		w_setmode (win, M_CLEAR);
		w_plot (win, x+wd/2+1, y+ht-1);
		if (wd < ht) {
			w_plot (win, x+wd/2+1, y+ht-2);
		}
		break;

	case 2: /* LEFT */
		w_line (win, x+wd-1, y,      x+wd-1, y+ht-1);
		w_line (win, x+wd-1, y,      x,      y+ht/2);
		w_line (win, x+wd-1, y+ht-1, x,      y+ht/2);

		w_line (win, x+wd-1, y+1,    x,      y+ht/2+1);

		w_setmode (win, M_CLEAR);
		w_plot (win, x, y+ht/2+1);
		if (wd > ht) {
			w_plot (win, x+1, y+ht/2+1);
		}
		break;

	case 3: /* RIGHT */
		w_line (win, x,   y,      x,      y+ht-1);
		w_line (win, x,   y,      x+wd-1, y+ht/2);
		w_line (win, x,   y+ht-1, x+wd-1, y+ht/2);

		w_line (win, x,   y+1,    x+wd-1, y+ht/2+1);
		w_line (win, x+1, y+1,    x+1,    y+ht-2);

		w_setmode (win, M_CLEAR);
		w_plot (win, x+wd-1, y+ht/2+1);
		if (wd > ht) {
			w_plot (win, x+wd-2, y+ht/2+1);
		}
		break;
	}
}

void
wt_arrow3d_release (WWIN *win, int x, int y, int wd, int ht, int dir)
{
	return wt_arrow3d (win, x, y, wd, ht, dir);
}

void
wt_text (WWIN *win, WFONT *fp, const char *str,
	 int x, int y, int wd, int ht, int align)
{
	int xpos, ypos;

	ypos = (ht - fp->height)/2;

	switch (align) {
	case AlignLeft:
		xpos = 0;
		break;
	case AlignRight:
		xpos = wd - w_strlen (fp, str);
		break;
	case AlignCenter:
	default:
		xpos = (wd - w_strlen (fp, str))/2;
		break;
	}
	w_printstring (win, x + xpos, y + ypos, str);
}
