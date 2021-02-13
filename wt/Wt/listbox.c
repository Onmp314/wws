/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Eero Tamminen.
 *
 * listbox widget.
 *
 * changes:
 * - options can be changed more freely when widget is realized.
 * - supports proportional fonts better, but as default uses
 *   a fixed one.
 * - resizes properly (oddie 08/00)
 *
 * $Id: listbox.c,v 1.5 2008-08-29 19:47:09 eero Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "wt_keys.h"
#include "toolkit.h"

/* for 3D looks... */
#define OFFSET		2			/* border for text invert box */
#define XBORDER		(WT_DEFAULT_XBORDER+4)	/* horizontal listbox border */
#define YBORDER		(WT_DEFAULT_YBORDER+1)	/* vertical -"- */

typedef struct
{
  widget_t w;
  short is_open;
  short queried;
  short is_realized;
  WFONT *font;
  short fsize;
  widget_t *scrollbar;
  short height;			/* list box height in chars */
  short chars;			/* list box width in chars */
  short width;			/* list box width in pixels minus borders */
  short items;			/* how many strings there is */
  char **list;			/* NULL terminated string list */
  short top;			/* first visible string */
  short current;		/* currently selected string */
  short scrollwd;
  /* called when item is clicked / shortcuts are used / char unknown */
  void (*select_cb)(widget_t *, char *text, int idx);
  void (*change_cb)(widget_t *, char *text, int idx);
  WEVENT * (*key_cb)(widget_t *, WEVENT *ev);
  short usrwd, usrht, usrheight;
} listbox_widget_t;

static void set_scrollbar(listbox_widget_t *w);


static void
cursor_invert(listbox_widget_t *w)
{
  int idx = w->current - w->top;
  if(idx >= 0 && idx < w->items - w->top)
  {
    w_pbox(w->w.win,
      XBORDER-OFFSET, YBORDER + idx * w->font->height,
      w->width + OFFSET*2, w->font->height);
  }
}

static void
printline(listbox_widget_t *w, const char *line, int y)
{
  WWIN *win = w->w.win;
  short width;

  width = w_strlen(w->font, line);
  if(width <= w->width)
  {
    w_printstring(win, XBORDER, y, line);
  }
  else
  {
    int idx = strlen(line);
    while (width > w->width && --idx > 0) {
      width -= w->font->widths[(unsigned)line[idx]];
    }
    if (idx) {
      char buf[idx+1];
      strncpy (buf, line, idx);
      buf[idx] = '\0';
      w_printstring(win, XBORDER, y, buf);
    }
  }
  if(width < w->width)
  {
    w_setmode(win, M_CLEAR);
    w_pbox(win, XBORDER + width, y, w->width - width, w->font->height);
    w_setmode(win, M_INVERS);
  }
}

/* draw the texts (other stuff is done straight in realize)
 * if not flag, clear the whole are before drawing strings
 */
static long
listbox_draw (listbox_widget_t *w, int just_contents)
{
  WWIN *win = w->w.win;
  short y, idx;

  y = YBORDER;
  /* redraw whole thing only when necessary, otherwise -- flicker */
  if(!just_contents)
  {
    short wd = w->width + 2*XBORDER;
    wt_box3d (w->w.win, 0, 0, wd, w->w.h);
    wt_box3d_press (w->w.win, 0, 0, wd, w->w.h, 0);
    w_setmode(win, M_CLEAR);
    w_pbox(win, XBORDER, YBORDER, w->width, w->w.h - YBORDER*2);
  }
  for(idx = w->top; idx < w->top + w->height; idx++)
  {
    if(idx < w->items)
      printline(w, w->list[idx], y);
    else if(just_contents)
    {
      w_setmode(win, M_CLEAR);
      w_pbox(win, XBORDER, y, w->width, w->font->height);
    }
    y += w->font->height;
  }
  w_setmode(win, M_INVERS);
  return 0;
}

