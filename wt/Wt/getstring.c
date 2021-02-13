/*
 * this file is part of "The W Toolkit".
 *
 * String input widget with scrolling and copy&paste for *fixed* width fonts.
 *
 * changes:
 * - string show width and font can now be changed while widget is realized.
 *
 * (w) 1996 by Eero Tamminen
 *
 * $Id: getstring.c,v 1.4 2000/09/10 10:16:51 eero Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include "Wt.h"
#include "wt_keys.h"
#include "toolkit.h"


/* which destructive keys from wt_keys.h I use... */
#define DESTRUCTIVE(k) (\
	k == WKEY_DEL  || k == KEY_DEL || k == KEY_BS ||\
	k == KEY_CLEAR || k == KEY_KILLINE)

#define MOVEMENT(k) (\
	k == WKEY_LEFT || k == KEY_LEFT || k == WKEY_RIGHT || k == KEY_RIGHT ||\
	k == WKEY_HOME || k == KEY_HOME || k == WKEY_END   || k == KEY_END   ||\
	k == WKEY_UP   || k == KEY_UP   || k == WKEY_DOWN  || k == KEY_DOWN  ||\
	k == WKEY_PGUP || k == KEY_PGUP || k == WKEY_PGDOWN || k == KEY_PGDOWN)


/* horizontal and vertical border offsets */
#define XBORDER		(WT_DEFAULT_XBORDER+4)
#define YBORDER		(WT_DEFAULT_YBORDER+4)
#define DEF_EDIT_WIDTH	32		/* default edit field width */


/* variables mean characters unless otherwise stated */
typedef struct
{
  widget_t w;
  short is_open;
  short is_realized;
  WFONT *font;
  short fsize;
  short mode;			/* GetstrNoBorder, GetstrWithBorder */
  short xborder;		/* horizontal border offset */
  char *text;			/* user buffer on which text is processed */
  char *mask;			/* allowed characters (NULL=all) */
  short bufsize;		/* current string buffer size */
  short maxlen;			/* the string buffer lenght */
  short textlen;		/* set / input text lenght */
  short showlen;		/* the string lenght on screen */
  short char_width;		/* fixed width character width */
  short cursor;			/* relative cursor position (in edit field) */
  short textpos;		/* string index to first char in edit box */
  short copystart;		/* painting starting postion (absolute) */
  short copyend;		/* painting ending position (absolute) */
  long  timer;			/* for copy&paste mouse checking */
  short cursor_on;
  /* call when a key is pressed / return is pressed */
  void (*key_cb) (widget_t *, char *text, int cursor);
  void (*ok_cb) (widget_t *, char *text, int cursor);
  short usrwd, usrht;
  short usrshowlen;		/* min. string lenght on screen */
} getstring_widget_t;


/* show/remove cursor */
static void cursor_invert(getstring_widget_t *w)
{
  w->cursor_on ^= 1;
  w_vline (w->w.win, w->xborder + w->char_width * w->cursor, 3, w->w.h - 4);
}

/* show the character places on empty edit positions */
static void fill_sublines(getstring_widget_t *w)
{
  short idx = w->textlen - w->textpos;

  if (idx < w->showlen)
  {
    w_setmode(w->w.win, M_CLEAR);
    w_pbox(w->w.win,
      w->xborder + w->char_width * idx, YBORDER,
      w->char_width * (w->showlen - idx), w->font->height);
    w_setmode(w->w.win, M_INVERS);
    while(idx < w->showlen)
    {
      w_hline (w->w.win,
        w->xborder + w->char_width * idx + 1, YBORDER + w->font->height - 1,
	w->xborder + w->char_width * (idx+1) - 1);
      idx++;
    }
  }
}

/* draw the initial (everything positioned to zero)
 * text and set default graphics mode M_INVERS
 */
static void
getstring_draw (getstring_widget_t * w)
{
  char bak;
  short idx;

  idx = w->textpos + w->showlen;
  idx = MIN(w->textlen, idx);
  bak = w->text[idx];
  w->text[idx] = 0;
  w_printstring (w->w.win, w->xborder, YBORDER, &w->text[w->textpos]);
  w->text[idx] = bak;

  fill_sublines(w);
  w_setmode (w->w.win, M_INVERS);
}

