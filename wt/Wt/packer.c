/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * $Id: packer.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 *
 * Tk-like packer widget (written from scratch). Supports the following
 * options (the first value is the default):
 *	-fill	{none,x,y,both}
 *	-expand	{0,1}
 *	-side	{top,bottom,left,right}
 *	-anchor	{center,n,ne,e,se,s,sw,w,nw}
 *	-padx	<padding in horizontal direction, default 0>
 *	-pady	<padding in vertical direction, default 0>
 *	-ipadx	<child internal padding in horizontal direction, default 0>
 *	-ipady	<child internal padding in vertical direction, default 0>
 *
 * The widgets are packed in the order in which they are specified using the
 * WT_PACK_WIDGET option. The WT_PACK_WIDGET config option marks widgets.
 * The WT_PACK_INFO options specifies the packing configuration for the
 * widgets that are currently marked unmarks them.
 *
 * Assuming you have created a packer widget `pack' which has three childs --
 * the buttons `b1', `b2' and `b3' -- the following example creates the
 * following layout:
 *
 * +--------------------------+
 * |+----------+ +-----------+|
 * ||    b2    | |    b3     ||
 * |+----------+ +-----------+|
 * |+------------------------+|
 * ||          b1            ||
 * |+------------------------+|
 * +--------------------------+
 *
 * wt_setopt (pack,
 *	WT_PACK_WIDGET, b1,
 *	WT_PACK_INFO,   "-side bottom -fill x",
 *	WT_PACK_WIDGET, b2,
 *	WT_PACK_WIDGET, b3,
 *	WT_PACK_INFO,   "-side left",
 *	WT_EOL);
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"


typedef enum {
	FillNone,
	FillX,
	FillY,
	FillBoth
} fill_style_t;

typedef enum {
	AnchorCenter,
	AnchorN,
	AnchorNE,
	AnchorE,
	AnchorSE,
	AnchorS,
	AnchorSW,
	AnchorW,
	AnchorNW
} anchor_type_t;

typedef enum {
	SideTop,
	SideBottom,
	SideRight,
	SideLeft
} side_t;

#define IS_VERT(s)	((s) == SideTop  || (s) == SideBottom)
#define IS_HORZ(s)	(!IS_VERT (s))

typedef struct {
	long x, y, w, h;
} rect_t;

typedef struct {
	fill_style_t	fill;
	anchor_type_t	anchor;
	side_t		side;
	long		padx, pady;
	long		ipadx, ipady;
	short		expand;
	rect_t		parcel;
	long		wd, ht;
} packer_info_t;


typedef struct {
	widget_t w;
	short is_open;
	short is_realized;

	long needed_wd, needed_ht;
	short usrwd, usrht;
} packer_widget_t;

/***********************************************************************/

static inline char *
skipspace (char *cp)
{
	while (*cp && isspace (*cp))
		++cp;
	return cp;
}

static inline char *
skipword (char *cp)
{
	while (*cp && isspace (*cp))
		++cp;
	while (*cp && !isspace (*cp))
		++cp;
	while (*cp && isspace (*cp))
		++cp;
	return cp;
}

#define SEQ(s1,s2)	(!strncasecmp (s1, s2, strlen (s2)))

static packer_info_t *
parse_packer_info (char *s)
{
	packer_info_t *p;
	char *cp;

	p = malloc (sizeof (packer_info_t));
	if (!p)
		return NULL;
	memset (p, 0, sizeof (packer_info_t));

	p->fill = FillNone;
	p->side = SideTop;
	p->anchor = AnchorCenter;

	if (!s)
		return p;

	for (cp = skipspace (s); *cp; ) {
		if (SEQ (cp, "-anchor")) {
			cp = skipword (cp);
			if (SEQ (cp, "center")) {
				p->anchor = AnchorCenter;
			} else if (SEQ (cp, "ne")) {
				p->anchor = AnchorNE;
			} else if (SEQ (cp, "n")) {
				p->anchor = AnchorN;
			} else if (SEQ (cp, "se")) {
				p->anchor = AnchorSE;
			} else if (SEQ (cp, "e")) {
				p->anchor = AnchorE;
			} else if (SEQ (cp, "sw")) {
				p->anchor = AnchorSW;
			} else if (SEQ (cp, "s")) {
				p->anchor = AnchorS;
			} else if (SEQ (cp, "nw")) {
				p->anchor = AnchorNW;
			} else if (SEQ (cp, "w")) {
				p->anchor = AnchorW;
			}
		} else if (SEQ (cp, "-side")) {
			cp = skipword (cp);
			if (SEQ (cp, "top")) {
				p->side = SideTop;
			} else if (SEQ (cp, "bottom")) {
				p->side = SideBottom;
			} else if (SEQ (cp, "left")) {
				p->side = SideLeft;
			} else if (SEQ (cp, "right")) {
				p->side = SideRight;
			}
		} else if (SEQ (cp, "-fill")) {
			cp = skipword (cp);
			if (SEQ (cp, "none")) {
				p->fill = FillNone;
			} else if (SEQ (cp, "x")) {
				p->fill = FillX;
			} else if (SEQ (cp, "y")) {
				p->fill = FillY;
			} else if (SEQ (cp, "both")) {
				p->fill = FillBoth;
			}
		} else if (SEQ (cp, "-expand")) {
			cp = skipword (cp);
			p->expand = atol (cp);
		} else if (SEQ (cp, "-padx")) {
			cp = skipword (cp);
			p->padx = atol (cp);
		} else if (SEQ (cp, "-pady")) {
			cp = skipword (cp);
			p->pady = atol (cp);
		} else if (SEQ (cp, "-ipadx")) {
			cp = skipword (cp);
			p->ipadx = atol (cp);
		} else if (SEQ (cp, "-ipady")) {
			cp = skipword (cp);
			p->ipady = atol (cp);
		}
		do {
			cp = skipword (cp);
		} while (*cp && *cp != '-');
	}
	return p;
}

static packer_info_t *
clone_packer_info (packer_info_t *p)
{
	packer_info_t *newp = malloc (sizeof (packer_info_t));
	if (!newp)
		return NULL;
	*newp = *p;
	return newp;
}

static void
free_packer_info (widget_t *w)
{
	if (w->geom_info && w->geom_info != (void *)1) {
		free (w->geom_info);
		w->geom_info = NULL;
	}
}

/***********************************************************************/

/*
 * increment `space' by the amount of space needed for widget `w'
 */
