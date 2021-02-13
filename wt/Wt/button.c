/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996 Kay Roemer, Eero Tamminen
 *
 * radio/switch/button/label(/infobox/icon) widget.
 *
 * - Widget can have both text and icon (or anything else which size is
 *   defined in user supplied BITMAP struct) besides the possible button
 *   stuff.  The icon is drawed by an application provided function.
 * - LabelText[Horz|Vert]Out mode flags can be used to control whether the
 *   text is written on or beside the icon (in direction defined by Align
 *   defines with WT_ALIGN_* options).
 * - WT_MODE can be used to change push/check/radiobuttons behaviour
 *   or to give labels a border.
 * - Bordered button types use same height, like do radio and check buttons.
 *   Image in the button may increase this though. Unbordered button types
 *   don't have 'redundand' empty space around them.
 * - Buttons in the radiobutton and checkbutton widgets are always the same
 *   size (see the BUTTON_SIZE define) and vertically centered to the widget.
 * - Widget text is redrawn in visually most economical way to prevent
 *   flickering with widgets which label text is frequently updated.
 *
 * Changes 25.5.1996 ++et:
 * - ButtonStatePressed Label widgets are re-inverted after their icon/text
 *   is changed & redrawn.
 *
 * Changes 28.7.96 yak:
 * - made WT_ALIGNMENT act as WT_ALIGN_HORZ for backward compatibility.
 * - fixed a bug in redraw_text() for align_horz == AlignLeft.
 *
 * Changes 18.8-96 ++et:
 * - A couple of ButtonModeRadio additions.
 * - Bugfix to label border drawing.
 *
 * $Id: button.c,v 1.2 2000-09-10 10:16:51 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

/* border sizes for the bordered widget */
#ifdef WTINYSCREEN
#define XBORDER		(WT_DEFAULT_XBORDER+2)
#define YBORDER		(WT_DEFAULT_YBORDER+2)
#else
#define XBORDER		(WT_DEFAULT_XBORDER+4)
#define YBORDER		(WT_DEFAULT_YBORDER+4)
#endif

/* for radio and check buttons */
#define BUTTON_SIZE	16
#define BUTTON_HALF	(BUTTON_SIZE/2-1)

/* button/text/icon widget component distances */
#ifdef WTINYSCREEN
#define XOFFSET		2
#define YOFFSET		0
#else
#define XOFFSET		4
#define YOFFSET		2
#endif

typedef struct {
	widget_t w;
	WFONT *font;
	short fsize;
	short is_open;
	short is_realized;
	enum { PushButton, RadioButton, CheckButton, Label } type;
	void (*press_cb) (widget_t *w, int pressed);
	short is_selected;
	short mode;				/* border flag */
	char *text;
	short halign;
	short valign;
	short align_mode;			/* text horz/vert out flags */
	short textlen;
	short oldwidth;
	short event_mask;
	WEVENT *(*event_cb) (widget_t *w, WEVENT *ev);
	void (*draw_fn) (widget_t *w, short x, short y, BITMAP *icon);
	BITMAP *icon;
	short usrwd, usrht;
} button_widget_t;


/* position opposite the text (if not AlignCenter) */
static void
draw_icon (button_widget_t *w)
{
	short xborder = 0, yborder = 0, xoffset, yoffset;

	if (!(w->icon && w->draw_fn))
		return;

	/* button type offsets and borders */
	if (w->type == RadioButton || w->type == CheckButton)
		xoffset = BUTTON_SIZE + XOFFSET;
	else {
		if (w->type != Label || w->mode == LabelModeWithBorder) {
			xborder = XBORDER;
			yborder = YBORDER;
		}
		xoffset = 0;
	}

	/* alignment specific offsets */

	switch (w->valign) {
	case AlignTop:
		yoffset = w->w.h - w->icon->height - yborder;
		break;
	case AlignBottom:
		yoffset = yborder;
		break;
	default:
		yoffset = (w->w.h - w->icon->height) >> 1;
	}

	switch (w->halign) {
	case AlignLeft:
		xoffset = w->w.w - w->icon->width - xborder;
		break;
	case AlignRight:
		xoffset += xborder;
		break;
	default:
		xoffset += (w->w.w - w->icon->width - xoffset) >> 1;
	}
	(*w->draw_fn) ((widget_t *)w, xoffset, yoffset, w->icon);
	if(w->type == Label && w->is_selected && w->is_realized)
	{
	  /* (re)invert the image */
	  w_setmode(w->w.win, M_INVERS);
	  w_pbox(w->w.win, xoffset, yoffset, w->icon->width, w->icon->height);
	}	
}

