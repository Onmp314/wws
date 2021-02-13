/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * text display widget with simple formatting capabilities.
 *
 * TODO:
 * - if width not set, set width to the width of longest line.
 *
 * $Id: text.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

/*
 * text widget
 */
typedef struct {
	widget_t w;
	uchar is_open;
	uchar is_realized;

	WFONT *font;
	short fsize;

	long x, y;
	long line_dist;

	char *text;
	short usrwd, usrht;
} text_widget_t;

/*
 * laod a new font
 */
static int
loadfont (text_widget_t *w, char *fname, short fsize)
{
	WFONT *fp;

	if(!(fp = wt_loadfont (fname, fsize, 0, 0))) {
		return -1;
	}
	w_unloadfont (w->font);
	w_setfont (w->w.win, fp);
	w->line_dist = fp->height;
	w->fsize = fsize;
	w->font = fp;
	return 0;
}

/*
 * do a linebreak
 */
static inline void
dolinebreak (text_widget_t *w)
{
	w->x = 0;
	w->y += w->line_dist;
}

/*
 * print out what is in the buffer and advance x and y position
 * accordingly.
 */
static void
doprint (text_widget_t *w, char *buf, int outp)
{
	int y, wd, ht;

	if (*buf) {
		wd = w_strlen (w->font, buf);
		ht = w->font->height;
		y = w->y + w->line_dist - ht;

		if (outp) {
			w_printstring (w->w.win, w->x, y, buf);
		}
		w->x += wd;
	}
}

/*
 * parse and print out the text.
 */
static int
parse_text (text_widget_t *w, int outp)
{
	char buf[100], *cp, *breakchar, *textp;
	int c, i, wd;

	if (!w->text)
		return 0;

	w_setmode (w->w.win, M_CLEAR);
	w_pbox (w->w.win, 0, 0, w->w.w, w->w.h);
	w_setmode (w->w.win, M_DRAW);

	w->x = w->y = 0;

	wd = 0;
	breakchar = cp = buf;
	textp = w->text;

	while ((c = *textp++)) {
		switch (c) {
		case '\t':
			/*
			 * horizontal tab.
			 */
			*cp = 0;
			doprint (w, buf, outp);
			breakchar = cp = buf;

			wd = 8*w->font->widths[' '];
			i = w->x + wd;
			w->x = i - i % wd;

			while (w->x > w->w.w) {
				w->x -= w->w.w;
				w->y += w->line_dist;
			}
			wd = 0;
			break;

		case '\n':
			/*
			 * linefeed.
			 */
			*cp = 0;
			doprint (w, buf, outp);
			breakchar = cp = buf;
			wd = 0;
			dolinebreak (w);
			break;

		case ' ':
			/*
			 * space.
			 */
			if (w->x + wd <= w->w.w) {
				breakchar = cp;
			}
			*cp++ = ' ';
			wd += w->font->widths[' '];
			break;

		default:
			/*
			 * nonspace character
			 */
			*cp++ = c;
			wd += w->font->widths[(uchar)c];
			break;
		}
		if (w->x + wd > w->w.w) {
			/*
			 * exceeded the right border. need a line-break.
			 */
			*cp = 0;
			if (breakchar > buf && breakchar < cp) {
				/*
				 * up to 'breakchar' everything in the
				 * buffer will fit into current line.
				 *
				 * 'breakchar' points to a character where
				 * we can nicely break the line (ie a space
				 * between two words).
				 */
				*breakchar++ = 0;
				doprint (w, buf, outp);
				while (*breakchar && isspace (*breakchar))
					++breakchar;
				wd = w_strlen (w->font, breakchar);
				strncpy (buf, breakchar, cp-breakchar);
				cp = buf + (cp-breakchar);
				breakchar = buf;
				dolinebreak (w);
			} else if (w->x > 0) {
				/*
				 * we have already something written to the
				 * current line and current word doesn't
				 * fit. break line and then print what we've
				 * got.
				 */
				dolinebreak (w);
				doprint (w, buf, outp);
				breakchar = cp = buf;
				wd = 0;
			} else {
				/*
				 * the line is empty but the word is so long
				 * that it doesn't fit into one line. unget
				 * last char that caused the overflow.
				 */
				--textp;
				*--cp = 0;
				doprint (w, buf, outp);
				breakchar = cp = buf;
				wd = 0;
				dolinebreak (w);
			}
		} else if (cp - buf >= sizeof (buf) - 2) {
			/*
			 * buffer full. flush it
			 */
			*cp = 0;
			doprint (w, buf, outp);
			breakchar = cp = buf;
			wd = 0;
		}
	}
	*cp = 0;
	doprint (w, buf, outp);

	if (w->x > 0) {
		dolinebreak (w);
	}
	return 0;
}

