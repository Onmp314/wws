/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * $Id: wt_dialog.c,v 1.2 2008-08-29 19:47:09 eero Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

#include "Icons/error.icon"
#include "Icons/question.icon"
#include "Icons/warning.icon"
#include "Icons/info.icon"

static BITMAP iconbm = { 32, 32, BM_PACKEDMONO, 4, 1, 1, NULL };

static void
getpos (widget_t *parent, int *xpos, int *ypos)
{
	short x, y;
	WWIN *pwin;

	if (parent && (pwin = wt_widget2win (parent))) {
		w_querywindowpos (pwin, 1, &x, &y);
		*xpos = x + 30;
		*ypos = y + 30;
	} else {
		w_querymousepos (WROOT, &x, &y);
		*xpos = x;
		*ypos = y;
	}
}

/*************************** An Entrybox ******************************/

static void
button_cb (widget_t *w, int pressed)
{
	widget_t *wp;
	long i = 0;

	/*
	 * set the usrval of the toplevel widget to the index of the
	 * pressed button
	 */
	if (pressed == 0) {
		for (wp = w; wp->parent; wp = wp->parent)
			;
		wt_getopt (w,  WT_USRVAL, &i, WT_EOL);
		wt_setopt (wp, WT_USRVAL, &i, WT_EOL);
	}
}

static void
close_cb (widget_t *w)
{
	widget_t *wp;
	long i = -1;

	/*
	 * make wt_entrybox() return -1
	 */
	for (wp = w; wp->parent; wp = wp->parent)
		;
	wt_setopt (wp, WT_USRVAL, &i, WT_EOL);
}

static void
draw_fn (widget_t *w, long x, long y, long wd, long ht)
{
	w_putblock (&iconbm, wt_widget2win (w), 0, 0);
}

int
wt_dialog (widget_t *parent, const char *msg, int type, const char *title, ...)
{
	widget_t *top, *shell, *vpane, *box1, *box2, *icon, *text, *but;
	va_list args;
	char *butname;
	int x, y;
	long i;

	switch (type) {
	default:
	case WT_DIAL_INFO:
		iconbm.data = info_bits;
		break;
	case WT_DIAL_ERROR:
		iconbm.data = error_bits;
		break;
	case WT_DIAL_WARN:
		iconbm.data = warning_bits;
		break;
	case WT_DIAL_QUEST:
		iconbm.data = question_bits;
		break;
	}
	getpos (parent, &x, &y);

	top = wt_create (wt_top_class, NULL);

	i = 0;
	wt_setopt (top,
		WT_USRVAL, &i,
		WT_EOL);

	shell = wt_create (wt_shell_class, top);

	wt_setopt (shell,
		WT_ACTION_CB, close_cb,
		WT_LABEL, title ?: "",
		WT_XPOS, &x,
		WT_YPOS, &y,
		WT_EOL);

	vpane = wt_create (wt_pane_class, shell);

	i = AlignRight;
	wt_setopt (vpane,
		WT_ALIGNMENT, &i,
		WT_EOL);

	box1 = wt_create (wt_box_class, vpane);
	box2 = wt_create (wt_box_class, vpane);

	icon = wt_create (wt_drawable_class, box1);

	i = 32;
	wt_setopt (icon,
		WT_WIDTH, &i,
		WT_HEIGHT, &i,
		WT_DRAW_FN, draw_fn,
		WT_EOL);

	text = wt_create (wt_text_class, box1);

	i = 200;
	wt_setopt (text,
		WT_WIDTH, &i,
		WT_LABEL, msg,
		WT_EOL);

	va_start (args, title);
	for (i = 1; (butname = va_arg (args, char *)); ++i) {
		but = wt_create (wt_pushbutton_class, box2);
		wt_setopt (but,
			WT_USRVAL, &i,
			WT_LABEL, butname,
			WT_ACTION_CB, button_cb,
			WT_EOL);
	}
	va_end (args);

	wt_realize (top);

	while (!wt_do_event ()) {
		wt_getopt (top, WT_USRVAL, &i, WT_EOL);
		if (i != 0)
			break;
	}
	wt_getopt (top, WT_USRVAL, &i, WT_EOL);
	wt_delete (top);

	return i;
}
