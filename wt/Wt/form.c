/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * form widget.
 *
 * $Id: form.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
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
	short usrwd, usrht;
} form_widget_t;


static long form_query_geometry (widget_t *, long *, long *, long *, long *);

static long
form_init (void)
{
	return 0;
}

static widget_t *
form_create (widget_class_t *cp)
{
	form_widget_t *wp = malloc (sizeof (form_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (form_widget_t));
	wp->w.class = wt_form_class;
	return (widget_t *)wp;
}

static long
form_delete (widget_t *_w)
{
	form_widget_t *w = (form_widget_t *)_w;
	widget_t *wp, *next;

	for (wp = w->w.childs; wp; wp = next) {
		next = wp->next;
		(*wp->class->delete) (wp);
	}
	if (w->is_realized)
		w_delete (w->w.win);
	free (w);
	return 0;
}

static long
form_close (widget_t *_w)
{
	form_widget_t *w = (form_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
form_open (widget_t *_w)
{
	form_widget_t *w = (form_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
form_addchild (widget_t *parent, widget_t *w)
{
	wt_add_before (parent, parent->childs, w);
	return 0;
}

static long
form_delchild (widget_t *parent, widget_t *w)
{
	wt_remove (w);
	return 0;
}

static long
form_realize (widget_t *_w, WWIN *parent)
{
	form_widget_t *w = (form_widget_t *)_w;
	widget_t *wp;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	form_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	w->w.win = wt_create_window (parent, wd, ht,
		W_MOVE|W_CONTAINER|W_NOBORDER, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;
	for (wp = w->w.childs; wp; wp = wp->next) {
		if ((*wp->class->realize) (wp, w->w.win) < 0) {
			w_delete (w->w.win);
			return -1;
		}
	}
	w->is_realized = 1;
	w->is_open = 1;
	w_open (w->w.win, w->w.x, w->w.y);
	return 0;
}

static long
form_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	form_widget_t *w = (form_widget_t *)_w;

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
form_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	form_widget_t *w = (form_widget_t *)_w;
	widget_t *wp;
	long x, y, wd, ht;

	*wdp = 0;
	*htp = 0;
	for (wp = w->w.childs; wp; wp = wp->next) {
		(*wp->class->query_geometry) (wp, &x, &y, &wd, &ht);

		if (x+wd > *wdp)
			*wdp = x+wd;
		if (y+ht > *htp)
			*htp = y+ht;
	}
	if (*wdp < MAX (10, w->usrwd))
		*wdp = MAX (10, w->usrwd);
	if (*htp < MAX (10, w->usrht))
		*htp = MAX (10, w->usrht);
	return 0;
}

static long
form_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	form_widget_t *w = (form_widget_t *)_w;
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
form_setopt (widget_t *_w, long key, void *val)
{
	form_widget_t *w = (form_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (form_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (form_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (form_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (form_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
form_getopt (widget_t *_w, long key, void *val)
{
	form_widget_t *w = (form_widget_t *)_w;

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

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
form_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long
form_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_form_class = {
	"form", 0,
	form_init,
	form_create,
	form_delete,
	form_close,
	form_open,
	form_addchild,
	form_delchild,
	form_realize,
	form_query_geometry,
	form_query_minsize,
	form_reshape,
	form_setopt,
	form_getopt,
	form_event,
	form_changes,
	form_changes
};

widget_class_t *wt_form_class = &_wt_form_class;
