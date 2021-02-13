/*
 * this file is part of "The W Toolkit".
 *
 * (w) 1997, Eero Tamminen.
 *
 * `popup' widget
 *
 * Converted from Kay's `list' source:
 * - Widget handles structs instead of concatenated strings.
 * - Callbacks and single child handling for hierarchical popups.
 * - Options for toggling popup behaviour on the fly.
 * - Changed drawing so that item may have both icon, text and marker
 *   aligned as dictated by the WT_ALIGNMENT option.
 * - Triangle mark on the right or left of the submenu entries
 *   (size = 1/2 x 1/4 font height).
 * - Checked entries.
 * - A character width between checkmark, icon, text, marker and window
 *   borders.
 * - Added disabled flag for entries.
 * - Limited popup position inside WROOT.
 *
 * NOTES:
 * - `w.w' and `w.h' include borders, because with popups window borders are
 *   drawn.  Use `w.win->width' and `w.win->height' instead (`usrwd' and
 *   `usrht' could have overruled `wd' and `ht').
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

#define WIN_BORDER	4	/* W window border width, should be on Wlib.h */

/* popup entry type flags */
#define EntryDisabled	1
#define EntrySep	2
#define EntryIcon	4
#define EntryString	8
#define EntryCheck	16
#define EntrySub	32

typedef struct {
	short	type;
	short	y, wd, ht;
	/* rest of the information (string, icon, submenu, callback) can be
	 * read from the widget's `menu' member using the same index.
	 */
} entry_t;

typedef struct {
	widget_t w;
	short	is_open;
	short	is_realized;
	WFONT	*font;
	short	fsize;
	char	*fname;

	/* submenu pointer will be in w.childs member */
	wt_menu_item *menu;	/* menu structure */
	entry_t *entries;	/* array of parsed entries */
	short	numents;	/* # entries */
	short	wd, ht;		/* optimal width, height */
	short	offset;		/* text offset with icons */
	short	align;		/* alignment */
	short	selected;	/* selected entry */
	short	last;		/* last opened submenu */

	long	timer;
	short	tracking;	/* mousebutton still pressed */
	short	transient;	/* if not set, wait first for MPRESS */

	void (*draw_fn) (widget_t *, short x, short y, BITMAP *icon);
	void (*delete_cb) (widget_t *);
	short usrwd, usrht;
} popup_widget_t;


/******************** Some Utility Functions *************************/

static void
free_entries (popup_widget_t *w)
{
	if (w->entries) {
		free (w->entries);
		w->entries = NULL;
		w->numents = 0;
	}
}

/* parse menu structure into internal represantation. the string set */
static int
parse_entries (popup_widget_t *w, wt_menu_item *menu)
{
	wt_menu_item *item;
	entry_t *entries, *ep;
	short y, iconwd, maxwd, numents, idx, fontw, fonth;

	for (numents = 0; menu[numents].type != MenuEnd; numents++)
		;

	entries = malloc (sizeof (entry_t) * numents);
	if (!entries)
		return -1;

	free_entries (w);
	w->numents = numents;
	w->entries = entries;
	w->menu = menu;

	memset (entries, 0, sizeof (entry_t) * numents);
	fontw = w->font->maxwidth;
	fonth = w->font->height;

	item = menu;
	ep = entries;
	y = iconwd = maxwd = 0;
	for (idx = 0; idx < numents; idx++, ep++, item++) {
		if (item->string) {
			/*
			 * string entry
			 */
			ep->type |= EntryString;
			ep->wd = w_strlen (w->font, item->string);
			ep->ht = fonth;
			ep->y = y;
		}
		if (item->icon) {
			/*
			 * user wants to draw something
			 */
			ep->type |= EntryIcon;
			if (iconwd < item->icon->width) {
				iconwd = item->icon->width;
			}
			if (ep->ht <= item->icon->height) {
				ep->ht = item->icon->height + 1;
			}
			if (ep->ht < fonth) {
				ep->ht = fonth;
			}
			ep->y = y;
		}
		if (!(item->string || item->icon)) {
			/*
			 * separator
			 */
			ep->type = EntrySep;
			ep->y = y;
			ep->wd = 0;
			ep->ht = fonth / 2;
		} else {
			if (item->type == MenuSub) {
				/* 
				 * Submenu...
				 */
				ep->type |= EntrySub;
				/* offset + marker width */
				ep->wd += fonth * 5 / 4;
			} else {
				if (item->type == MenuCheck) {
					ep->type |= EntryCheck;
					if (iconwd < fontw) {
						iconwd = fontw;
					}
					if (ep->ht < fonth) {
						ep->ht = fonth;
					}
				} else {
					if (!item->select_cb)
						ep->type |= EntryDisabled;
				}
			}
		}
		y += ep->ht;
		if (ep->wd > maxwd) {
			maxwd = ep->wd;
		}
	}
	if (iconwd) {
		/* + text offset */
		if (maxwd) {
			iconwd += fontw;
		}
		for (ep = entries, idx = 0; idx < numents; idx++, ep++) {
			ep->wd += iconwd;
		}
		w->offset = iconwd;
		maxwd += iconwd;
	} else {
		w->offset = 0;
	}
	/* add borders */
	w->wd = maxwd + fontw * 2;
	w->ht = y;
	return 0;
}

