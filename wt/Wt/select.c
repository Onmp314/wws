/* 
 * A text select widget for W toolkit
 *
 * Composed of getstring, arrow and listbox widgets.  Stores the list used
 * by listbox.  Implements autolocator between getstring and listbox.
 *
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef struct
{
	widget_t w;
	short is_realized;

	widget_t *getstring;
	widget_t *listbox;
	widget_t *button;

	/* variables for listbox */
	short from_list;	/* app accepts only strings from list */
	short list_width;	/* lenght of the longest string in list */
	short list_items;
	short selected;		/* last selected item on list */
	char **list;

	/* `popup' flag */
	short list_open;

	void (*action_cb) (widget_t *w, char *string, int index);
} select_widget_t;


/* 
 * select functions
 */

/* copy list to widget, set list vars and forward list to listbox */
static long set_list(select_widget_t *w, char **list)
{
	int lsize, ssize, max, width, idx;
	char *store;
	long wd;

	ssize = max = 0;
	for(idx = 0; list[idx]; idx++) {
		width = strlen(list[idx]);
		ssize += width + 1;
		if (width > max)
			max = width;
	}
	lsize = (idx + 1) * sizeof(char*);
	w->list_items = idx;
	w->list_width = max;

	if(!(store = malloc(lsize + ssize)))
		return -1;

	w->list = (char**)store;
	memcpy(store, list, lsize);
	store += lsize;

	for(idx = 0; idx < w->list_items; idx++) {
		w->list[idx] = store;
		strcpy(store, list[idx]);
		store += strlen(store) + 1;
	}
	w->list[idx] = NULL;

	wd = w->list_width;
	wt_setopt(w->getstring,
		WT_STRING_WIDTH, &wd,
		WT_STRING_LENGTH, &wd,
		WT_EOL);

	return wt_setopt(w->listbox, WT_LIST_ADDRESS, w->list, WT_EOL);
}

/* 
 * autolocator
 */

static int locate_first(char *string, char **list, int items)
{
	int index, len = strlen (string);

	if(!(len && list && items))
		return -1;

	for(index = 0; index < items; index++) {
		if (!strncmp (string, *list++, len))
		return index;
	}
	return -1;
}

/* just locating, no completion */
static void item_change_cb(widget_t *_w, char *name)
{
	select_widget_t *w = (select_widget_t*)(_w->parent);
	long idx;

	idx = locate_first(name, w->list, w->list_items);
	if(idx >= 0) {
		(*wt_listbox_class->setopt) (w->listbox, WT_CURSOR, &idx);
		w->selected = idx;
	} else if(!w->from_list) {
		(*wt_listbox_class->setopt) (w->listbox, WT_CURSOR, &idx);
		w->selected = idx;
	}
}

static WEVENT * route_list_key(widget_t *_w, WEVENT *ev)
{
	select_widget_t *w = (select_widget_t*)(_w->parent);
	/* route keys unknown to list into string widget */
	return (*wt_getstring_class->event) (w->getstring, ev);
}

static void list_change_cb(widget_t *_w, char *name, int index)
{
	select_widget_t *w = (select_widget_t*)(_w->parent);
	(*wt_getstring_class->setopt) (w->getstring, WT_STRING_ADDRESS, name);
}

static void item_cb(widget_t *_w, char *name)
{
	select_widget_t *w = (select_widget_t*)(_w->parent);
	long idx;

	if(w->from_list) {

		idx = locate_first(name, w->list, w->list_items);
		if(idx >= 0) {
			name = w->list[idx];
		} else {
			name = w->list[w->selected];
		}
		(*wt_getstring_class->setopt) (w->getstring, WT_STRING_ADDRESS, name);
	}
	if(w->action_cb)
		(*w->action_cb) (_w, name, w->selected);
}

static void list_cb(widget_t *_w, char *name, int index)
{
	select_widget_t *w = (select_widget_t*)(_w->parent);
	(*wt_getstring_class->setopt) (w->getstring, WT_STRING_ADDRESS, name);
	w->selected = index;
	item_cb(_w, w->list[index]);
}

/* 
 * other callbacks
 */

static void close_list(select_widget_t *w)
{
	wt_close(w->listbox);
	w->list_open = 0;
}

static void open_list(select_widget_t *w)
{
	(*wt_listbox_class->realize) (w->listbox, w->w.win);
	wt_open(w->listbox);
	w->list_open = 1;
}

static void button_cb(widget_t *_w, int pressed)
{
	select_widget_t *w = (select_widget_t *)(_w->parent);

	if (pressed)
		return;

	/* toggle list */
	if (w->list_open) {
		close_list(w);
	} else {
		open_list(w);
	}
}

/*
 * widget functions
 */

static long select_init (void)
{
	return 0;
}

