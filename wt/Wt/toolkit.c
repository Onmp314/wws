/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * CHANGES:
 * 10/97, ++eero:
 * - added wt_global structure and use of read_config.
 * - widget callbacks may remove the widget, therefore event processing is
 *   continued only when the widget event processing function returns the
 *   event.
 * 05/03, ++eero:
 * - added a few TRACE calls to critical functions
 *
 * $Id: toolkit.c,v 1.3 2003/05/25 12:19:24 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include "Wt.h"
#include "toolkit.h"
#include "../../lib/proto.h"	/* Wlib TRACE macros */

#define WT_MAGIC1	0x87654321L
#define WT_MAGIC2	0xdeadbeafL

wt_global_struct wt_global;


static input_t *glob_inputs;
static timeout_t *glob_timeouts;

static short run_end = 0;

static fd_set glob_rfdset, glob_wfdset, glob_efdset;

static widget_t *glob_focus;

static short glob_bind_event_mask;
static WEVENT * (*glob_bind_cb) (widget_t *, WEVENT *);

/**
 **
 ** INPUT HANDLER STUFF
 **
 **/

static void
wt_fdset_set (fd_set *s1, fd_set *s2)
{
	char *s, *d;
	int i;

	s = (char *)s2;
	d = (char *)s1;
	for (i = 0; i < sizeof (fd_set); ++i)
		*d++ |= *s++;
}

static void
wt_fdset_clr (fd_set *s1, fd_set *s2)
{
	char *s, *d;
	int i;

	s = (char *)s2;
	d = (char *)s1;
	for (i = 0; i < sizeof (fd_set); ++i)
		*d++ &= ~*s++;
}

static int
wt_fdset_isset (fd_set *s1, fd_set *s2)
{
	char *s, *d;
	int i;

	s = (char *)s2;
	d = (char *)s1;
	for (i = 0; i < sizeof (fd_set); ++i) {
		if (*d++ & *s++)
			return 1;
	}
	return 0;
}

long
wt_addinput (fd_set *r, fd_set *w, fd_set *e,
	void (*h) (long, fd_set *, fd_set *, fd_set *),
	long arg)
{
	static long theid = 0;
	input_t *ip;

	ip = malloc (sizeof (input_t));
	if (!ip)
		return -1;

	FD_ZERO (&ip->rd);
	FD_ZERO (&ip->wr);
	FD_ZERO (&ip->ex);

	if (r) {
		wt_fdset_set (&glob_rfdset, r);
		ip->rd = *r;
	}
	if (w) {
		wt_fdset_set (&glob_wfdset, w);
		ip->wr = *w;
	}
	if (e) {
		wt_fdset_set (&glob_efdset, e);
		ip->ex = *e;
	}
	ip->arg = arg;
	ip->handler = h;
	ip->id = theid;
	ip->refcnt = 1;
	ip->next = glob_inputs;
	glob_inputs = ip;

	if (++theid < 0)
		theid = 0;

	return ip->id;
}

void
wt_delinput (long handle)
{
	input_t **prev, *this;

	prev = &glob_inputs;
	for (this = *prev; this; prev = &this->next, this = *prev) {
		if (this->id == handle)
			break;
	}
	if (this && --this->refcnt <= 0) {
		wt_fdset_clr (&glob_rfdset, &this->rd);
		wt_fdset_clr (&glob_wfdset, &this->wr);
		wt_fdset_clr (&glob_efdset, &this->ex);

		*prev = this->next;
		free (this);
	}
}

void
wt_chginput (long handle, fd_set *rd, fd_set *wr, fd_set *ex)
{
	input_t *this;

	for (this = glob_inputs; this; this = this->next) {
		if (this->id == handle)
			break;
	}
	if (this) {
		wt_fdset_clr (&glob_rfdset, &this->rd);
		wt_fdset_clr (&glob_wfdset, &this->wr);
		wt_fdset_clr (&glob_efdset, &this->ex);

		if (rd) this->rd = *rd; else FD_ZERO (&this->rd);
		if (wr) this->wr = *wr; else FD_ZERO (&this->wr);
		if (ex) this->ex = *ex; else FD_ZERO (&this->ex);

		wt_fdset_set (&glob_rfdset, &this->rd);
		wt_fdset_set (&glob_wfdset, &this->wr);
		wt_fdset_set (&glob_efdset, &this->ex);
	}
}

