/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * list widget
 *
 * $Id: list.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef enum {
	EntryString, EntrySep, EntryIcon
} entry_type_t;

typedef struct {
	entry_type_t	type;
	char		*string;
	short		y, wd, ht;
} entry_t;


typedef struct {
	widget_t w;
	short	is_open;
	short	is_realized;

	WFONT	*font;
	short	fsize;

	entry_t	*entries;	/* array of entries */
	int	numents;	/* # entries */
	int	wd, ht;		/* optimal width, height */
	int	align;		/* alignment */
	int	sel_entry;	/* selected entry */

	int	mpressed;	/* mousbutton still pressed? */
	long	timer;

	void (*select_cb) (widget_t *, int entry, char *string);
	void (*draw_fn) (widget_t *, int entry, long x, long y,
			 long wd, long ht);
	short usrwd, usrht;
} list_widget_t;


/******************** Some Utility Functions *************************/

static void
free_entries (list_widget_t *w)
{
	int i;

	if (w->entries) {
		for (i = 0; i < w->numents; ++i) {
			if (w->entries[i].type == EntryString)
				free (w->entries[i].string);
		}
		free (w->entries);
		w->entries = NULL;
		w->numents = 0;
	}
}

/*
 * parse the WT_LABEL string into internal represantation. the string set
 * via WT_LABEL looks like this:
 *	"entry1\000entry2\000 ... entryN\000\000"
 *
 * each entry is either a simple string (must not start with @) or a special
 * entry starting with @:
 *
 * @separator			-- horizontal line
 * @icon [ :<width>,<height> ]	-- area where the user can draw to, you
 *				   can optionally specify the size.
 */
static int
parse_entries (list_widget_t *w, char *string)
{
	char *cp, *cp2;
	entry_t *entries, *ep;
	int y, maxwd, numents, i;

	cp = string;
	for (numents = 0; *cp; ++numents, cp += strlen (cp) + 1)
		;

	entries = malloc (sizeof (entry_t) * numents);
	if (!entries)
		return -1;

	memset (entries, 0, sizeof (entry_t) * numents);

	cp = string;
	ep = entries;
	y = maxwd = 0;
	for (i = numents; --i >= 0; ++ep, cp += strlen (cp) + 1) {
		if (!strncasecmp (cp, "@sep", 4)) {
			/*
			 * separator
			 */
			ep->type = EntrySep;
			ep->y = y;
			ep->wd = 0;
			ep->ht = w->font->height;
		} else if (!strncasecmp (cp, "@icon", 5)) {
			/*
			 * user wants to draw something
			 */
			cp2 = strchr (cp, ':');
			if (cp2) {
				ep->wd = atol (cp2);
				cp2 = strchr (cp2, ',');
				if (cp2) {
					ep->ht = atol (cp2);
				}
			}
			if (ep->ht <= 0) {
				ep->ht = w->font->height;
			}
			ep->type = EntryIcon;
			ep->y = y;
		} else {
			/*
			 * string entry
			 */
			ep->type = EntryString;
			ep->string = strdup (cp);
			ep->y = y;
			ep->wd = w_strlen (w->font, ep->string);
			ep->ht = w->font->height;
		}
		y += ep->ht;
		if (ep->wd > maxwd) {
			maxwd = ep->wd;
		}
	}
	free_entries (w);

	w->numents = numents;
	w->entries = entries;
	w->wd = maxwd;
	w->ht = y;

	return 0;
}

static void
draw_entries (list_widget_t *w)
{
	entry_t *ep;
	int i, x = 0;

	w_setmode (w->w.win, M_CLEAR);
	w_pbox (w->w.win, 0, 0, w->w.w, w->w.h);

	for (ep = w->entries, i = 0; i < w->numents; ++i, ++ep) {
		switch (ep->type) {
		case EntryString:
			wt_text (w->w.win, w->font, ep->string,
				 0, ep->y, w->w.w, ep->ht, w->align);
			break;

		case EntrySep:
			w_setmode (w->w.win, M_DRAW);
			w_hline (w->w.win, 2, ep->y + ep->ht/2, w->w.w - 3);
			break;

		case EntryIcon:
			if (!w->draw_fn) {
				break;
			}
			if (ep->wd <= 0) {
				ep->wd = w->w.w;
			}
			switch (w->align) {
			case AlignLeft:
				x = 0;
				break;
			case AlignCenter:
				x = (w->w.w - ep->wd)/2;
				break;
			case AlignRight:
				x = w->w.w - ep->wd;
				break;
			}
			(*w->draw_fn) ((widget_t *)w, i, x, ep->y,
					ep->wd, ep->ht);
			break;
		}
	}
}

static int
find_entry (list_widget_t *w, int y)
{
	entry_t *ep;
	int i;

	for (i = 0, ep = w->entries; i < w->numents; ++i, ++ep) {
		if (ep->y <= y && y < ep->y + ep->ht)
			return i;
	}
	return -1;
}

static void
invert_entry (list_widget_t *w, int entry)
{
	entry_t *ep;

	if (entry >= 0 && entry < w->numents) {
		ep = w->entries + entry;
		if (ep->type != EntrySep) {
			w_setmode (w->w.win, M_INVERS);
			w_pbox (w->w.win, 0, ep->y, w->w.w, ep->ht);
		}
	}
}

static void
select_notify (list_widget_t *w)
{
	char *string = NULL;

	if (w->select_cb) {
		if (w->sel_entry >= 0 && w->sel_entry < w->numents)
			string = w->entries[w->sel_entry].string;
		(*w->select_cb) ((widget_t *)w, w->sel_entry, string);
	}
}

