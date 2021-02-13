/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * scrollbar widget.
 *
 * $Id: scrollbar.c,v 1.2 2000/09/10 10:16:51 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

#ifdef WTINYSCREEN
#define SLWD 11
#else
#define SLWD	17
#endif
#define SLHT	(SLWD-1)

typedef struct {
	widget_t w;
	uchar is_open;
	uchar is_realized;
	uchar orient;

	long slscale;
	long slsize;
	long slpos;
	long slsize_abs;
	long slpos_abs;
	long slpos_old;
	long mouse_pos;
	long line_inc;
	long page_inc;
	long  timer;
	enum { Idle, Up, Down, Move, PgUp, PgDown } state;
	void (*move_cb) (widget_t *w, int pos, int pressed);
	short usrwd, usrht;
} scroll_widget_t;


static long scroll_query_geometry (widget_t *, long *, long *, long *, long *);


static inline long
sb_slsize_rel2abs (scroll_widget_t *w, long rel_slsize)
{
	long abs_size = (w->orient == OrientHorz) ? w->w.w : w->w.h;
	long abs_slsize = (abs_size - 2*SLHT) * rel_slsize / w->slscale;
	return MAX (SLHT/2, abs_slsize);
}

static inline long
sb_pos_rel2abs (scroll_widget_t *w, long rel_pos, long abs_slsize)
{
	long abs_size = (w->orient == OrientHorz) ? w->w.w : w->w.h;
	long scale = MAX (1, w->slscale - w->slsize);
	return (abs_size - 2*SLHT - abs_slsize) * rel_pos / scale;
}

static inline long
sb_pos_abs2rel (scroll_widget_t *w, long abs_pos, long abs_slsize)
{
	long abs_size = (w->orient == OrientHorz) ? w->w.w : w->w.h;
	long scale = w->slscale - w->slsize;
	return abs_pos * scale / (abs_size - 2*SLHT - abs_slsize);
}

static void
sb_draw_bar (scroll_widget_t *w)
{
	WWIN *win = w->w.win;

	w_setmode (win, M_CLEAR);
	w_pbox (win, 0, 0, w->w.w, w->w.h);

	wt_box3d (win, 0, 0, w->w.w, w->w.h);
	wt_box3d_press (win, 0, 0, w->w.w, w->w.h, 0);

	if (w->orient == OrientVert) {
		wt_arrow3d (win, 3, 3, SLWD-6, SLHT-4, 0);
		wt_arrow3d (win, 3, w->w.h-3-SLHT+4, SLWD-6, SLHT-4, 1);
	} else {
		wt_arrow3d (win, 3, 3, SLHT-4, SLWD-6, 2);
		wt_arrow3d (win, w->w.w-3-SLHT+4, 3, SLHT-4, SLWD-6, 3);
	}
}

static void
sb_draw_slider (scroll_widget_t *w)
{
	WWIN *win = w->w.win;
	long pos, slsize;

	w->slsize_abs = slsize = sb_slsize_rel2abs (w, w->slsize);
	w->slpos_abs = pos = sb_pos_rel2abs (w, w->slpos, slsize);

	w_setmode (win, M_CLEAR);
	if (w->orient == OrientVert) {
		w_pbox (win, 2, SLHT, SLWD-4, w->w.h - 2*SLHT);
		wt_box3d (win, 3, SLHT+pos, SLWD-6, slsize);
	} else {
		w_pbox (win, SLHT, 2, w->w.w - 2*SLHT, SLWD-4);
		wt_box3d (win, SLHT+pos, 3, slsize, SLWD-6);
	}
}

static void
sb_press_up (scroll_widget_t *w)
{
	WWIN *win = w->w.win;

	if (w->orient == OrientVert) {
		wt_arrow3d_press (win, 3, 3, SLWD-6, SLHT-4, 0);
	} else {
		wt_arrow3d_press (win, 3, 3, SLHT-4, SLWD-6, 2);
	}
}

static void
sb_release_up (scroll_widget_t *w)
{
	WWIN *win = w->w.win;

	if (w->orient == OrientVert) {
		wt_arrow3d_release (win, 3, 3, SLWD-6, SLHT-4, 0);
	} else {
		wt_arrow3d_release (win, 3, 3, SLHT-4, SLWD-6, 2);
	}
}

static void
sb_press_down (scroll_widget_t *w)
{
	WWIN *win = w->w.win;

	if (w->orient == OrientVert) {
		wt_arrow3d_press (win, 3, w->w.h-3-SLHT+4, SLWD-6, SLHT-4, 1);
	} else {
		wt_arrow3d_press (win, w->w.w-3-SLHT+4, 3, SLHT-4, SLWD-6, 3);
	}
}

