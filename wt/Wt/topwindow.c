/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * $Id: topwindow.c,v 1.2 2008-08-24 18:06:46 eero Exp $
 *
 * window widget. A toplevel window, which has no childs and which
 * parent *should* be the top widget returned by wt_init().
 *
 * The window size, event mask and handler *have to* be set before
 * widget is realized!
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef struct {
	widget_t w;
	char *title;
	short is_open;
	short is_realized;
	short is_hidden;		/* not opened at wt_realize? */
	short flags;
	void (*draw_cb) (WWIN *w, short width, short height);
	WEVENT *(*event_cb) (widget_t *w, WEVENT *ev);
	short usrwd, usrht;
} window_widget_t;


static long window_query_geometry (widget_t *, long *, long *, long *, long *);

static long
window_init (void)
{
	return 0;
}

static widget_t *
window_create (widget_class_t *cp)
{
	window_widget_t *wp = malloc (sizeof (window_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (window_widget_t));
	wp->w.x = UNDEF;
	wp->w.y = UNDEF;
	wp->w.class = wt_topwindow_class;
	return (widget_t *)wp;
}

static long
window_delete (widget_t *_w)
{
	window_widget_t *w = (window_widget_t *)_w;

	if (w->is_realized)
		w_delete (w->w.win);
	free (w);
	return 0;
}

static long
window_close (widget_t *_w)
{
	window_widget_t *w = (window_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
window_open (widget_t *_w)
{
	window_widget_t *w = (window_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
window_addchild (widget_t *parent, widget_t *w)
{
	return 0;
}

static long
window_delchild (widget_t *parent, widget_t *w)
{
	return 0;
}

static long
window_realize (widget_t *_w, WWIN *parent)
{
	window_widget_t *w = (window_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized || !(w->flags && w->event_cb))
		return -1;

	window_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	if (w->w.w < 1 || w->w.h < 1)
		return -1;

	w->w.win = wt_create_window (parent, w->w.w, w->w.h, w->flags, _w);
	if (!w->w.win)
		return -1;

	w->w.win->user_val = (long)w;

	if (w->title)
		w_settitle (w->w.win, w->title);

	if (w->draw_cb)
		w->draw_cb(w->w.win, w->w.w, w->w.h);

	if(!w->is_hidden)
	{
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}

	w->is_realized = 1;
	return 0;
}

static long
window_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	window_widget_t *w = (window_widget_t *)_w;

	*xp  = w->w.x;
	*yp  = w->w.y;
	if (w->w.w > 0 && w->w.h > 0) {
		*wdp = w->w.w;
		*htp = w->w.h;
	} else {
		(*_w->class->query_minsize) (_w, wdp, htp);
	}
	w->w.w = *wdp;
	w->w.h = *htp;
	return 0;
}

static long
window_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	window_widget_t *w = (window_widget_t *)_w;
	
	*wdp = w->usrwd;
	*htp = w->usrht;
	return 0;
}

static long
window_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	window_widget_t *w = (window_widget_t *)_w;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized)
			w_move (w->w.win, x, y);
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		if (w->is_realized)
			w_resize (w->w.win, wd, ht);
		w->w.w = wd;
		w->w.h = ht;
		ret = 1;
	}
	return ret;
}

static long
window_setopt (widget_t *_w, long key, void *val)
{
	window_widget_t *w = (window_widget_t *)_w;

	switch (key) {
	case WT_XPOS:
		window_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h);
		break;

	case WT_YPOS:
		window_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h);
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		window_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h);
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		window_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht);
		break;

	case WT_WINDOW_HIDDEN:
		w->is_hidden = *(long *)val;
		break;
		
	case WT_LABEL:
		w->title = (char *)val;
		if (w->is_realized)
			w_settitle (w->w.win, w->title);
		break;

	case WT_EVENT_MASK:
		w->flags = *(long *)val;
		break;

	case WT_DRAW_FN:
		w->draw_cb = val;
		break;

	case WT_EVENT_CB:
		w->event_cb = val;
		break;

	default:
		return -1;
	}
	return 0;
}

static long
window_getopt (widget_t *_w, long key, void *val)
{
	window_widget_t *w = (window_widget_t *)_w;

	switch (key) {
	case WT_XPOS:
		*(long *)val = w->w.x;
		break;

	case WT_YPOS:
		*(long *)val = w->w.y;
		break;

	case WT_WIDTH:
		*(long *)val = w->w.w;
		break;

	case WT_HEIGHT:
		*(long *)val = w->w.h;
		break;

	case WT_WINDOW_HIDDEN:
		*(long *)val = w->is_hidden;
		break;

	case WT_LABEL:
		*(char **)val = w->title;
		break;

	case WT_EVENT_MASK:
		*(long *)val = w->flags;
		break;

	case WT_DRAW_FN:
		*(void **)val = w->draw_cb;
		break;

	case WT_EVENT_CB:
		*(void **)val = w->event_cb;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
window_event (widget_t *w, WEVENT *ev)
{
	return ((window_widget_t *)w)->event_cb(w, ev);
}

static long
window_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_window_class = {
	"window", 0,
	window_init,
	window_create,
	window_delete,
	window_close,
	window_open,
	window_addchild,
	window_delchild,
	window_realize,
	window_query_geometry,
	window_query_minsize,
	window_reshape,
	window_setopt,
	window_getopt,
	window_event,
	window_changes,
	window_changes
};

widget_class_t *wt_topwindow_class = &_wt_window_class;
