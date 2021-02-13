/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * very simple and hackish drawing program....
 *
 * $Id: draw.c,v 1.2 2008-08-28 20:53:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <Wlib.h>
#include <Wt.h>

#define N_OPS	4
#define N_MODES	3

widget_t *top, *shell, *pack, *viewport, *oppane, *modepane,
  *buttonbox, *quitbutton, *savebutton, *opbuttons[N_OPS],
  *modebuttons[N_MODES], *drawable;

short draw_func = 0;
short draw_mode = 0;

long timer;
long startx, starty, mousex, mousey;
short state = 0;

#define DIST(x1,y1,x2,y2) (MAX(abs(x2-x1),abs(y2-y1)))

static void
draw_timer (long arg)
{
	WWIN *win = wt_widget2win (drawable);
	short mx, my;
	long x, y, wd, ht;

	if (w_querymousepos (win, &mx, &my))
		return;

	if (mx != mousex || my != mousey) {
		state = 2;
		switch (draw_func) {
		case 1:
			w_circle (win, startx, starty,
				DIST (startx, starty, mousex, mousey));
			w_circle (win, startx, starty,
				DIST (startx, starty, mx, my));
			break;
		case 2:
			w_line (win, startx, starty, mousex, mousey);
			w_line (win, startx, starty, mx, my);
			break;
		case 3:
			x = startx;
			y = starty;
			wd = mousex - startx;
			ht = mousey - starty;
			if (wd < 0) {
				x = x + wd - 1;
				wd = -wd;
			}
			if (ht < 0) {
				y = y + ht - 1;
				ht = -ht;
			}
			w_box (win, x, y, wd, ht);

			x = startx;
			y = starty;
			wd = mx - startx;
			ht = my - starty;
			if (wd < 0) {
				x = x + wd - 1;
				wd = -wd;
			}
			if (ht < 0) {
				y = y + ht - 1;
				ht = -ht;
			}
			w_box (win, x, y, wd, ht);
			break;
		}
	}
	mousex = mx;
	mousey = my;
	timer = wt_addtimeout (100, draw_timer, 0);
}

static WEVENT *
draw_event (widget_t *w, WEVENT *ev)
{
	long x, y, wd, ht;

	switch (ev->type) {
	case EVENT_MPRESS:
		if (!(ev->key & BUTTON_LEFT))
			return ev;
		state = 1;
		startx = mousex = ev->x;
		starty = mousey = ev->y;
		break;

	case EVENT_MRELEASE:
		if (!(ev->key & BUTTON_LEFT))
			return ev;
		switch (draw_mode) {
		case 0:
			w_setmode (ev->win, M_DRAW);
			break;
		case 1:
			w_setmode (ev->win, M_CLEAR);
			break;
		case 2:
			state = 0;
			wt_deltimeout (timer);
			return NULL;
		}

	case EVENT_INACTIVE:
		if (state < 2) {
			state = 0;
			wt_deltimeout (timer);
			return NULL;
		}
		if (ev->type == EVENT_INACTIVE)
			w_setmode (ev->win, M_INVERS);
		switch (draw_func) {
		case 1:
			w_circle (ev->win, startx, starty,
				DIST (startx, starty, mousex, mousey));
			break;
		case 2:
			w_line (ev->win, startx, starty, mousex, mousey);
			break;
		case 3:
			x = startx;
			y = starty;
			wd = mousex - startx;
			ht = mousey - starty;
			if (wd < 0) {
				x = x + wd - 1;
				wd = -wd;
			}
			if (ht < 0) {
				y = y + ht - 1;
				ht = -ht;
			}
			w_box (ev->win, x, y, wd, ht);
			break;
		}
		state = 0;
		wt_deltimeout (timer);
		return NULL;

	default:
		return ev;
	}
	if (draw_func == 0) {
		w_setmode (ev->win, M_DRAW);
		w_plot (ev->win, startx, starty);
		state = 0;
	} else {
		w_setmode (ev->win, M_INVERS);
		timer = wt_addtimeout (100, draw_timer, 0);
	}
	return NULL;
}

static void
opbutton_cb (widget_t *but, int pressed)
{
	long i;

	if (pressed) {
		for (i = 0; i < N_OPS; ++i) {
			if (opbuttons[i] == but)
				break;
		}
		if (i < N_OPS)
			draw_func = i;
	}
}

static void
opbutton_draw (widget_t *but, short x, short y, BITMAP *bm)
{
	WWIN *win = wt_widget2win (but);
	int wd, ht;
	long i;

	for (i = 0; i < N_OPS; ++i) {
		if (but == opbuttons[i])
			break;
	}
	if (i >= N_OPS)
		return;

	wd = bm->width;
	ht = bm->height;

	w_setmode (win, M_DRAW);
	switch (i) {
	case 0:
		w_pbox (win, x + wd/2 - 1, y + ht/2 - 1, 2, 2);
		break;
	case 1:
		w_circle (win, x + wd/2, y + ht/2, MIN(wd,ht)/2 - 2);
		break;
	case 2:
		w_line (win, x + 2, y + 2, x + wd - 3, y + ht - 3);
		break;
	case 3:
		w_box (win, x + 2, y + 2, wd - 4, ht - 4);
		break;
	}
}

