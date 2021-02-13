/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * viewport widget.
 *
 * $Id: viewport.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

/*
 * scrollbar parameter scale factor
 */
#define SLSCALE	ScrollbarScale

/*
 * changes mask
 */
#define VP_SIZE		0x01
#define VP_CSIZE	0x02
#define VP_CPOS		0x04

typedef struct {
	widget_t w;
	uchar is_open;
	uchar is_realized;
	uchar is_locked;
	uchar life_scroll;
	int fill;
	short sb_min_wd;
	short sb_min_ht;
	long  cx, cy, cw, ch;
	widget_t *vscroll;
	widget_t *hscroll;
	widget_t *child;
	WWIN *mainwin;
	short usrwd, usrht;
} viewp_widget_t;


static long viewp_query_geometry (widget_t *w, long *, long *, long *, long *);

static void
hscroll_callback (widget_t *w, int pos, int pressed)
{
	viewp_widget_t *vp = (viewp_widget_t *)w->parent;
	long x;

	if (pressed && !vp->life_scroll)
		return;

	x = - pos;
	if (x != vp->cx && vp->child) {
		vp->is_locked = 1;
		wt_reshape (vp->child, x, WT_UNSPEC, WT_UNSPEC, WT_UNSPEC);
		vp->cx = x;
		vp->is_locked = 0;
	}
}

static void
vscroll_callback (widget_t *w, int pos, int pressed)
{
	viewp_widget_t *vp = (viewp_widget_t *)w->parent;
	long y;

	if (pressed && !vp->life_scroll)
		return;

	y = - pos;
	if (y != vp->cy && vp->child) {
		vp->is_locked = 1;
		wt_reshape (vp->child, WT_UNSPEC, y, WT_UNSPEC, WT_UNSPEC);
		vp->cy = y;
		vp->is_locked = 0;
	}
}

static void
viewp_config (viewp_widget_t *w, short changes)
{
	long x, y, wd, ht;
	widget_t *child;

	w->is_locked = 1;

	if (changes & VP_SIZE) {
		wd = w->w.w - w->sb_min_wd + 1;
		ht = w->w.h - w->sb_min_wd + 1;
		wt_reshape (w->vscroll, wd, 0, WT_UNSPEC, ht);
		wt_reshape (w->hscroll, 0, ht, wd, WT_UNSPEC);
		/*
		 * resize w->w.win and w->mainwin
		 */
	}
	if ((child = w->child)) {
		(*child->class->query_geometry) (child, &x, &y, &wd, &ht);
		if (x > 0) {
			wt_reshape (child, 0, WT_UNSPEC, WT_UNSPEC, WT_UNSPEC);
		} else if (x < w->w.w - w->sb_min_wd - wd) {
			x = MIN (0, w->w.w - w->sb_min_wd - wd);
			wt_reshape (child, x, WT_UNSPEC, WT_UNSPEC, WT_UNSPEC);
		}
		if (y > 0) {
			wt_reshape (child, WT_UNSPEC, 0, WT_UNSPEC, WT_UNSPEC);
		} else if (y < w->w.h - w->sb_min_wd - ht) {
			y = MIN (0, w->w.h - w->sb_min_wd - ht);
			wt_reshape (child, WT_UNSPEC, y, WT_UNSPEC, WT_UNSPEC);
		}
		w->cx = x;
		w->cy = y;
		w->cw = wd;
		w->ch = ht;
	} else {
		w->cx = 0;
		w->cy = 0;
		w->cw = w->w.w - w->sb_min_wd;
		w->ch = w->w.h - w->sb_min_wd;
	}

	wd = w->w.w - w->sb_min_wd;
	ht = w->w.h - w->sb_min_wd;

	if (changes & (VP_CSIZE|VP_SIZE)) {
		x = w->cw;
		y = w->ch;

		(*wt_scrollbar_class->setopt) (w->hscroll, WT_TOTAL_SIZE, &x);
		(*wt_scrollbar_class->setopt) (w->vscroll, WT_TOTAL_SIZE, &y);

		(*wt_scrollbar_class->setopt) (w->hscroll, WT_SIZE, &wd);
		(*wt_scrollbar_class->setopt) (w->vscroll, WT_SIZE, &ht);

		(*wt_scrollbar_class->setopt) (w->hscroll, WT_PAGE_INC, &wd);
		(*wt_scrollbar_class->setopt) (w->vscroll, WT_PAGE_INC, &ht);

		x = wd / 4;
		y = ht / 4;

		(*wt_scrollbar_class->setopt) (w->hscroll, WT_LINE_INC, &x);
		(*wt_scrollbar_class->setopt) (w->vscroll, WT_LINE_INC, &y);
	}
	if (changes & (VP_CPOS|VP_SIZE)) {
		x = - w->cx;
		y = - w->cy;

		(*wt_scrollbar_class->setopt) (w->hscroll, WT_POSITION, &x);
		(*wt_scrollbar_class->setopt) (w->vscroll, WT_POSITION, &y);
	}
	w->is_locked = 0;
}

