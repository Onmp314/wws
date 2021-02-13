
/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * shell widget (toplevel window with title bar etc).
 *
 * Changes:
 * - WT_RESIZE_CB option, callback replaces child-resizing code (oddie 08/00)
 * - WT_MIN_WIDTH and HEIGHT options (oddie 08/00)
 *
 * $Id: shell.c,v 1.3 2008-08-29 19:47:09 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef struct {
	widget_t w;
	char  *title;
	short is_open;
	short is_realized;
	short is_iconified;
	short is_resizeable;
	short mode;
	WWIN *icon;
	WFONT *iconfont;
	char *iconstring;
	short icon_x, icon_y;
	void (*close_cb) (widget_t *w);
	void (*resize_cb) (widget_t *w,long owd,long oht);
	short minwd, minht;
	short usrwd, usrht;
} shell_widget_t;


static long shell_query_geometry (widget_t *, long *, long *, long *, long *);

static void
shell_iconify (shell_widget_t *w)
{
	short wd, ht, x, y;
	const char *name;

	if (w->is_realized && !w->is_iconified) {
		w_querywindowpos (w->w.win, 1, &x, &y);
		w_close (w->w.win);
		w->w.x = x;
		w->w.y = y;

		if (w->icon_x == UNDEF || w->icon_y == UNDEF) {
			w->icon_x = x;
			w->icon_y = y;
		}

		name = w->iconstring ?: (w->title ?: "Icon");
		wd = w_strlen (w->iconfont, name) + w->iconfont->height/2;
		ht = w->iconfont->height;
		w->icon = wt_create_window (WROOT, wd, ht, W_MOVE|EV_MOUSE,
				(widget_t *)w);
		if (!w->icon)
			return;
		w->icon->user_val = (long)w;

		w_setfont (w->icon, w->iconfont);
		wt_text (w->icon, w->iconfont, name, 0, 0, wd, ht, AlignCenter);
		w_open (w->icon, w->icon_x, w->icon_y);
		w->is_iconified = 1;
	}
}
	
static void
shell_deiconify (shell_widget_t *w)
{
	if (w->is_realized && w->is_iconified) {
		w_querywindowpos (w->icon, 1, &w->icon_x, &w->icon_y);
		w_delete (w->icon);
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_iconified = 0;
	}
}

static long
shell_init (void)
{
	return 0;
}

