/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * top level widget, there is one per application (returned by wt_init()).
 *
 * $Id: top.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

static long
top_init (void)
{
	return 0;
}

static widget_t *
top_create (widget_class_t *cp)
{
	widget_t *wp = malloc (sizeof (widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (widget_t));
	wp->class = wt_top_class;
	return wp;
}

static long
top_delete (widget_t *w)
{
	widget_t *wp, *next;

	for (wp = w->childs; wp; wp = next) {
		next = wp->next;
		(*wp->class->delete) (wp);
	}
	free (w);
	return 0;
}

static long
top_close (widget_t *w)
{
	widget_t *wp;

	for (wp = w->childs; wp; wp = wp->next) {
		(*wp->class->close) (wp);
	}
	return 0;
}

static long
top_open (widget_t *w)
{
	widget_t *wp;

	for (wp = w->childs; wp; wp = wp->next) {
		(*wp->class->open) (wp);
	}
	return 0;
}

static long
top_addchild (widget_t *parent, widget_t *w)
{
	wt_add_before (parent, parent->childs, w);
	return 0;
}

static long
top_delchild (widget_t *parent, widget_t *w)
{
	wt_remove (w);
	return 0;
}

static long
top_realize (widget_t *w, WWIN *parent)
{
	widget_t *wp;

	for (wp = w->childs; wp; wp = wp->next) {
		if ((*wp->class->realize) (wp, WROOT))
			return -1;
	}
	return 0;
}

static long
top_query_geometry (widget_t *w, long *x, long *y, long *wd, long *ht)
{
	*x = *y = *wd = *ht = 0;
	return 0;
}

static long
top_query_minsize (widget_t *w, long *wd, long *ht)
{
	*wd = *ht = 0;
	return 0;
}

static long
top_reshape (widget_t *w, long x, long y, long wd, long ht)
{
	return 0;
}

static long
top_setopt (widget_t *w, long key, void *val)
{
	return -1;
}

static long
top_getopt (widget_t *w, long key, void *val)
{
	return -1;
}

static WEVENT *
top_event (widget_t *w, WEVENT *ev)
{
	return ev;
}

static long
top_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_top_class = {
	"top", 0,
	top_init,
	top_create,
	top_delete,
	top_close,
	top_open,
	top_addchild,
	top_delchild,
	top_realize,
	top_query_geometry,
	top_query_minsize,
	top_reshape,
	top_setopt,
	top_getopt,
	top_event,
	top_changes,
	top_changes
};

widget_class_t *wt_top_class = &_wt_top_class;