static void
draw_check(popup_widget_t *w, entry_t *ep, int x, int mode)
{
	short check[10], fontw, fonth;

	fontw = w->font->maxwidth;
	fonth = w->font->height;

	check[0] = x;
	check[2] = x + fontw / 3;
	check[4] = x + fontw;
	check[6] = check[2];
	check[8] = check[0];
	check[1] = ep->y + ep->ht * 2 / 3;
	check[3] = check[1] + fonth / 3 - 1;
	check[5] = check[1] - fonth * 2 / 3 + 1;
	check[7] = check[3] - 1;
	check[9] = check[1];

	w_setmode (w->w.win, mode);
	/* not a convex one so can't fill */
	w_poly(w->w.win, 5, check);
}

static void
draw_entries (popup_widget_t *w)
{
	short triangle[8], idx, y, border, x, winwd, fonth;
	wt_menu_item *item;
	entry_t *ep;

	winwd = w->w.win->width;
	w_setmode (w->w.win, M_CLEAR);
	w_pbox (w->w.win, 0, 0, winwd, w->w.win->height);
	w_setpattern (w->w.win, MAX_GRAYSCALES * 2 / 3);
	border = w->font->maxwidth;
	fonth = w->font->height;

	/* horizontal co-ordinates for submenu markers */
	if (w->align == AlignRight) {
		triangle[0] = border + fonth / 4;
		triangle[2] = border;
	} else {
		triangle[0] = winwd - border - fonth / 4;
		triangle[2] = winwd - border;
	}
	triangle[4] = triangle[0];
	triangle[6] = triangle[0];

	item = w->menu;
	ep = w->entries;
	for (idx = 0; idx < w->numents; idx++, ep++, item++) {
		if (ep->type & EntrySep) {
			w_setmode (w->w.win, M_DRAW);
			w_dhline (w->w.win, 2, ep->y + ep->ht/2, winwd - 3);
			continue;
		}
		x = border;
		/* icons or checkmarks in the popup? */
		if (w->offset) {
			switch(w->align) {
			case AlignRight:
				x = winwd - border - w->offset;
				break;
			case AlignCenter:
				x = border + (winwd - 2*border - ep->wd) / 2;
				break;
			}
		}
		/* drawable icon? If checkmark, is it set? */
		if ((ep->type & EntryIcon) && w->draw_fn &&
		    (!(ep->type & EntryCheck) || item->sub)) {
			y = ep->y + (ep->ht - item->icon->height) / 2;
			(*w->draw_fn) ((widget_t *)w, x, y, item->icon);
		} else {
			if ((ep->type & EntryCheck) && item->sub) {
			        draw_check(w, ep, x, M_DRAW);
			}
		}
		if (w->offset) {
			x += w->offset;
		}
		if (ep->type & EntryString) {
			switch(w->align) {
			case AlignCenter:
				x += (winwd - 2*border - ep->wd)/2;
				break;
			case AlignRight:
				x = winwd - border - ep->wd;
				if (ep->type & EntrySub) {
					/* discount marker width from ep->wd */
					x += fonth * 5 / 4;
				}
				break;
			}
			y = ep->y + (ep->ht - fonth) / 2;
			w_printstring(w->w.win, x, y, item->string);
		}
		if (ep->type & EntrySub) {
			triangle[1] = ep->y + ep->ht / 4;
			triangle[3] = triangle[1] + fonth / 4;
			triangle[5] = triangle[1] + fonth / 2;
			triangle[7] = triangle[1];
			w_setmode (w->w.win, M_DRAW);
			w_ppoly(w->w.win, 4, triangle);
		}
		if (ep->type & EntryDisabled) {
			w_setmode(w->w.win, M_CLEAR);
			w_dpbox (w->w.win, 0, ep->y, winwd, ep->ht);
		}
	}
}

