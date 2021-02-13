/*
 * This file is part of "The W Toolkit".
 *
 * (w) 1997, Eero Tamminen.
 *
 * a menu widget
 *
 * This interprets the given menu structs, creates needed pane with
 * pushbuttons and opens up popup according to user actions.
 *
 * Note that widget may not use a copy of the menu structure, because
 * submenu pointers may be changed during widget life.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef struct {
	widget_t w;
	short is_realized;
	char *fname;		/* font name for children */

	short align;		/* on which side of the button popup is put */
	widget_t *pane;		/* button row container */
	widget_t *popup;	/* realized popup */
	wt_menu_item *menu;	/* menu structure */
	short index;		/* last opened menu item */

	/* needed when deleting old buttons when menu struct is changed */
	widget_t *last;		/* last menu button */
	short count;		/* number of buttons */

	/* function that is used to draw all the `icons' */
	void (*draw_fn) (widget_t *w, short x, short y, BITMAP *icon);
} menu_widget_t;


/* 
 * menu functions
 */

static void
delete_popup(menu_widget_t *w)
{
	if (w->popup) {
		wt_delete(w->popup);
		w->popup = NULL;
	}
}

static void
delete_popup_cb(widget_t *pop)
{
	/* menu <- popup */
	menu_widget_t *w = (menu_widget_t *)(pop->parent);
	delete_popup(w);
}

/* opens a `popup' under user selected menu button (`anchor') */
static widget_t *
create_popup(menu_widget_t *w, widget_t *anchor, wt_menu_item *menu)
{
	widget_t *sub;
	long x, y, wd, ht;
	short xx, yy;

	if (!(sub = wt_create(wt_popup_class, (widget_t *)w))) {
		return NULL;
	}
	/* set options */
	x = PopupTransient;
	(*sub->class->setopt) (sub, WT_STATE, &x);
	(*sub->class->setopt) (sub, WT_LIST_ADDRESS, menu);
	(*sub->class->setopt) (sub, WT_ACTION_CB, delete_popup_cb);
	if (w->draw_fn) {
		(*sub->class->setopt) (sub, WT_DRAW_FN, w->draw_fn);
	}
	if (w->fname) {
		(*sub->class->setopt) (sub, WT_FONT, w->fname);
	}

	/* calculate size, parent root position and popup position */
	(*sub->class->query_geometry) (sub, &x, &x, &wd, &ht);
	w_querywindowpos(wt_widget2win(anchor), 1, &xx, &yy);
	x = xx;  y = yy;
	switch(w->align) {
		case AlignLeft:
			x -= wd;
			break;
		case AlignRight:
			x += anchor->w;
			break;
		case AlignTop:
			y -= ht;
			break;
		/* AlignBottom */
		default:
			y += anchor->h;
			break;
	}
	/* show it on screen */
	wt_reshape(sub, x, y, WT_UNSPEC, WT_UNSPEC);
	(*sub->class->realize) (sub, WROOT);
	return sub;
}

static void
menubutton_cb(widget_t *_w, int pressed)
{
	/* menu <- pane <- button */
	menu_widget_t *w = (menu_widget_t *)_w->parent->parent;
	wt_menu_item *item;
	WEVENT ev;

	/* USRVAL needs wt_getopt instead of just straight getopt method */
	wt_getopt(_w, WT_USRVAL, &item, WT_EOL);

	/* button just pressed? */
	if (pressed > 0) {
		if (item->type == MenuSub) {
			delete_popup(w);
			if (item->select_cb) {
				/* eg. create the submenu */
				(*item->select_cb) ((widget_t *)w, item);
			}
			if (item->sub) {
				w->popup = create_popup (w, _w, item->sub);
			}
		}
		return;
	}

	/* button released on non-submenu item */
	if (!w->popup) {
		if (pressed) {
			/* released outside menubutton */
			return;
		}
		if (item->type == MenuCheck) {
			if (item->sub) {
				item->sub = NULL;
			} else {
				item->sub = (wt_menu_item*)1;
			}
		}
		if (item->select_cb) {
			(*item->select_cb) ((widget_t *)w, item);
		}
		return;
	}

	/* released outside of menubutton? */
	if (pressed) {
		/* redirect event */
		ev.type = EVENT_MRELEASE;
		ev.win = wt_widget2win(w->popup);
		w_querymousepos(ev.win, &ev.x, &ev.y);
		(*w->popup->class->event) (w->popup, &ev);
	} else {
		long state = PopupPersistant;
		/* let popup wait EVENT_MPRESS and handle things itself */
		(*w->popup->class->setopt) (w->popup, WT_STATE, &state);
	}
}

/* called after menu structure has been changed to create and setopt
 * all the buttons needed.
 */
static void
create_buttons(menu_widget_t *w, wt_menu_item *menu)
{
	long idx, val;
	widget_t *wp, *prev;
	wt_menu_item *item;

	delete_popup(w);
	/* remove earlier menu button. This has to be reverse to the
	 * order (after/before) that is used in addchild method.
	 */
	while(w->last && w->count-- >= 0) {
		prev = w->last->prev;
		wt_delete(w->last);
		w->last = prev;
	}
	w->last = wp = NULL;
	w->menu = menu;
	if (!menu) {
		return;
	}
	item = menu;
	w->count = 0;
	for (idx = 0; item->type != MenuEnd; item++, idx++) {
		if (!(item->string || item->icon)) {
			continue;
		}
		if (item->type == MenuCheck) {
			if (!(wp = wt_create(wt_checkbutton_class, w->pane))) {
				continue;
			}
			if (item->sub) {
				val = ButtonStatePressed;
				(*wp->class->setopt) (wp, WT_STATE, &val);
			}
		} else {
			if (!(wp = wt_create(wt_pushbutton_class, w->pane))) {
				continue;
			}
		}
		if (item->string) {
			if (w->fname) {
				(*wp->class->setopt) (wp, WT_FONT, w->fname);
			}
			(*wp->class->setopt) (wp, WT_LABEL, item->string);
		}
		if (item->icon) {
			(*wp->class->setopt) (wp, WT_DRAW_FN, w->draw_fn);
			(*wp->class->setopt) (wp, WT_ICON, item->icon);
		}
		/* USRVAL needs wt_setopt */
		wt_setopt(wp,
			WT_ACTION_CB, menubutton_cb,
			WT_USRVAL, &item,
			WT_EOL);
		w->count++;
	}
	w->last = wp;
}

