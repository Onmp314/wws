/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * $Id: wt_entrybox.c,v 1.2 2008-08-29 19:47:09 eero Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

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
entrybox_button_cb (widget_t *w, int pressed)
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
entrybox_getstr_cb (widget_t *w, char *text, int cursor_pos)
{
	widget_t *wp;
	long i = 1;

	/*
	 * same as if first button hab been pressed
	 */
	for (wp = w; wp->parent; wp = wp->parent)
		;
	wt_setopt (wp, WT_USRVAL, &i, WT_EOL);
}

static void
entrybox_close_cb (widget_t *w)
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

/*
 * return -1 if user clicked the close-gadget or an error occured,
 * the number of the button that was pressed (1 for the first,
 * 2 for the second, ...). If the user presses return 1 is returned.
 */
int
wt_entrybox (widget_t *parent,
	char *buf, long buflen,
	const char *title,
	const char *caption,
	...)
{
	widget_t *top, *shell, *vpane, *box1, *box2, *label, *entry, *but;
	va_list args;
	char *butname, *res;
	int x, y;
	long i;

	getpos (parent, &x, &y);

	top = wt_create (wt_top_class, NULL);

	i = 0;
	wt_setopt (top,
		WT_USRVAL, &i,
		WT_EOL);

	shell = wt_create (wt_shell_class, top);

	wt_setopt (shell,
		WT_ACTION_CB, entrybox_close_cb,
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

	label = wt_create (wt_label_class, box1);

	wt_setopt (label,
		WT_LABEL, caption,
		WT_EOL);

	entry = wt_create (wt_getstring_class, box1);

	i = MIN (26, buflen);
	wt_setopt (entry,
		WT_STRING_ADDRESS, buf,
		WT_STRING_LENGTH, &buflen,
		WT_STRING_WIDTH, &i,
		WT_ACTION_CB, entrybox_getstr_cb,
		WT_EOL);

	va_start (args, caption);
	for (i = 1; (butname = va_arg (args, char *)); ++i) {
		but = wt_create (wt_pushbutton_class, box2);
		wt_setopt (but,
			WT_USRVAL, &i,
			WT_LABEL, butname,
			WT_ACTION_CB, entrybox_button_cb,
			WT_EOL);
	}
	va_end (args);

	wt_realize (top);
	wt_getfocus (entry);

	while (!wt_do_event ()) {
		wt_getopt (top, WT_USRVAL, &i, WT_EOL);
		if (i != 0)
			break;
	}
	/*
	 * copy the edited string back to the user buffer
	 */
	wt_getopt (entry, WT_STRING_ADDRESS, &res, WT_EOL);
	strncpy (buf, res, buflen);
	buf[buflen-1] = '\0';

	wt_getopt (top, WT_USRVAL, &i, WT_EOL);
	wt_delete (top);

	return i;
}
