/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Eero Tamminen.
 *
 * iconedit widget.
 *
 * $Id: iconedit.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 *
 * Icon and edit area both got a box and 1 pixel white border around them.
 * Icon is on the right from the edit area (hardcoded).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

#define TIMEOUT		40	/* how fast pixels are plot */

/* default sizes */
#define DEF_ICON	16
#define DEF_BLOCK	8

#define ICON_XPOS	(w->icon_w * w->zoom + 5 + 2 + 3)
#define ICON_YPOS	3

typedef struct
  {
    widget_t w;
    short is_open;
    short is_realized;
    short icon_w;
    short icon_h;
    short zoom;
    short edit;
    long timer;
    short mx, my;
    short usrwd, usrht;
  } iconedit_widget_t;


static long
iconedit_draw (iconedit_widget_t * w)
{
  int wd, ht;

  wd = w->icon_w * w->zoom;
  ht = w->icon_h * w->zoom;

  wt_box3d (w->w.win, 0, 0, wd + 5, ht + 5);
  w_box (w->w.win, wd + 8, 1, w->icon_w + 4, w->icon_h + 4);
  return 0;
}

static void
timer_callback (long _w)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;
  short mx, my;

  if (w_querymousepos (w->w.win, &mx, &my))
    return;

  mx = (mx - 3) / w->zoom;
  my = (my - 3) / w->zoom;
  if (mx >= 0 && mx < w->icon_w && my >= 0 && my < w->icon_h &&
      (mx != w->mx || my != w->my))
    {
      w->mx = mx;
      w->my = my;
      w_plot (w->w.win, mx + w->icon_w * w->zoom + 7 + 3, my + 3);
      w_pbox (w->w.win, mx * w->zoom + 3, my * w->zoom + 3,
              w->zoom - 1, w->zoom - 1);
    }
  w->timer = wt_addtimeout (TIMEOUT, timer_callback, _w);
}

static long
iconedit_query_geometry (widget_t * _w, long *xp, long *yp, long *wdp, long *htp)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;

  *xp = w->w.x;
  *yp = w->w.y;
  *wdp = w->w.w;
  *htp = w->w.h;
  if (*wdp > 0 && *htp > 0)
    return 0;

  if (*wdp <= 0)
    *wdp = w->icon_w * (w->zoom + 1) + 5 + 2 + 6;
  if (*htp <= 0)
    *htp = w->icon_h * w->zoom + 5;
  return 0;
}

static long
iconedit_query_minsize (widget_t *_w, long *wdp, long *htp)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;

  *wdp = w->icon_w * (w->zoom + 1) + 5 + 2 + 6;
  *wdp = MAX (*wdp, w->usrwd);
  *htp = w->icon_h * w->zoom + 5;
  *htp = MAX (*htp, w->usrht);
  return 0;
}

static long
iconedit_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;
  long minwd, minht;
  int ret = 0;

  if (x != w->w.x || y != w->w.y) {
    if (w->is_realized)
      w_move (w->w.win, x, y);
    w->w.x = x;
    w->w.y = y;
    ret = 1;
  }
  if (wd != w->w.w || ht != w->w.h) {
    iconedit_query_minsize (_w, &minwd, &minht);
    if (wd < minwd)
      wd = minwd;
    if (ht < minht)
      ht = minht;

    if (wd != w->w.w || ht != w->w.h) {
      w->w.w = wd;
      w->w.h = ht;
      if (w->is_realized) {
        w_resize (w->w.win, wd, ht);
      }
      ret = 1;
    }
  }
  return ret;
}

static long
iconedit_init (void)
{
  return 0;
}

static widget_t *
iconedit_create (widget_class_t *cp)
{
  iconedit_widget_t *wp = malloc (sizeof (iconedit_widget_t));
  if (!wp)
    return NULL;
  memset (wp, 0, sizeof (iconedit_widget_t));

  wp->w.class = wt_iconedit_class;
  wp->icon_w = DEF_ICON;
  wp->icon_h = DEF_ICON;
  wp->zoom = DEF_BLOCK;

  return (widget_t *) wp;
}

static long
iconedit_delete (widget_t * _w)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;

  wt_deltimeout (w->timer);
  if (w->is_realized)
    w_delete (w->w.win);
  free (w);
  return 0;
}

static long
iconedit_close (widget_t * _w)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;

  if (w->is_realized && w->is_open)
    {
      w_close (w->w.win);
      w->is_open = 0;
    }
  return 0;
}

static long
iconedit_open (widget_t * _w)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;

  if (w->is_realized && !w->is_open)
    {
      w_open (w->w.win, w->w.x, w->w.y);
      w->is_open = 1;
    }
  return 0;
}

static long
iconedit_addchild (widget_t * parent, widget_t * w)
{
  return -1;
}

static long
iconedit_delchild (widget_t * parent, widget_t * w)
{
  return -1;
}