static void
redraw_text (button_widget_t *w)
{
	short xborder = 0, yborder = 0, xoffset, yoffset, width, height;
	WWIN *win = w->w.win;

	if (!(w->text && (width = w_strlen (w->font, w->text))))
		return;

	/* button type offsets and borders */
	if (w->type == RadioButton || w->type == CheckButton)
		xoffset = BUTTON_SIZE + XOFFSET;
	else {
		if (w->type != Label || w->mode == LabelModeWithBorder) {
			xborder = XBORDER;
			yborder = YBORDER;
		}
		xoffset = 0;
	}

	/* alignment specific offsets */

	height = w->font->height;

	switch (w->valign) {
	case AlignTop:
		yoffset = yborder;
		break;
	case AlignBottom:
		yoffset = w->w.h - height - yborder;
		break;
	default:
		yoffset = (w->w.h - height) >> 1;
	}

	switch (w->halign) {
	case AlignLeft:
		xoffset += xborder;
		break;
	case AlignRight:
		xoffset = w->w.w - width - xborder;
		break;
	default:
		xoffset += (w->w.w - width - xoffset) >> 1;
	}

	xborder = w->oldwidth - width;
	/* need to clear old text? */
	if (xborder > 0) {
		w_setmode (win, M_CLEAR);
		switch (w->halign) {
		case AlignLeft:
			w_pbox (win, xoffset+width, yoffset, xborder, height);
			break;
		case AlignRight:
			w_pbox (win, xoffset-xborder, yoffset, xborder, height);
			break;
		default:
			xborder = (xborder >> 1) + 1;
			w_pbox (win, xoffset-xborder, yoffset, xborder, height);
			w_pbox (win, xoffset+width, yoffset, xborder, height);
		}
	}
	w_printstring (win, xoffset, yoffset, w->text);
	if(w->type == Label && w->is_selected && w->is_realized)
	{
	  /* (re)invert the widget */
	  w_setmode(w->w.win, M_INVERS);
	  if(xborder <= 0)
		w_pbox(w->w.win, xoffset, yoffset, width, height);
	  else
	  	if(w->halign == AlignCenter)
			w_pbox(w->w.win, xoffset-xborder, yoffset, width+xborder*2, height);
		else
			w_pbox(w->w.win, xoffset, yoffset, width+xborder, height);
	}	
	w->oldwidth = width;
}

static void
button_draw (button_widget_t *w)
{
	WWIN *win = w->w.win;
	int wd = w->w.w, ht = w->w.h;

	w_setmode (win, M_CLEAR);
	w_pbox (win, 0, 0, wd, ht);

	switch (w->type) {
	case Label:
		if (w->mode == LabelModeWithBorder) {
			w_setmode(win, M_DRAW);
			w_box(win, 0, 0, wd, ht);
		}
		break;

	case PushButton:
		wt_box3d (w->w.win, 0, 0, wd, ht);
		break;

	case RadioButton:
		ht = ht / 2 - 1;
		wt_circle3d (w->w.win, BUTTON_HALF, ht, BUTTON_HALF);
		wt_circle3d_press (w->w.win, BUTTON_HALF, ht, BUTTON_HALF);
		break;

	case CheckButton:
		ht = ht / 2 - BUTTON_HALF - 1;
		wt_box3d (w->w.win, 0, ht, BUTTON_SIZE, BUTTON_SIZE);
		wt_box3d_press (w->w.win, 0, ht, BUTTON_SIZE, BUTTON_SIZE, 0);
		break;
	}
	draw_icon (w);
	redraw_text (w);
}

static long
button_press (button_widget_t *w)
{
	int ht = w->w.h;

	switch (w->type) {
	case Label:
	  	w_setmode (w->w.win, M_INVERS);
		w_pbox (w->w.win, 0, 0, w->w.w, ht);
		break;

	case PushButton:
		wt_box3d_press (w->w.win, 0, 0, w->w.w, ht, 1);
		break;

	case RadioButton:
		ht = ht / 2 - 1;
		wt_circle3d_mark (w->w.win, BUTTON_HALF, ht, BUTTON_HALF);
		break;

	case CheckButton:
		ht = ht / 2 - BUTTON_HALF - 1;
		wt_box3d_mark (w->w.win, 0, ht, BUTTON_SIZE, BUTTON_SIZE);
		break;

	default:
		return 0;
	}
	return 0;
}