/* 
 * widget functions
 */

static long
menu_init (void)
{
	return 0;
}

static widget_t *
menu_create (widget_class_t *cp)
{
	menu_widget_t *wp = calloc (1, sizeof (menu_widget_t));
	long a;
	if (!wp)
		return NULL;

	wp->w.class = wt_menu_class;
	wp->pane = wt_create (wt_pane_class, (widget_t *)wp);
	if(!wp->pane) {
		free(wp);
		return NULL;
	}

	a = OrientHorz;
	(*wp->pane->class->setopt) (wp->pane, WT_ORIENTATION, &a);
	wp->align = AlignBottom;
	return (widget_t *)wp;
}

static long
menu_delete (widget_t *_w)
{
	menu_widget_t *w = (menu_widget_t *)_w;

	delete_popup (w);
	wt_delete (w->pane);
	free (w);
	return 0;
}

static long
menu_close (widget_t *_w)
{
	menu_widget_t *w = (menu_widget_t *)_w;
	if (w->popup) {
		wt_close(w->popup);
	}
	wt_close(w->pane);
	return 0;
}

static long
menu_open (widget_t *_w)
{
	menu_widget_t *w = (menu_widget_t *)_w;
	if (w->popup) {
		wt_open(w->popup);
	}
	wt_open (w->pane);
	return 0;
}

static long
menu_addchild (widget_t *parent, widget_t *wc)
{
	menu_widget_t *w = (menu_widget_t *)parent;
	if (w->pane) {
		if (wc->class == wt_popup_class) {
			delete_popup (w);
			wt_add_after (parent, parent->childs, wc);
			return 0;
		}
		return (*w->pane->class->addchild) (w->pane, wc);
	}
	/*  should be w->pane */
	wt_add_after (parent, parent->childs, wc);
	return 0;
}

static long
menu_delchild (widget_t *parent, widget_t *w)
{
	wt_remove(w);
	return 0;
}

static long
menu_realize (widget_t *_w, WWIN *parent)
{
	menu_widget_t *w = (menu_widget_t *)_w;
	return (*w->pane->class->realize) (w->pane, parent);
}

static long
menu_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	menu_widget_t *w = (menu_widget_t *)_w;
	return (*w->pane->class->query_geometry) (w->pane, xp, yp, wdp, htp);
}

static long
menu_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	menu_widget_t *w = (menu_widget_t *)_w;
	return (*w->pane->class->query_minsize) (w->pane, wdp, htp);
}

static long
menu_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	menu_widget_t *w = (menu_widget_t *)_w;
	return (*w->pane->class->reshape) (w->pane, x, y, wd, ht);
}

static long
menu_setopt (widget_t *_w, long key, void *val)
{
	menu_widget_t *w = (menu_widget_t *)_w;
	widget_t *wp;

	switch (key) {
	case WT_XPOS:
	case WT_YPOS:
	case WT_WIDTH:
	case WT_HEIGHT:
	case WT_ORIENTATION:
		return (*w->pane->class->setopt) (w->pane, key, val);

	case WT_ALIGNMENT:
		if (*(long *)val == AlignFill) {
			/* forward this to pane... */
			return (*w->pane->class->setopt) (w->pane, key, val);
		}
		w->align = *(long *)val;
		break;

	case WT_LIST_ADDRESS:
		create_buttons(w, (wt_menu_item *)val);
		break;

	case WT_DRAW_FN:
		w->draw_fn = val;
		if (w->popup) {
			(*w->popup->class->setopt) (w->popup, key, val);
		}
		break;

	case WT_FONT:
		if (w->fname) {
			free(w->fname);
		}
		if (val) {
			w->fname = strdup((char *)val);
			for (wp = w->pane->childs; wp; wp = wp->next) {
				(*wp->class->setopt) (wp, key, val);
			}
		}
		break;

	default:
		return -1;
	}
	return 0;
}

static long
menu_getopt (widget_t *_w, long key, void *val)
{
	menu_widget_t *w = (menu_widget_t *)_w;

	switch (key) {
	case WT_XPOS:
	case WT_YPOS:
	case WT_WIDTH:
	case WT_HEIGHT:
	case WT_ORIENTATION:
		return (*w->pane->class->getopt) (w->pane, key, val);

	case WT_ALIGNMENT:
		*(long *)val = w->align;
		break;

	case WT_LIST_ADDRESS:
		*(wt_menu_item **)val = w->menu;
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
menu_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long
menu_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_menu_class = {
	"menu", 0,
	menu_init,
	menu_create,
	menu_delete,
	menu_close,
	menu_open,
	menu_addchild,
	menu_delchild,
	menu_realize,
	menu_query_geometry,
	menu_query_minsize,
	menu_reshape,
	menu_setopt,
	menu_getopt,
	menu_event,
	menu_changes,
	menu_changes
};

widget_class_t *wt_menu_class = &_wt_menu_class;