/* move to end position and draw text */
static void
draw_to_endpos(getstring_widget_t * w)
{
  if((w->textpos = w->textlen - w->showlen) > 0)
    w->cursor = w->showlen;
  else
  {
    w->textpos = 0;
    w->cursor = w->textlen;
  }
  w->copystart = w->copyend = w->textpos + w->cursor;
  getstring_draw(w);
}

static void
redraw_getstring(getstring_widget_t *w)
{
  short cursor_on = w->cursor_on;
  if (cursor_on)
    cursor_invert (w);

  w_setmode (w->w.win, M_CLEAR);
  w_pbox (w->w.win, 0, 0, w->w.w, w->w.h);
  w_setmode (w->w.win, M_DRAW);

  if (w->mode == GetstrModeWithBorder)
  {
    wt_box3d (w->w.win, 0, 0, w->w.w, w->w.h);
    wt_box3d_press (w->w.win, 0, 0, w->w.w, w->w.h, 0);
  }
  /* cursor to last character & redraw the widget */
  draw_to_endpos (w);

  if (cursor_on)
    cursor_invert (w);
}

static long
getstring_query_geometry (widget_t * _w, long *xp, long *yp, long *wdp, long *htp)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;

  *xp = w->w.x;
  *yp = w->w.y;

  (*_w->class->query_minsize) (_w, wdp, htp);
  *wdp = MAX (w->w.w, *wdp);

  return 0;
}

static long
getstring_query_minsize (widget_t * _w, long *wdp, long *htp)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;

  if (w->usrshowlen > w->showlen && w->usrshowlen <= w->maxlen)
    w->showlen = w->usrshowlen;

  *wdp = w->char_width * w->showlen + 2 * XBORDER;
  *htp = w->font->height + 2 * YBORDER;
  if (*wdp < w->usrwd)
    *wdp = w->usrwd;

  return 0;
}

static long
getstring_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
  getstring_widget_t *w = (getstring_widget_t *)_w;
  long minwd, minht, ret = 0;
  int def_wd;

  if (x != w->w.x || y != w->w.y) {
    if (w->is_realized)
      w_move (w->w.win, x, y);
    w->w.x = x;
    w->w.y = y;
    ret = 1;
  }
  if (wd != w->w.w) {

    def_wd = (wd - 2*XBORDER) / w->char_width;
    if (def_wd < 1)
      w->showlen = 1;
    else
      w->showlen = MIN (w->maxlen, def_wd);

    getstring_query_minsize (_w, &minwd, &minht);
    if (wd < minwd)
      wd = minwd;
    ht = minht;

    if (wd != w->w.w || ht != w->w.h) {
      w->w.w = wd;
      w->w.h = ht;
      w->xborder = (w->w.w - w->char_width * w->showlen) / 2;
      if (w->is_realized) {
        w_resize (w->w.win, wd, ht);
	if (w->timer >= 0) {
	  wt_deltimeout (w->timer);
	  w->timer = -1;
	}
        getstring_query_geometry (_w, &x, &y, &wd, &ht);
        w->w.w = wd;

	redraw_getstring(w);
      }
      ret = 1;
    }
  }
  return ret;
}

static long
check_size(getstring_widget_t *w)
{
  long wd, ht;
  if(!w->is_realized)
    return 0;
  getstring_query_minsize((widget_t *)w, &wd, &ht);
  if (w->w.w > wd)
    wd = w->w.w;
  if (w->w.h > ht)
    ht = w->w.h;
  return getstring_reshape((widget_t *)w, w->w.x, w->w.y, wd, ht);
}

/* scroll the text field contents to left */
static void
scroll_left(getstring_widget_t *w)
{
  if(w->textpos + w->showlen >= w->textlen)
    return;
  w->textpos++;
  w_bitblk(w->w.win, 
    w->xborder + w->char_width, YBORDER,
    w->char_width * (w->showlen - 1), w->font->height,
    w->xborder, YBORDER);
  w_printchar(w->w.win,
    w->xborder + w->char_width * (w->showlen-1), YBORDER,
    w->text[w->textpos + w->showlen - 1]);
  if(MAX(w->copystart, w->copyend) >= w->textpos + w->showlen &&
     MIN(w->copystart, w->copyend) <  w->textpos + w->showlen)
    w_pbox (w->w.win,
        w->xborder + w->char_width * (w->showlen-1), YBORDER,
        w->char_width, w->font->height);
}