static inline void
get_space_one (rect_t *space, widget_t *w)
{
	packer_info_t *p = w->geom_info;
	long wd, ht;

	wt_minsize (w, &p->wd, &p->ht);
	wd = p->wd + 2*(p->padx + p->ipadx);
	ht = p->ht + 2*(p->pady + p->ipady);

	switch (p->side) {
	case SideTop:
	case SideBottom:
		space->h += ht;
		if (space->w < wd)
			space->w = wd;
		break;

	case SideLeft:
	case SideRight:
		space->w += wd;
		if (space->h < ht)
			space->h = ht;
		break;
	}
}

/*
 * get the minimal space needed for packing the widgets pointed at by 'w'.
 */
static int
get_space (widget_t *w, long *wd, long *ht)
{
	widget_t *wp;
	rect_t space;

	if (!w) {
		*wd = 0;
		*ht = 0;
		return 0;
	}
	for (wp = w; wp->next; wp = wp->next)
		;
	space.w = 0;
	space.h = 0;
	for ( ;; wp = wp->prev) {
		if (!wp->geom_info || wp->geom_info == (void *)1) {
			wp->geom_info = parse_packer_info (NULL);
			if (!wp->geom_info)
				return -1;
		}
		get_space_one (&space, wp);
		if (wp == w)
			break;
	}
	*wd = space.w;
	*ht = space.h;
	return 0;
}

/*
 * get the amount of space by which the widgets parcel must be expanded
 * to fill the remaining space in the packer window.
 */
static inline long
get_expand_space (rect_t *space, widget_t *wp)
{
	long isvert, i, exp_space;
	packer_info_t *p;

	p = wp->geom_info;
	isvert = IS_VERT (p->side);
	exp_space = isvert ? space->h : space->w;
	for (i = 0; wp; wp = wp->next) {
		p = wp->geom_info;
		if (isvert ^ IS_VERT (p->side))
			break;
		if (p->expand)
			++i;
		exp_space -= isvert ? p->ht + 2*(p->ipady + p->pady)
				    : p->wd + 2*(p->ipadx + p->padx);
	}
	if (wp && i > 0) {
		long wd, ht;
		get_space (wp, &wd, &ht);
		exp_space -= isvert ? ht : wd;
	}
	return (i > 0 && exp_space > 0) ? exp_space/i : 0;
}

/*
 * allocate a parcel for widget `w' and reduce `space' by the space needed
 * for the parcel.
 */