static void
wt_do_input (fd_set *r, fd_set *w, fd_set *e)
{
	input_t *ip, *next;

	for (ip = glob_inputs; ip; ip = next) {
		next = ip->next;
		if (wt_fdset_isset (&ip->rd, r) ||
		    wt_fdset_isset (&ip->wr, w) ||
		    wt_fdset_isset (&ip->ex, e)) {
			/*
			 * have have to use reference couting to prevent
			 * freeing 'this' in the handler and getting
			 * dangling pointers here...
			 */
			++ip->refcnt;
			(*ip->handler) (ip->arg, r, w, e);
			next = ip->next;
			if (--ip->refcnt <= 0)
				wt_delinput (ip->id);
		}
	}
}

/**
 **
 ** TIMEOUT STUFF
 **
 **/

static void
wt_update_timeouts (void)
{
	static long last = 0;
	static short init = 1;
	long diff, curr;

	curr = w_gettime ();
	if (init || !glob_timeouts || curr - last < 0) {
		last = curr;
		init = 0;
	} else {
		diff = curr - last;
		glob_timeouts->delta -= diff;
		last = curr;
	}
}

static long
wt_can_sleep (void)
{
	wt_update_timeouts ();
	if (glob_timeouts)
		return MAX (glob_timeouts->delta, 1);
	return 10000;
}

static void
wt_do_timeouts (void)
{
	timeout_t *tp;

	wt_update_timeouts ();
	while ((tp = glob_timeouts) && tp->delta <= 0) {
		glob_timeouts = tp->next;
		if (glob_timeouts)
			glob_timeouts->delta += tp->delta;
		(*tp->handler) (tp->arg);
		free (tp);
		wt_update_timeouts ();
	}
}

long
wt_addtimeout (long delta, void (*f) (long arg), long arg)
{
	static long theid = 0;
	timeout_t **prev, *this, *tp;

	if (delta <= 0)
		return -1;

	tp = malloc (sizeof (timeout_t));
	if (!tp)
		return -1;

	tp->handler = f;
	tp->arg = arg;
	tp->id = theid;

	if (++theid < 0)
		theid = 0;

	wt_update_timeouts ();
	prev = &glob_timeouts;
	for (this = *prev; this; prev = &this->next, this = *prev) {
		if (this->delta <= delta)
			delta -= this->delta;
		else {
			this->delta -= delta;
			break;
		}
	}
	tp->delta = delta;
	tp->next = this;
	*prev = tp;
	return tp->id;
}

void
wt_deltimeout (long handle)
{
	timeout_t **prev, *this, *freeme;

	prev = &glob_timeouts;
	for (this = *prev; this; prev = &this->next, this = *prev) {
		if (this->id == handle)
			break;
	}
	if (this) {
		freeme = this;
		*prev = this->next;
		if (this->next)
			this->next->delta += this->delta;
		free (this);
	}
}

/**
 **
 ** TOOLKIT INITIALIZATION
 **
 **/