static void
invert_check(popup_widget_t *w, entry_t *ep, wt_menu_item *item)
{
	short x, winwd, border;

	winwd = w->w.win->width;
	border = w->font->maxwidth;
	x = border;
	switch(w->align) {
		case AlignRight:
			x = winwd - border - w->offset;
			break;
		case AlignCenter:
			x = border + (winwd - 2*border - ep->wd) / 2;
			break;
	}
	if (item->sub) {
		item->sub = NULL;
		draw_check(w, ep, x, M_CLEAR);
	} else {
		item->sub = (wt_menu_item *)1;
		draw_check(w, ep, x, M_DRAW);
	}
}

static void
invert_entry (popup_widget_t *w, short entry)
{
	entry_t *ep;

	if (entry >= 0 && entry < w->numents) {
		ep = w->entries + entry;
		if (ep->type & (EntrySep | EntryDisabled)) {
			return;
		}
		w_setmode (w->w.win, M_INVERS);
		w_pbox (w->w.win, 0, ep->y, w->w.win->width, ep->ht);
	}
}

static short
find_entry (popup_widget_t *w, short x, short y)
{
	entry_t *ep;
	int idx;

	for (ep = w->entries, idx = 0; idx < w->numents; idx++, ep++) {
		if (ep->y <= y && y < ep->y + ep->ht)
			return idx;
	}
	return -1;
}

static void
delete_submenu(popup_widget_t *w)
{
	if (w->w.childs) {
		wt_delete(w->w.childs);
		w->w.childs = NULL;
	}
	w->last = -1;
}

static void
delete_popup_cb(widget_t *sub)
{
	popup_widget_t *popup = (popup_widget_t *)(sub->parent);
	delete_submenu(popup);
	popup->delete_cb((widget_t *)popup);
}

static void
create_submenu(popup_widget_t *w, long y, wt_menu_item *menu)
{
	widget_t *sub;
	long x, wd, ht;
	short xx, yy;

	if (!(sub = wt_create(wt_popup_class, (widget_t *)w))) {
		return;
	}
	/* set options */
	x = PopupTransient;
	(*sub->class->setopt) (sub, WT_STATE, &x);
	(*sub->class->setopt) (sub, WT_LIST_ADDRESS, menu);
	(*sub->class->setopt) (sub, WT_ACTION_CB, delete_popup_cb);
	if (w->align != AlignLeft) {
		x = w->align;
		(*sub->class->setopt) (sub, WT_ALIGNMENT, &x);
	}
	if (w->draw_fn) {
		(*sub->class->setopt) (sub, WT_DRAW_FN, w->draw_fn);
	}
	if (w->fsize) {
		x = w->fsize;
		(*sub->class->setopt) (sub, WT_FONTSIZE, &x);
	}
	if (w->fname) {
		(*sub->class->setopt) (sub, WT_FONT, w->fname);
	}

	/* calculate popup position */
	(*sub->class->query_geometry) (sub, &x, &x, &wd, &ht);
	w_querywindowpos(w->w.win, 1, &xx, &yy);
	x = xx + w->w.win->width - w->font->maxwidth / 2;
	if (x + wd >= WROOT->width) {
		x = xx - wd + w->font->maxwidth / 2;
	}
	y = yy + w->entries[w->selected].y;

	/* show it on screen */
	wt_reshape(sub, x, y, WT_UNSPEC, WT_UNSPEC);
	(*sub->class->realize) (sub, WROOT);
}