static void
get_parcel (rect_t *space, widget_t *w)
{
	packer_info_t *p = w->geom_info;
	long wd, ht, exp_space;

	wd = p->wd + 2*(p->padx + p->ipadx);
	ht = p->ht + 2*(p->pady + p->ipady);

	exp_space = p->expand ? get_expand_space (space, w) : 0;

	switch (p->side) {
	case SideTop:
		ht += exp_space;
		p->parcel.x = space->x;
		p->parcel.y = space->y;
		p->parcel.w = MAX (wd, space->w);
		p->parcel.h = ht;
		space->y   += ht;
		space->h   -= ht;
		break;

	case SideBottom:
		ht += exp_space;
		p->parcel.x = space->x;
		p->parcel.y = space->y + space->h - ht;
		p->parcel.w = MAX (wd, space->w);
		p->parcel.h = ht;
		space->h   -= ht;
		break;

	case SideLeft:
		wd += exp_space;
		p->parcel.x = space->x;
		p->parcel.y = space->y;
		p->parcel.w = wd;
		p->parcel.h = MAX (ht, space->h);
		space->x   += wd;
		space->w   -= wd;
		break;

	case SideRight:
		wd += exp_space;
		p->parcel.x = space->x + space->w - wd;
		p->parcel.y = space->y;
		p->parcel.w = wd;
		p->parcel.h = MAX (ht, space->h);
		space->w   -= wd;
		break;
	}
}

/*
 * place a widget within its parcel
 */
static void
place_widget (widget_t *w)
{
	packer_info_t *p = w->geom_info;
	long x, y, wd, ht;

	wd = (p->fill == FillX || p->fill == FillBoth)
		? p->parcel.w - 2*p->padx
		: p->wd + 2*p->ipadx;

	ht = (p->fill == FillY || p->fill == FillBoth)
		? p->parcel.h - 2*p->pady
		: p->ht + 2*p->ipady;

	wt_reshape (w, WT_UNSPEC, WT_UNSPEC, wd, ht);
	/*
	 * the widget might not be resizeable, so ask again
	 */
	wt_geometry (w, &x, &y, &wd, &ht);

	switch (p->anchor) {
	case AnchorSW:
	case AnchorW:
	case AnchorNW:
		x = p->parcel.x + p->padx;
		break;
	case AnchorN:
	case AnchorCenter:
	case AnchorS:
		x = p->parcel.x + (p->parcel.w - wd)/2;
		break;
	case AnchorNE:
	case AnchorE:
	case AnchorSE:
		x = p->parcel.x + p->parcel.w - wd - p->padx;
		break;
	}
	switch (p->anchor) {
	case AnchorNW:
	case AnchorN:
	case AnchorNE:
		y = p->parcel.y + p->pady;
		break;
	case AnchorW:
	case AnchorCenter:
	case AnchorE:
		y = p->parcel.y + (p->parcel.h - ht)/2;
		break;
	case AnchorSW:
	case AnchorS:
	case AnchorSE:
		y = p->parcel.y + p->parcel.h - ht - p->pady;
		break;
	}
	wt_reshape (w, x, y, WT_UNSPEC, WT_UNSPEC);
}

/*
 * place all the child widgets
 */
static int
pack_widgets (packer_widget_t *w, int firsttime)
{
	widget_t *wp;
	WWIN *win = w->w.win;
	rect_t space;

	if (!w->w.childs)
		return 0;

	if (get_space (w->w.childs, &w->needed_wd, &w->needed_ht) < 0)
		return -1;

	space.x = 0;
	space.y = 0;
	space.w = MAX (w->w.w, w->needed_wd);
	space.h = MAX (w->w.h, w->needed_ht);

	for (wp = w->w.childs; wp; wp = wp->next) {
		get_parcel (&space, wp);
		place_widget (wp);
		if (firsttime && (*wp->class->realize) (wp, win) < 0)
			return -1;
	}
	return 0;
}

/***********************************************************************/

static long packer_query_geometry (widget_t *, long *, long *, long *, long *);

static long
packer_init (void)
{
	return 0;
}