widget_t *
wt_init (void)
{
	WSERVER *serv;
	widget_t *top;

	TRACESTART();
	serv = w_init ();
	if (!serv) {
		TRACEPRINT(("wt_init() -> NULL\n"));
		TRACEEND();
		return NULL;
	}

	wt_global.screen_wd = serv->width;
	wt_global.screen_ht = serv->height;
	wt_global.screen_bits = serv->planes;
	wt_global.screen_shared = serv->sharedcolors;
	wt_global.font_normal = serv->fname;
	wt_global.font_fixed = serv->fname;
	wt_global.font_size = serv->fsize;

	wt_global.bg_color = 0;
	wt_global.fg_color = 1;
	wt_global.border_offset = 2;

	read_config("wtrc");

	if (!wt_top_class->initialized) {
		if ((*wt_top_class->init) () < 0) {
			TRACEPRINT(("wt_init() -> NULL\n"));
			TRACEEND();
			return NULL;
		}
		wt_top_class->initialized = 1;
	}
	top = (*wt_top_class->create) (wt_top_class);
	if (!top) {
		TRACEPRINT(("wt_init() -> NULL\n"));
		TRACEEND();
		return NULL;
	}
	TRACEPRINT(("wt_init() -> %p\n", top));
	TRACEEND();

	top->magic = WT_MAGIC1;
	return top;
}

/**
 **
 ** WIDGET STUFF
 **
 **/

static widget_t *
wt_win2widget (WWIN *win)
{
	widget_t *w = (widget_t *)win->user_val;

	if (w->magic != WT_MAGIC1) {
		if (w->magic == WT_MAGIC2) {
			printf ("Wt: win->user_val points to deleted widegt!\n");
		} else {
			printf ("Wt: win->user_val widget has bad magic!\n");
		}
		return NULL;
	}
	return w;
}

inline WWIN *
wt_widget2win (widget_t *w)
{
	return w->win;
}

widget_t *
wt_create (widget_class_t *class, widget_t *parent)
{
	widget_t *wp;
	TRACESTART();

	if (!class->initialized) {
		if ((*class->init) () < 0) {
			TRACEPRINT(("wt_create(%p,%p) -> NULL\n",class,parent));
			TRACEEND();
			return NULL;
		}
		class->initialized = 1;
	}
	wp = (*class->create) (class);
	if (!wp) {
		TRACEPRINT(("wt_create(%p,%p) -> NULL\n",class,parent));
		TRACEEND();
		return NULL;
	}
	if (parent && (*parent->class->addchild) (parent, wp) < 0) {
		(*class->delete) (wp);
		TRACEPRINT(("wt_create(%p,%p) -> NULL\n",class,parent));
		TRACEEND();
		return NULL;
	}
	TRACEPRINT(("wt_create(%p,%p) -> %p\n",class,parent,wp));
	TRACEEND();

	wp->magic = WT_MAGIC1;
	return wp;
}

long
wt_delete (widget_t *w)
{
	TRACESTART();
	if (w->parent) {
		(*w->parent->class->delchild) (w->parent, w);
	}
	/*
	 * classes that can receive the input focus must call
	 * wt_ungetfocus() on the widget being deleted!
	 */
	w->magic = WT_MAGIC2;
	(*w->class->close) (w);
	(*w->class->delete) (w);
	TRACEPRINT(("wt_delete(%p)\n", w));
	TRACEEND();
	return 0;
}

long
wt_open (widget_t *w)
{
	return (*w->class->open) (w);
}

long
wt_close (widget_t *w)
{
	return (*w->class->close) (w);
}

long
wt_realize (widget_t *top)
{
	WWIN *parent = WROOT;
	long ret;

	TRACESTART();
	if (top->class != wt_top_class && top->parent)
		parent = wt_widget2win (top->parent);
	ret = (*top->class->realize) (top, parent);
	TRACEPRINT(("wt_realize(%p) -> %ld\n", top,ret));
	TRACEEND();
	return ret;
}

/*
 * dispatch an event to widgets.
 */
