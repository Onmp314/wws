/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * box widget.
 *
 * $Id: box.c,v 1.2 2000-09-10 15:21:08 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef struct {
	widget_t w;
	short is_open;
	short is_realized;
	short orient;
	short xdist, ydist;
	short usrwd, usrht;
} box_widget_t;

static long box_query_geometry (widget_t *, long *, long *, long *, long *);

static void
layout (box_widget_t *w)
{
	widget_t *child;
	long curx, cury, ch, cw, wd, ht;

	if (w->orient == OrientHorz) {
		long maxx, maxh;

		maxx = w->w.w <= 0 ? LONG_MAX : w->w.w;
		curx = w->xdist;
		cury = w->ydist;
		maxh = 0;
		wd = 0;
		ht = 0;

		for (child = w->w.childs; child; child = child->next) {
			wt_geometry (child, NULL, NULL, &cw, &ch);
			if (maxh > 0 && curx+cw >= maxx) {
				cury += maxh + w->ydist;
				curx = w->xdist;
				maxh = 0;
			}
			wt_reshape (child, curx, cury, WT_UNSPEC, WT_UNSPEC);
			if (curx + cw > wd)
				wd = curx + cw;
			if (cury + ch > ht)
				ht = cury + ch;

			curx += cw + w->xdist;
			if (ch > maxh)
				maxh = ch;
		}
	} else {
		long maxy, maxw;

		maxy = w->w.h <= 0 ? LONG_MAX : w->w.h;
		curx = w->xdist;
		cury = w->ydist;
		maxw = 0;
		wd = 0;
		ht = 0;

		for (child = w->w.childs; child; child = child->next) {
			wt_geometry (child, NULL, NULL, &cw, &ch);
			if (maxw > 0 && cury+ch >= maxy) {
				curx += maxw + w->xdist;
				cury = w->ydist;
				maxw = 0;
			}
			wt_reshape (child, curx, cury, WT_UNSPEC, WT_UNSPEC);
			if (curx + cw > wd)
				wd = curx + cw;
			if (cury + ch > wd)
				wd = cury + ch;

			cury += ch + w->ydist;
			if (cw > maxw)
				maxw = cw;
		}
	}
}

static long
box_init (void)
{
	return 0;
}

static widget_t *
box_create (widget_class_t *cp)
{
	box_widget_t *wp = malloc (sizeof (box_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (box_widget_t));
	wp->w.class = wt_box_class;
	wp->xdist = 6;
	wp->ydist = 6;
	wp->orient = OrientHorz;
	return (widget_t *)wp;
}

static long
box_delete (widget_t *_w)
{
	box_widget_t *w = (box_widget_t *)_w;
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
box_close (widget_t *_w)
{
	box_widget_t *w = (box_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
box_open (widget_t *_w)
{
	box_widget_t *w = (box_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
box_addchild (widget_t *parent, widget_t *w)
{
	widget_t *wp;

	for (wp = parent->childs; wp && wp->next; wp = wp->next)
		;
	wt_add_after (parent, wp, w);
	return 0;
}

static long
box_delchild (widget_t *parent, widget_t *w)
{
	wt_remove (w);
	return 0;
}

static long
box_realize (widget_t *_w, WWIN *parent)
{
	box_widget_t *w = (box_widget_t *)_w;
	widget_t *wp;
	long wd, ht, x, y;

	if (w->is_realized)
		return -1;

	box_query_geometry (_w, &x, &y, &wd, &ht);

	w->w.w = wd;
	w->w.h = ht;

	w->w.win = wt_create_window (parent, wd, ht,
		W_NOBORDER|W_CONTAINER|W_MOVE, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;
	layout (w);
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
box_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	box_widget_t *w = (box_widget_t *)_w;
	
	*xp = w->w.x;
	*yp = w->w.y;
	if (w->w.w > 0 && w->w.h > 0) {
		*wdp = w->w.w;
		*htp = w->w.h;
	} else {
		(*_w->class->query_minsize) (_w, wdp, htp);
	}
	return 0;
}

static long
box_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	box_widget_t *w = (box_widget_t *)_w;
	widget_t *child;
	long curx, cury, ch, cw;

	if (w->orient == OrientHorz) {
		long maxx, maxh;

		maxx = w->usrwd <= 0 ? LONG_MAX : w->usrwd;
		curx = w->xdist;
		cury = w->ydist;
		maxh = 0;
		*wdp = 0;
		*htp = 0;

		for (child = w->w.childs; child; child = child->next) {
			wt_geometry (child, NULL, NULL, &cw, &ch);
			if (maxh > 0 && curx+cw >= maxx) {
				cury += maxh + w->ydist;
				curx = w->xdist;
				maxh = 0;
			}
			if (curx + cw > *wdp)
				*wdp = curx + cw;
			if (cury + ch > *htp)
				*htp = cury + ch;

			curx += cw + w->xdist;
			if (ch > maxh)
				maxh = ch;
		}
		*wdp = (*wdp > w->xdist) ? *wdp + w->xdist : 10;
		*htp = (*htp > w->ydist) ? *htp + w->ydist : 10;
	} else {
		long maxy, maxw;

		maxy = w->usrht <= 0 ? LONG_MAX : w->usrht;
		curx = w->xdist;
		cury = w->ydist;
		maxw = 0;
		*wdp = 0;
		*htp = 0;

		for (child = w->w.childs; child; child = child->next) {
			wt_geometry (child, NULL, NULL, &cw, &ch);
			if (maxw > 0 && cury+ch >= maxy) {
				curx += maxw + w->xdist;
				cury = w->ydist;
				maxw = 0;
			}
			if (curx + cw > *wdp)
				*wdp = curx + cw;
			if (cury + ch > *htp)
				*htp = cury + ch;

			cury += ch + w->ydist;
			if (cw > maxw)
				maxw = cw;
		}
		*wdp = (*wdp > w->xdist) ? *wdp + w->xdist : 10;
		*htp = (*htp > w->ydist) ? *htp + w->ydist : 10;	
	}
	*wdp = MAX (w->usrwd, *wdp);
	*htp = MAX (w->usrht, *htp);
	return 0;
}

static long
box_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	box_widget_t *w = (box_widget_t *)_w;
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
			layout (w);
		}
		ret = 1;
	}
	return ret;
}

static long
box_setopt (widget_t *_w, long key, void *val)
{
	box_widget_t *w = (box_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (box_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (box_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (box_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (box_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_ORIENTATION:
		w->orient = *(long *)val;
		break;

	case WT_HDIST:
		w->xdist = *(long *)val;
		break;

	case WT_VDIST:
		w->ydist = *(long *)val;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
box_getopt (widget_t *_w, long key, void *val)
{
	box_widget_t *w = (box_widget_t *)_w;

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

	case WT_ORIENTATION:
		*(long *)val = w->orient;
		break;

	case WT_HDIST:
		*(long *)val = w->xdist;
		break;

	case WT_VDIST:
		*(long *)val = w->ydist;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
box_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long
box_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_box_class = {
	"box", 0,
	box_init,
	box_create,
	box_delete,
	box_close,
	box_open,
	box_addchild,
	box_delchild,
	box_realize,
	box_query_geometry,
	box_query_minsize,
	box_reshape,
	box_setopt,
	box_getopt,
	box_event,
	box_changes,
	box_changes
};

widget_class_t *wt_box_class = &_wt_box_class;