static widget_t *
packer_create (widget_class_t *cp)
{
	packer_widget_t *wp = malloc (sizeof (packer_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (packer_widget_t));
	wp->w.class = wt_packer_class;
	return (widget_t *)wp;
}

static long
packer_delete (widget_t *_w)
{
	packer_widget_t *w = (packer_widget_t *)_w;
	widget_t *wp, *next;

	for (wp = w->w.childs; wp; wp = next) {
		next = wp->next;
		free_packer_info (wp);
		(*wp->class->delete) (wp);
	}
	if (w->is_realized)
		w_delete (w->w.win);
	free (w);
	return 0;
}

static long
packer_close (widget_t *_w)
{
	packer_widget_t *w = (packer_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
packer_open (widget_t *_w)
{
	packer_widget_t *w = (packer_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
packer_addchild (widget_t *parent, widget_t *w)
{
	widget_t *wp;

	for (wp = parent->childs; wp && wp->next; wp = wp->next)
		;
	wt_add_after (parent, wp, w);
	return 0;
}

static long
packer_delchild (widget_t *parent, widget_t *w)
{
	free_packer_info (w);
	wt_remove (w);
	return 0;
}

static long
packer_realize (widget_t *_w, WWIN *parent)
{
	packer_widget_t *w = (packer_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	packer_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	w->w.win = wt_create_window (parent, wd, ht,
		W_MOVE|W_CONTAINER|W_NOBORDER, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;
	if (pack_widgets (w, 1) < 0) {
		w_delete (w->w.win);
		return -1;
	}
	w->is_realized = 1;
	w->is_open = 1;
	w_open (w->w.win, w->w.x, w->w.y);
	return 0;
}

static long
packer_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	packer_widget_t *w = (packer_widget_t *)_w;

	*xp = w->w.x;
	*yp = w->w.y;

	if (w->w.w > 0 && w->w.h > 0) {
		*wdp = w->w.w;
		*htp = w->w.h;
	} else {
		(*_w->class->query_minsize) (_w, wdp, htp);
	}
	if (*wdp <= 0)
		*wdp = 10;
	if (*htp <= 0)
		*htp = 10;
	return 0;
}

static long
packer_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	packer_widget_t *w = (packer_widget_t *)_w;

	get_space (_w->childs, wdp, htp);
	*wdp = MAX (*wdp, w->usrwd);
	*htp = MAX (*htp, w->usrht);
	return 0;
}

static long
packer_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	packer_widget_t *w = (packer_widget_t *)_w;
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
			pack_widgets (w, 0);
		}
		ret = 1;
	}
	return ret;
}

static int
add_managed_child (packer_widget_t *w, widget_t *child)
{
	int not_managed = 0;
	widget_t *wp, *prev;

	for (wp = w->w.childs; wp; wp = wp->next) {
		if (wp == child) {
			if (!child->geom_info) {
				wt_remove (child);
				not_managed = 1;
			}
			free_packer_info (wp);
			wp->geom_info = (void *)1;
			break;
		}
	}
	if (!wp) {
		return -1;
	}
	if (not_managed) {
		prev = NULL;
		for (wp = w->w.childs; wp && wp->geom_info; ) {
			prev = wp;
			wp = wp->next;
		}
		if (prev) {
			wt_add_after ((widget_t *)w, prev, child);
		} else {
			wt_add_before ((widget_t *)w, w->w.childs, child);
		}
	}
	return 0;
}

static long
packer_setopt (widget_t *_w, long key, void *val)
{
	packer_widget_t *w = (packer_widget_t *)_w;
	packer_info_t *p;
	widget_t *wp;
	short mask = 0;
	int i;

	switch (key) {
	case WT_XPOS:
		if (packer_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (packer_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (packer_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (packer_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_PACK_WIDGET:
		if (add_managed_child (w, val) < 0)
			return -1;
		break;

	case WT_PACK_INFO:
		p = parse_packer_info (val);
		if (!p)
			return -1;
		for (i = 0, wp = w->w.childs; wp; wp = wp->next) {
			if (wp->geom_info == (void *)1) {
				if (i == 0) {
					i = 1;
					wp->geom_info = p;
				} else {
					wp->geom_info = clone_packer_info (p);
					if (!wp->geom_info)
						break;
				}
			}
		}
		if (i == 0)
			free (p);
		if (wp != NULL)
			return -1;
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
packer_getopt (widget_t *_w, long key, void *val)
{
	packer_widget_t *w = (packer_widget_t *)_w;

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

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
packer_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long
packer_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_packer_class = {
	"packer", 0,
	packer_init,
	packer_create,
	packer_delete,
	packer_close,
	packer_open,
	packer_addchild,
	packer_delchild,
	packer_realize,
	packer_query_geometry,
	packer_query_minsize,
	packer_reshape,
	packer_setopt,
	packer_getopt,
	packer_event,
	packer_changes,
	packer_changes
};

widget_class_t *wt_packer_class = &_wt_packer_class;