static void
modebutton_cb (widget_t *but, int pressed)
{
	int i;

	if (pressed) {
		for (i = 0; i < N_MODES; ++i) {
			if (modebuttons[i] == but)
				break;
		}
		if (i < N_MODES)
			draw_mode = i;
	}
}

static void
save_cb (widget_t *but, int pressed)
{
	char fname[128] = "blah.gif";

	if (!pressed) {
		wt_entrybox (shell, fname, sizeof (fname) - 1,
			"Draw - Save File", "Enter filename",
			"Save", "Cancel", NULL);
	}
}

static void
quit_cb (widget_t *but, int pressed)
{
	static int i = 0;
	int r;

	if (pressed)
		return;

	r = wt_dialog (shell,
		"Unsaved data may be lost when leaving.\nQuit anyway?",
		i++ % 5, "Draw - Quit",
		"Quit", "Cancel", NULL);

	if (r == 1)
		wt_break (1);
}

static wt_menu_item popup_menu[] = {
	{ "Entry 1",		0, "", 0, 0, MenuItem },
	{ "Entry 2",		0, "", 0, 0, MenuItem },
	{ 0,0,0,0,0, MenuItem },	/* separator */
	{ "Another Entry",	0, "", 0, 0, MenuItem },
	{ 0,0,0,0,0, MenuEnd }
};

static WEVENT *
global_event (widget_t *w, WEVENT *ev)
{
	static int in_popup = 0;

	if (in_popup || ev->type != EVENT_MPRESS || !(ev->key & BUTTON_RIGHT))
		return ev;

	in_popup = 1;

	wt_popup_cb(top, popup_menu, 0);
#if 0	/* old style */
	wt_popup_cb ("Entry 1\000Long Entry 2\000@sep\000Another Entry\000",
		NULL);
#endif
	in_popup = 0;
	return NULL;
}

static int
create_widgets (void)
{
	static const char *modenames[] = { "Draw", "Clr", "Xor" };
	static BITMAP bm;
	long i, j, wd, fsize;

	shell = wt_create (wt_shell_class, top);

	wt_setopt (shell,
		WT_LABEL, " W Draw ",
		WT_EOL);

	pack = wt_create (wt_packer_class, shell);

	buttonbox = wt_create (wt_box_class, pack);
	viewport = wt_create (wt_viewport_class, pack);
	oppane = wt_create (wt_pane_class, pack);
	modepane = wt_create (wt_pane_class, pack);

	i = 200;
	wt_setopt (viewport,
		WT_WIDTH, &i,
		WT_HEIGHT, &i,
		WT_EOL);

	drawable = wt_create (wt_drawable_class, viewport);

	i = 500;
	j = EV_MOUSE|EV_ACTIVE;
	wt_setopt (drawable,
		WT_WIDTH, &i,
		WT_HEIGHT, &i,
		WT_EVENT_MASK, &j,
		WT_EVENT_CB, draw_event,
		WT_EOL);

	savebutton = wt_create (wt_button_class, buttonbox);

	wt_setopt (savebutton,
		WT_LABEL, "Save",
		WT_ACTION_CB, save_cb,
		WT_EOL);

	quitbutton = wt_create (wt_button_class, buttonbox);

	wt_setopt (quitbutton,
		WT_LABEL, "Quit",
		WT_ACTION_CB, quit_cb,
		WT_EOL);

	i = 1;
	wt_setopt (oppane,
		WT_VDIST, &i,
		WT_EOL);

	wt_setopt (modepane,
		WT_HDIST, &i,
		WT_EOL);

	wd = 30;
	bm.width = wd;
	bm.height = wd;
	j = ButtonModeRadio;
	for (i = 0; i < N_OPS; ++i) {
		opbuttons[i] = wt_create (wt_button_class, oppane);
		wt_setopt (opbuttons[i],
			WT_WIDTH, &wd,
			WT_HEIGHT, &wd,
			WT_ACTION_CB, opbutton_cb,
			WT_DRAW_FN, opbutton_draw,
			WT_ICON, &bm,
			WT_MODE, &j,
			WT_EOL);
	}
	fsize = 8;
	for (i = 0; i < N_MODES; ++i) {
		modebuttons[i] = wt_create (wt_button_class, modepane);
		wt_setopt (modebuttons[i],
			WT_WIDTH, &wd,
			WT_ACTION_CB, modebutton_cb,
			WT_LABEL, modenames[i],
			WT_FONTSIZE, &fsize,
			WT_MODE, &j,
			WT_EOL);
	}
	i = ButtonStatePressed;
	wt_setopt (opbuttons[0],
		WT_STATE, &i,
		WT_EOL);

	wt_setopt (modebuttons[0],
		WT_STATE, &i,
		WT_EOL);

	wt_setopt (pack,
		WT_PACK_WIDGET, buttonbox,
		WT_PACK_INFO, "-side bottom -anchor e",
		WT_PACK_WIDGET, viewport,
		WT_PACK_INFO, "-side right",
		WT_PACK_WIDGET, oppane,
		WT_PACK_WIDGET, modepane,
		WT_PACK_INFO, "-side top -pady 4 -padx 4",
		WT_EOL);

	wt_bind (drawable, EV_MOUSE, global_event);

	return 0;
}

int
main (int argc, char *argv[])
{
	top = wt_init ();

	if (create_widgets ())
		return 1;

	wt_realize (top);
	wt_run ();
	return 0;
}