static void
open_submenu(popup_widget_t *w)
{
	entry_t *ep;
	wt_menu_item *item;
	short index = w->selected;

	if (index >= 0 && index < w->numents) {
		if (index == w->last) {
			/* don't recreate same menu */
			return;
		}
		delete_submenu(w);
		ep = w->entries + index;
		if (!(ep->type & EntrySub)) {
			return;
		}
		item = &(w->menu[index]);
		if (item->select_cb) {
			(*item->select_cb) ((widget_t*)w, item);
		}
		if (item->sub) {
			create_submenu(w, ep->y, item->sub);
		}
		w->last = index;
	}
}

/* release -> select or be gone */
static void
select_item (popup_widget_t *w)
{
	entry_t *ep;
	wt_menu_item *item;
	short index = w->selected;

	delete_submenu(w);
	ep = w->entries + index;
	if (index >= 0 && index < w->numents &&
	    !(ep->type & (EntrySub | EntrySep | EntryDisabled))) {
		item = &(w->menu[index]);
		if (ep->type & EntryCheck) {
			invert_check(w, ep, item);
			if (!item->select_cb) {
				/* multiple selections? */
				return;
			}
		}
		(*item->select_cb) ((widget_t *)w, item);
	}
	w->delete_cb((widget_t *)w);
}

static void
move_timer (long _w)
{
	popup_widget_t *w = (popup_widget_t *)_w;
	short entry, x, y;

	w_querymousepos (w->w.win, &x, &y);
	entry = find_entry (w, x, y);
	if (entry != w->selected) {
		invert_entry (w, w->selected);
		invert_entry (w, entry);
		w->selected = entry;
		if (w->transient)
			open_submenu(w);
	}
	w->timer = wt_addtimeout (100, move_timer, _w);
}

/*
 * load a new font
 */
static int
loadfont (popup_widget_t *w, char *fname, short fsize)
{
	WFONT *fp;

	if(!(fp = wt_loadfont (fname, fsize, 0, 0))) {
		return -1;
	}
	w_unloadfont (w->font);
	w_setfont (w->w.win, fp);
	w->fsize = fsize;
	w->font = fp;
	return 0;
}

/*************************** Widget Code *****************************/

static long popup_query_geometry (widget_t *, long *, long *, long *, long *);

static long
popup_init (void)
{
	return 0;
}

static widget_t *
popup_create (widget_class_t *cp)
{
	popup_widget_t *wp = calloc (1, sizeof (popup_widget_t));
	if (!wp)
		return NULL;

	wp->w.class = wt_popup_class;
	if (loadfont (wp, NULL, 0)) {
		free (wp);
		return NULL;
	}
	wp->align = AlignLeft;
	wp->selected = -1;
	wp->timer = -1;
	wp->last = -1;

	return (widget_t *)wp;
}

static long
popup_delete (widget_t *_w)
{
	popup_widget_t *w = (popup_widget_t *)_w;

	if (w->timer >= 0) {
		wt_deltimeout (w->timer);
	}
	delete_submenu(w);
	if (w->is_realized) {
		w_delete (w->w.win);
	}
	w_unloadfont (w->font);
	if (w->fname) {
		free(w->fname);
	}
	free_entries (w);
	free (w);
	return 0;
}

