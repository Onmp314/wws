/* 
 * A range widget for W toolkit using a scrollbar and three labels.
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
	short is_open;
	short is_realized;

	widget_t *minw;
	widget_t *valw;		/* label widgets */
	widget_t *maxw;

	widget_t *scrollbar;	/* selection scrollbar */
	short orientation;	/* vertical or horizontal slider? */
	int steps;		/* how many `steps' in the scrollbar */

	short decimals;		/* how many decimals used on labels */
	float min;		/* values as floats */
	float max;

	/* reshape variables */
	short labely;
	short wmax;		/* label widths */
	short wmin;
	short wval;
} range_widget_t;


static void slider_cb (widget_t *_w, int offset, int pressed)
{
	range_widget_t *w = (range_widget_t*)(_w->parent);
	char value[16];
	float valf;

	if(w->max > w->min)
		valf = w->min + (w->max - w->min) * (float)offset / (float)w->steps;
	else
		valf = w->min - (w->min - w->max) * (float)offset / (float)w->steps;
	sprintf(value, "%.*f", w->decimals, valf);
	wt_setopt(w->valw, WT_LABEL, value, WT_EOL);
}

static void show_value(range_widget_t *w, char *value)
{
	float valf;
	long vali;

	sscanf(value, "%f", &valf);
	if(w->max > w->min)
		vali = (valf - w->min) * w->steps / (w->max - w->min);
	else
		vali = (valf - w->max) * w->steps / (w->min - w->max);
	(*wt_scrollbar_class->setopt) (w->scrollbar, WT_POSITION, &vali);
	(*wt_label_class->setopt) (w->valw, WT_LABEL, value);
}

/* 
 * widget code
 */

static long range_init (void)
{
	return 0;
}

static widget_t *range_create(widget_class_t *cp)
{
	range_widget_t *wp = calloc (1, sizeof (range_widget_t));
	long a;

	if (!wp)
		return NULL;
	wp->w.class = wt_range_class;

	wp->scrollbar = wt_create (wt_scrollbar_class, NULL);
	wp->minw = wt_create (wt_label_class, NULL);
	wp->valw = wt_create (wt_label_class, NULL);
	wp->maxw = wt_create (wt_label_class, NULL);

	wt_add_before((widget_t*)wp, NULL, wp->scrollbar);
	wt_add_before((widget_t*)wp, NULL, wp->minw);
	wt_add_before((widget_t*)wp, NULL, wp->valw);
	wt_add_before((widget_t*)wp, NULL, wp->maxw);

	a = 1;
	wt_setopt(wp->scrollbar,
		WT_LINE_INC, &a,
		WT_ACTION_CB, slider_cb,
		WT_EOL);
	(*wt_scrollbar_class->getopt) (wp->scrollbar, WT_ORIENTATION, &a);
	wp->orientation = a;
	wp->steps = 100;

	return (widget_t *)wp;
}

static long range_delete(widget_t *_w)
{
	range_widget_t *w = (range_widget_t *)_w;

	wt_remove(w->scrollbar);
	wt_remove(w->maxw);
	wt_remove(w->valw);
	wt_remove(w->minw);
	wt_delete(w->scrollbar);
	wt_delete(w->maxw);
	wt_delete(w->valw);
	wt_delete(w->minw);
	free(w);
	return 0;
}

static long range_close(widget_t *_w)
{
	range_widget_t *w = (range_widget_t *)_w;

	if(w->is_realized && w->is_open) {
		wt_close(w->maxw);
		wt_close(w->valw);
		wt_close(w->minw);
		wt_close(w->scrollbar);
		w->is_open = 0;
	}
	return 0;
}

static long range_open(widget_t *_w)
{
	range_widget_t *w = (range_widget_t *)_w;

	if(w->is_realized && !w->is_open) {
		wt_open(w->scrollbar);
		wt_open(w->minw);
		wt_open(w->valw);
		wt_open(w->maxw);
		w->is_open = 1;
	}
	return 0;
}

static long range_addchild(widget_t *parent, widget_t *w)
{
	return -1;
}

static long range_delchild(widget_t *parent, widget_t *w)
{
	return -1;
}

static long range_realize(widget_t *_w, WWIN *parent)
{
	range_widget_t *w = (range_widget_t *)_w;
	char *mins, *maxs, *vals;
	float valf;

	if(w->is_realized)
		return -1;

	(*wt_label_class->getopt) (w->minw, WT_LABEL, &mins);
	(*wt_label_class->getopt) (w->valw, WT_LABEL, &vals);
	(*wt_label_class->getopt) (w->maxw, WT_LABEL, &maxs);
	if(!(w->steps > 0 && maxs && mins))
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
	show_value(w, vals);
	(*wt_scrollbar_class->realize) (w->scrollbar, parent);

	if (w->orientation == OrientHorz) {
		/* show min & max values only on horizontal slider */
		(*wt_label_class->realize) (w->minw, parent);
		(*wt_label_class->realize) (w->maxw, parent);
	}
	(*wt_label_class->realize) (w->valw, parent);
	w->is_realized = 1;
	w->is_open = 1;
	return 0;
}