void
wt_dispatch_event (WEVENT *ev)
{
	widget_t *wp;
	short evtype;

	wp = wt_win2widget (ev->win);

	switch (ev->type) {
	case EVENT_KEY:
		if (glob_focus) {
			wp = glob_focus;
		}
		evtype = EV_KEYS;
		break;

	case EVENT_MPRESS:
	case EVENT_MRELEASE:
		evtype = EV_MOUSE;
		break;

	case EVENT_ACTIVE:
	case EVENT_INACTIVE:
		evtype = EV_ACTIVE;
		break;

	case EVENT_RESIZE:
		evtype = EV_DEFAULT;

	default:
		evtype = 0;
		break;
	}
	if (wp) {
		if ((!evtype || (wp->event_mask & evtype))) {
			ev = (*wp->class->event) (wp, ev);
		}
		if (ev && wp->bind_cb && (wp->bind_event_mask & evtype)) {
			ev = (*wp->bind_cb) (wp, ev);
		}
		if (ev && glob_bind_cb && (glob_bind_event_mask & evtype)) {
			ev = (*glob_bind_cb) (wp, ev);
		}

		/* do help / property handling */
	}
}

/*
 * wait for next event, timeout or io take necessary actions and return.
 * GADGET_EXIT returns negative number, widgets itself should use a positive
 * number when calling wt_break().
 */
long
wt_do_event (void)
{
	fd_set rfdset, wfdset, efdset;
	WEVENT *ev;

	rfdset = glob_rfdset;
	wfdset = glob_wfdset;
	efdset = glob_efdset;
	ev = w_queryevent (&rfdset, &wfdset, &efdset, wt_can_sleep ());
	wt_do_input (&rfdset, &wfdset, &efdset);
	wt_do_timeouts ();
	if (ev) {
		if (ev->type == EVENT_GADGET && ev->key == GADGET_EXIT)
			return -1;
		wt_dispatch_event (ev);
	}
	return 0;
}

/*
 * main event loop
 */
long
wt_run (void)
{
	long ret;

	TRACESTART();
	run_end = 0;
	while (!run_end) {
		ret = wt_do_event ();
		if (!run_end && ret)
			run_end = ret;
	}
	TRACEPRINT(("wt_run() -> %d\n", run_end));
	TRACEEND();
	return run_end;
}

long
wt_break (long r)
{
	run_end = r;
	return 0;
}

long
wt_getopt (widget_t *w, ...)
{
	va_list args;
	long key, i;
	void *arg;

	va_start (args, w);
	for (i=1; (key = va_arg (args, long)) != WT_EOL; ++i) {
		arg = va_arg (args, void *);
		switch (key) {
		case WT_USRVAL:
			*(long *)arg = w->usrval;
			break;

		default:
			if ((*w->class->getopt) (w, key, arg) < 0) {
				fprintf(stderr, "%s getopt (%ld:%p) error\n",
					w->class->name, key, arg);
				return i;
			}
			break;
		}
	}
	va_end (args);
	return 0;
}

long
wt_setopt (widget_t *w, ...)
{
	va_list args;
	long key, i;
	void *arg;

	va_start (args, w);
	for (i=1; (key = va_arg (args, long)) != WT_EOL; ++i) {
		arg = va_arg (args, void *);
		switch (key) {
		case WT_USRVAL:
			w->usrval = *(long *)arg;
			break;

		default:
			if ((*w->class->setopt) (w, key, arg) < 0) {
				fprintf(stderr, "%s setopt (%ld:%p) error\n",
					w->class->name, key, arg);
				return i;
			}
			break;
		}
	}
	va_end (args);
	return 0;
}

/*
 * remove a widget from its widget tree
 */
void
wt_remove (widget_t *w)
{
	if (w->prev) {
		w->prev->next = w->next;
	} else if (w->parent) {
		w->parent->childs = w->next;
	}
	if (w->next) {
		w->next->prev = w->prev;
	}
}

/*
 * add the new window `neww' into the childlist of window parent
 * right after window `w'.
 */
void
wt_add_after (widget_t *parent, widget_t *w, widget_t *neww)
{
	neww->prev = w;
	neww->next = NULL;
	if (w) {
		if (w->next) {
			neww->next = w->next;
			w->next->prev = neww;
		}
		w->next = neww;
	}
	neww->parent = parent;
	if (!neww->prev) {
		parent->childs = neww;
	}
}

