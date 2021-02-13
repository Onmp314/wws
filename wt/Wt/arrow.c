/*
 * an arrow widget
 *
 * behaves like push/checkbutton, but doesn't contain text and you can
 * align arrow to a desired direction.
 *
 * If `w.win' member is set before realization, widget will use the set
 * (normally parent's) window and widget position will be used as
 * offset inside the window rather than the window position.
 *
 * (w) 1997, Eero Tamminen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

#define MIN_W	12
#define MIN_H	12

typedef struct {
	widget_t w;
	short is_open;
	short is_realized;
	short own_window;	/* flag: own, not parent's window */
	short orient;		/* arrow direction */
	short state;		/* down / up */
	short mode;		/* push / toggle */
	void (*action_cb) (widget_t *w, int pressed);
} arrow_widget_t;

static long
arrow_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp);

/* 
 * arrow functions
 */

static void
press_arrow(arrow_widget_t *w)
{
	int x, y;
	if (w->is_realized) {
		if (w->own_window) {
			x = 0;
			y = 0;
		} else {
			x = w->w.x;
			y = w->w.y;
		}
		wt_box3d_press (w->w.win, x, y, w->w.w, w->w.h, 1);
		w->state = ButtonStatePressed;
	}
}

static void
release_arrow(arrow_widget_t *w)
{
	int x, y;
	if (w->is_realized) {
		if (w->own_window) {
			x = 0;
			y = 0;
		} else {
			x = w->w.x;
			y = w->w.y;
		}
		wt_box3d_release (w->w.win, x, y, w->w.w, w->w.h, 1);
		w->state = ButtonStateReleased;
	}
}


static void
redraw_arrow(arrow_widget_t *w)
{
	int x, y, wd, ht, dir;
	if (!w->is_realized)
		return;

	w_setmode (w->w.win, M_CLEAR);
	if (w->own_window) {
		x = 0;
		y = 0;
	} else {
		x = w->w.x;
		y = w->w.y;
	}
	w_pbox (w->w.win, x, y, w->w.w, w->w.h);
	switch (w->orient) {
		case AlignTop:		dir = 0; break;
		case AlignBottom:	dir = 1; break;
		case AlignLeft:		dir = 2; break;
		case AlignRight:	dir = 3; break;
		default:		dir = 1;
	}
	wt_box3d (w->w.win, x, y, w->w.w, w->w.h);
	wd = w->w.w / 2;  ht = w->w.h / 2;
	wt_arrow3d_press (w->w.win, x + wd/2, y + ht/2, wd, ht, dir);
	if (w->state == ButtonStatePressed)
		press_arrow (w);
}

/* 
 * widget functions
 */

static long
arrow_init (void)
{
	return 0;
}

static widget_t *
arrow_create (widget_class_t *cp)
{
	arrow_widget_t *wp = calloc (1, sizeof (arrow_widget_t));
	if (!wp)
		return NULL;

	wp->w.class = wt_arrow_class;
	wp->state = ButtonStateReleased;
	wp->mode = ButtonModePush;
	wp->orient = AlignBottom;

	return (widget_t *)wp;
}

static long
arrow_delete (widget_t *_w)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;

	if (w->is_realized) {
		release_arrow (w);
		w_delete (w->w.win);
	}
	free (w);
	return 0;
}

static long
arrow_close (widget_t *_w)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		release_arrow (w);
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
arrow_open (widget_t *_w)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
arrow_addchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
arrow_delchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
arrow_realize (widget_t *_w, WWIN *parent)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	arrow_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.x = x;
	w->w.y = y;
	w->w.w = wd;
	w->w.h = ht;

	if (!w->w.win) {
		w->own_window = 1;
		w->w.win = wt_create_window (parent, wd, ht,
			W_MOVE|W_NOBORDER|EV_MOUSE, _w);
		if (!w->w.win)
			return -1;
		w->w.win->user_val = (long)w;
	}
	w->is_realized = 1;
	redraw_arrow(w);

	w_open (w->w.win, w->w.x, w->w.y);
	w->is_open = 1;
	return 0;
}

static long
arrow_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;

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
arrow_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;

	if (w->orient == AlignLeft || w->orient == AlignRight) {
		*htp = MIN_W;
		*wdp = MIN_H;
	} else {
		*wdp = MIN_W;
		*htp = MIN_H;
	}
	return 0;
}

static long
arrow_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;
	long minwd, minht;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized && w->own_window)
			w_move (w->w.win, x, y);
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		arrow_query_minsize (_w, &minwd, &minht);
		if (wd < minwd)
			wd = minwd;
		if (ht < minht)
			ht = minht;
		if (wd != w->w.w || ht != w->w.h) {
			w->w.w = wd;
			w->w.h = ht;
			if (w->is_realized) {
				if (w->own_window) {
					w_resize (w->w.win, wd, ht);
				}
				redraw_arrow(w);
			}
			ret = 1;
		}
	}
	return ret;
}

static long
arrow_setopt (widget_t *_w, long key, void *val)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (arrow_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (arrow_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		if (arrow_reshape (_w, w->w.x, w->w.y, *(long*)val, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		if (arrow_reshape (_w, w->w.x, w->w.y, w->w.w, *(long*)val))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_ALIGNMENT:
		w->orient = *(long *)val;
		redraw_arrow(w);
		break;

	case WT_STATE:
		if (*(long*)val == ButtonStatePressed) {
			press_arrow(w);
		} else {
			release_arrow(w);
		}
		break;

	case WT_MODE:
		if (*(long*)val == ButtonModePush || *(long*)val == ButtonModeToggle) {
			w->mode = *(long*)val;
		}
		break;

	case WT_ACTION_CB:
		w->action_cb = val;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
arrow_getopt (widget_t *_w, long key, void *val)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;

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

	case WT_ALIGNMENT:
		*(long *)val = w->orient;
		break;

	case WT_STATE:
		*(long *)val = w->state;
		break;

	case WT_MODE:
		*(long *)val = w->mode;
		break;

	case WT_ACTION_CB:
		*(long *)val = (long)w->action_cb;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
arrow_event (widget_t *_w, WEVENT *ev)
{
	arrow_widget_t *w = (arrow_widget_t *)_w;
	int in;

	switch (ev->type) {
	case EVENT_MPRESS:
		if (w->state == ButtonStatePressed) {
			release_arrow (w);
		} else {
			press_arrow (w);
		}
		if(w->action_cb)
			w->action_cb(_w, 1);
		break;

	case EVENT_MRELEASE:
		in = (ev->x >= 0 && ev->x < w->w.w && ev->y >= 0 && ev->y < w->w.h);
		if (!(w->mode == ButtonModeToggle && in)) {
			if (w->state == ButtonStatePressed) {
				release_arrow (w);
			} else {
				press_arrow (w);
			}
		}
		if (w->action_cb) {
			if(in)
				w->action_cb(_w, 0);
			else
				w->action_cb(_w, -1);
		}
		break;

	default:
		return ev;
	}
	return NULL;
}

static long
arrow_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_arrow_class = {
	"arrow", 0,
	arrow_init,
	arrow_create,
	arrow_delete,
	arrow_close,
	arrow_open,
	arrow_addchild,
	arrow_delchild,
	arrow_realize,
	arrow_query_geometry,
	arrow_query_minsize,
	arrow_reshape,
	arrow_setopt,
	arrow_getopt,
	arrow_event,
	arrow_changes,
	arrow_changes
};

widget_class_t *wt_arrow_class = &_wt_arrow_class;