static void
sb_release_down (scroll_widget_t *w)
{
	WWIN *win = w->w.win;

	if (w->orient == OrientVert) {
		wt_arrow3d_release (win, 3, w->w.h-3-SLHT+4, SLWD-6, SLHT-4,1);
	} else {
		wt_arrow3d_release (win, w->w.w-3-SLHT+4, 3, SLHT-4, SLWD-6,3);
	}
}

static void
sb_press_slider (scroll_widget_t *w)
{
	WWIN *win = w->w.win;
	long slsize = w->slsize_abs;
	long pos = w->slpos_abs;

	if (w->orient == OrientVert) {
		wt_box3d_press (win, 3, SLHT+pos, SLWD-6, slsize, 0);
	} else {
		wt_box3d_press (win, SLHT+pos, 3, slsize, SLWD-6, 0);
	}
}

static void
sb_release_slider (scroll_widget_t *w)
{
	WWIN *win = w->w.win;
	long slsize = w->slsize_abs;
	long pos = w->slpos_abs;

	if (w->orient == OrientVert) {
		wt_box3d_release (win, 3, SLHT+pos, SLWD-6, slsize, 0);
	} else {
		wt_box3d_release (win, SLHT+pos, 3, slsize, SLWD-6, 0);
	}
}

static long
sb_move_slider (scroll_widget_t *w, long abs_pos)
{
	long tsize = w->orient ? w->w.w : w->w.h;

	if (abs_pos < 0)
		abs_pos = 0;
	else if (abs_pos > tsize-2*SLHT-w->slsize_abs)
		abs_pos = tsize-2*SLHT-w->slsize_abs;

	if (abs_pos == w->slpos_abs)
		return -1;

	if (!w->is_realized)
		return abs_pos;

	w_setmode (w->w.win, M_CLEAR);
	if (w->orient == OrientVert) {
		w_bitblk (w->w.win, 3, SLHT+w->slpos_abs,
			SLWD-6, w->slsize_abs,
			3, SLHT+abs_pos);
		if (abs_pos < w->slpos_abs) {
			w_pbox (w->w.win, 3, SLHT+abs_pos+w->slsize_abs,
				SLWD-6, w->slpos_abs - abs_pos);
		} else {
			w_pbox (w->w.win, 3, SLHT+w->slpos_abs,
				SLWD-6, abs_pos - w->slpos_abs);
		}
	} else {
		w_bitblk (w->w.win, SLHT+w->slpos_abs, 3,
			w->slsize_abs, SLWD-6,
			SLHT+abs_pos, 3);
		if (abs_pos < w->slpos_abs) {
			w_pbox (w->w.win, SLHT+abs_pos+w->slsize_abs, 3,
				w->slpos_abs - abs_pos, SLWD-6);
		} else {
			w_pbox (w->w.win, SLHT+w->slpos_abs, 3,
				abs_pos - w->slpos_abs, SLWD-6);
		}
	}
	return abs_pos;
}

static void
sb_move_timer (long arg)
{
	scroll_widget_t *w = (scroll_widget_t *)arg;
	short x, y;
	long r = -1;

	if (w_querymousepos (w->w.win, &x, &y) < 0) {
		w->timer = -1;
		return;
	}
	if (w->orient == OrientVert) {
		if (y != w->mouse_pos) {
			r = sb_move_slider (w, w->slpos_abs + y - w->mouse_pos);
			if (r >= 0) w->mouse_pos = y;
		}
	} else {
		if (x != w->mouse_pos) {
			r = sb_move_slider (w, w->slpos_abs + x - w->mouse_pos);
			if (r >= 0) w->mouse_pos = x;
		}
	}
	if (r >= 0) {
		w->slpos = sb_pos_abs2rel (w, r, w->slsize_abs);
		w->slpos_abs = r;
		if (w->move_cb)
			(*w->move_cb) ((widget_t *)w, w->slpos, 1);
	}
	w->timer = wt_addtimeout (100, sb_move_timer, (long)w);
}

static void
sb_updown_timer (long arg)
{
	scroll_widget_t *w = (scroll_widget_t *)arg;
	short slpos, slpos_abs;

	switch (w->state) {
	case Up:
		slpos = w->slpos - w->line_inc;
		break;
	case Down:
		slpos = w->slpos + w->line_inc;
		break;
	case PgUp:
		slpos = w->slpos - w->page_inc;
		break;
	case PgDown:
		slpos = w->slpos + w->page_inc;
		break;
	default:
		w->timer = -1;
		return;
	}
	if (slpos < 0)
		slpos = 0;
	else if (slpos > w->slscale - w->slsize)
		slpos = w->slscale - w->slsize;

	if (slpos == w->slpos)
		return;

	w->slpos = slpos;
	slpos_abs = sb_pos_rel2abs (w, slpos, w->slsize_abs);
	sb_move_slider (w, slpos_abs);
	w->slpos_abs = slpos_abs;
	if (w->move_cb)
		(*w->move_cb) ((widget_t *)w, slpos, 0);

	w->timer = wt_addtimeout (w->timer < 0 ? 600 : 200,
			sb_updown_timer, (long)w);
}

