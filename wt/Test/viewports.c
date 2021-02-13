/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * tests the viewport and scrollbar widgets.
 *
 * $Id: viewports.c,v 1.2 1999/05/16 15:05:38 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>
#include <Wt.h>

static widget_t *top, *shell, *viewport, *drawable;

static void
draw_function (widget_t *w, long x, long y, long wd, long ht)
{
	WWIN *win = wt_widget2win (w);

	w_setmode (win, M_DRAW);
	w_circle (win, wd/2, ht/2, MIN (wd, ht)/2);
}

int
main ()
{
	long i;

	top = wt_init ();
	if (!top) return 1;

	shell = wt_create (wt_shell_class, top);
	wt_setopt (shell, WT_LABEL, " Viewport ", WT_EOL);

	viewport = wt_create (wt_viewport_class, shell);
	i = 200;
	wt_setopt (viewport, WT_WIDTH, &i, WT_HEIGHT, &i, WT_EOL);

	drawable = wt_create (wt_drawable_class, viewport);
	i = 400;
	wt_setopt (drawable, WT_WIDTH, &i, WT_HEIGHT, &i,
		WT_DRAW_FN, draw_function,
		WT_EOL);

	wt_realize (top);
	wt_run ();
	return 0;
}