static long
button_release (button_widget_t *w)
{
	int ht = w->w.h;

	switch (w->type) {
	case Label:
	  	w_setmode (w->w.win, M_INVERS);
		w_pbox (w->w.win, 0, 0, w->w.w, ht);
		break;

	case PushButton:
		wt_box3d_release (w->w.win, 0, 0, w->w.w, ht, 1);
		break;

	case RadioButton:
		ht = ht / 2 - 1;
		wt_circle3d_unmark (w->w.win, BUTTON_HALF, ht, BUTTON_HALF);
		break;

	case CheckButton:
		ht = ht / 2 - BUTTON_HALF - 1;
		wt_box3d_unmark (w->w.win, 0, ht, BUTTON_SIZE, BUTTON_SIZE);
		break;

	default:
		return 0;
	}
	return 0;
}

static void
button_unselect (widget_t *parent)
{
	button_widget_t *bw;
	widget_t *w;
	long state = 0;

	for (w = parent->childs; w; w = w->next) {
		bw = (button_widget_t *)w;
		if (w->class == wt_radiobutton_class ||
		    (w->class == wt_pushbutton_class &&
		     bw->mode == ButtonModeRadio)) {
			if (bw->is_selected)
				(*w->class->setopt) (w, WT_STATE, &state);
		}
	}
}

static long
button_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	button_widget_t *w = (button_widget_t *)_w;

	*xp = w->w.x;
	*yp = w->w.y;
	(*_w->class->query_minsize) (_w, wdp, htp);
	if (w->w.w > *wdp)
		*wdp = w->w.w;
	if (w->w.h > *htp)
		*htp = w->w.h;
	return 0;
}

static long
button_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	button_widget_t *w = (button_widget_t *)_w;
	int wd = 0, ht = 0;

	if (w->text) {
		wd = w_strlen (w->font, w->text);
	}
	ht = w->font->height;

	if (w->icon) {
		if (w->text && w->halign != AlignCenter &&
		    (w->align_mode & LabelTextHorzOut))
			wd += w->icon->width + XOFFSET;
		else
			wd = MAX(wd, w->icon->width);

		if (w->text && w->valign != AlignCenter &&
		    (w->align_mode & LabelTextVertOut))
			ht += w->icon->height + YOFFSET;
		else
			ht = MAX(ht, w->icon->height);
	}

	if (w->type == RadioButton || w->type == CheckButton) {
		if (ht < BUTTON_SIZE)
			ht = BUTTON_SIZE;
		wd += BUTTON_SIZE + XOFFSET;
	} else {
		if (w->type != Label || w->mode == LabelModeWithBorder) {
			wd += 2 * XBORDER;
			ht += 2 * YBORDER;
		}
	}
	*wdp = MAX (wd, w->usrwd);
	*htp = MAX (ht, w->usrht);
	return 0;
}

static long
button_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	button_widget_t *w = (button_widget_t *)_w;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized)
			w_move (w->w.win, x, y);
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		long minwd, minht;
		button_query_minsize (_w, &minwd, &minht);
		if (wd < minwd)
			wd = minwd;
		if (ht < minht)
			ht = minht;
		if (wd != w->w.w || ht != w->w.h) {
			w->w.w = wd;
			w->w.h = ht;
			if (w->is_realized) {
				w_resize (w->w.win, wd, ht);
				button_draw (w);
				if (w->is_selected)
					button_press (w);
#if 0
				if (w->text && !strcmp ("aa", w->text)) {
					w_flush();
					sleep (1);
				}
#endif
			}
			ret = 1;
		}
	}
	return ret;
}

static int
button_loadfont (button_widget_t *w, char *fname, short fsize)
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

	
static long
button_init (void)
{
	return 0;
}