static long
listbox_query_geometry (widget_t * _w, long *xp, long *yp, long *wdp, long *htp)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;

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
listbox_query_minsize (widget_t * _w, long *wdp, long *htp)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;
  int len, width, items, height;
  long size;

  items = 0;
  width = w->font->maxwidth * w->chars;

  if (!w->list) {
    items = 1;
  } else {
    /* get longest string width (in pixels) */
    while(w->list[items])
    {
      if((len = w_strlen(w->font, w->list[items])) > width)
	width = len;
      items++;
    }
  }
  /* width + 'extra' width + borders */
  *wdp = width + w->scrollwd + OFFSET + 2*XBORDER;

  if(w->usrheight)
    height = w->usrheight;
  else
    height = items;
  *htp = height * w->font->height + YBORDER*2;

  wt_minsize (w->scrollbar, NULL, &size);

  *wdp = MAX (*wdp, w->usrwd);
  *htp = MAX (*htp, w->usrht);
  *htp = MAX (*htp, size);
  return 0;
}

static void
listbox_resized (listbox_widget_t *w)
{
  w->height = (w->w.h - 2*YBORDER) / w->font->height;
  w->width = w->w.w - w->scrollwd - OFFSET - 2*XBORDER;;
}

static long
listbox_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	listbox_widget_t *w = (listbox_widget_t *)_w;
	int ret = 0;

	if (wd != w->w.w || ht != w->w.h) {
		long minwd, minht;
		listbox_query_minsize (_w, &minwd, &minht);
		if (wd < minwd)
			wd = minwd;
		if (ht < minht)
			ht = minht;
		if (wd != w->w.w || ht != w->w.h) {
			w->w.w = wd;
			w->w.h = ht;
			if (w->is_realized) {
				w_resize (w->w.win, wd - w->scrollwd - OFFSET, ht);
				w_setmode(w->w.win,M_CLEAR);
				w_pbox(w->w.win,0,0, wd - w->scrollwd - OFFSET, ht);
				w_setmode(w->w.win,M_INVERS);
				listbox_resized (w);
				listbox_draw (w, 0);
			  wt_reshape (w->scrollbar, x + w->width + 2*XBORDER + OFFSET, 
					y, WT_UNSPEC, w->w.h);
				set_scrollbar (w);
				cursor_invert(w);
			}
			ret = 1;
		}
	}
	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized) {
			w_move (w->w.win, x, y);
			wt_reshape (w->scrollbar,
				x + w->width + 2*XBORDER + OFFSET, y,
				WT_UNSPEC, WT_UNSPEC);
		}
		w->w.x = x;
		w->w.y = y;
		ret = 1;
	}
	return ret;
}

static void
scroll_up(listbox_widget_t *w)
{
  int y;

  if(w->height < 2)
    return;

  y = (w->height-1) * w->font->height;
  w_bitblk(w->w.win,
    XBORDER, YBORDER + w->font->height, w->width, y,
    XBORDER, YBORDER);

  y += YBORDER;
  if(w->top + w->height < w->items)
    printline(w, w->list[w->top + w->height], y);
  else
    printline(w, "", y);
  w->top++;
}

static void
scroll_down(listbox_widget_t *w)
{
  if(w->height < 2)
    return;

  w_bitblk(w->w.win,
    XBORDER, YBORDER,
    w->width, (w->height-1) * w->font->height,
    XBORDER, YBORDER + w->font->height);

  w->top--;
  printline(w, w->list[w->top], YBORDER);
}

static void
listbox_redraw(listbox_widget_t *w, short newtop)
{
  if(newtop-1 == w->top)
  {
    scroll_up(w);
    return;
  }
  if(newtop+1 == w->top)
  {
    scroll_down(w);
    return;
  }
  w->top = newtop;
  /* redraw completely */  
  listbox_draw(w, 1);
}

static void
scroll_callback (widget_t *_w, int pos, int pressed)
{
  listbox_widget_t *w = (listbox_widget_t *) _w->parent;
  short newtop;

  if((newtop = pos) == w->top)
    return;

  cursor_invert(w);
  listbox_redraw(w, newtop);
  cursor_invert(w);
}

static long
listbox_init (void)
{
  return 0;
}

static int
listbox_loadfont (listbox_widget_t *w, char *fname, short fsize)
{
  WFONT *fp;

  if (!(fp = wt_loadfont (fname, fsize, 0, 1)))
    return -1;

  w_unloadfont (w->font);
  w_setfont (w->w.win, fp);
  w->fsize = fsize;
  w->font = fp;
  return 0;
}