static long
range_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	range_widget_t *w = (range_widget_t *)_w;

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
range_query_minsize(widget_t *_w, long *wdp, long *htp)
{
	range_widget_t *w = (range_widget_t *)_w;
	long wmin, wmax, wval, labely;

	(*wt_scrollbar_class->query_minsize) (w->scrollbar, wdp, htp);
	(*wt_label_class->query_minsize) (w->minw, &wmin, &labely);
	(*wt_label_class->query_minsize) (w->maxw, &wmax, &labely);
	wval = (wmax > wmin ? wmax : wmin);
	w->wmin = wmin;
	w->wmax = wmax;
	w->wval = wval;
	w->labely = labely;
	*htp += labely + WT_DEFAULT_YBORDER;
	if (w->orientation == OrientHorz)
		wval += wval * 3 + (4 * WT_DEFAULT_XBORDER);
	if(*wdp < wval)
		*wdp = wval;
	return 0;
}

static long
range_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	range_widget_t *w = (range_widget_t *)_w;
	long ww, hh, ret = 0;

	if (x != w->w.x || y != w->w.y) {
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	if (wd != w->w.w || ht != w->w.h) {
		range_query_minsize(_w, &ww, &hh);
		if (ht < hh)
			ht = hh;
		if (wd < ww)
			wd = ww;
		if (wd != w->w.w || ht != w->w.h) {
			w->w.h = ht;
			w->w.w = wd;
			ret = 1;
		}
	}

	/* reshape scrollbar and reposition label(s) */
	if (w->orientation == OrientHorz) {
		(*wt_scrollbar_class->reshape) (w->scrollbar, x, y, wd, 0);

		(*wt_scrollbar_class->getopt) (w->scrollbar, WT_HEIGHT, &hh);
		y += hh + WT_DEFAULT_YBORDER;
		(*wt_label_class->reshape) (w->minw, x, y, 0, 0);
		(*wt_label_class->reshape) (w->maxw, x + wd - w->wmax, y, 0, 0);
		x += w->wmin + (wd - w->wmin - w->wval - w->wmax) / 2;
		(*wt_label_class->reshape) (w->valw, x, y, 0, 0);
	} else {
		(*wt_scrollbar_class->getopt) (w->scrollbar, WT_WIDTH, &ww);
		ww = x + (wd - ww) / 2;
		ht -= w->labely + WT_DEFAULT_YBORDER;
		(*wt_scrollbar_class->reshape) (w->scrollbar, ww, y, 0, ht);

		y += ht + WT_DEFAULT_YBORDER;
		x += (wd - w->wval) / 2;
		(*wt_label_class->reshape) (w->valw, x, y, 0, 0);
	}
	return ret;
}

static long
range_setopt (widget_t *_w, long key, void *val)
{
	range_widget_t *w = (range_widget_t *)_w;
	short mask = 0;
	long a, b;

	switch (key) {
	case WT_XPOS:
		if (range_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (range_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		if (range_reshape (_w, w->w.x, w->w.y, *(long *)val, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		if (range_reshape (_w, w->w.x, w->w.y, w->w.w, *(long *)val))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_ORIENTATION:
		if (*(long*)val != OrientHorz && *(long*)val != OrientVert)
			return -1;
		w->orientation = *(long*)val;
		(*wt_scrollbar_class->setopt) (w->scrollbar, key, val);
		if (w->is_realized && range_reshape (_w, w->w.x, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_FONT:
		(*wt_label_class->setopt) (w->minw, key, val);
		(*wt_label_class->setopt) (w->valw, key, val);
		(*wt_label_class->setopt) (w->maxw, key, val);
		if (w->is_realized && range_reshape (_w, w->w.x, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_VALUE:
		show_value(w, (char *)val);
		break;

	case WT_VALUE_MIN:
		if (w->is_realized)
			sscanf((char *)val, "%f", &w->min);
		return (*wt_label_class->setopt) (w->minw, WT_LABEL, val);

	case WT_VALUE_MAX:
		if (w->is_realized)
			sscanf((char *)val, "%f", &w->max);
		return (*wt_label_class->setopt) (w->maxw, WT_LABEL, val);

	case WT_VALUE_DECIMALS:
		if(val && *(long*)val)
			w->decimals = *(long*)val;
		else
			w->decimals = 0;
		break;

	case WT_VALUE_STEPS:
	  	if (*(long*)val < 1)
			return -1;
		w->steps = *(long*)val;
		a = isqrt(w->steps);
		b = a + w->steps;
		wt_setopt(w->scrollbar,
			WT_TOTAL_SIZE, &b,
			WT_PAGE_INC, &a,
			WT_SIZE, &a,
			WT_EOL);
		break;

	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
range_getopt (widget_t *_w, long key, void *val)
{
	range_widget_t *w = (range_widget_t *)_w;

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

	case WT_ORIENTATION:
		return (*wt_scrollbar_class->getopt) (w->scrollbar, key, val);

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

	case WT_VALUE_STEPS:
		*(long*)val = w->steps;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT * range_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long range_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_range_class = {
	"range", 0,
	range_init,
	range_create,
	range_delete,
	range_close,
	range_open,
	range_addchild,
	range_delchild,
	range_realize,
	range_query_geometry,
	range_query_minsize,
	range_reshape,
	range_setopt,
	range_getopt,
	range_event,
	range_changes,
	range_changes
};

widget_class_t *wt_range_class = &_wt_range_class;
