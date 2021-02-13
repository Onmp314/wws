/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * drawable widget.
 *
 * $Id: drawable.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef struct {
	widget_t w;
	short is_open;
	short is_realized;
	short evmask;
	void (*draw_fn) (widget_t *, long x, long y, long wd, long ht);
	WEVENT *(*event_cb) (widget_t *, WEVENT *ev);
	short usrwd, usrht;
} drawable_widget_t;


static long drawable_query_geometry (widget_t *, long *, long *, long *, long *);

static long
drawable_init (void)
{
	return 0;
}

static widget_t *
drawable_create (widget_class_t *cp)
{
	drawable_widget_t *wp = malloc (sizeof (drawable_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (drawable_widget_t));
	wp->w.class = wt_drawable_class;
	return (widget_t *)wp;
}

static long
drawable_delete (widget_t *_w)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;

	if (w->is_realized)
		w_delete (w->w.win);
	free (w);
	return 0;
}

static long
drawable_close (widget_t *_w)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
drawable_open (widget_t *_w)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
drawable_addchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
drawable_delchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
drawable_realize (widget_t *_w, WWIN *parent)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	drawable_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	w->w.win = wt_create_window (parent, wd, ht,
			W_NOBORDER|W_MOVE|w->evmask, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;

	w->is_realized = 1;
	w->is_open = 1;
	if (w->draw_fn)
		(*w->draw_fn) (_w, 0, 0, w->w.w, w->w.h);
	w_open (w->w.win, w->w.x, w->w.y);
	return 0;
}

static long
drawable_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;

	*xp = w->w.x;
	*yp = w->w.y;
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
drawable_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;

	*wdp = MAX (w->usrwd, 16);
	*htp = MAX (w->usrht, 16);
	return 0;
}

static long
drawable_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized)
			w_move (w->w.win, x, y);
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		w->w.w = wd;
		w->w.h = ht;
		if (w->is_realized) {
			w_resize (w->w.win, wd, ht);
			if (w->draw_fn)
				(*w->draw_fn) (_w, 0, 0, w->w.w, w->w.h);
		}
		ret = 1;
	}
	return ret;
}

static long
drawable_setopt (widget_t *_w, long key, void *val)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (drawable_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (drawable_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (drawable_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (drawable_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_EVENT_CB:
		w->event_cb = val;
		break;

	case WT_EVENT_MASK:
		w->evmask = *(long *)val & (EV_KEYS|EV_MOUSE|EV_ACTIVE);
		break;

	case WT_DRAW_FN:
		w->draw_fn = val;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
drawable_getopt (widget_t *_w, long key, void *val)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;

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

	case WT_EVENT_CB:
		*(void **)val = w->event_cb;
		break;

	case WT_EVENT_MASK:
		*(long *)val = w->evmask;
		break;

	case WT_DRAW_FN:
		*(void **)val = w->draw_fn;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
drawable_event (widget_t *_w, WEVENT *ev)
{
	drawable_widget_t *w = (drawable_widget_t *)_w;

	if (w->event_cb)
		return (*w->event_cb) (_w, ev);
	return ev;
}

static long
drawable_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_drawable_class = {
	"drawable", 0,
	drawable_init,
	drawable_create,
	drawable_delete,
	drawable_close,
	drawable_open,
	drawable_addchild,
	drawable_delchild,
	drawable_realize,
	drawable_query_geometry,
	drawable_query_minsize,
	drawable_reshape,
	drawable_setopt,
	drawable_getopt,
	drawable_event,
	drawable_changes,
	drawable_changes
};

widget_class_t *wt_drawable_class = &_wt_drawable_class;