/* scroll the text field contents to right */
static void
scroll_right(getstring_widget_t *w)
{
  if(w->textpos <= 0)
    return;
  w->textpos--;
  w_bitblk(w->w.win, w->xborder, YBORDER,
    w->char_width * (w->showlen - 1), w->font->height,
    w->xborder + w->char_width, YBORDER);
  w_printchar(w->w.win, w->xborder, YBORDER, w->text[w->textpos]);
  if(MIN(w->copystart, w->copyend) <= w->textpos &&
     MAX(w->copystart, w->copyend) >  w->textpos)
    w_pbox (w->w.win, w->xborder, YBORDER, w->char_width, w->font->height);
}

/* delete one character from the text string left of the cursor
 * and update the text field accordingly (scroll if needed).
 * check for whether there's something to delete is done elsewhere!
 */
static void
backspace_key(getstring_widget_t *w)
{
  short idx;

  /* move the rest of the characters */
  idx = w->textpos + w->cursor;
  while(idx <= w->textlen)
  {
    /* overlap, so better do manually, copy zero too... */
    w->text[idx-1] = w->text[idx];
    idx++;
  }
  w->textlen--;

  if(w->cursor <= 0)
  {
    w->textpos--;
    return;
  }
  /* need to move editfield contents? */
  if(w->textpos + w->cursor <= w->textlen && w->cursor < w->showlen)
  {
    idx = w->textlen - w->textpos - w->cursor + 1;
    if(idx > w->showlen - w->cursor)
      idx = w->showlen - w->cursor;
    w_bitblk(w->w.win, 
      w->xborder + w->char_width * w->cursor, YBORDER,
      w->char_width * idx, w->font->height,
      w->xborder + w->char_width * (w->cursor - 1), YBORDER);
  }
  /* need to replace last character with a new one? */
  if(w->textlen >= w->textpos + w->showlen)
    w_printchar(w->w.win,
      w->xborder + w->char_width * (w->showlen-1), YBORDER,
      w->text[w->textpos + w->showlen - 1]);
  else
  {
    /* or with a 'line'? */
    w_setmode(w->w.win, M_CLEAR);
    w_pbox(w->w.win,
      w->xborder + w->char_width * (w->textlen - w->textpos), YBORDER,
      w->char_width, w->font->height);
    w_setmode(w->w.win, M_INVERS);
    w_hline (w->w.win,
      w->xborder + w->char_width * (w->textlen - w->textpos) + 1,
      YBORDER + w->font->height - 1,
      w->xborder + w->char_width * (w->textlen - w->textpos + 1) - 1);
  }
  w->cursor--;
}

/* add one character to the text string right of the cursor
 * and update the text field accordingly (scroll if needed)
 */
static void
add_key(getstring_widget_t *w, short key)
{
  short idx;

  if(w->textlen >= w->maxlen)				/* enough space? */
    return;
 
  idx = w->textlen;
  while(idx >= w->textpos + w->cursor)
  {
    /* overlap, so better do manually, copy zero too... */
    w->text[idx+1] = w->text[idx];
    idx--;
  }
  w->text[w->textpos + w->cursor] = key;
  w->textlen++;

  if(w->cursor >= w->showlen)	/* at the end of editfield -> just scroll */
  {
    /* have to trick because editfield and string don't correlate before */
    scroll_left(w);
    return;
  }
  /* not at the text end? */
  if(w->textpos + w->cursor < w->textlen)
  {
    idx = w->textlen - w->textpos - w->cursor - 1;
    if(idx >= w->showlen - w->cursor)
      idx = w->showlen - w->cursor - 1;
    w_bitblk(w->w.win, 
      w->xborder + w->char_width * w->cursor, YBORDER,
      w->char_width * idx, w->font->height,
      w->xborder + w->char_width * (w->cursor + 1), YBORDER);
  }
  w_printchar(w->w.win, w->xborder + w->char_width * w->cursor, YBORDER, key);
  w->cursor++;
}