static void
sb_cancel (scroll_widget_t *w)
{
	switch (w->state) {
	case Up:
		sb_release_up (w);
		break;
	case Down:
		sb_release_down (w);
		break;
	case PgUp:
	case PgDown:
		break;
	case Move:
		sb_release_slider (w);
		if (w->move_cb && w->slpos_old != w->slpos)
			(*w->move_cb) ((widget_t *)w, w->slpos, 0);
		break;
	default:
		return;
	}
	wt_deltimeout (w->timer);
	w->timer = -1;
	w->state = Idle;
}

static long
scroll_init (void)
{
	return 0;
}

static widget_t *
scroll_create (widget_class_t *cp)
{
	scroll_widget_t *wp = malloc (sizeof (scroll_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (scroll_widget_t));
	wp->w.class = wt_scrollbar_class;
	wp->state = Idle;

	wp->slscale  = 1024;
	wp->slsize   = 0;
	wp->slpos    = 0;
	wp->line_inc = 0;
	wp->page_inc = 0;
	wp->orient = OrientVert;
	wp->timer = -1;
	return (widget_t *)wp;
}

static long
scroll_delete (widget_t *_w)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;

	if (w->is_realized) {
		sb_cancel (w);
		w_delete (w->w.win);
	}
	free (w);
	return 0;
}

static long
scroll_close (widget_t *_w)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		sb_cancel (w);
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
scroll_open (widget_t *_w)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
scroll_addchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
scroll_delchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
scroll_realize (widget_t *_w, WWIN *parent)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	if (w->slsize == 0)
		w->slsize = w->slscale;
	if (w->page_inc == 0)
		w->page_inc = w->slsize;
	if (w->line_inc == 0)
		w->line_inc = 1;

	scroll_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.x = x;
	w->w.y = y;
	w->w.w = wd;
	w->w.h = ht;

	w->w.win = wt_create_window (parent, wd, ht,
		W_MOVE|W_NOBORDER|EV_ACTIVE|EV_MOUSE, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;

	sb_draw_bar (w);
	sb_draw_slider (w);

	w->is_realized = 1;
	w->is_open = 1;
	w_open (w->w.win, w->w.x, w->w.y);
	return 0;
}

static long
scroll_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;

	*xp = w->w.x;
	*yp = w->w.y;
	(*_w->class->query_minsize) (_w, wdp, htp);
	if (w->orient == OrientHorz) {
		/*
		 * horizontal
		 */
		*wdp = MAX (*wdp, w->w.w);
	} else {
		/*
		 * vertical
		 */
		*htp = MAX (*htp, w->w.h);
	}
	return 0;
}

static long
scroll_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;

	if (w->orient == OrientHorz) {
		*htp = SLWD;
		*wdp = MAX (3*SLHT, w->usrwd);
	} else {
		*wdp = SLWD;
		*htp = MAX (3*SLHT, w->usrht);
	}
	return 0;
}

static long
scroll_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;
	long minwd, minht;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized)
			w_move (w->w.win, x, y);
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		scroll_query_minsize (_w, &minwd, &minht);
		if (w->orient == OrientHorz) {
			ht = SLWD;
			if (wd < minwd)
				wd = minwd;
		} else {
			wd = SLWD;
			if (ht < minht)
				ht = minht;
		}
		if (wd != w->w.w || ht != w->w.h) {
			w->w.w = wd;
			w->w.h = ht;
			if (w->is_realized) {
				w_resize (w->w.win, wd, ht);
				sb_cancel (w);
				sb_draw_bar (w);
				sb_draw_slider (w);
			}
			ret = 1;
		}
	}
	return ret;
}