static long
popup_close (widget_t *_w)
{
	popup_widget_t *w = (popup_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		if (w->w.childs) {
			wt_close(w->w.childs);
		}
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
popup_open (widget_t *_w)
{
	popup_widget_t *w = (popup_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		if (w->w.childs) {
			wt_open(w->w.childs);
		}
		w->is_open = 1;
	}
	return 0;
}

static long
popup_addchild (widget_t *parent, widget_t *w)
{
	if (w->class == wt_popup_class) {
		delete_submenu ((popup_widget_t *)parent);
		wt_add_before (parent, parent->childs, w);
		return 0;
	}
	return -1;
}

static long
popup_delchild (widget_t *parent, widget_t *w)
{
	wt_remove(w);
	return 0;
}

static void
start_tracking(popup_widget_t *w)
{
	short dummy;
	w->transient = 1;
	if (!w_querymousepos(w->w.win, &dummy, &dummy))
		move_timer ((long)w);
}

static long
popup_realize (widget_t *_w, WWIN *parent)
{
	popup_widget_t *w = (popup_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	popup_query_geometry (_w, &x, &y, &wd, &ht);
	wd -= WIN_BORDER * 2;
	ht -= WIN_BORDER * 2;
	/* to get sub-popups open this can't be W_TOP */
	w->w.win = wt_create_window (parent, wd, ht,
			W_MOVE|EV_ACTIVE|EV_MOUSE, _w);
	if (!w->w.win)
		return -1;
	w_setfont (w->w.win, w->font);

	if (w->entries) {
		draw_entries (w);
	}
	w_open (w->w.win, w->w.x, w->w.y);
	w->is_realized = 1;
	w->is_open = 1;
	if (w->tracking) {
		start_tracking(w);
	}
	return 0;
}

static long
popup_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	popup_widget_t *w = (popup_widget_t *)_w;

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

/* no minsize until widget is given some entries */
static long
popup_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	popup_widget_t *w = (popup_widget_t *)_w;

	*wdp = w->wd;
	*wdp = MAX (*wdp, w->usrwd);
	*htp = w->ht;
	*htp = MAX (*htp, w->usrht);
	/* account for window borders */
	*wdp += WIN_BORDER * 2;
	*htp += WIN_BORDER * 2;
	return 0;
}

static long
popup_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	popup_widget_t *w = (popup_widget_t *)_w;
	long ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized) {
			w_move (w->w.win, x, y);
		} else {
			if (x < 0) {
				x = 0;
			}
			if (y < 0) {
				y = 0;
			}
			if (x + w->w.w > WROOT->width) {
				x = WROOT->width - w->w.w;
			}
			if (y + w->w.h > WROOT->height) {
				y = WROOT->height - w->w.h;
			}
		}
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		w->w.w = wd;
		w->w.h = ht;
		if (w->is_realized) {
			w_resize (w->w.win,
				wd - WIN_BORDER*2, ht - WIN_BORDER*2);
			draw_entries (w);
			if (w->selected >= 0)
				invert_entry (w, w->selected);
		}
		ret = 1;
	}
	return ret;
}

