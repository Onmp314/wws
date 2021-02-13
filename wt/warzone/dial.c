/* 
 * A dial widget for W toolkit
 *
 * Dial is either a half or a full circle one.
 *
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <Wlib.h>
#include <Wt.h>
#include "dial.h"

#define DIAL_OFFSET	5	/* `needle' of the dial edge */
#define MIN_DIAL_WIDTH	(33 + DIAL_OFFSET * 2)

typedef struct
{
	widget_t w;
	short is_open;
	short is_realized;

	widget_t *minw;		/* labels */
	widget_t *valw;
	widget_t *maxw;

	float min;
	float max;
	short decimals;		/* how many decimals used on labels */

	/* dial variables */
	short dial;		/* dial flag */
	short mode;		/* Dial[Half|Full]Circle */
	long timer;
	double angle;		/* dial angle */
	short align;		/* angle start/end pos for full circle */
	short radius;
	short mousex;		/* mouse `direction' */
	short mousey;
	short offx;		/* old `direction' */
	short offy;

	/* reshape variables */
	short winx;
	short labely;
	short wmax;		/* label widths */
	short wmin;
	short wval;

	/* needed for conditional realize */
	WWIN *parent;
} dial_widget_t;


/* M_INVERS is the default mode */
static void draw_dialwin(dial_widget_t *w)
{
	WWIN *win = w->w.win;
	w_setmode(win, M_CLEAR);
	w_pbox(win, 0, 0, win->width, win->height);
	w_setmode(win, M_INVERS);
	w_pcircle(win, w->radius, w->radius, w->radius);
	w_setmode(win, M_CLEAR);
	w_pcircle(win, w->radius+1, w->radius+1, w->radius-1);
	w_setmode(win, M_INVERS);
	if (w->mode == DialHalfCircle)
		w_hline(win, 0, w->radius, 2*w->radius);
	w->dial = 0;
}

static inline void invert_dial(dial_widget_t *w)
{
	w_line(w->w.win, w->radius, w->radius-1, w->mousex, w->mousey);
}

static void dial_on(dial_widget_t *w)
{
	if (!w->dial) {
		invert_dial(w);
		w->dial = 1;
	}
}

static void dial_off(dial_widget_t *w)
{
	if (w->dial) {
		invert_dial(w);
		w->dial = 0;
	}
}

static void angle2pos(dial_widget_t *w, short *x, short *y)
{
	*x = w->radius +   (short)(cos(w->angle) * (w->radius-DIAL_OFFSET));
	*y = w->radius-1 - (short)(sin(w->angle) * (w->radius-DIAL_OFFSET));
}

static double pos2angle(dial_widget_t *w, short x, short y)
{
	double angle;

	x -= w->radius;
	y = w->radius-1 - y;
	if(x == 0) {
		if (y >= 0 || w->mode == DialHalfCircle)
			return M_PI / 2.0;
		else
			return M_PI * 1.5;
	}
	if (y <= 0 && w->mode == DialHalfCircle) {
		if(x < 0)
			return M_PI;
		else
			return 0.0;
	}
	angle = atan((double)ABS(y) / (double)ABS(x));
	if(x < 0)
		angle = M_PI - angle;
	if(y < 0)
		angle = (M_PI * 2.0) -angle;

	return angle;
}

static inline double tocircle(double angle)
{
	if (angle < 0.0)
		return M_PI * 2.0 - angle;
	if (angle > M_PI * 2.0)
		return angle - M_PI * 2.0;
	return angle;
}

static double dial2user(dial_widget_t *w)
{
	double angle;
	switch (w->align) {
		case AlignTop:
			angle = w->angle - M_PI / 2.0;
			break;
		case AlignBottom:
			if (w->mode == DialHalfCircle) {
				angle = w->angle - M_PI / 2.0;
			} else {
				angle = w->angle + M_PI / 2.0;
			}
			break;
		case AlignLeft:
			angle = M_PI - w->angle;
			break;
		default:
			angle = w->angle;
	}
	return tocircle(angle);
}

static double user2dial(dial_widget_t *w, double angle)
{
	switch (w->align) {
		case AlignTop:
			angle += M_PI / 2.0;
			break;
		case AlignBottom:
			if (w->mode == DialHalfCircle) {
				angle -= M_PI / 2.0;
			} else {
				angle += M_PI / 2.0;
			}
			break;
		case AlignLeft:
			angle = M_PI - angle;
			break;
	}
	return tocircle(angle);
}

