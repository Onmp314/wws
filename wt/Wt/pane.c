/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * pane widget.
 *
 * - Fixed a slight mismatch in aligning. ++eero 8/1996
 *
 * $Id: pane.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
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
	short align;
	short dist;
	short usrwd, usrht;
} pane_widget_t;

static long pane_query_geometry (widget_t *, long *, long *, long *, long *);
static long pane_query_minsize (widget_t *, long *, long *);

static void
layout (pane_widget_t *w)
{
	long wd, ht, curx, cury, space, nchilds;
	widget_t *wp;

	for (nchilds=0, wp = w->w.childs; wp; ++nchilds, wp = wp->next)
		;
	if (w->orient == OrientHorz) {
		/*
		 * horizontal
		 */
		wt_minsize ((widget_t *)w, &space, NULL);
		if ((space = w->w.w - space) < 0)
			space = 0;
		curx = cury = 0;
		for (wp = w->w.childs; wp; wp = wp->next) {
			long minwd;
			wt_minsize (wp, &minwd, NULL);
			wd = minwd + space/nchilds;
			ht = (w->align == AlignFill) ? w->w.h : WT_UNSPEC;
			wt_reshape (wp, WT_UNSPEC, WT_UNSPEC, wd, ht);
			wt_geometry (wp, NULL, NULL, &wd, &ht);
			switch (w->align) {
			case AlignTop:
			case AlignFill:
				cury = 0;
				break;
			case AlignBottom:
				cury = w->w.h - ht;
				break;
			case AlignCenter:
			default:
				cury = (w->w.h - ht)/2;
				break;
			}
			wt_reshape (wp, curx, cury, WT_UNSPEC, WT_UNSPEC);
			curx += wd + w->dist;
			if ((space -= (wd - minwd)) < 0)
				space = 0;
			--nchilds;
		}
	} else {
		/*
		 * vertical
		 */
		wt_minsize ((widget_t *)w, NULL, &space);
		if ((space = w->w.h - space) < 0)
			space = 0;
		curx = cury = 0;
		for (wp = w->w.childs; wp; wp = wp->next) {
			long minht;
			wt_minsize (wp, NULL, &minht);
			ht = minht + space/nchilds;
			wd = (w->align == AlignFill) ? w->w.w : WT_UNSPEC;
			wt_reshape (wp, WT_UNSPEC, WT_UNSPEC, wd, ht);
			wt_geometry (wp, NULL, NULL, &wd, &ht);
			switch (w->align) {
			case AlignLeft:
			case AlignFill:
				curx = 0;
				break;
			case AlignRight:
				curx = w->w.w - wd;
				break;
			case AlignCenter:
			default:
				curx = (w->w.w - wd)/2;
				break;
			}
			wt_reshape (wp, curx, cury, WT_UNSPEC, WT_UNSPEC);
			cury += ht + w->dist;
			if ((space -= (ht - minht)) < 0)
				space = 0;
			--nchilds;
		}
	}
}

static long
pane_init (void)
{
	return 0;
}

static widget_t *
pane_create (widget_class_t *cp)
{
	pane_widget_t *wp = malloc (sizeof (pane_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (pane_widget_t));
	wp->w.class = wt_pane_class;
	wp->orient = OrientVert;
	wp->align = AlignCenter;
	wp->dist = 2;
	return (widget_t *)wp;
}

static long
pane_delete (widget_t *_w)
{
	pane_widget_t *w = (pane_widget_t *)_w;
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
pane_close (widget_t *_w)
{
	pane_widget_t *w = (pane_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
pane_open (widget_t *_w)
{
	pane_widget_t *w = (pane_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
pane_addchild (widget_t *parent, widget_t *w)
{
	widget_t *wp;

	for (wp = parent->childs; wp && wp->next; wp = wp->next)
		;
	wt_add_after (parent, wp, w);
	return 0;
}

static long
pane_delchild (widget_t *parent, widget_t *w)
{
	wt_remove (w);
	return 0;
}

static long
pane_realize (widget_t *_w, WWIN *parent)
{
	pane_widget_t *w = (pane_widget_t *)_w;
	long x, y, wd, ht;
	widget_t *wp;

	if (w->is_realized)
		return -1;

	pane_query_geometry (_w, &x, &y, &wd, &ht);
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
pane_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	pane_widget_t *w = (pane_widget_t *)_w;

	*xp = w->w.x;
	*yp = w->w.y;
	if (w->w.w > 0 && w->w.h > 0) {
		*wdp = w->w.w;
		*htp = w->w.h;
		return 0;
	} else {
		(*_w->class->query_minsize) (_w, wdp, htp);
	}
	w->w.w = *wdp;
	w->w.h = *htp;
	return 0;
}

static long
pane_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	pane_widget_t *w = (pane_widget_t *)_w;
	long maxw, maxh, totw, toth, cw, ch;
	widget_t *child;

	maxw = 0;
	maxh = 0;
	totw = 0;
	toth = 0;
	for (child = w->w.childs; child; child = child->next) {
		(*child->class->query_minsize) (child, &cw, &ch);
		if (cw > maxw)
			maxw = cw;
		if (ch > maxh)
			maxh = ch;
		totw += cw + w->dist;
		toth += ch + w->dist;
	}
	totw -= w->dist;
	toth -= w->dist;
	if (w->orient == OrientHorz) {
		/*
		 * horizontal
		 */
		*wdp = totw > 0 ? totw : 10;
		*htp = maxh > 0 ? maxh : 10;
	} else {
		/*
		 * vertical
		 */
		*wdp = maxw > 0 ? maxw : 10;
		*htp = toth > 0 ? toth : 10;
	}
	*wdp = MAX (w->usrwd, *wdp);
	*htp = MAX (w->usrht, *htp);
	return 0;
}

static long
pane_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	pane_widget_t *w = (pane_widget_t *)_w;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized)
			w_move (w->w.win, x, y);
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		if (w->align != AlignFill) {
			long minwd, minht;
			wt_minsize (_w, &minwd, &minht);
			if (w->orient == OrientHorz)
				ht = minht;
			else
				wd = minwd;
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
	}
	return ret;
}

static long
pane_setopt (widget_t *_w, long key, void *val)
{
	pane_widget_t *w = (pane_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (pane_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (pane_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (pane_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (pane_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_ORIENTATION:
		w->orient = *(long *)val;
		break;

	case WT_ALIGNMENT:
		w->align = *(long *)val;
		break;

	case WT_HDIST:
	case WT_VDIST:
		w->dist = *(long *)val;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
pane_getopt (widget_t *_w, long key, void *val)
{
	pane_widget_t *w = (pane_widget_t *)_w;

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

	case WT_ALIGNMENT:
		*(long *)val = w->align;
		break;

	case WT_HDIST:
	case WT_VDIST:
		*(long *)val = w->dist;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
pane_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long
pane_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_pane_class = {
	"pane", 0,
	pane_init,
	pane_create,
	pane_delete,
	pane_close,
	pane_open,
	pane_addchild,
	pane_delchild,
	pane_realize,
	pane_query_geometry,
	pane_query_minsize,
	pane_reshape,
	pane_setopt,
	pane_getopt,
	pane_event,
	pane_changes,
	pane_changes
};

widget_class_t *wt_pane_class = &_wt_pane_class;