static long
scroll_setopt (widget_t *_w, long key, void *val)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;
	long slpos;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (scroll_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (scroll_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (scroll_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (scroll_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_ACTION_CB:
		w->move_cb = val;
		break;

	case WT_ORIENTATION:
		if (w->is_realized) {
			return -1;
		}
		w->orient = *(long *)val;
		break;

	case WT_SIZE:
		if (w->slsize == *(long *)val)
			break;
		w->slsize = *(long *)val;
		if (w->slsize < 0)
			w->slsize = 0;
		else if (w->slsize > w->slscale)
			w->slsize = w->slscale;
		if (w->slpos > w->slscale - w->slsize)
			w->slpos = w->slscale - w->slsize;
		if (w->is_realized) {
			sb_draw_slider (w);
			if (w->state == Move)
				sb_press_slider (w);
		}
		break;

	case WT_POSITION:
		if (w->slpos == *(long *)val)
			break;
		w->slpos = *(long *)val;
		if (w->slpos < 0)
			w->slpos = 0;
		else if (w->slpos > w->slscale - w->slsize)
			w->slpos = w->slscale - w->slsize;
		slpos = sb_pos_rel2abs (w, w->slpos, w->slsize_abs);
		slpos = sb_move_slider (w, slpos);
		if (slpos >= 0) {
			w->slpos_abs = slpos;
#if 0
			if (w->move_cb) {
				(*w->move_cb) ((widget_t *)w, w->slpos,
					w->state == Move);
			}
#endif
		}
		break;

	case WT_LINE_INC:
		w->line_inc = *(long *)val;
		break;

	case WT_PAGE_INC:
		w->page_inc = *(long *)val;
		break;

	case WT_TOTAL_SIZE:
		if (w->slscale == *(long *)val)
			break;
		w->slscale = *(long *)val;
		if (w->slscale < 0)
			w->slscale = 0;
		if (w->slsize > w->slscale)
			w->slsize = w->slscale;
		if (w->slpos > w->slscale - w->slsize)
			w->slpos = w->slscale - w->slsize;
		if (w->is_realized) {
			sb_draw_slider (w);
			if (w->state == Move)
				sb_press_slider (w);
		}
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
scroll_getopt (widget_t *_w, long key, void *val)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;

	switch (key) {
	case WT_XPOS:
		*(long *)val = w->w.x;
		break;

	case WT_YPOS:
		*(long *)val = w->w.y;
		break;

	case WT_WIDTH:
		*(long *)val = w->orient ? MAX (w->w.w, 3*SLHT) : SLWD;
		break;

	case WT_HEIGHT:
		*(long *)val = w->orient ? SLWD : MAX (w->w.h, 3*SLHT);
		break;

	case WT_ACTION_CB:
		*(long *)val = (long)w->move_cb;
		break;

	case WT_ORIENTATION:
		*(long *)val = w->orient;
		break;

	case WT_SIZE:
		*(long *)val = w->slsize;
		break;

	case WT_POSITION:
		*(long *)val = w->slpos;
		break;

	case WT_LINE_INC:
		*(long *)val = w->line_inc;
		break;

	case WT_PAGE_INC:
		*(long *)val = w->page_inc;
		break;

	case WT_TOTAL_SIZE:
		*(long *)val = w->slscale;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
scroll_event (widget_t *_w, WEVENT *ev)
{
	scroll_widget_t *w = (scroll_widget_t *)_w;
	long mpos, tsize;

	switch (ev->type) {
	case EVENT_MPRESS:
		if (!(ev->key & BUTTON_LEFT))
			return ev;

		if (w->state != Idle)
			break;
		if (w->orient == OrientHorz) {
			tsize = w->w.w;
			mpos = ev->x;
		} else {
			tsize = w->w.h;
			mpos = ev->y;
		}
		if (mpos < SLHT) {
			/*
			 * UP button pressed
			 */
			sb_press_up (w);
			w->state = Up;
			sb_updown_timer ((long)w);
		} else if (mpos >= tsize - SLHT) {
			/*
			 * DOWN button pressed
			 */
			sb_press_down (w);
			w->state = Down;
			sb_updown_timer ((long)w);
		} else if (mpos >= SLHT+w->slpos_abs &&
			   mpos <  SLHT+w->slpos_abs+w->slsize_abs) {
			/*
			 * SLIDER pressed
			 */
			sb_press_slider (w);
			w->slpos_old = w->slpos;
			w->timer = wt_addtimeout (100, sb_move_timer, (long)w);
			w->state = Move;
			w->mouse_pos = mpos;
		} else if (mpos < SLHT+w->slpos_abs) {
			/*
			 * clicked above slider
			 */
			w->state = PgUp;
			sb_updown_timer ((long)w);
		} else {
			/*
			 * clicked below slider
			 */
			w->state = PgDown;
			sb_updown_timer ((long)w);
		}
		break;

	case EVENT_MRELEASE:
		if (!(ev->key & BUTTON_LEFT))
			return ev;

		sb_cancel (w);
		break;

	default:
		return ev;
	}

	return NULL;
}

static long
scroll_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_scroll_class = {
	"scrollbar", 0,
	scroll_init,
	scroll_create,
	scroll_delete,
	scroll_close,
	scroll_open,
	scroll_addchild,
	scroll_delchild,
	scroll_realize,
	scroll_query_geometry,
	scroll_query_minsize,
	scroll_reshape,
	scroll_setopt,
	scroll_getopt,
	scroll_event,
	scroll_changes,
	scroll_changes
};

widget_class_t *wt_scrollbar_class = &_wt_scroll_class;