/*
 * add the new window `neww' into the childlist of window parent
 * right before window `w'.
 */
void
wt_add_before (widget_t *parent, widget_t *w, widget_t *neww)
{
	neww->next = w;
	neww->prev = NULL;
	if (w) {
		if (w->prev) {
			neww->prev = w->prev;
			w->prev->next = neww;
		}
		w->prev = neww;
	}
	neww->parent = parent;
	if (!neww->prev) {
		parent->childs = neww;
	}
}

/*
 * notify parent of a widget configuration change
 */
void
wt_change_notify (widget_t *w, short changes)
{
	widget_t *wp;

	if (w->parent)
		(*w->parent->class->child_change) (w->parent, w, changes);
	if ((changes &= ~WT_CHANGED_POS)) {
		for (wp = w->childs; wp; wp = wp->next)
			(*wp->class->parent_change) (wp, w, changes);
	}
}

/*
 * create a new W window. This is needed because TeSche has messed up
 * w_createChild() in w1r3 so it is no impossible to create childs of
 * the root window using w_createChild()...
 */
WWIN *
wt_create_window (WWIN *parent, short wd, short ht, short flags, widget_t *w)
{
	WWIN *win;

	TRACESTART();
	w->event_mask |= (flags | EV_DEFAULT);
	flags |= (EV_KEYS|EV_MOUSE|EV_ACTIVE);

	if (parent == WROOT) {
		win = w_create (wd, ht, flags);
	} else {
		win = w_createChild (parent, wd, ht, flags);
	}
	if (win) {
		win->user_val = (long)w;
	}
	TRACEPRINT(("wt_create_window() -> %p\n", win));
	TRACEEND();
	return win;
}

/*
 * make widget 'w' receive keyboard events
 */
long
wt_getfocus (widget_t *w)
{
	if (w == glob_focus)
		return 0;

	if (!w->class->focus)
		/*
		 * this widget class cannot get the keyboard focus
		 */
		return -1;

	if (glob_focus)
		/*
		 * leave focus
		 */
		(*glob_focus->class->focus) (glob_focus, 0);

	glob_focus = w;
	(*w->class->focus) (w, 1);
	return 0;
}

void
wt_ungetfocus (widget_t *w)
{
	if (w == glob_focus)
		glob_focus = NULL;
}

void
wt_bind (widget_t *w, short events, WEVENT * (*cb) (widget_t *, WEVENT *))
{
	if (!w) {
		/*
		 * global event handler
		 */
		glob_bind_event_mask = events;
		glob_bind_cb = cb;
	} else {
		/*
		 * for widget w
		 */
		w->bind_cb = cb;
		w->bind_event_mask = events;
	}
}

long
wt_geometry (widget_t *w, long *xp, long *yp, long *wp, long *hp)
{
	long x, y, wd, ht, r;
	r = (*w->class->query_geometry) (w, &x, &y, &wd, &ht);
	if (xp) *xp = x;
	if (yp) *yp = y;
	if (wp) *wp = wd;
	if (hp) *hp = ht;
	return r;
}

long
wt_minsize (widget_t *w, long *wp, long *hp)
{
	long wd, ht, r;
	r = (*w->class->query_minsize) (w, &wd, &ht);
	if (wp) *wp = wd;
	if (hp) *hp = ht;
	return r;
}

long
wt_reshape (widget_t *w, long x, long y, long wd, long ht)
{
	long ox, oy, owd, oht;

	if (x == WT_UNSPEC || y == WT_UNSPEC ||
	    wd == WT_UNSPEC || ht == WT_UNSPEC) {
		(*w->class->query_geometry) (w, &ox, &oy, &owd, &oht);
		if (x == WT_UNSPEC)
			x = ox;
		if (y == WT_UNSPEC)
			y = oy;
		if (wd == WT_UNSPEC)
			wd = owd;
		if (ht == WT_UNSPEC)
			ht = oht;
	}
	return (*w->class->reshape) (w, x, y, wd, ht);
}