/* key masking.
 * Mask is composed of acceptable chars and char ranges.
 * "abcde", "0-9a-z", "--+"
 */
static inline short char_in(short key, char *mask)
{
  short cc, lc = 256;
  for(;;)
  {
    if(!(cc = *mask++))
      return 0;
    if(cc == '-')
    {
      if((lc < key && key < *mask) || key == *mask)
        return 1;
      if(*mask)
        mask++;
    }
    else if(key == (lc = cc))
      return 1;
  }
}

/* delegate key events */
static void process_key(getstring_widget_t *w, short key)
{
  short changed = 0;

  switch(key)
  {
    /* escape sequences */
    case KEY_BS:
      if(w->cursor < 1 && w->textpos < 1)		/* nothing to do? */
        return;
      backspace_key(w);
      changed = 1;
      break;

    case KEY_DEL:
    case WKEY_DEL:
      if(w->textpos + w->cursor >= w->textlen)		/* nothing to do? */
        return;
      if(w->cursor < w->showlen)
      {
	w->cursor++;
	backspace_key(w);
      }
      else
      {
	/* no need to move editfield contents... */
	key = w->textpos + w->cursor;
	do
	{
	  w->text[key] = w->text[key+1];
	} while(++key < w->textlen);
	w->textlen--;
      }
      changed = 1;
      break;

    case KEY_KILLINE:
      if(w->textpos + w->cursor >= w->textlen)		/* nothing to do? */
        return;
      /* delete to the end of the line */
      w->textlen = w->textpos + w->cursor;
      w->text[w->textlen] = 0;
      fill_sublines(w);
      changed = 1;
      break;

    case KEY_CLEAR:
      if(w->textlen < 1)				/* nothing to do? */
        return;
      w_putselection (W_SEL_TEXT, w->text, w->textlen);
      w->text[0] = w->textpos = w->cursor = w->textlen = 0;
      fill_sublines(w);
      changed = 1;
      break;

    case KEY_LEFT:
    case WKEY_LEFT:
      if(w->cursor > 0 || w->textpos > 0) {
        if(w->cursor <= 0)
	  scroll_right(w);
	else
	  w->cursor--;
      }
      break;

    case KEY_RIGHT:
    case WKEY_RIGHT:
      if(w->textpos + w->cursor < w->textlen) {
        if(w->cursor >= w->showlen)
	  scroll_left(w);
	else
	  w->cursor++;
      }
      break;

    case KEY_HOME:
    case WKEY_HOME:
      if(w->textpos == 0)
        w->cursor = 0;
      if(w->textpos > 0)
      {
      	w->textpos = w->cursor = 0;
	getstring_draw(w);
      }
      break;

    case KEY_END:
    case WKEY_END:
      if(w->textpos + w->cursor < w->textlen) {		/* something to do? */
        if(w->textpos + w->showlen < w->textlen)
          draw_to_endpos(w);
	else
	  w->cursor = w->textlen - w->textpos;
      }
      break;

    case '\r':
    case '\n':
      if(w->ok_cb)
	w->ok_cb((widget_t *)w, w->text, w->textpos + w->cursor);
      break;

    default:
      /* printable? */
      if(key >= ' ' && key <= 0xff && (!w->mask || char_in(key, w->mask)))
	{
          add_key(w, key);
	  changed = 1;
	}
  }
  /* inform about text change? */
  if(w->key_cb && changed)
    w->key_cb((widget_t*)w, w->text, w->textpos + w->cursor);
}

/* paint text between x (relative) and copyend (absolute). set copyend to x */
static void paint_text(getstring_widget_t *w, short x)
{
  int max = MIN(w->showlen, w->textlen - w->textpos);
  int pos = w->copyend - w->textpos;

  /* need scrolling? */
  if(x < 0)
  {
    if(w->textpos > 0)
    {
      scroll_right(w);
      if(pos < max)
        pos++;
    }
    x = 0;
  }
  if(x > max)
  {
    if(w->textpos + w->showlen < w->textlen)
    {
      scroll_left(w);
      if(pos > 0)
        pos--;
    }
    x = max;
  }

  if (x != pos)
  {
    if (x > pos)
      w_pbox (w->w.win, w->xborder + w->char_width * pos,
         YBORDER, w->char_width * (x - pos), w->font->height);
    else
      w_pbox (w->w.win, w->xborder + w->char_width * x,
         YBORDER, w->char_width * (pos - x), w->font->height);
  }

  w->copyend = w->textpos + x;
}