static widget_t *
shell_create (widget_class_t *cp)
{
	shell_widget_t *wp = malloc (sizeof (shell_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (shell_widget_t));
	wp->w.x = UNDEF;
	wp->w.y = UNDEF;
	wp->w.class = wt_shell_class;
	wp->iconfont = wt_loadfont (NULL, 0, 0, 0);
	if (!wp->iconfont) {
		free (wp);
		return NULL;
	}
	wp->mode = ShellModeMain;
	wp->is_resizeable = 1;
	wp->icon_x = UNDEF;
	wp->icon_y = UNDEF;
	return (widget_t *)wp;
}

static long
shell_delete (widget_t *_w)
{
	shell_widget_t *w = (shell_widget_t *)_w;
	widget_t *wp, *next;

	for (wp = w->w.childs; wp; wp = next) {
		next = wp->next;
		(*wp->class->delete) (wp);
	}
	if (w->is_realized)
		w_delete (w->w.win);
	w_unloadfont (w->iconfont);
	if (w->title)
		free (w->title);
	if (w->iconstring)
		free (w->iconstring);
	free (w);
	return 0;
}

static long
shell_close (widget_t *_w)
{
	shell_widget_t *w = (shell_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
shell_open (widget_t *_w)
{
	shell_widget_t *w = (shell_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
shell_addchild (widget_t *parent, widget_t *w)
{
	wt_add_before (parent, parent->childs, w);
	return 0;
}

static long
shell_delchild (widget_t *parent, widget_t *w)
{
	wt_remove (w);
	return 0;
}

static long
shell_realize (widget_t *_w, WWIN *parent)
{
	shell_widget_t *w = (shell_widget_t *)_w;
	widget_t *wp;
	long x, y, wd, ht;
	short flags;

	if (w->is_realized)
		return -1;

	shell_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	flags = W_MOVE|W_CONTAINER;
	switch (w->mode) {
	case ShellModeMain:
		flags |= W_TITLE|W_CLOSE|W_ICON;
		if (w->is_resizeable)
			flags |= W_RESIZE;
		break;

	case ShellModePopup:
		break;

	case ShellModeNoBorder:
		flags |= W_NOBORDER;
		break;
	}
	w->w.win = wt_create_window (parent, wd, ht, flags, _w);
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
	if (w->title)
		w_settitle (w->w.win, w->title);
	if (w_open (w->w.win, w->w.x, w->w.y) < 0)
		return -1;
	return 0;
}

static long
shell_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	shell_widget_t *w = (shell_widget_t *)_w;

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
shell_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	shell_widget_t *w = (shell_widget_t *)_w;
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
	*wdp = MAX (w->usrwd, *wdp);
	*htp = MAX (w->usrht, *htp);
	if (*wdp < 10)
		*wdp = 10;
	if (*htp < 10)
		*htp = 10;
	return 0;
}

static long
shell_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	shell_widget_t *w = (shell_widget_t *)_w;
	widget_t *wp;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized)
			w_move (w->w.win, x, y);
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}

	wd = MAX(wd,w->minwd);
	ht = MAX(ht,w->minht);
	if (wd != w->w.w || ht != w->w.h) {
		long owd,oht;
		owd=w->w.w;
		oht=w->w.h;
		w->w.w = wd;
		w->w.h = ht;
		if (w->is_realized) {
			short ox, oy;
			w_querywindowpos (w->w.win, 1, &ox, &oy);
			w_close (w->w.win);
			w_resize (w->w.win, wd, ht);
			if(w->resize_cb) {
				(*w->resize_cb)(_w,owd,oht);
			} else {
				for (wp = w->w.childs; wp; wp = wp->next) {
					wt_geometry (wp, &x, &y, NULL, NULL);
					wd = MAX (0, w->w.w - x);
					ht = MAX (0, w->w.h - y);
					wt_reshape (wp, x, y, wd, ht);
				}	
			}
			w_open (w->w.win, ox, oy);
		}
		ret = 1;
	}
	return ret;
}

static long
shell_setopt (widget_t *_w, long key, void *val)
{
	shell_widget_t *w = (shell_widget_t *)_w;
	short mask = 0;
	char *cp;

	switch (key) {
	case WT_XPOS:
		if (shell_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (shell_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (shell_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (shell_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_MIN_WIDTH:
		w->minwd = MAX (0, *(long *)val);
		break;

	case WT_MIN_HEIGHT:
		w->minht = MAX (0, *(long *)val);
		break;

	case WT_LABEL:
		if (!val) {
			if (w->title)
				free (w->title);
			w->title = NULL;
			if (w->is_realized)
				w_settitle (w->w.win, "");
			break;
		}
		cp = strdup ((char *)val);
		if (!cp) {
			return -1;
		}
		if (w->title)
			free (w->title);
		w->title = cp;
		if (w->is_realized) {
			w_settitle (w->w.win, w->title);
		}
		break;

	case WT_MODE:
		w->mode = *(long *)val;
		break;

	case WT_ICON_STRING:
		if (!val) {
			if (w->iconstring)
				free (w->iconstring);
			w->iconstring = NULL;
			break;
		}
		cp = strdup ((char *)val);
		if (!cp) {
			return -1;
		}
		if (w->iconstring)
			free (w->iconstring);
		w->iconstring = cp;
		break;

	case WT_ACTION_CB:
		w->close_cb = val;
		break;

	case WT_RESIZE_CB:
		w->resize_cb = val;
		break;

	case WT_RESIZEABLE:
		w->is_resizeable = *(long *)val;
		break;
		
	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
shell_getopt (widget_t *_w, long key, void *val)
{
	shell_widget_t *w = (shell_widget_t *)_w;

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

	case WT_MIN_WIDTH:
		*(long *)val = w->minwd;
		break;

	case WT_MIN_HEIGHT:
		*(long *)val = w->minht;
		break;

	case WT_LABEL:
		*(char **)val = w->title;
		break;

	case WT_ICON_STRING:
		*(char **)val = w->iconstring;
		break;

	case WT_MODE:
		*(long *)val = w->mode;
		break;

	case WT_ACTION_CB:
		*(void **)val = w->close_cb;
		break;

	case WT_RESIZE_CB:
		*(void **)val = w->resize_cb;
		break;

	case WT_RESIZEABLE:
		*(long *)val = w->is_resizeable;
		break;
		
	default:
		return -1;
	}
	return 0;
}

static WEVENT *
shell_event (widget_t *_w, WEVENT *ev)
{
	shell_widget_t *w = (shell_widget_t *)_w;

	if (ev->win == w->w.win) {
		switch (ev->type) {
		case EVENT_GADGET:
			switch (ev->key) {
			case GADGET_CLOSE:
				if (w->close_cb) {
					(*w->close_cb) (_w);
				} else {
					wt_break (1);
				}
				break;

			case GADGET_ICON:
				shell_iconify (w);
				break;
			}
			break;

		case EVENT_RESIZE:
			wt_change_notify (_w, WT_CHANGED_SIZE);
			shell_reshape (_w, ev->x, ev->y, ev->w, ev->h);
			break;

		default:
			return ev;
		}
	} else {
		if (w->is_iconified && w->icon == ev->win) {
			switch (ev->type) {
			case EVENT_MPRESS:
				if (ev->key & BUTTON_LEFT) {
					shell_deiconify (w);
					break;
				}
			default:
				return ev;
			}
		}
	}
	return NULL;
}

static long
shell_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_shell_class = {
	"shell", 0,
	shell_init,
	shell_create,
	shell_delete,
	shell_close,
	shell_open,
	shell_addchild,
	shell_delchild,
	shell_realize,
	shell_query_geometry,
	shell_query_minsize,
	shell_reshape,
	shell_setopt,
	shell_getopt,
	shell_event,
	shell_changes,
	shell_changes
};

widget_class_t *wt_shell_class = &_wt_shell_class;