static void show_value(dial_widget_t *w, char *str)
{
	double angle;
	float value;

	(*wt_label_class->setopt) (w->valw, WT_LABEL, str);
	if (!w->is_realized)
		return;

	sscanf(str, "%f", &value);
	if (w->mode == DialHalfCircle) {
		angle = M_PI;
	} else {
		angle = M_PI * 2;
	}
	if(w->max > w->min) {
		angle = (value - w->min) * angle / (w->max - w->min);
	} else {
		angle = (value - w->max) * angle / (w->min - w->max);
	}
	w->angle = user2dial(w, angle);

	dial_off(w);
	angle2pos(w, &w->mousex, &w->mousey);
	dial_on(w);
}

static void timer_cb(long arg)
{ 
	dial_widget_t *w = (dial_widget_t*)arg;
	char value[16];
	double angle;
	float valf;
	short x, y;

	w_querymousepos(w->w.win, &x, &y);
	if(x != w->offx || y != w->offy) {
		w->offx = x;
		w->offy = y;
		w->angle = pos2angle(w, x, y);
		angle2pos(w, &x, &y);
		dial_off(w);
		w->mousex = x;
		w->mousey = y;
	        dial_on(w);

		angle = dial2user(w);
		if (w->mode == DialHalfCircle) {
			angle /= M_PI;
		} else {
			angle /= M_PI * 2;
		}
		if(w->max > w->min) {
			valf = w->min + (w->max - w->min) * angle;
		} else {
			valf = w->min - (w->min - w->max) * angle;
		}
		sprintf(value, "%.*f", w->decimals, valf);
		wt_setopt(w->valw, WT_LABEL, value, WT_EOL);
	}
	w->timer = wt_addtimeout(100, timer_cb, arg);
}

/*
 * widget functions
 */

static long dial_init (void)
{
	return 0;
}

static widget_t *dial_create(widget_class_t *cp)
{
	dial_widget_t *wp = calloc (1, sizeof (dial_widget_t));

	if (!wp)
		return NULL;
	wp->w.class = wt_dial_class;

	wp->minw = wt_create (wt_label_class, NULL);
	wp->valw = wt_create (wt_label_class, NULL);
	wp->maxw = wt_create (wt_label_class, NULL);

	wt_add_before((widget_t*)wp, NULL, wp->minw);
	wt_add_before((widget_t*)wp, NULL, wp->valw);
	wt_add_before((widget_t*)wp, NULL, wp->maxw);

	wp->mode = DialHalfCircle;
	wp->align = AlignRight;
	wp->timer = -1;

	return (widget_t *)wp;
}

static long dial_delete(widget_t *_w)
{
	dial_widget_t *w = (dial_widget_t *)_w;

	w_delete(w->w.win);
	wt_remove(w->maxw);
	wt_remove(w->valw);
	wt_remove(w->minw);
	wt_delete(w->maxw);
	wt_delete(w->valw);
	wt_delete(w->minw);
	free(w);
	return 0;
}