/* process painting with mouse at intervals */
static void timer_callback (long _w)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;
  short mx, my;

  if (w_querymousepos (w->w.win, &mx, &my) < 0)
  {
    w->timer = -1;
    return;
  }
  mx = (mx - w->xborder) / w->char_width;
  cursor_invert(w);
  paint_text(w, mx);
  cursor_invert(w);
  w->timer = wt_addtimeout (100, timer_callback, _w);
}

static void clear_selection(getstring_widget_t *w)
{
  if(w->copyend != w->copystart)
  {
    /* remove selection if any */
    if(w->copystart < w->textpos)
      paint_text(w, 0);
    else if(w->copystart > w->textpos + w->showlen)
      paint_text(w, w->showlen);
    else
      paint_text(w, w->copystart - w->textpos);
  }
}

static void remove_selection(getstring_widget_t *w)
{
  /* copyend is always visible (text scrolls with painting) */
  if(w->copyend < w->copystart)
  {
    memcpy(&w->text[w->copyend], &w->text[w->copystart],
      w->textlen - w->copystart + 1);
    w->textlen -= w->copystart - w->copyend;
    w->cursor = w->copyend - w->textpos;
  }
  else
  {
    memcpy(&w->text[w->copystart], &w->text[w->copyend],
      w->textlen - w->copyend + 1);
    w->textlen -= w->copyend - w->copystart;
    if(w->copystart < w->textpos)
    {
      w->textpos = w->copystart;
      w->cursor = 0;
    }
  }
  getstring_draw(w);
}

/* check cursor position etc. */
static void cursor_position(getstring_widget_t *w, short x)
{
  if(x + w->textpos > w->textlen)
    x = w->textlen - w->textpos;
  else if (x < 0)
    x = 0;
  w->copystart = w->copyend = w->textpos + x;
  w->cursor = x;
}

/* process widget events */
static WEVENT *getstring_event (widget_t * _w, WEVENT * ev)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;
  short len, key, locate;

  switch (ev->type)
    {
    case EVENT_MPRESS:
      /* get keyboard focus */
      wt_getfocus (_w);

      /* cursor and selection off */
      cursor_invert(w);
      clear_selection(w);

      /* new cursor positions */
      cursor_position(w, (ev->x - w->xborder) / w->char_width);

      /* text painting (selection...) */
      if(ev->key == BUTTON_LEFT)
      {
        cursor_invert(w);
	timer_callback((long)_w);
      }
      else
        /* paste buffer to cursor position */
        if(ev->key == BUTTON_RIGHT)
        {
	  w_selection_t *sel = w_getselection (W_SEL_TEXT);
	  if (sel) {
	    len = -1;
	    while(++len < sel->len)
	      add_key(w, sel->data[len]);
	    cursor_position(w, w->cursor);
	    cursor_invert(w);
	    if(w->key_cb)
	      /* inform about text change */
	      w->key_cb((widget_t*)w, w->text, w->textpos + w->cursor);
	    w_freeselection (sel);
	  }
        }
      break;

    case EVENT_MRELEASE:
      if(w->timer >= 0)
      {
        wt_deltimeout(w->timer);
	w->timer = -1;
      }
      /* end painting and copy selected area to buffer */
      if(w->copyend != w->copystart && ev->key == BUTTON_LEFT)
      {
        /* copy painted text into internal (static) buffer */
	if(w->copyend < w->copystart)
	  len = w->copystart - w->copyend;
	else
	  len = w->copyend - w->copystart;
	w_putselection (W_SEL_TEXT,
		&w->text[MIN(w->copyend, w->copystart)], len);
      }
      break;

    case EVENT_KEY:
      key = ev->key;
      /* check for autolocator fed key events... */
      if(w->cursor_on)
      {
        cursor_invert(w);
	locate = 0;
      }
      else
        locate = 1;

      /* remove selection if needed */
      if(w->copyend != w->copystart)
      {
        if(MOVEMENT(key))
	  clear_selection(w);
        else
	{
	  remove_selection(w);

	  /* just the selection? */
	  if(DESTRUCTIVE(key))
	  {
	    if(w->key_cb)
	      /* inform about text change */
	      w->key_cb((widget_t*)w, w->text, w->textpos + w->cursor);
	    cursor_position(w, w->cursor);
            if(!locate)
	      cursor_invert(w);
	    return NULL;
	  }
	}
      }
      /* insert a key (changes cursor position) */
      process_key(w, key);
      cursor_position(w, w->cursor);
      if(!locate)
        cursor_invert(w);
      break;
    }
  return NULL;
}