/*
 * load a new font
 */
static int
loadfont (list_widget_t *w, char *fname, short fsize)
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

static void
move_timer (long _w)
{
	list_widget_t *w = (list_widget_t *)_w;
	short x, y;
	int entry;

	if (w_querymousepos (w->w.win, &x, &y))
		return;

	entry = find_entry (w, y);
	if (entry != w->sel_entry) {
		invert_entry (w, w->sel_entry);
		invert_entry (w, entry);
		w->sel_entry = entry;
	}
	w->timer = wt_addtimeout (100, move_timer, _w);
}

/*************************** Widget Code *****************************/

static long list_query_geometry (widget_t *, long *, long *, long *, long *);

static long
list_init (void)
{
	return 0;
}

static widget_t *
list_create (widget_class_t *cp)
{
	list_widget_t *wp = malloc (sizeof (list_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (list_widget_t));
	wp->w.class = wt_list_class;

	if (loadfont (wp, NULL, 0)) {
		free (wp);
		return NULL;
	}
	wp->align = AlignLeft;
	wp->sel_entry = -1;
	return (widget_t *)wp;
}

static long
list_delete (widget_t *_w)
{
	list_widget_t *w = (list_widget_t *)_w;

	wt_deltimeout (w->timer);
	if (w->is_realized)
		w_delete (w->w.win);
	w_unloadfont (w->font);
	free_entries (w);
	free (w);
	return 0;
}

static long
list_close (widget_t *_w)
{
	list_widget_t *w = (list_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
list_open (widget_t *_w)
{
	list_widget_t *w = (list_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
list_addchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
list_delchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
list_realize (widget_t *_w, WWIN *parent)
{
	list_widget_t *w = (list_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	list_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	w->w.win = wt_create_window (parent, wd, ht,
			W_NOBORDER|W_MOVE|EV_ACTIVE|EV_MOUSE, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;
	w_setfont (w->w.win, w->font);

	if (w->entries)
		draw_entries (w);

	w->is_realized = 1;
	w->is_open = 1;
	w_open (w->w.win, w->w.x, w->w.y);
	return 0;
}

static long
list_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	list_widget_t *w = (list_widget_t *)_w;

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
list_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	list_widget_t *w = (list_widget_t *)_w;

	*wdp = (w->wd > 0 ? w->wd : 100);
	*wdp = MAX (*wdp, w->usrwd);
	*htp = (w->ht > 0 ? w->ht : 100);
	*htp = MAX (*htp, w->usrht);
	return 0;
}

static long
list_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	list_widget_t *w = (list_widget_t *)_w;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized) {
			w_move (w->w.win, x, y);
		}
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		w->w.w = wd;
		w->w.h = ht;
		if (w->is_realized) {
			w_resize (w->w.win, wd, ht);
			draw_entries (w);
			if (w->sel_entry >= 0)
				invert_entry (w, w->sel_entry);
		}
		ret = 1;
	}
	return ret;
}

static long
list_setopt (widget_t *_w, long key, void *val)
{
	list_widget_t *w = (list_widget_t *)_w;
	short mask = 0;

	switch (key) {
	case WT_XPOS:
		if (list_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (list_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (list_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (list_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_ALIGNMENT:
		w->align = *(long *)val;
		break;

	case WT_ACTION_CB:
		w->select_cb = val;
		break;

	case WT_DRAW_FN:
		w->draw_fn = val;
		break;

	case WT_FONTSIZE:
		if (loadfont (w, NULL, *(long *)val)) {
			return -1;
		}
		break;

	case WT_FONT:
		if (loadfont (w, (char *)val, w->fsize)) {
			return -1;
		}
		break;

	case WT_LABEL:
		if (parse_entries (w, val)) {
			return -1;
		}
		if (w->is_realized) {
			draw_entries (w);
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
list_getopt (widget_t *_w, long key, void *val)
{
	list_widget_t *w = (list_widget_t *)_w;

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

	case WT_ACTION_CB:
		*(void **)val = w->select_cb;
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
list_event (widget_t *_w, WEVENT *ev)
{
	list_widget_t *w = (list_widget_t *)_w;

	switch (ev->type) {
	case EVENT_MPRESS:
		w->mpressed = 1;
		move_timer ((long)w);
		break;

	case EVENT_MRELEASE:
		if (!w->mpressed) {
			break;
		}
		w->mpressed = 0;
		/*
		 * not do not inverting the selected entry when
		 * releasing the mouse button!
		 */
		select_notify (w);
		break;

	case EVENT_ACTIVE:
		/*
		 * XXX What if the mouse was already pressed when the
		 * widget was created? But unfortunately there is no
		 * way to find out the mouse button state...
		 */
		if (w->mpressed) {
			move_timer ((long)w);
		}
		break;

	case EVENT_INACTIVE:
		invert_entry (w, w->sel_entry);
		w->sel_entry = -1;
		wt_deltimeout (w->timer);
		break;

	default:
		return ev;
	}

	return NULL;
}

static long
list_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_list_class = {
	"list", 0,
	list_init,
	list_create,
	list_delete,
	list_close,
	list_open,
	list_addchild,
	list_delchild,
	list_realize,
	list_query_geometry,
	list_query_minsize,
	list_reshape,
	list_setopt,
	list_getopt,
	list_event,
	list_changes,
	list_changes
};

widget_class_t *wt_list_class = &_wt_list_class;