static long
iconedit_realize (widget_t * _w, WWIN * parent)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;
  long x, y, wd, ht;

  if (w->is_realized)
    return -1;

  iconedit_query_geometry (_w, &x, &y, &wd, &ht);
  w->w.x = x;
  w->w.y = y;
  w->w.w = wd;
  w->w.h = ht;
  w->w.win = wt_create_window (parent, wd, ht,
			    W_MOVE | W_NOBORDER | EV_ACTIVE | EV_MOUSE, _w);
  if (!w->w.win)
    return -1;

  iconedit_draw (w);
  w->w.win->user_val = (long) w;
  w->is_realized = 1;
  w->is_open = 1;
  w_open (w->w.win, w->w.x, w->w.y);
  return 0;
}

static long
iconedit_setopt (widget_t * _w, long key, void *val)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;
  short mask = 0;
  short x, y;

  switch (key)
    {
    case WT_XPOS:
      if (iconedit_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_YPOS:
      if (iconedit_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_WIDTH:
      w->usrwd = MAX (0, *(long *)val);
      if (iconedit_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_HEIGHT:
      w->usrht = MAX (0, *(long *)val);
      if (iconedit_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_ICON_WIDTH:
      /* the icon (not window) size in pixels
       * can be set only before the widget is realized
       */
      if (!w->is_realized)
	w->icon_w = *(long *) val;
      break;

    case WT_ICON_HEIGHT:
      if (!w->is_realized)
	w->icon_h = *(long *) val;
      break;

    case WT_UNIT_SIZE:
      if (!w->is_realized)
        w->zoom = *(long *) val;
      break;

    case WT_REFRESH:
      if (w->is_realized) {

	w_setmode(w->w.win, M_CLEAR);
	w_pbox(w->w.win, 3, 3, w->icon_w * w->zoom, w->icon_h * w->zoom);
	w_setmode(w->w.win, M_DRAW);
	
 	for(y = 0; y < w->icon_h; ++y) {
	  for(x = 0; x < w->icon_w; ++x) {
	    /* faster to use getblock... */
	    if (w_test(w->w.win, ICON_XPOS + x, ICON_YPOS + y))
	      w_pbox (w->w.win, x * w->zoom + 3, y * w->zoom + 3,
		  w->zoom - 1, w->zoom - 1);
          }
        }
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
iconedit_getopt (widget_t * _w, long key, void *val)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;

  switch (key)
    {
    /* return the icon position for get/putblocks... */
    case WT_ICON_XPOS:
      *(long *) val = ICON_XPOS;
      break;

    case WT_ICON_YPOS:
      *(long *) val = ICON_YPOS;
      break;

    case WT_ICON_WIDTH:
      *(long *) val = w->icon_w;
      break;

    case WT_ICON_HEIGHT:
      *(long *) val = w->icon_h;
      break;

    case WT_UNIT_SIZE:
      *(long *) val = w->zoom;
      break;

    case WT_XPOS:
      *(long *) val = w->w.x;
      break;

    case WT_YPOS:
      *(long *) val = w->w.y;
      break;

    case WT_WIDTH:
      *(long *) val = w->w.w;
      break;

    case WT_HEIGHT:
      *(long *) val = w->w.h;
      break;

    default:
      return -1;
    }
  return 0;
}

static WEVENT *
iconedit_event (widget_t * _w, WEVENT * ev)
{
  iconedit_widget_t *w = (iconedit_widget_t *) _w;
  short mode = 0;

  switch (ev->type)
    {
    case EVENT_MPRESS:
      if (ev->key & BUTTON_LEFT)
	mode = M_DRAW;
      else
	mode = M_CLEAR;

      w_setmode (ev->win, mode);

      w->edit = 1;			/* editing in process */
      w->mx = w->my = -1;
      timer_callback ((long) w);
      break;

    case EVENT_ACTIVE:
      if (w->edit)
	timer_callback ((long) w);	/* if still editing add callback */
      break;

    case EVENT_INACTIVE:
      if (w->edit)			/* disable draw callback */
	wt_deltimeout (w->timer);
      break;

    case EVENT_MRELEASE:
      wt_deltimeout (w->timer);		/* editing ended */
      w->edit = 0;
      break;


    default:
      return ev;
    }

  return NULL;
}

static long
iconedit_changes (widget_t * w, widget_t * w2, short changes)
{
  return 0;
}

static widget_class_t _wt_iconedit_class =
{
  "iconedit", 0,
  iconedit_init,
  iconedit_create,
  iconedit_delete,
  iconedit_close,
  iconedit_open,
  iconedit_addchild,
  iconedit_delchild,
  iconedit_realize,
  iconedit_query_geometry,
  iconedit_query_minsize,
  iconedit_reshape,
  iconedit_setopt,
  iconedit_getopt,
  iconedit_event,
  iconedit_changes,
  iconedit_changes
};

widget_class_t *wt_iconedit_class = &_wt_iconedit_class;