/* ------------------------- */

static long
getstring_init (void)
{
  return 0;
}

static int
getstring_loadfont (getstring_widget_t *w, char *fname, short fsize)
{
  WFONT *fp;

  if (!(fp = wt_loadfont (fname, fsize, 0, 1)))
    return -1;

  w_unloadfont (w->font);
  w_setfont (w->w.win, fp);
  w->char_width = fp->maxwidth;
  w->fsize = fsize;
  w->font = fp;
  return 0;
}

static widget_t *
getstring_create (widget_class_t * cp)
{
  getstring_widget_t *wp = malloc (sizeof (getstring_widget_t));
  if (!wp)
    return NULL;
  memset (wp, 0, sizeof (getstring_widget_t));
  wp->w.class = wt_getstring_class;

  wp->timer = -1;
  wp->mode = GetstrModeWithBorder;
  wp->usrshowlen = DEF_EDIT_WIDTH;
  wp->xborder = XBORDER;

  if (getstring_loadfont(wp, NULL, 0)) {
    free(wp);
    return NULL;
  }
  return (widget_t *) wp;
}

static long
getstring_delete (widget_t * _w)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;

  wt_ungetfocus (_w);
  wt_deltimeout (w->timer);
  if (w->is_realized)
    w_delete (w->w.win);
  w_unloadfont (w->font);
  if(w->text)
    free(w->text);
  if(w->mask)
    free(w->mask);
  free (w);
  return 0;
}

static long
getstring_close (widget_t * _w)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;

  if (w->is_realized && w->is_open)
    {
      w_close (w->w.win);
      w->is_open = 0;
    }
  return 0;
}

static long
getstring_open (widget_t * _w)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;

  if (w->is_realized && !w->is_open)
    {
      w_open (w->w.win, w->w.x, w->w.y);
      w->is_open = 1;
    }
  return 0;
}

static long
getstring_addchild (widget_t * parent, widget_t * w)
{
  return -1;
}

static long
getstring_delchild (widget_t * parent, widget_t * w)
{
  return -1;
}

static long
getstring_realize (widget_t * _w, WWIN * parent)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;
  long x, y, wd, ht;

  if (w->is_realized || !w->text)
    return -1;

  getstring_query_geometry (_w, &x, &y, &wd, &ht);
  w->w.x = x;
  w->w.y = y;
  w->w.w = wd;
  w->w.h = ht;

  w->w.win = wt_create_window (parent, wd, ht,
	W_NOBORDER | W_MOVE | EV_KEYS | EV_MOUSE, _w);
  if (!w->w.win)
    return -1;

  w->w.win->user_val = (long) w;
  w_setfont (w->w.win, w->font);
  redraw_getstring(w);
  w_open (w->w.win, w->w.x, w->w.y);
  w->is_realized = 1;
  w->is_open = 1;
  return 0;
}

