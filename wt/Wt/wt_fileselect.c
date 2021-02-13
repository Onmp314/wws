/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * $Id: wt_fileselect.c,v 1.2 2008-08-29 19:47:09 eero Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
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

/*************************** A fileselector ******************************/

static void
fsel_cb (widget_t *w, char *file)
{
	widget_t *wp;
	char *res;

	if (file) {
		res = strdup (file);
	} else {
		res = NULL;
	}
	for (wp = w; wp->parent; wp = wp->parent)
		;
	wt_setopt (wp, WT_USRVAL, &res, WT_EOL);
}

/*
 * returns NULL if selection way aborted or failed, a pointer to
 * a malloc()ed area that holds the selected path/file otherwise.
 */
char *
wt_fileselect (widget_t *parent, const char *title, const char *path,
	       const char *mask, const char *file)
{
	widget_t *top, *fsel;
	int x, y;
	char *res;
	long i;

	getpos (parent, &x, &y);

	top = wt_create (wt_top_class, NULL);

	i = 1;
	wt_setopt (top,
		WT_USRVAL, &i,
		WT_EOL);

	fsel = wt_create (wt_filesel_class, top);

	wt_setopt (fsel,
		WT_ACTION_CB, fsel_cb,
		WT_LABEL, title ?: "",
		WT_XPOS, &x,
		WT_YPOS, &y,
		WT_FILESEL_PATH, path ?: "/",
		WT_FILESEL_MASK, mask ?: "*",
		WT_FILESEL_FILE, file ?: "",
		WT_ACTION_CB, fsel_cb,
		WT_EOL);

	wt_realize (top);
	while (!wt_do_event ()) {
		wt_getopt (top, WT_USRVAL, &i, WT_EOL);
		if (i != 1)
			break;
	}
	wt_getopt (top, WT_USRVAL, &res, WT_EOL);
	wt_delete (top);

	return res;
}