static widget_t *select_create(widget_class_t *cp)
{
	select_widget_t *wp = calloc (1, sizeof (select_widget_t));
	long a, b;

	if (!wp)
		return NULL;
	wp->w.class = wt_select_class;

	wp->button    = wt_create (wt_arrow_class, NULL);
	wp->getstring = wt_create (wt_getstring_class, NULL);
	wp->listbox   = wt_create (wt_listbox_class, NULL);

	a = ButtonModeToggle;
	b = AlignBottom;
	wt_setopt(wp->button,
		WT_MODE, &a,
		WT_ALIGNMENT, &b,
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_setopt(wp->getstring,
		WT_CHANGE_CB, item_change_cb,
		WT_ACTION_CB, item_cb,
		WT_EOL);

	wt_setopt(wp->listbox,
		WT_INKEY_CB,  route_list_key,
		WT_CHANGE_CB, list_change_cb,
		WT_ACTION_CB, list_cb,
		WT_EOL);

	wt_add_before((widget_t*)wp, NULL, wp->getstring);
	wt_add_before((widget_t*)wp, NULL, wp->listbox);
	wt_add_before((widget_t*)wp, NULL, wp->button);

	wp->from_list = 1;
	return (widget_t *)wp;
}

static long select_delete(widget_t *_w)
{
	select_widget_t *w = (select_widget_t *)_w;

	wt_remove(w->listbox);
	wt_remove(w->button);
	wt_remove(w->getstring);
	wt_delete(w->listbox);
	wt_delete(w->button);
	wt_delete(w->getstring);
	free(w);
	return 0;
}

static long select_close(widget_t *_w)
{
	select_widget_t *w = (select_widget_t *)_w;

	wt_close(w->listbox);
	wt_close(w->button);
	wt_close(w->getstring);
	return 0;
}

static long select_open(widget_t *_w)
{
	select_widget_t *w = (select_widget_t *)_w;

	wt_open(w->getstring);
	wt_open(w->button);
	if(w->list_open)
		open_list(w);
	return 0;
}

static long select_addchild(widget_t *parent, widget_t *child)
{
	return -1;
}

static long select_delchild(widget_t *parent, widget_t *w)
{
	return -1;
}

static long select_realize(widget_t *_w, WWIN *parent)
{
	select_widget_t *w = (select_widget_t *)_w;

	if(w->is_realized || !w->list)
		return -1;

	(*wt_getstring_class->setopt) (w->getstring, WT_LIST_ADDRESS, w->list[w->selected]);
	(*wt_getstring_class->realize) (w->getstring, parent);
	(*wt_arrow_class->realize) (w->button, parent);

	w->w.win = parent;
	w->is_realized = 1;
	return 0;
}


static long
select_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	select_widget_t *w = (select_widget_t *)_w;

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
select_query_minsize(widget_t *_w, long *wdp, long *htp)
{
	select_widget_t *w = (select_widget_t *)_w;
	long ww, hh;

	(*wt_getstring_class->query_minsize) (w->getstring, &ww, &hh);
	*wdp = ww + 2 + hh;
	*htp = hh;
	return 0;
}

static long
select_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	select_widget_t *w = (select_widget_t *)_w;
	long ww, hh, ret = 0;

	if (x != w->w.x || y != w->w.y) {
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	select_query_minsize(_w, &ww, &hh);
	if (wd != w->w.w || ht != w->w.h) {
		if (wd < ww)
			wd = ww;
		if (ht != hh)
			ht = hh;
		if (wd != w->w.w || ht != w->w.h) {
			w->w.h = ht;
			w->w.w = wd;
			ret = 1;
		}
	}

	/* reshape scrollbar and reposition label(s) */
	(*wt_listbox_class->reshape) (w->listbox, x, y + hh + 1, 0, 0);
	(*wt_getstring_class->reshape) (w->getstring, x, y, wd - hh - 2, 0);
	(*wt_arrow_class->reshape) (w->button, x + wd - hh, y, hh, hh);
	return ret;
}

static long
select_setopt (widget_t *_w, long key, void *val)
{
	select_widget_t *w = (select_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (select_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (select_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		if (select_reshape (_w, w->w.x, w->w.y, *(long *)val, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		if (select_reshape (_w, w->w.x, w->w.y, w->w.w, *(long *)val))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_FONT:
		(*wt_getstring_class->setopt) (w->getstring, key, val);
		(*wt_listbox_class->setopt) (w->listbox, key, val);
		if (w->is_realized && select_reshape (_w, w->w.x, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_LIST_ADDRESS:
		if (set_list(w, (char **)val))
			return -1;
		if (w->is_realized && select_reshape (_w, w->w.x, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_LIST_HEIGHT:
		(*wt_listbox_class->setopt) (w->listbox, key, val);
		break;

	case WT_CURSOR:
		if (*(long*)val < 0 || *(long*)val >= w->list_items)
			return -1;
		w->selected = *(long*)val;
		if (w->is_realized) {
			(*wt_listbox_class->setopt) (w->listbox, key, val);
			(*wt_getstring_class->setopt) (w->getstring, WT_STRING_ADDRESS, w->list[w->selected]);
		}
		break;

	case WT_LIST_ONLY:
		if (val) {
			w->from_list = 1;
		} else {
			w->from_list = 0;
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
select_getopt (widget_t *_w, long key, void *val)
{
	select_widget_t *w = (select_widget_t *)_w;

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

	case WT_FONT:
		return (*wt_getstring_class->getopt) (w->getstring, key, val);

	case WT_LIST_ADDRESS:
		*(char ***)val = w->list;
		break;

	case WT_LIST_HEIGHT:
		(*wt_listbox_class->getopt) (w->listbox, key, val);
		break;

	case WT_CURSOR:
		*(long *)val = w->selected;
		break;

	case WT_ACTION_CB:
		*(long *)val = (long)w->action_cb;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT * select_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long select_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_select_class = {
	"select", 0,
	select_init,
	select_create,
	select_delete,
	select_close,
	select_open,
	select_addchild,
	select_delchild,
	select_realize,
	select_query_geometry,
	select_query_minsize,
	select_reshape,
	select_setopt,
	select_getopt,
	select_event,
	select_changes,
	select_changes
};

widget_class_t *wt_select_class = &_wt_select_class;
