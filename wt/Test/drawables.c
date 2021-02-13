/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * test program to play a bit with the widgets.
 *
 * $Id: drawables.c,v 1.3 1999/05/16 15:05:38 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>
#include <Wt.h>

static widget_t *top, *shell, *button, *quit, *box, *pane, *hpane, *drawable;

static void
button_callback (widget_t *button, int pressed)
{
	wt_setopt (button,
		WT_LABEL, pressed == 1 ? "Thanks." : "Press me!",
		WT_EOL);
}

static void
quit_callback (widget_t *wp, int pressed)
{
	/*
	 * press == -1 means the mouse pointer left the button-area
	 * while the left mouse button was still pressed.
	 */
	if (pressed == 0)
		/*
		 * this makes wt_run() return with return-value 1.
		 */
		wt_break (1);
}

/*
 * called when drawable is created
 */
static void
draw_function (widget_t *w, long x, long y, long wd, long ht)
{
	WWIN *win = wt_widget2win (w);

	w_setmode (win, M_DRAW);
	w_box (win, 0, 0, wd, ht);
	w_hline (win, 1, 1, wd - 3);
	w_vline (win, 1, 2, ht - 3);
}

static void
event_callback (widget_t *drawable, WEVENT *ev)
{
	switch (ev->type) {
	case EVENT_MPRESS:
		if (ev->key & BUTTON_LEFT)
			w_plot (ev->win, ev->x, ev->y);
		break;
	}
}

int
main (int argc, char *argv[])
{
	long i, wd;

	top = wt_init ();
	if (!top)
		return 1;

	shell = wt_create (wt_shell_class, top);
	if (!shell) return 2;

	wt_setopt (shell,
		WT_LABEL, " WtTest ",
		WT_EOL);

	pane = wt_create (wt_pane_class, shell);
	if (!pane) return 3;

	hpane = wt_create (wt_box_class, pane);
	if (!hpane) return 4;

	i = OrientHorz;
	wt_setopt (hpane,
		WT_ORIENTATION, &i,
		WT_EOL);

	drawable = wt_create (wt_drawable_class, hpane);
	if (!drawable) return 5;

	wd = 120;
	i = EV_MOUSE;
	wt_setopt (drawable,
		WT_EVENT_MASK, &i,
		WT_EVENT_CB, event_callback,
		WT_DRAW_FN, draw_function,
		WT_HEIGHT, &wd,
		WT_WIDTH, &wd,
		WT_EOL);

	box = wt_create (wt_box_class, pane);
	if (!box) return 6;

	quit = wt_create (wt_button_class, box);
	if (!quit) return 7;

	wt_setopt (quit,
		WT_LABEL, "Quit",
		WT_ACTION_CB, quit_callback,
		WT_EOL);

	button = wt_create (wt_button_class, box);
	if (!button) return 8;

	wt_setopt (button,
		WT_LABEL, "Press me!",
		WT_ACTION_CB, button_callback,
		WT_EOL);

	if (wt_realize (top) < 0)
		return 9;

	wt_run ();
	return 0;
}