static long
popup_setopt (widget_t *_w, long key, void *val)
{
	popup_widget_t *w = (popup_widget_t *)_w;
	widget_t *sub = w->w.childs;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (popup_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (popup_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (popup_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (popup_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_FONTSIZE:
		if (loadfont (w, NULL, *(long *)val)) {
			return -1;
		}
		if (w->menu) {
			parse_entries(w, w->menu);
			if (w->is_realized) {
				if (popup_reshape (_w, w->w.x, w->w.y, w->wd + WIN_BORDER*2, w->ht + WIN_BORDER*2))
					mask |= WT_CHANGED_SIZE;
			}
		}
		break;

	case WT_FONT:
		if (loadfont (w, (char *)val, w->fsize)) {
			return -1;
		}
		if (val) {
			w->fname = strdup((char *)val);
		} else {
			w->fname = NULL;
		}
		if (w->menu) {
			parse_entries(w, w->menu);
			if (w->is_realized) {
				if (popup_reshape (_w, w->w.x, w->w.y, w->wd + WIN_BORDER*2, w->ht + WIN_BORDER*2))
					mask |= WT_CHANGED_SIZE;
			}
		}
		break;

	case WT_LIST_ADDRESS:
		if (parse_entries (w, val)) {
			return -1;
		}
		if (w->is_realized) {
			if (popup_reshape (_w, w->w.x, w->w.y, w->wd + WIN_BORDER*2, w->ht + WIN_BORDER*2))
				mask |= WT_CHANGED_SIZE;
		}
		break;

	case WT_ALIGNMENT:
		w->align = *(long *)val;
		break;

	case WT_STATE:
		if (*(long *)val == PopupTransient) {
			if (w->is_realized) {
				start_tracking(w);
			}
			w->tracking = 1;
		} else {
			if (w->is_realized) {
				w->transient = 0;
			} else {
				w->tracking = 0;
			}
		}
		if (sub) {
			(*sub->class->setopt) (sub, key, val);
		}
		break;

	case WT_DRAW_FN:
		w->draw_fn = val;
		break;

	case WT_ACTION_CB:
		w->delete_cb = val;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
popup_getopt (widget_t *_w, long key, void *val)
{
	popup_widget_t *w = (popup_widget_t *)_w;

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
		*(WFONT **)val = w->font;
		break;

	case WT_ALIGNMENT:
		*(long *)val = w->align;
		break;

	case WT_LIST_ADDRESS:
		*(wt_menu_item **)val = w->menu;
		break;

	case WT_STATE:
		if (w->tracking) {
			*(long *)val = PopupTransient;
		} else {
			*(long *)val = PopupPersistant;
		}
		break;

	case WT_DRAW_FN:
		*(void **)val = w->draw_fn;
		break;

	case WT_ACTION_CB:
		*(void **)val = w->delete_cb;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
popup_event (widget_t *_w, WEVENT *ev)
{
	popup_widget_t *w = (popup_widget_t *)_w;
	widget_t *sub = w->w.childs;
	WEVENT sev;
	long x, y;

	switch (ev->type) {
	case EVENT_MPRESS:
		delete_submenu(w);
		if (ev->key != BUTTON_LEFT) {
			/* removes us so don't reference widget after this */
			w->delete_cb(_w);
			return NULL;
		}
		w->tracking = 1;
		if (w->timer < 0) {
			move_timer ((long)w);
		}
		if (!w->transient) {
			/* open submenu if one selected (transient ones are
			 * opened automatically through move_timer()).
			 */
			open_submenu(w);
		}
		break;

	case EVENT_MRELEASE:
		if (sub) {
			/* am persistant and released on same in item? */
			if (!w->transient && w->selected >= 0 && w->selected == w->last) {
				x = PopupPersistant;
				/* let sub wait and handle things itself */
				(*sub->class->setopt) (sub, WT_STATE, &x);
			} else {
				/* redirect event */
				sev.type = EVENT_MRELEASE;
				sev.win = wt_widget2win(sub);
				wt_getopt(sub, WT_XPOS, &x, WT_YPOS, &y, WT_EOL);
				sev.x = ev->x - w->w.x + x;
				sev.y = ev->y - w->w.y + y;
				(*sub->class->event) (sub, &sev);
			}
			break;
		}
		/*
		 * not clicked on (empty) submenu entry and tracking mouse
		 * moves.
		 */
		if (w->last < 0 && w->tracking) {
			/* may remove us, so don't reference widget after
			 * this
			 */
			select_item (w);
			return NULL;
		}
		break;

	case EVENT_ACTIVE:
		/*
		 * If mouse was already pressed when the widget is created,
		 * application may have use WT_LIST_TRACKING option and
		 * event redirection.
		 */
		if (w->tracking && w->timer < 0) {
			move_timer ((long)w);
		}
		break;

	case EVENT_INACTIVE:
		invert_entry (w, w->selected);
		w->selected = -1;
		if (w->timer >= 0) {
			wt_deltimeout (w->timer);
			w->timer = -1;
		}
		break;

	default:
		return ev;
	}

	return NULL;
}

static long
popup_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_popup_class = {
	"popup", 0,
	popup_init,
	popup_create,
	popup_delete,
	popup_close,
	popup_open,
	popup_addchild,
	popup_delchild,
	popup_realize,
	popup_query_geometry,
	popup_query_minsize,
	popup_reshape,
	popup_setopt,
	popup_getopt,
	popup_event,
	popup_changes,
	popup_changes
};

widget_class_t *wt_popup_class = &_wt_popup_class;