/************************** Widget Code ******************************/

static long text_query_geometry (widget_t *, long *, long *, long *, long *);

static long
text_init (void)
{
	return 0;
}

static widget_t *
text_create (widget_class_t *cp)
{
	text_widget_t *wp;

	wp = malloc (sizeof (text_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (text_widget_t));
	wp->w.class = wt_text_class;

	if (loadfont (wp, NULL, 0)) {
		free (wp);
		return NULL;
	}
	return (widget_t *)wp;
}

static long
text_delete (widget_t *_w)
{
	text_widget_t *w = (text_widget_t *)_w;

	if (w->is_realized)
		w_delete (w->w.win);

	w_unloadfont (w->font);
	free (w);
	return 0;
}

static long
text_close (widget_t *_w)
{
	text_widget_t *w = (text_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
text_open (widget_t *_w)
{
	text_widget_t *w = (text_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y);
		w->is_open = 1;
	}
	return 0;
}

static long
text_addchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
text_delchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
text_realize (widget_t *_w, WWIN *parent)
{
	text_widget_t *w = (text_widget_t *)_w;
	long x, y, wd, ht;

	if (w->is_realized)
		return -1;

	text_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	w->w.win = wt_create_window (parent, wd, ht,
					W_NOBORDER|W_MOVE, _w);
	if (!w->w.win)
		return -1;

	w->w.win->user_val = (long)w;
	w->is_realized = 1;
	w_setfont (w->w.win, w->font);

	parse_text (w, 1);

	w->is_open = 1;
	w_open (w->w.win, w->w.x, w->w.y);
	return 0;
}

static long
text_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	text_widget_t *w = (text_widget_t *)_w;

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
text_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	text_widget_t *w = (text_widget_t *)_w;

	*wdp = w->usrwd > 0 ? w->usrwd : 50;
	if (!w->text) {
		*htp = 50;
	} else {
		parse_text (w, 0);
		*htp = w->y;
	}
	*htp = MAX (*htp, w->usrht);
	return 0;
}

static long
text_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	text_widget_t *w = (text_widget_t *)_w;
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
		text_query_minsize (_w, &minwd, &minht);
		if (wd < minwd)
			wd = minwd;
		if (ht < minht)
			ht = minht;
		if (wd != w->w.w || ht != w->w.h) {
			w->w.w = wd;
			w->w.h = ht;
			if (w->is_realized) {
				w_resize (w->w.win, wd, ht);
				parse_text (w, 1);
			}
			ret = 1;
		}
	}
	return ret;
}

static long
text_setopt (widget_t *_w, long key, void *val)
{
	text_widget_t *w = (text_widget_t *)_w;
	short mask = 0;
	char *cp;

	switch (key) {
	case WT_XPOS:
		if (text_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (text_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (text_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (text_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_LABEL:
		if (!(cp = strdup (val))) {
			return -1;
		}
		if (w->text) {
			free (w->text);
		}
		w->text = cp;
		if (w->is_realized) {
			parse_text (w, 1);
		}
		break;

	case WT_FONTSIZE:
		if (loadfont (w, NULL, *(long *)val))
			return -1;
		break;

	case WT_FONT:
		if (loadfont (w, (char*)val, w->fsize))
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
text_getopt (widget_t *_w, long key, void *val)
{
	text_widget_t *w = (text_widget_t *)_w;

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

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
text_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long
text_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_text_class = {
	"text", 0,
	text_init,
	text_create,
	text_delete,
	text_close,
	text_open,
	text_addchild,
	text_delchild,
	text_realize,
	text_query_geometry,
	text_query_minsize,
	text_reshape,
	text_setopt,
	text_getopt,
	text_event,
	text_changes,
	text_changes
};

widget_class_t *wt_text_class = &_wt_text_class;