static int
reshape_child (viewp_widget_t *w)
{
	long wd, ht;
	int mask = 0;

	wd = MAX (w->w.w - w->sb_min_wd, w->sb_min_ht);
	ht = MAX (w->w.h - w->sb_min_wd, w->sb_min_ht);

	if (w->child) switch (w->fill) {
	case AlignFillHorz:
		ht = WT_UNSPEC;
		mask |= VP_CSIZE|VP_CPOS;
		break;
	case AlignFillVert:
		wd = WT_UNSPEC;
		mask |= VP_CSIZE|VP_CPOS;
		break;
	case AlignFill:
		mask |= VP_CSIZE|VP_CPOS;
		break;
	}
	if (mask & VP_CSIZE) {
		wt_reshape (w->child, WT_UNSPEC, WT_UNSPEC, wd, ht);
	}
	return mask;
}

static long
viewp_init (void)
{
	return 0;
}

static widget_t *
viewp_create (widget_class_t *cp)
{
	long i;

	viewp_widget_t *wp = malloc (sizeof (viewp_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (viewp_widget_t));
	wp->w.class = wt_viewport_class;

	wp->life_scroll = ViewpModeLazyScroll;

	wp->hscroll = wt_create (wt_scrollbar_class, (widget_t *)wp);
	if (!wp->hscroll) {
		free (wp);
		return NULL;
	}

	wp->vscroll = wt_create (wt_scrollbar_class, (widget_t *)wp);
	if (!wp->vscroll) {
		wt_delete (wp->hscroll);
		free (wp);
		return NULL;
	}

	i = OrientHorz;
	(*wt_scrollbar_class->setopt) (wp->hscroll, WT_ORIENTATION, &i);

	/*
	 * query minimum extents of the scrollbar
	 */
	(*wt_scrollbar_class->getopt) (wp->vscroll, WT_HEIGHT, &i);
	wp->sb_min_ht = i;

	(*wt_scrollbar_class->getopt) (wp->vscroll, WT_WIDTH, &i);
	wp->sb_min_wd = i+1;

	(*wt_scrollbar_class->setopt) (wp->vscroll, WT_ACTION_CB,
		vscroll_callback);
	(*wt_scrollbar_class->setopt) (wp->hscroll, WT_ACTION_CB,
		hscroll_callback);

	return (widget_t *)wp;
}

static long
viewp_delete (widget_t *_w)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;
	widget_t *wp, *next;

	for (wp = w->w.childs; wp; wp = next) {
		next = wp->next;
		(*wp->class->delete) (wp);
	}
	if (w->is_realized) {
		w_delete (w->w.win);
		w_delete (w->mainwin);
	}
	free (w);
	return 0;
}

static long
viewp_close (widget_t *_w)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->mainwin);
		w->is_open = 0;
	}
	return 0;
}

static long
viewp_open (widget_t *_w)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->mainwin, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
viewp_addchild (widget_t *_parent, widget_t *w)
{
	viewp_widget_t *parent = (viewp_widget_t *)_parent;

	wt_add_before (_parent, _parent->childs, w);
	if (w != parent->hscroll && w != parent->vscroll) {
		parent->child = w;
		if (parent->is_realized)
			viewp_config (parent, VP_CSIZE|VP_CPOS);
	}
	return 0;
}

static long
viewp_delchild (widget_t *_parent, widget_t *w)
{
	viewp_widget_t *parent = (viewp_widget_t *)_parent;

	wt_remove (w);
	if (w == parent->child) {
		parent->child = NULL;
		if (parent->is_realized)
			viewp_config (parent, VP_CSIZE|VP_CPOS);
	}
	return 0;
}