static long
getstring_setopt (widget_t * _w, long key, void *val)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;
  short len, mask = 0;
  char *tmp;

  switch (key)
    {
    case WT_XPOS:
      if (getstring_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_YPOS:
      if (getstring_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_WIDTH:
      w->usrwd = MAX (0, *(long *)val);
      if (getstring_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_HEIGHT:
      w->usrht = MAX (0, *(long *)val);
      if (getstring_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_FONTSIZE:
      if(getstring_loadfont (w, NULL, *(long *) val))
        return -1;
      if(check_size(w))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_FONT:
      if(getstring_loadfont (w, (char *) val, w->fsize))
        return -1;
      if(check_size(w))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_STRING_ADDRESS:
      len = strlen ((char*)val);
      if(!w->text || len >= w->bufsize)
      {
	if(!(tmp = malloc(len+1)))
	  return -1;
	w->bufsize = len+1;
	if(w->text)
	  free(w->text);
	w->text = tmp;
      }
      strcpy(w->text, (char*)val);
      w->textlen = len;
      if(!w->maxlen)
	w->maxlen = len;

      if (w->is_realized)
      {
        short cursor_on = w->cursor_on;
	if (cursor_on)
	  cursor_invert (w);
	draw_to_endpos (w);
	if (cursor_on)
	  cursor_invert (w);
	if(w->timer >= 0)
	{
	  wt_deltimeout(w->timer);
	  w->timer = -1;
	}
      }
      break;

    case WT_STRING_LENGTH:
      w->maxlen = *(long *) val;
      if(w->maxlen >= w->bufsize)
      {
	if((tmp = malloc(w->maxlen+1)))
	{
	  if(w->text)
	  {
	    strcpy(tmp, w->text);
	    free(w->text);
	  }
	  else
	    tmp[0] = 0;		/* textlen is zero as default */
	  w->bufsize = w->maxlen+1;
	  w->text = tmp;
	}
        else
	  w->maxlen = (w->bufsize > 0 ? w->bufsize-1 : 0);
      }
      if(w->showlen > w->maxlen || !w->showlen)
        w->showlen = w->maxlen;
      break;

    case WT_STRING_MASK:
      len = strlen((char*)val);
      if(!w->mask || strlen(w->mask) < len)
      {
	if(!(tmp = malloc(len+1)))
	  return -1;
	if(w->mask)
	  free(w->mask);
	w->mask = tmp;
      }
      strcpy(w->mask, (char*)val);
      break;

    case WT_STRING_WIDTH:
      w->showlen = *(long *) val;
      w->usrshowlen = w->showlen;
      if(check_size(w))
        mask |= WT_CHANGED_SIZE;
      break;

    case WT_CHANGE_CB:
      w->key_cb = val;
      break;

    case WT_ACTION_CB:
      w->ok_cb = val;
      break;

    case WT_MODE:
      if (w->is_realized)
        return -1;
      w->mode = *(long *)val;
      break;

    default:
      return -1;
    }
  if (mask && w->is_realized)
    wt_change_notify (_w, mask);

  return 0;
}

static long
getstring_getopt (widget_t * _w, long key, void *val)
{
  getstring_widget_t *w = (getstring_widget_t *) _w;

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

    case WT_STRING_ADDRESS:
      *(char **) val = w->text;
      break;

    case WT_STRING_MASK:
      *(char **) val = w->mask;
      break;

    case WT_STRING_LENGTH:
      *(long *) val = w->maxlen;
      break;

    case WT_STRING_WIDTH:
      *(long *) val = w->showlen;
      break;

    case WT_CHANGE_CB:
      *(void **)val = w->key_cb;
      break;

    case WT_ACTION_CB:
      *(void **)val = w->ok_cb;
      break;

    case WT_MODE:
      *(long *)val = w->mode;
      break;

    default:
      return -1;
    }
  return 0;
}

static long
getstring_changes (widget_t * w, widget_t * w2, short changes)
{
  return 0;
}

static long
getstring_focus (widget_t *_w, int enter)
{
  getstring_widget_t *w = (getstring_widget_t *)_w;

  cursor_invert (w);
  if (!enter && w->timer >= 0) {
    wt_deltimeout (w->timer);
    w->timer = -1;
  }
  return 0;
}

static widget_class_t _wt_getstring_class =
{
  "getstring", 0,
  getstring_init,
  getstring_create,
  getstring_delete,
  getstring_close,
  getstring_open,
  getstring_addchild,
  getstring_delchild,
  getstring_realize,
  getstring_query_geometry,
  getstring_query_minsize,
  getstring_reshape,
  getstring_setopt,
  getstring_getopt,
  getstring_event,
  getstring_changes,
  getstring_changes,
  getstring_focus
};

widget_class_t *wt_getstring_class = &_wt_getstring_class;