static widget_t *
button_create (widget_class_t *cp)
{
	button_widget_t *wp = malloc (sizeof (button_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (button_widget_t));
	wp->w.class = cp;

	/* defaults */
	wp->valign = AlignCenter;
	wp->align_mode = LabelTextHorzOut;	/* no text over image */

	if (cp == wt_pushbutton_class) {
		wp->type = PushButton;
		wp->mode = ButtonModePush;
		wp->halign = AlignCenter;
	} else if (cp == wt_checkbutton_class) {
		wp->type = CheckButton;
		wp->mode = ButtonModeToggle;
		wp->halign = AlignLeft;
	} else if (cp == wt_radiobutton_class) {
		wp->type = RadioButton;
		wp->mode = ButtonModeRadio;
		wp->halign = AlignLeft;
	} else {
		wp->type = Label;
		wp->mode = LabelModeNoBorder;
		wp->halign = AlignCenter;
		wp->valign = AlignBottom;
		wp->align_mode = LabelTextVertOut;
	}

	if (button_loadfont(wp, NULL, 0)) {
		free(wp);
		return NULL;
	}
	return (widget_t *)wp;
}

static long
button_delete (widget_t *_w)
{
	button_widget_t *w = (button_widget_t *)_w;

	if (w->is_realized)
		w_delete (w->w.win);
	w_unloadfont (w->font);
	if (w->text)
		free (w->text);
	free (w);
	return 0;
}

static long
button_close (widget_t *_w)
{
	button_widget_t *w = (button_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
button_open (widget_t *_w)
{
	button_widget_t *w = (button_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
button_addchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
button_delchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
button_realize (widget_t *_w, WWIN *parent)
{
	button_widget_t *w = (button_widget_t *)_w;
	long x, y, wd, ht;
	short events;

	if (w->is_realized)
		return -1;

	button_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.x = x;
	w->w.y = y;
	w->w.w = wd;
	w->w.h = ht;

	if (w->type == Label)
		events = W_NOBORDER | W_MOVE | w->event_mask;
	else
		events = W_NOBORDER | W_MOVE | EV_MOUSE;
	w->w.win = wt_create_window (parent, wd, ht, events, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;
	w_setfont (w->w.win, w->font);
	button_draw (w);
	if (w->is_selected)
		button_press (w);
	w_open (w->w.win, w->w.x, w->w.y);
	w->is_realized = 1;
	w->is_open = 1;
	return 0;
}

static long
button_setopt (widget_t *_w, long key, void *val)
{
	button_widget_t *w = (button_widget_t *)_w;
	short len, mask = 0;
	int needredraw = 0;
	char *tmp;

	switch (key) {
	case WT_XPOS:
		if (button_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (button_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (button_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (button_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_LABEL:
		if (val) {
			len = strlen ((char *)val);
			if (len >= w->textlen)
			{
				if (!(tmp = malloc (len+1)))
					return -1;
				w->textlen = len+1;
				if (w->text)
					free (w->text);
				w->text = tmp;
			}
			strcpy (w->text, (char*)val);
			if (w->is_realized) {
				redraw_text (w);
			}
		} else {
			if (w->text)
				free (w->text);
			w->text = NULL;
		}
		return 0;

	case WT_FONTSIZE:
		if (button_loadfont (w, NULL, *(long *)val)) {
			return -1;
		}
		needredraw = 1;
		break;

	case WT_FONT:
		if (button_loadfont (w, (char *)val, w->fsize)) {
			return -1;
		}
		needredraw = 1;
		break;

	case WT_MODE:
		w->mode = *(long *)val;
		needredraw = 1;
		break;

	case WT_STATE:
		if (w->is_selected && *(long *)val == ButtonStateReleased) {
			if (w->is_realized) {
				if (w->press_cb)
					(*w->press_cb) (_w, 0);
				button_release (w);
			}
			w->is_selected = 0;
		} else if (!w->is_selected &&
			   *(long *)val == ButtonStatePressed) {
			if (w->is_realized) {
				if (w->mode == ButtonModeRadio)
					button_unselect (_w->parent);
				if (w->press_cb)
					(*w->press_cb) (_w, 1);
				button_press (w);
			}
			w->is_selected = 1;
		}
		break;

	case WT_ALIGN_MODE:
		w->align_mode = *(long *)val;
		needredraw = 1;
		break;

	case WT_ALIGN_HORZ:
	case WT_ALIGNMENT:
		w->halign = *(long *)val;
		needredraw = 1;
		break;

	case WT_ALIGN_VERT:
		w->valign = *(long *)val;
		needredraw = 1;
		break;

	case WT_ICON:
		w->icon = (BITMAP *)val;
		needredraw = 1;
		break;

	case WT_DRAW_FN:
		w->draw_fn = val;
		needredraw = 1;
		break;

	case WT_ACTION_CB:
		w->press_cb = val;
		break;

	case WT_EVENT_MASK:
		w->event_mask = *(long *)val;
		break;

	case WT_EVENT_CB:
		w->event_cb = val;
		break;

	default:
		return -1;
	}

	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	if (w->is_realized && needredraw) {
		button_draw (w);
		if (w->is_selected) {
			button_press (w);
		}
	}
	return 0;
}

static long
button_getopt (widget_t *_w, long key, void *val)
{
	button_widget_t *w = (button_widget_t *)_w;

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

	case WT_LABEL:
		*(char **)val = w->text;
		break;

	case WT_FONTSIZE:
		*(long *)val = w->fsize;
		break;

	case WT_MODE:
		*(long *)val = w->mode;
		break;

	case WT_STATE:
		*(long *)val = w->is_selected
			? ButtonStatePressed
			: ButtonStateReleased;
		break;

	case WT_ALIGN_MODE:
		*(long *)val = w->align_mode;
		break;

	case WT_ALIGN_HORZ:
	case WT_ALIGNMENT:
		*(long *)val = w->halign;
		break;

	case WT_ALIGN_VERT:
		*(long *)val = w->valign;
		break;

	case WT_ICON:
		*(BITMAP **)val = w->icon;
		break;

	case WT_DRAW_FN:
		*(void **)val = w->draw_fn;
		break;

	case WT_ACTION_CB:
		*(void **)val = w->press_cb;
		break;

	case WT_EVENT_MASK:
		*(long *)val = w->event_mask;
		break;

	case WT_EVENT_CB:
		*(void **)val = w->event_cb;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
button_event (widget_t *_w, WEVENT *ev)
{
	button_widget_t *w = (button_widget_t *)_w;

	if (w->type == Label) {
		if (w->event_cb)
			return (*w->event_cb) (_w, ev);
		else
			return ev;
	}

	switch (ev->type) {
	case EVENT_MPRESS:
		if (!(ev->key & BUTTON_LEFT))
			break;

		switch (w->mode) {
		case ButtonModePush:
		case ButtonModeRadio:
			if (!w->is_selected) {
				if (w->mode == ButtonModeRadio)
					button_unselect (_w->parent);
				w->is_selected = 1;
				button_press (w);
				/* may delete widget so we can't reference
				 * it after calling this
				 * */
				if (w->press_cb)
					(*w->press_cb) (_w, 1);
			}
			break;

		case ButtonModeToggle:
			if (w->press_cb)
				(*w->press_cb) (_w, !w->is_selected);
			if (w->is_selected) {
				w->is_selected = 0;
				button_release (w);
			} else {
				w->is_selected = 1;
				button_press (w);
			}
			break;
		}
		break;

	case EVENT_MRELEASE:
		if (!(ev->key & BUTTON_LEFT))
			break;

		if (w->mode == ButtonModePush && w->is_selected) {
			w->is_selected = 0;
			button_release (w);
			/* may delete widget so we can't reference it after
			 * calling this
			 */
			if (w->press_cb) {
				short x, y, r;
				r = w_querymousepos (w->w.win, &x, &y);
				(*w->press_cb) (_w, r ? -1 : 0);
			}
		}
		break;

	default:
		return ev;
	}

	return NULL;
}

static long
button_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_pushbutton_class = {
	"pushbutton", 0,
	button_init,
	button_create,
	button_delete,
	button_close,
	button_open,
	button_addchild,
	button_delchild,
	button_realize,
	button_query_geometry,
	button_query_minsize,
	button_reshape,
	button_setopt,
	button_getopt,
	button_event,
	button_changes,
	button_changes
};

static widget_class_t _wt_radiobutton_class = {
	"radiobutton", 0,
	button_init,
	button_create,
	button_delete,
	button_close,
	button_open,
	button_addchild,
	button_delchild,
	button_realize,
	button_query_geometry,
	button_query_minsize,
	button_reshape,
	button_setopt,
	button_getopt,
	button_event,
	button_changes,
	button_changes
};

static widget_class_t _wt_checkbutton_class = {
	"checkbutton", 0,
	button_init,
	button_create,
	button_delete,
	button_close,
	button_open,
	button_addchild,
	button_delchild,
	button_realize,
	button_query_geometry,
	button_query_minsize,
	button_reshape,
	button_setopt,
	button_getopt,
	button_event,
	button_changes,
	button_changes
};

static widget_class_t _wt_label_class = {
	"label", 0,
	button_init,
	button_create,
	button_delete,
	button_close,
	button_open,
	button_addchild,
	button_delchild,
	button_realize,
	button_query_geometry,
	button_query_minsize,
	button_reshape,
	button_setopt,
	button_getopt,
	button_event,
	button_changes,
	button_changes
};

widget_class_t *wt_pushbutton_class = &_wt_pushbutton_class;
widget_class_t *wt_radiobutton_class = &_wt_radiobutton_class;
widget_class_t *wt_checkbutton_class = &_wt_checkbutton_class;
widget_class_t *wt_label_class  = &_wt_label_class;