static widget_t *
listbox_create (widget_class_t *cp)
{
  int i;
  listbox_widget_t *wp = calloc (1, sizeof (listbox_widget_t));
  if (!wp)
    return NULL;
  wp->w.class = wt_listbox_class;

  if (listbox_loadfont (wp, NULL, 0))
  {
    free (wp);
    return NULL;
  }

  wp->scrollbar = wt_create (wt_scrollbar_class, NULL);
  if (!wp->scrollbar)
  {
    w_unloadfont (wp->font);
    free (wp);
    return NULL;
  }
  wt_add_after((widget_t *)wp, NULL, wp->scrollbar);

  (*wt_scrollbar_class->setopt) (wp->scrollbar, WT_ACTION_CB, scroll_callback);
  (*wt_scrollbar_class->getopt) (wp->scrollbar, WT_WIDTH, &i);
  wp->scrollwd = i;

  wp->current = -1;
  return (widget_t *) wp;
}

static long
listbox_delete (widget_t * _w)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;

  wt_ungetfocus (_w);
  if (w->is_realized)
    w_delete (w->w.win);
  wt_remove (w->scrollbar);
  wt_delete (w->scrollbar);
  w_unloadfont (w->font);
  free (w);
  return 0;
}

static long
listbox_close (widget_t * _w)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;

  if (w->is_realized && w->is_open)
    {
      wt_close(w->scrollbar);
      w_close (w->w.win);
      w->is_open = 0;
    }
  return 0;
}

static long
listbox_open (widget_t * _w)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;

  if (w->is_realized && !w->is_open)
    {
      w_open (w->w.win, w->w.x, w->w.y);
      wt_open(w->scrollbar);
      w->is_open = 1;
    }
  return 0;
}

static long
listbox_addchild (widget_t *parent, widget_t *child)
{
  return -1;
}

static long
listbox_delchild (widget_t *parent, widget_t *child)
{
  return -1;
}

static void
set_scrollbar(listbox_widget_t *w)
{
  long i;

  if(w->items <= w->height)
  {
    i = w->height;
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_TOTAL_SIZE, &i);
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_SIZE, &i);

    i = 0;
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_POSITION, &i);
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_PAGE_INC, &i);
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_LINE_INC, &i);
  }
  else
  {
    i = w->items;
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_TOTAL_SIZE, &i);

    i = w->top;
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_POSITION, &i);

    i = w->height;
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_SIZE, &i);
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_PAGE_INC, &i);

    i = 1;
    (*wt_scrollbar_class->setopt) (w->scrollbar, WT_LINE_INC, &i);
  }
}

/* this could also resize the window window in case new strings were
 * too long for the old window. Currently app has to take care that
 * this isn't the case.
 */
static void
show_new_list(listbox_widget_t *w)
{
  cursor_invert(w);
  w->current = -1;
  w->top = 0;
  for(w->items = 0; w->list[w->items]; w->items++)
    ;

  listbox_draw (w, 0);
  set_scrollbar(w);
}

static long
listbox_realize (widget_t * _w, WWIN * parent)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;
  long x, y, wd, ht;

  if (w->is_realized || !(w->list && w->font))
    return -1;

  listbox_query_geometry (_w, &x, &y, &wd, &ht);
  w->w.w = wd;
  w->w.h = ht;

  w->w.win = wt_create_window (parent, w->w.w - w->scrollwd - OFFSET, w->w.h,
	W_MOVE | W_NOBORDER | EV_MOUSE | EV_KEYS, _w);
  if (!w->w.win)
    return -1;
  w->w.win->user_val = (long) w;

  w_setfont (w->w.win, w->font);
  listbox_resized (w);
  show_new_list (w);

  wt_reshape (w->scrollbar, x + w->width + 2*XBORDER + OFFSET, y, WT_UNSPEC, w->w.h);
  (*wt_scrollbar_class->realize) (w->scrollbar, parent);

  w_open (w->w.win, w->w.x, w->w.y);
  w_setmode (w->w.win, M_INVERS);
  w->is_realized = 1;
  w->is_open = 1;
  return 0;
}