static long dial_close(widget_t *_w)
{
	dial_widget_t *w = (dial_widget_t *)_w;

	if(w->is_realized && w->is_open) {
		wt_close(w->maxw);
		wt_close(w->valw);
		wt_close(w->minw);
		w_close(w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static void openclose_minmax(dial_widget_t *w)
{
	if (!(w->is_realized && w->is_open))
		return;

	/* realize / open / close labels for min & max values as needed */
	if (w->mode == DialHalfCircle &&
	   (w->align == AlignLeft || w->align == AlignRight)) {
		(*wt_label_class->realize) (w->minw, w->parent);
		(*wt_label_class->realize) (w->maxw, w->parent);
		wt_open(w->minw);
		wt_open(w->maxw);
	} else {
		wt_close(w->minw);
		wt_close(w->maxw);
	}
}

static inline void open_dialwin(dial_widget_t *w)
{
	w_open(w->w.win, w->winx, w->w.y);
}

static long dial_open(widget_t *_w)
{
	dial_widget_t *w = (dial_widget_t *)_w;

	if(w->is_realized && !w->is_open) {
		open_dialwin(w);
		openclose_minmax(w);
		wt_open(w->valw);
		w->is_open = 1;
	}
	return 0;
}

static long dial_addchild(widget_t *parent, widget_t *w)
{
	return -1;
}

static long dial_delchild(widget_t *parent, widget_t *w)
{
	return -1;
}

static long dial_reshape (widget_t *_w, long x, long y, long wd, long ht);

static long dial_realize(widget_t *_w, WWIN *parent)
{
	dial_widget_t *w = (dial_widget_t *)_w;
	char *mins, *maxs, *vals;
	short wd, ht;
	float valf;
	WWIN *win;

	if (w->is_realized)
		return -1;

	(*wt_label_class->getopt) (w->minw, WT_LABEL, &mins);
	(*wt_label_class->getopt) (w->valw, WT_LABEL, &vals);
	(*wt_label_class->getopt) (w->maxw, WT_LABEL, &maxs);
	if(!(maxs && mins))
		return -1;

	sscanf(maxs, "%f", &w->max);
	sscanf(mins, "%f", &w->min);

	if(!vals) {
		char avg[16];
		int len1 = strlen(mins), len2 = strlen(maxs);
		len2 = (len2 > len1 ? len2 : len1);
		valf = (w->min + w->max) / 2.0;
		sprintf(avg, "%*.*f", len2, w->decimals, valf);
		vals = avg;
	}

	if (!w->radius) {
		dial_reshape(_w, w->w.x, w->w.y, w->w.w, w->w.h);
	}
	wd = w->radius * 2 + 1;
	if (w->mode == DialHalfCircle) {
		ht = w->radius + 1;
	} else {
		ht = wd;
	}
	win = wt_create_window (parent, wd, ht,	W_NOBORDER|W_MOVE|EV_MOUSE, _w);
	if (!win)
		return -1;
	win->user_val = (long) w;
	w->w.win = win;

	w->is_realized = 1;
	draw_dialwin(w);
	show_value(w, vals);

	w->is_open = 1;
	w->parent = parent;
	(*wt_label_class->realize) (w->valw, parent);
	openclose_minmax(w);
	open_dialwin(w);
	return 0;
}


static long
dial_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	dial_widget_t *w = (dial_widget_t *)_w;

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

/* minimum size defined by the space needed by the labels */
static long dial_query_minsize(widget_t *_w, long *wdp, long *htp)
{
	dial_widget_t *w = (dial_widget_t *)_w;
	long wmin, wmax, wval, labely;

	(*wt_label_class->query_minsize) (w->minw, &wmin, &labely);
	(*wt_label_class->query_minsize) (w->maxw, &wmax, &labely);
	wval = (wmax > wmin ? wmax : wmin);
	w->wmin = wmin;
	w->wmax = wmax;
	w->wval = wval;
	w->labely = labely;
	if (w->mode == DialHalfCircle) {
		wval = wval * 3 + (4 * WT_DEFAULT_XBORDER);
	} else {
		wval = wval * 5;
	}
	if (wval < MIN_DIAL_WIDTH)
		wval = MIN_DIAL_WIDTH;
	if (w->mode == DialHalfCircle) {
		*htp = wval / 2 + WT_DEFAULT_YBORDER + labely;
	} else {
		*htp = wval + WT_DEFAULT_YBORDER + labely;
	}
	*wdp = wval;
	if (w->radius * 2 < wval)
		w->radius = wval / 2;
	return 0;
}

static inline void move_dialwin(dial_widget_t *w)
{
	if (w->is_realized)
		w_move(w->w.win, w->winx, w->w.y);
}

static void resize_dialwin(dial_widget_t *w)
{
	short wd, ht;
	if (w->is_realized) {
		wd = w->radius * 2 + 1;
		if (w->mode == DialHalfCircle) {
			ht = w->radius + 1;
		} else {
			ht = wd;
		}
		if (wd != w->w.win->width || ht != w->w.win->height) {
			w_resize(w->w.win, wd, ht);
			draw_dialwin(w);
			dial_on(w);
		}
	}
}

static long dial_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	dial_widget_t *w = (dial_widget_t *)_w;
	long ww, hh, ret = 0;

	if (x != w->w.x || y != w->w.y) {
		w->winx += x - w->w.x;
		w->w.x = x;
		w->w.y = y;
		move_dialwin(w);
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		dial_query_minsize(_w, &ww, &hh);
		if (ht < hh)
			ht = hh;
		if (wd < ww)
			wd = ww;
		if (wd != w->w.w || ht != w->w.h) {
			w->w.w = wd;
			w->w.h = ht;
			ret = 1;

			hh = ht;
			if (w->mode == DialHalfCircle) {
				hh = (hh - w->labely - WT_DEFAULT_YBORDER) * 2;
			}
			ww = MIN(wd, hh) / 2;
			if (ww != w->radius) {
				w->radius = ww;
				resize_dialwin(w);
			}
			ww = w->w.w / 2 - ww;
			if (ww != w->winx) {
				w->winx = ww;
				move_dialwin(w);
			}
		}
	}	

	/* reposition labels */
	if (w->mode == DialHalfCircle) {
		y += w->radius + WT_DEFAULT_YBORDER;
		x = w->radius * 2 + 1 - w->wmin;
		if (w->align == AlignRight) {
			(*wt_label_class->reshape) (w->maxw, w->winx, y, 0, 0);
			(*wt_label_class->reshape) (w->minw, w->winx + x, y, 0, 0);
		} else if (w->align == AlignLeft) { 
			(*wt_label_class->reshape) (w->maxw, w->winx + x, y, 0, 0);
			(*wt_label_class->reshape) (w->minw, w->winx, y, 0, 0);
		}
	} else {
		y += w->radius - w->labely / 2;
	}
	x = w->radius - w->wval / 2;
	(*wt_label_class->reshape) (w->valw, w->winx + x, y, 0, 0);
	return ret;
}

static long dial_setopt (widget_t *_w, long key, void *val)
{
	dial_widget_t *w = (dial_widget_t *)_w;
	short mask = 0;
	char *value;

	switch (key) {
	case WT_XPOS:
		if (dial_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (dial_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		if (dial_reshape (_w, w->w.x, w->w.y, *(long *)val, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		if (dial_reshape (_w, w->w.x, w->w.y, w->w.w, *(long *)val))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_FONT:
		(*wt_label_class->setopt) (w->minw, key, val);
		(*wt_label_class->setopt) (w->valw, key, val);
		(*wt_label_class->setopt) (w->maxw, key, val);
		if (dial_reshape (_w, w->w.x, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_VALUE:
		show_value(w, (char *)val);
		break;

	case WT_VALUE_MIN:
		return (*wt_label_class->setopt) (w->minw, WT_LABEL, val);

	case WT_VALUE_MAX:
		return (*wt_label_class->setopt) (w->maxw, WT_LABEL, val);

	case WT_VALUE_DECIMALS:
		if(val && *(long*)val) {
			w->decimals = *(long*)val;
		} else {
			w->decimals = 0;
		}
		break;

	case WT_MODE:
		if (*(long*)val != DialHalfCircle && *(long*)val != DialFullCircle)
			return -1;
		if (*(long*)val != w->mode) {
			w->mode = *(long*)val;
			resize_dialwin(w);
			openclose_minmax(w);
		}
		mask |= WT_CHANGED_SIZE;
		break;

	case WT_ALIGNMENT:
		if (w->align == *(long*)val)
			break;
		w->align = *(long*)val;
		if (w->is_realized) {
			openclose_minmax(w);
			(*wt_label_class->getopt) (w->valw, WT_LABEL, &value);
			show_value(w, value);
		}
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long dial_getopt (widget_t *_w, long key, void *val)
{
	dial_widget_t *w = (dial_widget_t *)_w;

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
		return (*wt_label_class->getopt) (w->minw, key, val);

	case WT_VALUE:
		return (*wt_label_class->getopt) (w->valw, WT_LABEL, val);

	case WT_VALUE_MIN:
		return (*wt_label_class->getopt) (w->minw, WT_LABEL, val);

	case WT_VALUE_MAX:
		return (*wt_label_class->getopt) (w->maxw, WT_LABEL, val);

	case WT_VALUE_DECIMALS:
		*(long*)val = w->decimals;
		break;

	case WT_MODE:
		*(long*)val = w->mode;
		break;

	case WT_ALIGNMENT:
		*(long*)val = w->align;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT * dial_event (widget_t *_w, WEVENT *ev)
{
	dial_widget_t *w = (dial_widget_t*)_w;

	switch(ev->type) {

	case EVENT_MPRESS:
		w->timer = wt_addtimeout(100, timer_cb, (long)w);
		break;

	case EVENT_MRELEASE:
		if(w->timer >= 0) {
			wt_deltimeout(w->timer);
			w->timer = -1;
		}
		break;

	default:
		return ev;
	}

	return NULL;
}

static long dial_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_dial_class = {
	"dial", 0,
	dial_init,
	dial_create,
	dial_delete,
	dial_close,
	dial_open,
	dial_addchild,
	dial_delchild,
	dial_realize,
	dial_query_geometry,
	dial_query_minsize,
	dial_reshape,
	dial_setopt,
	dial_getopt,
	dial_event,
	dial_changes,
	dial_changes
};

widget_class_t *wt_dial_class = &_wt_dial_class;