static long
viewp_realize (widget_t *_w, WWIN *parent)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	viewp_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	wd -= w->sb_min_wd;
	ht -= w->sb_min_wd;

	w->mainwin = wt_create_window (parent, w->w.w, w->w.h,
		W_MOVE|W_NOBORDER|W_CONTAINER, _w);
	if (!w->mainwin)
		return -1;

	w->w.win = wt_create_window (w->mainwin, wd, ht,
		W_MOVE|W_NOBORDER|W_CONTAINER, _w);
	if (!w->w.win) {
		w_delete (w->mainwin);
		return -1;
	}
	w->w.win->user_val = (long)w;

	reshape_child (w);
	viewp_config (w, VP_SIZE|VP_CSIZE|VP_CPOS);

	if (w->child) {
		(*w->child->class->realize) (w->child, w->w.win);
	}
	(*wt_scrollbar_class->realize) (w->hscroll, w->mainwin);
	(*wt_scrollbar_class->realize) (w->vscroll, w->mainwin);

	w->is_realized = 1;
	w->is_open = 1;
	w_open (w->w.win, 0, 0);
	w_open (w->mainwin, w->w.x, w->w.y);
	return 0;
}

static long
viewp_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;
	
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
viewp_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;

	*wdp = *htp = 50;

	*wdp = MAX (w->sb_min_ht, *wdp);
	*htp = MAX (w->sb_min_ht, *wdp);
	*wdp = MAX (*wdp + w->sb_min_wd, w->usrwd);
	*htp = MAX (*wdp + w->sb_min_wd, w->usrht);
	return 0;
}

static long
viewp_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized) {
			w_move (w->mainwin, x, y);
		}
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		long minwd, minht;
		wt_minsize (_w, &minwd, &minht);
		if (wd < minwd)
			wd = minwd;
		if (ht < minht)
			ht = minht;
		if (wd != w->w.w || ht != w->w.h) {
			int mask = VP_SIZE;
			w->w.w = wd;
			w->w.h = ht;
			if (w->is_realized)
				w_resize (w->mainwin, wd, ht);
			wd = MAX (wd - w->sb_min_wd, w->sb_min_ht);
			ht = MAX (ht - w->sb_min_wd, w->sb_min_ht);
			if (w->is_realized)
				w_resize (w->w.win, wd, ht);

			mask |= reshape_child (w);
			if (w->is_realized)
				viewp_config (w, mask);
			ret = 1;
		}
	}
	return ret;
}

static long
viewp_setopt (widget_t *_w, long key, void *val)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (viewp_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (viewp_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (viewp_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (viewp_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_MODE:
		w->life_scroll = *(long *)val;
		break;

	case WT_ALIGNMENT:
		w->fill = *(long *)val;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
viewp_getopt (widget_t *_w, long key, void *val)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;

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

	case WT_MODE:
		*(long *)val = w->life_scroll;
		break;

	case WT_ALIGNMENT:
		*(long *)val = w->fill;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT*
viewp_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long
viewp_child_change (widget_t *_w, widget_t *child, short changes)
{
	viewp_widget_t *w = (viewp_widget_t *)_w;
	short mask = 0;

	if (w->child && w->child == child && w->is_realized && !w->is_locked) {
		if (changes & WT_CHANGED_SIZE)
			mask |= VP_CSIZE;
		if (changes & WT_CHANGED_POS)
			mask |= VP_CPOS;
		viewp_config (w, mask);
	}
	return 0;
}

static long
viewp_parent_change (widget_t *_w, widget_t *parent, short changes)
{
	return 0;
}

static widget_class_t _wt_viewp_class = {
	"viewport", 0,
	viewp_init,
	viewp_create,
	viewp_delete,
	viewp_close,
	viewp_open,
	viewp_addchild,
	viewp_delchild,
	viewp_realize,
	viewp_query_geometry,
	viewp_query_minsize,
	viewp_reshape,
	viewp_setopt,
	viewp_getopt,
	viewp_event,
	viewp_child_change,
	viewp_parent_change
};

widget_class_t *wt_viewport_class = &_wt_viewp_class;