static void
set_cursorpos(listbox_widget_t *w, long cursor)
{
  int start;

  if(w->current >= 0)
    cursor_invert(w);

  if(cursor < 0)
  {
    w->current = -1;
    return;
  }
  if(cursor >= w->items)
    cursor = w->items - 1;

  start = w->top;
  w->current = cursor;
  if(cursor < start || cursor >= start + w->height)
  {
    /* has to change list 'window' position */
    start = cursor;
    if(start > w->items - w->height)
    {
      start = w->items - w->height;
      if(start < 0)
        start = 0;
    }
    /* redraw */
    listbox_redraw(w, start);
    set_scrollbar(w);
  } 
  /* set cursor */
  cursor_invert(w);
}

static long
check_size(listbox_widget_t *w)
{
  long wd, ht;
  if(!w->is_realized)
    return 0;
  listbox_query_minsize((widget_t *)w, &wd, &ht);
  if (w->w.w > wd)
    wd = w->w.w;
  if (w->w.h > ht)
    ht = w->w.h;
  return listbox_reshape((widget_t *)w, w->w.x, w->w.y, wd, ht);
}

static long
listbox_setopt (widget_t * _w, long key, void *val)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;
  short mask = 0;

  switch (key)
    {
    case WT_XPOS:
      if (listbox_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_YPOS:
      if (listbox_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_WIDTH:
      w->usrwd = MAX (0, *(long *)val);
      if (listbox_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_HEIGHT:
      w->usrht = MAX (0, *(long *)val);
      if (listbox_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_FONTSIZE:
      if (listbox_loadfont (w, NULL, *(long *) val))
	return -1;
      if(check_size(w))
	mask |= WT_CHANGED_SIZE;
      break;

    case WT_FONT:
      if (listbox_loadfont (w, (char *) val, w->fsize))
	return -1;
      if(check_size(w))
	mask |= WT_CHANGED_SIZE;
      break;

    case WT_LIST_WIDTH:
      /* works best for fixed with fonts! */
      w->chars = *(long *) val;
      if(w->is_realized)
      {
        long wd;
        wd = w->font->maxwidth * w->chars + w->scrollwd + OFFSET + 2*XBORDER;
	if (wd > w->w.w && listbox_reshape (_w, w->w.x, w->w.y, wd, w->w.h))
          mask |= WT_CHANGED_SIZE;
      }
      break;

    case WT_LIST_HEIGHT:
      w->usrheight = *(long *) val;
      if(check_size(w))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_LIST_ADDRESS:
      w->list = (char **) val;
      if(w->is_realized)
        show_new_list(w);
      break;

    case WT_CURSOR:
      if(w->is_realized)
        set_cursorpos(w, *(long *)val);
      else
        w->current = *(long *)val;
      break;

    case WT_INKEY_CB:
      w->key_cb = val;
      break;

    case WT_ACTION_CB:
      w->select_cb = val;
      break;

    case WT_CHANGE_CB:
      w->change_cb = val;
      break;

    default:
      return -1;
    }

  if (mask && w->is_realized)
    wt_change_notify (_w, mask);

  return 0;
}

static long
listbox_getopt (widget_t * _w, long key, void *val)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;

  switch (key)
    {
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

    case WT_FONT:
      *(WFONT **)val = w->font;
      break;

    case WT_LIST_HEIGHT:
      *(long *) val = w->usrheight;
      break;

    case WT_LIST_ADDRESS:
      *(char ***) val = w->list;
      break;

    case WT_CURSOR:
      *(long *)val = w->current;
      break;

    case WT_INKEY_CB:
      *(void **)val = w->key_cb;
      break;

    case WT_ACTION_CB:
      *(void **)val = w->select_cb;
      break;

    case WT_CHANGE_CB:
      *(void **)val = w->change_cb;
      break;

    default:
      return -1;
    }
  return 0;
}

static WEVENT *
listbox_event (widget_t * _w, WEVENT * ev)
{
  listbox_widget_t *w = (listbox_widget_t *) _w;
  short new, old, tmp;

  switch (ev->type)
  {
    case EVENT_MPRESS:
      /* get keyboard focus */
      wt_getfocus (_w);

      /* current selection / new selection on listbox */
      old = w->current - w->top;
      new = (ev->y - YBORDER) / w->font->height;

      if(new != old && new >= 0 && new < w->height && new < w->items - w->top
         && ev->x > XBORDER && ev->x < w->width + XBORDER)
      {
        /* deselect old */
	cursor_invert(w);
	/* select new */
	w->current = new + w->top;
	cursor_invert(w);

	/* If there were a double click mouse event, I could add it to do
	 * selection return.  Single click would then signify just cursor
	 * position change.  I could of course do double click check with
	 * timeouts, but that would need changing when W server itself gets
	 * double clicks...
	 */
	if(w->select_cb)
	  w->select_cb((widget_t *)w, w->list[w->current], w->current);
      }
      break;


    case EVENT_KEY:
      new = -1;
      old = w->current;
      switch(ev->key)
      {
        case KEY_UP:
	case WKEY_UP:
	  if(w->current < 0)
	  {
	    w->current = w->top + w->height - 1;
	    if(w->current >= w->items)
	      w->current = w->items - 1;
	  }
	  else if(w->current > 0)
	    w->current--;
	  break;

	case KEY_DOWN:
	case WKEY_DOWN:
	  if(w->current < 0)
	    w->current = w->top;
	  else if(w->current+1 < w->items)
	    w->current++;
	  break;

	case KEY_PGUP:
	case WKEY_PGUP:
	  new = w->top - w->height;
	  if(new < 0)
	    new = 0;
	  if(w->current < 0)
	  {
	    w->current = new + w->height - 1;
	    if(w->current >= w->items)
	      w->current = w->items - 1;
	  }
	  else
	  {
	    w->current -= w->height;
	    if(w->current < 0)
	      w->current = 0;
	  }
	  break;

	case KEY_PGDOWN:
	case WKEY_PGDOWN:
	  new = w->top + w->height;
	  if(new >= w->items)
	    new = w->items - 1;
	  if(w->current < 0)
	    w->current = new;
	  else
	  {
	    w->current += w->height;
	    if(w->current >= w->items)
	      w->current = w->items-1;
	  }
	  break;
	  
	case KEY_HOME:
	case WKEY_HOME:
	  w->current = 0;
          break;

	case KEY_END:
	case WKEY_END:
	  w->current = w->items - 1;
	  break;

	case '\n':
	case '\r':
	  if(w->select_cb && w->current >= 0)
	    w->select_cb((widget_t *)w, w->list[w->current], w->current);
	  break;

	default:
	  if(w->key_cb) {
	    /* let app wonder about the key... */
	    return w->key_cb((widget_t *)w, ev);
          }
	  return NULL;
      }
      /* no need to draw anything? */
      if(new < 0 && old == w->current)
        break;

      /* hide old cursor */
      tmp = w->current;
      w->current = old;
      cursor_invert(w);
      w->current = tmp;

      if((new >= 0 && new != w->top) ||
         tmp < w->top || tmp >= w->top + w->height)
      {
        long pos;

        if(new < 0)
	{
	  /* calculate new list 'window' position */
	  if(tmp == w->top + w->height)
	    new = w->top + 1;
	  else
	    new = tmp;
	  if(new > w->items - w->height)
	  {
	    new = w->items - w->height;
	    if(new < 0)
	      new = 0;
	  }
	}
	/* redraw */
	listbox_redraw(w, new);

	pos = new;
	wt_setopt(w->scrollbar, WT_POSITION, &pos, WT_EOL);

      }
      /* show new cursor and notify about cursor position change */
      cursor_invert(w);
      if(w->change_cb)
        w->change_cb((widget_t *)w, w->list[w->current], w->current);
      break;

    default:
      return ev;
  }

  return NULL;
}

static long
listbox_changes (widget_t * w, widget_t * w2, short changes)
{
  return 0;
}

static long
listbox_focus (widget_t *_w, int enter)
{
  return 0;
}

static widget_class_t _wt_listbox_class =
{
  "listbox", 0,
  listbox_init,
  listbox_create,
  listbox_delete,
  listbox_close,
  listbox_open,
  listbox_addchild,
  listbox_delchild,
  listbox_realize,
  listbox_query_geometry,
  listbox_query_minsize,
  listbox_reshape,
  listbox_setopt,
  listbox_getopt,
  listbox_event,
  listbox_changes,
  listbox_changes,
  listbox_focus
};

widget_class_t *wt_listbox_class = &_wt_listbox_class;
