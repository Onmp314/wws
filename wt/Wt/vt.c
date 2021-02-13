/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * VT 52 widget with scrollback buffer and xterm-like selection.
 *
 * CHANGES
 *
 * ++eero 5/98:
 * - Changed to support new W / Wt font loading syntax.
 * - VT uses now font effects if styles can't be loaded as W got now
 *   also slanted and (poor) bold effects.  Changed effect flags to
 *   use W font flags for simplicity's sake and added dim effect.
 * - Added xterm style mouse event support besides earlier cut/paste.
 * - Added WT_VT_REVERSE option (changes bgmode which affects text style).
 * - Use new WEVENT structure member 'time' to check for double/triple
 *   clicks as earlier way had 1 sec resolution for 1 sec check...
 * - Cursor is now shown when selection is over although selection
 *   is still valid.
 * ++eero 6/98:
 * - Added color support for ncurses programs for both VT52 colors codes (in
 *   terminfo) and ANSI (xterm defaults).  The colors will be first 8 shared
 *   colors W server offers or the ones ones user has defined in ~/.wtrc
 *   (eg.  'vt_color1 = 0xffffff').  In color mode VT_REVERSE changes color
 *   indeces instead of using reverse font style.
 * - Rewrote history attribute storing/restoring to use macros that hide
 *   what attributes we'll be using (ie. do we save color to history etc).
 * ++bsittler, 2000.07.04:
 * - Colors are reset by attribute resets.
 * - Initial reversed state is remembered even on color displays.
 * - Improved handling of ESC '[' ... 'm' strings,
 *   including ESC '[' '0' 'm' (equivalent to ESC 'G'.)
 * - Resized widgets properly initialize new character attributes
 *
 * TODO:
 * - Check in vt_loadfonts() that loaded (base) font isn't proportional.
 * - Add timer for cursor blinking (w->curblink flag).
 * - Add vt100 scrolling region support.
 * - Convert to vt100/xterm/ansi.
 *
 * $Id: vt.c,v 1.3 2008-08-29 19:47:09 eero Exp $
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"


/* select VT features:
 * - support xterm style mouse event key-sequences
 * - support colors
 * - re/store colors from/to history
 */
#define XTERM_EMU
#define VT_COLORS
#define COLOR_HISTORY	/* widget takes 'w->screensize' bytes more mem */


/* flags for styles which won't be emulated if they can be loaded */
#define ATTR_BD	1
#define ATTR_SL	2
#define ATTR_FONT_MASK	(ATTR_BD|ATTR_SL)
#define DEF_STYLE_MASK	(F_REVERSE|F_UNDERLINE|F_LIGHT)	/* emulated */


#define DEF_CLICKTIME	800	/* ev->time for double/triple click */
#define DEF_BGCHAR	' '
#define DEF_COLS	80
#define DEF_ROWS	24


/* number of colors, MAX_COLORS if enough available, zero otherwise */
static int Colors;

#ifdef VT_COLORS
#define MAX_COLORS	8	/* standard number for eg ANSI */

/* this is the default mapping for shared server colors (same as wterm) */
static int ColorMapping[MAX_COLORS] = { 1, 2, 3, 7, 4, 6, 5, 0 };

/* these are read from config file when the widget class is initialized */
static int RGB[MAX_COLORS][3];
#endif


typedef struct {

#if defined(VT_COLORS) && defined(COLOR_HISTORY)

  unsigned bg:4;		/* max 16 possibilities for both... */
  unsigned fg:4;		/* fore- and background colors */

#define CMP_ATTRIBUTES(x,y) \
(x->bg != y->bg || x->fg != y->fg || x->attr != y->attr || x->mode != y->mode)

#define STORE_ATTRIBUTES(w,vt)  vt->bg = w->curbg;  vt->fg = w->curfg;\
	vt->attr = w->curattr;  vt->mode = w->bgmode

#define RESTORE_ATTRIBUTES(w,vt)  vt_setattr(w, vt->attr);\
	vt_set_fgcol(w, vt->fg+'0'); vt_set_bgcol(w, vt->bg+'0');\
	w_setmode(w->w.win, vt->mode)

# else	/* !(VT_COLORS && COLOR_HISTORY) */

#define CMP_ATTRIBUTES(x,y)	 (x->attr != y->attr || x->mode != y->mode)
#define STORE_ATTRIBUTES(w,vt)	 vt->attr = w->curattr;  vt->mode = w->bgmode
#define RESTORE_ATTRIBUTES(w,vt) vt_setattr(w, vt->attr);\
	w_setmode(w->w.win, vt->mode)

# endif	/* !(VT_COLORS && COLOR_HISTORY) */

  unsigned mode:2;		/* background mode */
  unsigned attr:6;		/* font attributes */
  u_char c;			/* character */

} vtchar_t;


#define BUFSIZE		80

typedef struct _vt_widget_t {
  widget_t w;
  short    is_open;
  short    is_realized;

  ulong    clicktime;	/* time of last mouse click */
  short	   nclicks;	/* # mouse clicks since clicktime */

  short	   inselect;	/* selection valid */
  long	   selx0,sely0; /* selection start */
  long	   selx1,sely1; /* selection end */
  long	   seltimer;	/* selection timer */

  long	   histsize;	/* max. history size */
  long	   curhistsize;	/* lines currently in history */

  long     wd, ht;	/* visible width and height in chars */
  long	   screenht;	/* ht + histsize */
  long     screen_size; /* wd * screenht */
  vtchar_t *screen;	/* circular screen buffer */
  long     offs;	/* screen[offs*wd] is top left char in vt window */
  long	   curoffs;	/* screen[curoffs*wd] is top left char currently
			   shown in the window (!= offs only if user has
			   scrolled back the window) */

  long     curx, cury;	/* cursor pos */
  long     savx, savy;	/* saved cursor pos */
  u_char   curvis;	/* cursor currently shown */
  u_char   curon;	/* cursor on/off */
  u_char   curblink;	/* cursor blinking on/off */
  u_char   autowrap;	/* autowrap on/off */
  u_char   visbell;	/* visual bell on/off */
  u_char   curattr;	/* display attributes */
  unsigned curbg:4;	/* bg color index */
  unsigned curfg:4;	/* fg color index */
  u_char   style_mask;	/* attribute mask for style effects / read fonts */
  u_char   font_mask;	/* attribute mask for real styles */
  u_char   reversed;	/* for WT_VT_REVERSE option */
  u_char   bgmode;	/* background mode (M_CLEAR/M_DRAW) */
  short	   fontht;	/* font infos */
  short    fontwd;
  char     *fontfamily;	/* font familyname */
  WFONT	   *fonts[ATTR_FONT_MASK+1];

#ifdef VT_COLORS
  short    color[MAX_COLORS];	/* color handles (mapping) */
#endif

#ifdef XTERM_EMU
#define MAX_PARAMETERS	6
  short    xtermemu;	/* xterm mouse event mode */
  short    params;	/* # of vt parameters */
  short	   parameter[MAX_PARAMETERS];
#endif

  long     outx, outy;
  u_char   outbuf[BUFSIZE+1];
  u_char   *outbufp;	/* output buffering */

  void     (*outfn) (struct _vt_widget_t *, u_char c);

  void	   (*hist_cb) (widget_t *w, long histpos, long histsize,
  		       long rows, long cols, wt_opaque_t *);
  short usrwd, usrht, usrcols, usrrows;
} vt_widget_t;


static void putc_normal (vt_widget_t *, u_char c);
static void putc_esc    (vt_widget_t *, u_char c);
static void putc_esc_y1 (vt_widget_t *, u_char c);
static void putc_esc_y2 (vt_widget_t *, u_char c);
static void putc_esc_c  (vt_widget_t *, u_char c);
static void putc_esc_b  (vt_widget_t *, u_char c);


/**********************************************************************/

static inline vtchar_t *
vt_screen_pos (vt_widget_t *w, int x, int y)
{
  if ((y += w->curoffs) >= w->screenht)
    y -= w->screenht;
  return &w->screen[y * w->wd + x];
}

static inline vtchar_t *
vt_screen_nl (vt_widget_t *w, vtchar_t *vcp)
{
  if (vcp - w->screen >= w->screen_size)
    vcp -= w->screen_size;
  return vcp;
}

static inline vtchar_t *
vt_screen_pl (vt_widget_t *w, vtchar_t *vcp)
{
  if (vcp < w->screen)
    vcp += w->screen_size;
  return vcp;
}

static void
vt_flush (vt_widget_t *w)
{
  vtchar_t *vcp;
  char *cp;

  if (w->outbufp > w->outbuf) {
    *w->outbufp = 0;
    w_printstring (w->w.win, w->outx*w->fontwd, w->outy*w->fontht, w->outbuf);

    vcp = vt_screen_pos (w, w->outx, w->outy);
    for (cp = w->outbuf; *cp; ++vcp) {
      STORE_ATTRIBUTES(w, vcp);
      vcp->c = *cp++;
    }
    w->outbufp = w->outbuf;
  }
}

static inline void
vt_addchar (vt_widget_t *w, u_char c)
{
  if (w->outbufp - w->outbuf >= BUFSIZE)
    vt_flush (w);
  if (w->outbuf == w->outbufp) {
    w->outx = w->curx;
    w->outy = w->cury;
  }
  *w->outbufp++ = c;
}

static void
vt_cursor_show (vt_widget_t *w)
{
  if (w->curon && !w->curvis && w->offs == w->curoffs) {
    w->curvis = 1;
    w_setmode (w->w.win, M_INVERS);
    w_pbox (w->w.win, w->curx * w->fontwd, w->cury * w->fontht,
    	    w->fontwd, w->fontht);
  }
}

static void
vt_cursor_hide (vt_widget_t *w)
{
  if (w->curvis) {
    w->curvis = 0;
    w_setmode (w->w.win, M_INVERS);
    w_pbox (w->w.win, w->curx * w->fontwd, w->cury * w->fontht,
    	    w->fontwd, w->fontht);
  }
}

static void
vt_setattr (vt_widget_t *w, u_char attr)
{
  vt_flush (w);

  if ((attr ^ w->curattr) & w->font_mask) {
    short style = 0;

    if (attr & F_BOLD)
      style |= ATTR_BD;
    if (attr & F_ITALIC)
      style |= ATTR_SL;
    w_setfont (w->w.win, w->fonts[style]);
  }
  if ((attr ^ w->curattr) & w->style_mask) {
    w_settextstyle (w->w.win, attr & w->style_mask);
  }
  w->curattr = attr & (w->font_mask | w->style_mask);
}

static void
vt_change_bgmode(vt_widget_t *w, u_char reverse)
{
  if (reverse) {
    if (w->bgmode != M_DRAW) {
      w->bgmode = M_DRAW;
      if (w->is_realized) {
        vt_setattr (w, w->curattr ^ F_REVERSE);
      }
    }
  } else {
    if (w->bgmode != M_CLEAR) {
      w->bgmode = M_CLEAR;
      if (w->is_realized) {
        vt_setattr (w, w->curattr ^ F_REVERSE);
      }
    }
  }
}

static inline void
vt_set_fgcol (vt_widget_t *w, u_char col)
{
  col -= '0';

#ifdef VT_COLORS
  if (Colors) {
    if (col < Colors)
      vt_flush (w);
      w_setForegroundColor(w->w.win, w->color[col]);
#ifdef COLOR_HISTORY
      w->curfg = col;
#endif
  } else
#endif
  {
    vt_change_bgmode(w, col);
  }
}

static inline void
vt_set_bgcol (vt_widget_t *w, u_char col)
{
  col -= '0';

#ifdef VT_COLORS
  if (Colors) {
    if (col < Colors)
      vt_flush (w);
      w_setBackgroundColor(w->w.win, w->color[col]);
#ifdef COLOR_HISTORY
      w->curbg = col;
#endif
  } else
#endif
  {
    vt_change_bgmode(w, !(col));
  }
}

#define SWAP(x, y) { (x) ^= (y); (y) ^= (x); (x) ^= (y); }

static void
vt_select_paint (vt_widget_t *w, long x0, long y0, long x1, long y1)
{
  long ht, wd;

  w_setmode (w->w.win, M_INVERS);

  if (y0 > y1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  } else if (y0 == y1 && x0 > x1) {
    SWAP(x0, x1);
  }

  ht = w->fontht;
  wd = w->fontwd;

  if (y0 == y1) {
    w_pbox (w->w.win, x0*wd, y0*ht, (x1-x0)*wd, ht);
  } else {
    if (x0 < w->wd)
      w_pbox (w->w.win, x0*wd, y0*ht, (w->wd-x0)*wd, ht);
    if (y0+1 < y1)
      w_pbox (w->w.win, 0, (y0+1)*ht, w->wd*wd, (y1-y0-1)*ht);
    w_pbox (w->w.win, 0, y1*ht, x1*wd, ht);
  }
}

static void
vt_unselect (vt_widget_t *w)
{
  if (w->inselect) {
    vt_select_paint (w, w->selx0, w->sely0, w->selx1, w->sely1);
    if (w->seltimer >= 0) {
      wt_deltimeout (w->seltimer);
      w->seltimer = -1;
    }
    w->inselect = 0;
  }
}

static void
vt_select_start (vt_widget_t *w, long x, long y)
{
  vt_unselect (w);
  vt_cursor_hide (w);
  w->inselect = 1;

  if ((x /= w->fontwd) < 0)
    x = 0;
  else if (x > w->wd)
    x = w->wd;
  if ((y /= w->fontht) < 0)
    y = 0;
  else if (y >= w->ht)
    y = w->ht-1;

  w->selx0 = w->selx1 = x;
  w->sely0 = w->sely1 = y;
  vt_select_paint (w, x, y, x, y);
}

static void
vt_select_drag (vt_widget_t *w, long x, long y)
{
  if (!w->inselect)
    return;

  if ((x /= w->fontwd) < 0)
    x = 0;
  else if (x > w->wd)
    x = w->wd;
  if ((y /= w->fontht) < 0)
    y = 0;
  else if (y >= w->ht)
    y = w->ht-1;

  if (x != w->selx1 || y != w->sely1) {
    vt_select_paint (w, w->selx1, w->sely1, x, y);
    w->selx1 = x;
    w->sely1 = y;
  }
}

static void
vt_select_end (vt_widget_t *w)
{
  long x0, y0, x1, y1, len, i, j;
  char *buf, *cp;
  vtchar_t *vcp;

  if (!w->inselect)
    return;

  if (w->seltimer >= 0) {
    wt_deltimeout (w->seltimer);
    w->seltimer = -1;
  }

  x0 = w->selx0;
  y0 = w->sely0;
  x1 = w->selx1;
  y1 = w->sely1;

  if (y0 > y1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  } else if (y0 == y1 && x0 > x1) {
    SWAP(x0, x1);
  }

  len = (y1 - y0 + 1) * (w->wd + 1);
  cp = buf = malloc (len+1);
  assert (buf);

  for (i = y0; i <= y1; ++i) {
    long first = (i == y0) ? x0 : 0;
    long last  = (i == y1) ? x1 : w->wd;
    vcp = vt_screen_pos (w, 0, i);

    while (last > first && vcp[last-1].c == ' ')
      --last;
    for (j = first; j < last; ++j)
      *cp++ = vcp[j].c;
    if (i < y1 || x1 == w->wd)
      *cp++ = '\n';
  }
  *cp = 0;

  if (w_putselection (W_SEL_TEXT, buf, strlen (buf)) < 0)
    w_beep ();
  free (buf);

  if (y0 == y1 && x0 == x1)
    vt_unselect (w);

  vt_cursor_show (w);
}

static void
vt_select_word (vt_widget_t *w, long x, long y)
{
  vtchar_t *vcp;
  long i, isws;

  vt_unselect (w);
  vt_cursor_hide (w);
  w->inselect = 1;

  if ((x /= w->fontwd) < 0)
    x = 0;
  else if (x >= w->wd)
    x = w->wd-1;
  if ((y /= w->fontht) < 0)
    y = 0;
  else if (y >= w->ht)
    y = w->ht-1;

  vcp = vt_screen_pos (w, x, y);
  isws = !!isspace (vcp->c);
  for (i = x; i >= 0; --i, --vcp) {
    if (isws != !!isspace(vcp->c))
      break;
  }
  w->selx0 = i+1;
  w->sely0 = y;

  vcp = vt_screen_pos (w, x, y);
  for (i = x; i < w->wd; ++i, ++vcp) {
    if (isws != !!isspace(vcp->c))
      break;
  }
  w->selx1 = i;
  w->sely1 = y;

  vt_select_paint (w, w->selx0, w->sely0, w->selx1, w->sely1);
  vt_select_end (w);
}

static void
vt_select_line (vt_widget_t *w, long x, long y)
{
  vt_unselect (w);
  vt_cursor_hide (w);
  w->inselect = 1;

  if ((y /= w->fontht) < 0)
    y = 0;
  else if (y >= w->ht)
    y = w->ht-1;

  w->selx0 = 0;
  w->sely0 = y;
  w->selx1 = w->wd;
  w->sely1 = y;

  vt_select_paint (w, w->selx0, w->sely0, w->selx1, w->sely1);
  vt_select_end (w);
}

static void
vt_redraw (vt_widget_t *w, long y, long ymax)
{
  vtchar_t save, new, *vcp = vt_screen_pos (w, 0, y);
  int i, j, xpos, ypos;
  char buf[BUFSIZE+1];
  char *cp;

  STORE_ATTRIBUTES(w, (&save));

  new = *vcp;
  RESTORE_ATTRIBUTES(w, vcp);

  xpos = 0;
  ypos = y * w->fontht;
  cp = buf;

  for (i = y; i < ymax; ++i) {
    for (j = 0; j < w->wd; ++j, ++vcp) {

      if (CMP_ATTRIBUTES((&new), vcp) || cp - buf >= BUFSIZE) {

	*cp = 0;
	w_printstring (w->w.win, xpos, ypos, buf);
	xpos += (cp - buf) * w->fontwd;
	cp = buf;

	new = *vcp;
	RESTORE_ATTRIBUTES(w, vcp);
      }
      *cp++ = vcp->c;
    }
    *cp = 0;
    cp = buf;
    w_printstring (w->w.win, xpos, ypos, buf);
    xpos = 0;
    ypos += w->fontht;
    vcp = vt_screen_nl (w, vcp);
  }
  RESTORE_ATTRIBUTES(w, (&save));
}

static void
vt_vmove (vt_widget_t *w, long offs)
{
  long oldoffs, dist, y, ymax;

  /*
   * make sure you cannot scroll back more than size of history
   */
  if (offs < 0)
    offs = 0;
  else if (offs > w->curhistsize)
    offs = w->curhistsize;
  oldoffs = w->offs - w->curoffs;
  if (oldoffs < 0)
    oldoffs += w->screenht;

  if ((dist = abs (offs - oldoffs)) == 0)
    return;

  vt_unselect (w);
  vt_cursor_hide (w);

  if (dist < w->ht) {
    if (offs > oldoffs) {
      /* move backwards in history */
      y = 0;
      ymax = dist;
      dist *= w->fontht;
      w_vscroll (w->w.win, 0, 0, w->w.w, w->w.h - dist, dist);
    } else {
      /* move forwards in history */
      y = w->ht - dist;
      ymax = w->ht;
      dist *= w->fontht;
      w_vscroll (w->w.win, 0, dist, w->w.w, w->w.h - dist, 0);
    }
  } else {
    y = 0;
    ymax = w->ht;
  }

  if ((w->curoffs = w->offs - offs) < 0)
    w->curoffs += w->screenht;

  vt_redraw (w, y, ymax);
  vt_cursor_show (w);

  if (w->hist_cb)
    (*w->hist_cb) ((widget_t *)w, offs, -1, -1, -1, NULL);
}

static void
vt_output (vt_widget_t *w, u_char *cp, long len)
{
  long ohistsize = w->curhistsize;

  vt_vmove (w, 0);
  vt_unselect (w);

  vt_cursor_hide (w);
  while (--len >= 0)
    (*w->outfn) (w, *cp++);
  vt_flush (w);
  vt_cursor_show (w);

  if (ohistsize != w->curhistsize && w->hist_cb)
    (*w->hist_cb) ((widget_t *)w, -1, w->curhistsize, -1, -1, NULL);
}

static int
vt_resize (vt_widget_t *w, long wd, long ht, long screenht)
{
  vtchar_t *nscreen, *vcp1, *vcp2;
  int i, j;

  if (wd < 1)
    wd = 1;
  if (ht < 1)
    ht = 1;
  if (screenht < ht)
    screenht = ht;

  if (wd == w->wd && ht == w->ht && screenht == w->screenht)
    return 0;

  nscreen = malloc (wd * screenht * sizeof (vtchar_t));
  if (!nscreen)
    return -1;

  vt_flush (w);
  vt_vmove (w, 0);
  vt_unselect (w);
  vt_cursor_hide (w);

  w_resize (w->w.win, wd * w->fontwd, ht * w->fontht);

  for (i = wd * screenht; --i >= 0; ) {
    STORE_ATTRIBUTES(w, (nscreen+i));
    nscreen[i].c = DEF_BGCHAR;
  }
  for (i = MIN (w->screenht, screenht); --i >= 0; ) {
    j = (MIN (w->ht, ht) - 1 - i) % screenht;
    if (j < 0)
      j += screenht;
    vcp1 = &nscreen[j * wd];

    j = (w->curoffs + MIN (w->ht, ht) - 1 - i) % w->screenht;
    if (j < 0)
      j += w->screenht;
    vcp2 = &w->screen[j * w->wd];

    for (j = MIN (w->wd, wd); --j >= 0; ) {
      *vcp1++ = *vcp2++;
    }
  }
  w->wd = wd;
  w->ht = ht;
  w->screenht = screenht;
  w->screen_size = wd * screenht;
  w->curoffs = w->offs = 0;
  free (w->screen);
  w->screen = nscreen;

  w->curx = MIN (w->wd-1, w->curx);
  w->cury = MIN (w->ht-1, w->cury);
  w->savx = MIN (w->wd-1, w->savx);
  w->savy = MIN (w->ht-1, w->savy);

  vt_redraw (w, 0, w->ht);
  vt_cursor_show (w);

  if (w->hist_cb)
    (*w->hist_cb) ((widget_t *)w, -1, w->curhistsize, w->wd, w->ht, NULL);

  return 0;
}

/************************************************************************/

static void
vt_bell (vt_widget_t *w)
{
  if (w->visbell) {
    w_setmode (w->w.win, M_INVERS);
    w_pbox (w->w.win, 0, 0, w->w.w, w->w.h);
    w_test (w->w.win, 0, 0);
    w_pbox (w->w.win, 0, 0, w->w.w, w->w.h);
  } else {
    w_beep ();
  }
}

static inline void
vt_cursor_on (vt_widget_t *w)
{
  w->curon = 1;
}

static inline void
vt_cursor_off (vt_widget_t *w)
{
  w->curon = 0;
}

static inline void
vt_cursor_xpos (vt_widget_t *w, long x)
{
  vt_flush (w);
  w->curx = (x - 32) & 0x7f;
  if (w->curx >= w->wd)
    w->curx = w->wd-1;
  else if (w->curx < 0)
    w->curx = 0;
}

static inline void
vt_cursor_ypos (vt_widget_t *w, long y)
{
  vt_flush (w);
  w->cury = (y - 32) & 0x7f;
  if (w->cury >= w->ht)
    w->cury = w->ht-1;
  else if (w->cury < 0)
    w->cury = 0;
}

static inline void
vt_cursor_up (vt_widget_t *w)
{
  vt_flush (w);
  if (--w->cury < 0)
    w->cury = 0;
}

static inline void
vt_cursor_down (vt_widget_t *w)
{
  vt_flush (w);
  if (++w->cury >= w->ht)
    w->cury = w->ht-1;
}

static inline void
vt_cursor_left (vt_widget_t *w)
{
  vt_flush (w);
  if (--w->curx < 0)
    w->curx = 0;
}

static inline void
vt_cursor_right (vt_widget_t *w)
{
  vt_flush (w);
  if (++w->curx >= w->wd)
    w->curx = w->wd-1;
}

static inline void
vt_cursor_home (vt_widget_t *w)
{
  vt_flush (w);
  w->curx = w->cury = 0;
}

static inline void
vt_cursor_save (vt_widget_t *w)
{
  w->savx = w->curx;
  w->savy = w->cury;
}

static inline void
vt_cursor_restore (vt_widget_t *w)
{
  vt_flush (w);
  w->curx = w->savx;
  w->cury = w->savy;
}

static inline void
vt_clear_screen (vt_widget_t *w)
{
  int i, j;
  vtchar_t *vcp;

  vt_flush (w);
  vcp = vt_screen_pos (w, 0, 0);
  for (i = 0; i < w->ht; ++i) {
    for (j = 0; j < w->wd; ++j, ++vcp) {
      STORE_ATTRIBUTES(w, vcp);
      vcp->c = DEF_BGCHAR;
    }
    vcp = vt_screen_nl (w, vcp);
  }
  w_setmode (w->w.win, w->bgmode);
  w_pbox (w->w.win, 0, 0, w->w.w, w->w.h);
}

static inline void
vt_clear_line (vt_widget_t *w)
{
  int i;
  vtchar_t *vcp;

  vt_flush (w);
  vcp = vt_screen_pos (w, 0, w->cury);
  for (i = 0; i < w->wd; ++i, ++vcp) {
    STORE_ATTRIBUTES(w, vcp);
    vcp->c = DEF_BGCHAR;
  }
  w_setmode (w->w.win, w->bgmode);
  w_pbox (w->w.win, 0, w->cury * w->fontht, w->w.w, w->fontht);
}

static inline void
vt_clear_eop (vt_widget_t *w)
{
  int i, j;
  vtchar_t *vcp;

  vt_flush (w);
  vcp = vt_screen_pos (w, w->curx, w->cury);
  for (i = w->curx; i < w->wd; ++i, ++vcp) {
    STORE_ATTRIBUTES(w, vcp);
    vcp->c = DEF_BGCHAR;
  }
  vcp = vt_screen_nl (w, vcp);
  for (i = w->cury+1; i < w->ht; ++i) {
    for (j = 0; j < w->wd; ++j, ++vcp) {
      STORE_ATTRIBUTES(w, vcp);
      vcp->c = DEF_BGCHAR;
    }
    vcp = vt_screen_nl (w, vcp);
  }
  w_setmode (w->w.win, w->bgmode);
  if (w->cury < w->ht-1) {
    w_pbox (w->w.win, 0, (w->cury + 1) * w->fontht,
	    w->w.w, (w->ht - 1 - w->cury) * w->fontht);
  }
  if (w->curx < w->wd-1) {
    w_pbox (w->w.win, w->curx * w->fontwd, w->cury * w->fontht,
  	    (w->wd - w->curx) * w->fontwd, w->fontht);
  }
}

static inline void
vt_clear_sop (vt_widget_t *w)
{
  int i, j;
  vtchar_t *vcp;

  vt_flush (w);
  vcp = vt_screen_pos (w, 0, 0);
  for (i = 0; i < w->cury; ++i) {
    for (j = 0; j < w->wd; ++j, ++vcp) {
      STORE_ATTRIBUTES(w, vcp);
      vcp->c = DEF_BGCHAR;
    }
    vcp = vt_screen_nl (w, vcp);
  }
  for (i = 0; i < w->curx; ++i, ++vcp) {
    STORE_ATTRIBUTES(w, vcp);
    vcp->c = DEF_BGCHAR;
  }
  w_setmode (w->w.win, w->bgmode);
  if (w->cury > 0)
    w_pbox (w->w.win, 0, 0, w->w.w, w->cury * w->fontht);
  if (w->curx > 0)
    w_pbox (w->w.win, 0, w->cury * w->fontht, w->curx * w->fontwd, w->fontht);
}

static inline void
vt_clear_eol (vt_widget_t *w)
{
  int i;
  vtchar_t *vcp;

  vt_flush (w);
  vcp = vt_screen_pos (w, w->curx, w->cury);
  for (i = w->curx; i < w->wd; ++i, ++vcp) {
    STORE_ATTRIBUTES(w, vcp);
    vcp->c = DEF_BGCHAR;
  }
  if (w->curx < w->wd-1) {
    w_setmode (w->w.win, w->bgmode);
    w_pbox (w->w.win, w->curx * w->fontwd, w->cury * w->fontht,
  	    (w->wd - w->curx) * w->fontwd, w->fontht);
  }
}

static inline void
vt_clear_sol (vt_widget_t *w)
{
  int i;
  vtchar_t *vcp;

  vt_flush (w);
  vcp = vt_screen_pos (w, 0, w->cury);
  for (i = 0; i < w->curx; ++i, ++vcp) {
    STORE_ATTRIBUTES(w, vcp);
    vcp->c = DEF_BGCHAR;
  }
  if (w->curx > 0) {
    w_setmode (w->w.win, w->bgmode);
    w_pbox (w->w.win, 0, w->cury * w->fontht, w->curx * w->fontwd, w->fontht);
  }
}

static inline void
vt_insert_line (vt_widget_t *w)
{
  int i, j;
  vtchar_t *vcp1, *vcp2;

  vt_flush (w);
  if (w->cury < w->ht-1) {
    vcp1 = vt_screen_pos (w, w->wd-1, w->ht-2);
    vcp2 = vt_screen_pos (w, w->wd-1, w->ht-1);

    for (i = w->cury; i < w->ht-1; ++i) {
      for (j = 0; j < w->wd; ++j)
        *vcp2-- = *vcp1--;
      vcp1 = vt_screen_pl (w, vcp1);
      vcp2 = vt_screen_pl (w, vcp2);
    }
    w_vscroll (w->w.win, 0, w->cury * w->fontht,
	       w->w.w, (w->ht - 1 - w->cury) * w->fontht,
	       (w->cury + 1) * w->fontht);
  }
  vcp1 = vt_screen_pos (w, 0, w->cury);
  for (i = 0; i < w->wd; ++i, ++vcp1) {
    STORE_ATTRIBUTES(w, vcp1);
    vcp1->c = DEF_BGCHAR;
  }
  w_setmode (w->w.win, w->bgmode);
  w_pbox (w->w.win, 0, w->cury * w->fontht, w->w.w, w->fontht);
  w->curx = 0;
}

static inline void
vt_rev_index (vt_widget_t *w)
{
  int i, j;
  vtchar_t *vcp1, *vcp2;

  vt_flush (w);
  if (--w->cury < 0) {
    w->cury = 0;
    vcp1 = vt_screen_pos (w, w->wd-1, w->ht-2);
    vcp2 = vt_screen_pos (w, w->wd-1, w->ht-1);

    for (i = w->cury; i < w->ht-1; ++i) {
      for (j = 0; j < w->wd; ++j)
        *vcp2-- = *vcp1--;
      vcp1 = vt_screen_pl (w, vcp1);
      vcp2 = vt_screen_pl (w, vcp2);
    }
    w_vscroll (w->w.win, 0, w->cury * w->fontht,
	       w->w.w, (w->ht - 1 - w->cury) * w->fontht,
	       (w->cury + 1) * w->fontht);

    vcp1 = vt_screen_pos (w, 0, w->cury);
    for (i = 0; i < w->wd; ++i, ++vcp1) {
      STORE_ATTRIBUTES(w, vcp1);
      vcp1->c = DEF_BGCHAR;
    }
    w_setmode (w->w.win, w->bgmode);
    w_pbox (w->w.win, 0, w->cury * w->fontht, w->w.w, w->fontht);
  }
}

static inline void
vt_delete_line (vt_widget_t *w)
{
  int i, j;
  vtchar_t *vcp1, *vcp2;

  vt_flush (w);
  if (w->cury < w->ht-1) {
    vcp1 = vt_screen_pos (w, 0, w->cury+1);
    vcp2 = vt_screen_pos (w, 0, w->cury);

    for (i = w->cury+1; i < w->ht; ++i) {
      for (j = 0; j < w->wd; ++j)
        *vcp2++ = *vcp1++;
      vcp1 = vt_screen_nl (w, vcp1);
      vcp2 = vt_screen_nl (w, vcp2);
    }
    w_vscroll (w->w.win, 0, (w->cury + 1) * w->fontht,
	       w->w.w, (w->ht - 1 - w->cury) * w->fontht,
	       w->cury * w->fontht);
  }
  vcp1 = vt_screen_pos (w, 0, w->ht-1);
  for (i = 0; i < w->wd; ++i, ++vcp1) {
    STORE_ATTRIBUTES(w, vcp1);
    vcp1->c = DEF_BGCHAR;
  }

  w_setmode (w->w.win, w->bgmode);
  w_pbox (w->w.win, 0, (w->ht - 1) * w->fontht, w->w.w, w->fontht);
  w->curx = 0;
}

static inline void
vt_scroll (vt_widget_t *w)
{
  int i;
  vtchar_t *vcp;

  vt_flush (w);
  if (++w->offs >= w->screenht)
    w->offs -= w->screenht;
  w->curoffs = w->offs;

  if (w->curhistsize < w->histsize)
    ++w->curhistsize;

  vcp = vt_screen_pos (w, 0, w->ht-1);
  for (i = 0; i < w->wd; ++i, ++vcp) {
    STORE_ATTRIBUTES(w, vcp);
    vcp->c = DEF_BGCHAR;
  }
  w_vscroll (w->w.win, 0, w->fontht, w->w.w, w->w.h - w->fontht, 0);
  w_setmode (w->w.win, w->bgmode);
  w_pbox (w->w.win, 0, (w->ht - 1) * w->fontht, w->w.w, w->fontht);
}


static inline void
vt_normal (vt_widget_t *w)
{
  /* cursor on & normalize attributes, not colors
   */
#ifdef VT_COLORS
  if (Colors) {
    /* switch color indeces instead of black/white in color mode
     */
    if (w->reversed) {
      vt_set_bgcol(w, 0 + '0');
      vt_set_fgcol(w, Colors-1 + '0');
    } else {
      vt_set_bgcol(w, Colors-1 + '0');
      vt_set_fgcol(w, 0 + '0');
    }
     w->bgmode = M_CLEAR;
     vt_setattr (w, F_NORMAL);
  } else
#endif	/* VT_COLORS */
  if (w->reversed) {
    w->bgmode = M_DRAW;
    vt_setattr (w, F_REVERSE);
  } else {
    w->bgmode = M_CLEAR;
    vt_setattr (w, F_NORMAL);
  }
  vt_cursor_on(w);
}

/* vt_setattr() will do the flush
 */
static inline void
vt_reverse_on (vt_widget_t *w)
{
  if (w->bgmode == M_DRAW) {
    vt_setattr (w, w->curattr & ~F_REVERSE);
  } else {
    vt_setattr (w, w->curattr | F_REVERSE);
  }
}

static inline void
vt_reverse_off (vt_widget_t *w)
{
  if (w->bgmode == M_DRAW) {
    vt_setattr (w, w->curattr | F_REVERSE);
  } else {
    vt_setattr (w, w->curattr & ~F_REVERSE);
  }
}

static inline void
vt_bold_on (vt_widget_t *w)
{
  vt_setattr (w, w->curattr | F_BOLD);
}

static inline void
vt_bold_off (vt_widget_t *w)
{
  vt_setattr (w, w->curattr & ~F_BOLD);
}

static inline void
vt_italic_on (vt_widget_t *w)
{
  vt_setattr (w, w->curattr | F_ITALIC);
}

static inline void
vt_italic_off (vt_widget_t *w)
{
  vt_setattr (w, w->curattr & ~F_ITALIC);
}

static inline void
vt_underline_on (vt_widget_t *w)
{
  vt_setattr (w, w->curattr | F_UNDERLINE);
}

static inline void
vt_underline_off (vt_widget_t *w)
{
  vt_setattr (w, w->curattr & ~F_UNDERLINE);
}

static inline void
vt_dim_on (vt_widget_t *w)
{
  vt_setattr (w, w->curattr | F_LIGHT);
}

static inline void
vt_dim_off (vt_widget_t *w)
{
  vt_setattr (w, w->curattr & ~F_LIGHT);
}

static inline void
vt_autowrap_on (vt_widget_t *w)
{
  w->autowrap = 1;
}

static inline void
vt_autowrap_off (vt_widget_t *w)
{
  w->autowrap = 0;
}

static inline void
vt_backspace (vt_widget_t *w)
{
  vt_flush (w);
  if (--w->curx < 0)
    w->curx = 0;
}

static inline void
vt_tab (vt_widget_t *w)
{
  vt_flush (w);
  w->curx = ((w->curx >> 3) + 1) << 3;
  if (w->curx >= w->wd)
    w->curx = w->wd-1;
}

static inline void
vt_lf (vt_widget_t *w)
{
  vt_flush (w);
  if (++w->cury >= w->ht) {
    vt_scroll (w);
    w->cury = w->ht-1;
  }
}

static inline void
vt_cr (vt_widget_t *w)
{
  vt_flush (w);
  w->curx = 0;
}

static inline void
vt_print (vt_widget_t *w, u_char c)
{
  vt_addchar (w, c);
  if (++w->curx >= w->wd) {
    vt_flush (w);
    if (!w->autowrap) {
      w->curx = w->wd-1;
    } else {
      w->curx = 0;
      if (++w->cury >= w->ht) {
	vt_scroll (w);
	w->cury = w->ht-1;
      }
    }
  }
}

/***************************** xterm 'emulator' *****************************/

#ifdef XTERM_EMU

/* '\E[?xxx', '\Exx;xxm' xterm terminal parameter handling
 *
 * ATM this implements xterm style color and mouse mode setting as many
 * programs seem to have these hardcoded.
 *
 * It would be easy to add here more.
 */
static void
putc_xparam(vt_widget_t *w, u_char c)
{
  int value, i;

  /* read next parameter? */
  if (c == ';' && w->params < MAX_PARAMETERS-1) {
    w->params++;
    return;
  }

  /* parameter? */
  if (c >= '0' && c <= '9')  {
    w->parameter[w->params] *= 10;
    w->parameter[w->params] += c - '0';
    return;
  }

  /* h = set mode
   * l = clear (reset) mode
   * r = restore mode (from restored)
   * s = save mode
   * m = set color???
   */
  if (c == 'h' || c == 'l') {
    value = 0;

    /* mouse mode setting */
    for (i = w->params; i >= 0; i--) {

      switch(w->parameter[i]) {

        /* set mouse emulation? */
	case 9:
	  value |= 1;	/* x10 mode */
	  break;
	case 1000:	/* report */
	case 1001:	/* track */
	  value |= 2;
	  break;
      }
    }
    if (c == 'l') {
      w->xtermemu &= ~value;
    } else {
      w->xtermemu |= value;
    }

  } else {

    if (c == 'm') {

      /* set colors and other modes */
      for (i = 0; i <= w->params; i++) {

	value = w->parameter[i];
	if (value >= 30 && value <= 37) {
	  vt_set_fgcol(w, value - 30 + '0');
	  continue;
	}
	if (value >= 40 && value <= 47) {
	  vt_set_bgcol(w, value - 40 + '0');
	  continue;
	}
	switch (value) {
	  case 0:
	    vt_normal(w);
	    break;
	  /* case 1: 	bold */
	  /* case 2:	dim */
	  /* case 4:	underline */
	  /* case 5:	blink */
	  /* case 7:	standout */
	  /* case 8:	invisible */
	  /* 10/11	alternate character set off/on */
	}
      }
    }
  }

  /* clear parameters */
  while (w->params >= 0) {
    w->parameter[w->params--] = 0;
  }
  w->params = 0;
  w->outfn = putc_normal;
}

/* hopefully this minimal vtxxx '\E[' code handler helps if VT52 isn't
 * enough for you program (at least it will be easier to add xterm
 * compatible functionality).
 */
static void
putc_xterm(vt_widget_t *w, u_char c)
{
  if (c >= '0' && c <= '9') {
    w->parameter[w->params] = c - '0';
    w->outfn = putc_xparam;
    return;
  }

  switch(c) {

    case '?':	/* xtra parameters */
      w->outfn = putc_xparam;
      return;

    /* these are already implemented by VT52 code */

    /* case 'A':  cursor up */
    /* case 'B':  cursor down */
    /* case 'C':  cursor right */
    /* case 'D':  cursor left */

    /* case '[':  ->function keys */
  }
  putc_xparam(w, c);
}

#endif /* XTERM_EMU */

/***************************** VT52 emulator *******************************/

static void
putc_esc_c (vt_widget_t *w, u_char c)
{
  w->outfn = putc_normal;
  vt_set_bgcol (w, c);
}

static void
putc_esc_b (vt_widget_t *w, u_char c)
{
  w->outfn = putc_normal;
  vt_set_fgcol (w, c);
}

static void
putc_esc_y2 (vt_widget_t *w, u_char c)
{
  w->outfn = putc_normal;
  vt_cursor_xpos (w, c);
}

static void
putc_esc_y1 (vt_widget_t *w, u_char c)
{
  w->outfn = putc_esc_y2;
  vt_cursor_ypos (w, c);
}

static void
putc_esc (vt_widget_t *w, u_char c)
{
  w->outfn = putc_normal;
  switch (c) {
#ifdef XTERM_EMU
/* xterm key codes */
    case '[':
      w->outfn = putc_xterm;
      break;
#endif /* XTERM_EMU */
    case 'A':	/* cursor up */
      vt_cursor_up (w);
      break;
    case 'B':	/* cursor down */
      vt_cursor_down (w);
      break;
    case 'C':	/* cursor right */
      vt_cursor_right (w);
      break;
    case 'D':	/* cursor left */
      vt_cursor_left (w);
      break;
    case 'E':	/* clear+home */
      vt_clear_screen (w);
      vt_cursor_home (w);
      break;
    case 'H':	/* home */
      vt_cursor_home (w);
      break;
    case 'I':	/* reverse index */
      vt_rev_index (w);
      break;
    case 'J':	/* erase to end of page */
      vt_clear_eop (w);
      break;
    case 'K':	/* erase to end of line */
      vt_clear_eol (w);
      break;
    case 'L':	/* insert line */
      vt_insert_line (w);
      break;
    case 'M':	/* delete line */
      vt_delete_line (w);
      break;
    case 'Y':	/* goto xy */
      w->outfn = putc_esc_y1;
      break;
    case 'b':	/* set fg col */
      w->outfn = putc_esc_b;
      break;
    case 'c':	/* set bg col */
      w->outfn = putc_esc_c;
      break;
    case 'd':	/* erase to start of page */
      vt_clear_sop (w);
      break;
    case 'e':	/* cursor on */
      vt_cursor_on (w);
      break;
    case 'f':	/* cursor off */
      vt_cursor_off (w);
      break;
    case 'j':	/* save cursor */
      vt_cursor_save (w);
      break;
    case 'k':	/* restore cursor */
      vt_cursor_restore (w);
      break;
    case 'o':	/* erase to start of line */
      vt_clear_sol (w);
      break;
    case 'p':	/* reverse on */
      vt_reverse_on (w);
      break;
    case 'q':	/* reverse off */
      vt_reverse_off (w);
      break;
    case 'v':	/* autowrap on */
      vt_autowrap_on (w);
      break;
    case 'w':	/*  autowrap off */
      vt_autowrap_off (w);
      break;

    case 'G':	/* clear all attributes */
      vt_normal (w);
      break;
    case 'g':	/* bold on */
      vt_bold_on (w);
      break;
    case 'h':	/* bold off */
      vt_bold_off (w);
      break;
    case 'i':	/* underline on */
      vt_underline_on (w);
      break;
    case 'm':	/* underline off */
      vt_underline_off (w);
      break;
    case 'n':	/* italic on */
      vt_italic_on (w);
      break;
    case 'r':	/* italic off */
      vt_italic_off (w);
      break;
    case 's':	/* dim on */
      vt_dim_on (w);
      break;
    case 't':	/* dim off */
      vt_dim_off (w);
      break;
  }
}

static void
putc_normal (vt_widget_t *w, u_char c)
{
  switch (c) {
    case 0:
      break;
    case 7: 	/* bell */
      vt_bell (w);
      break;
    case 8: 	/* backspace */
      vt_backspace (w);
      break;
    case 9: 	/* tab */
      vt_tab (w);
      break;
    case 10: 	/* linefeed */
      vt_lf (w);
      vt_cr (w);
      break;
    case 13:	/* carriage return */
      vt_cr (w);
      break;
    case 27:	/* escape */
      w->outfn = putc_esc;
      break;
    case 127:	/* delete */
      break;
    default:
      vt_print (w, c);
      break;
  }
}

/************ VT specific init / exit ************************/

static int
vt_loadfonts (vt_widget_t *w, char *basename, short fsize)
{
  WFONT *fp;
  if (!(fp = wt_loadfont (basename, fsize, 0, 1))) {
      return -1;
  }
  w->fonts[0] = fp;

  w->style_mask = DEF_STYLE_MASK;
  w->font_mask = 0;

  /* emulate only necessary styles
   */
  if ((ATTR_BD & ATTR_FONT_MASK) &&
      (w->fonts[ATTR_BD] = wt_loadfont (basename, fsize, F_BOLD, 1))) {
    w->font_mask |= F_BOLD;
    w->style_mask &= ~F_BOLD;
  } else {
    w->style_mask |= F_BOLD;
  }

  if ((ATTR_SL & ATTR_FONT_MASK) &&
  (w->fonts[ATTR_SL] = wt_loadfont (basename, fsize, F_ITALIC, 1))) {
    w->font_mask |= F_ITALIC;
    w->style_mask &= ~F_ITALIC;
  } else {
    w->style_mask |= F_ITALIC;
  }

  /* if either of above styles would be an effect, don't bother with this...
   */
  if (((w->font_mask & F_BOLD) && (w->font_mask & F_ITALIC)) &&
      !(w->fonts[ATTR_SL|ATTR_BD] = wt_loadfont (basename, fsize, F_BOLD|F_ITALIC, 1))) {
    /* have to use some font... */
    w->fonts[ATTR_SL|ATTR_BD] = wt_loadfont (basename, fsize, 0, 1);
  }

  w->fontwd = w->fonts[0]->maxwidth;
  w->fontht = w->fonts[0]->height;
  return 0;
}

static void
vt_unloadfonts (vt_widget_t *w)
{
  int i;

  for (i = 0; i <= ATTR_FONT_MASK; i++) {
    if (w->fonts[i]) {
      w_unloadfont (w->fonts[i]);
      w->fonts[i] = NULL;
    }
  }
}

static int
vt_initialize (vt_widget_t *w, long wd, long ht, long screenht)
{
  vtchar_t *vcp;
  int i;

  w->curvis = 0;
  w->curon = 1;
  w->curblink = 0;
  w->autowrap = 1;

#ifdef VT_COLORS
  if (Colors) {
    for (i = 0; i < Colors; i++) {
      if (ColorMapping[i] < 0) {
        w->color[i] = w_allocColor(w->w.win, RGB[i][0], RGB[i][1], RGB[i][2]);
      } else {
        /* use a shared color */
        w->color[i] = ColorMapping[i];
      }
    }
  }
#endif	/* VT_COLORS */

  /* initialize attributes */
  vt_normal(w);

  if (w->bgmode == M_DRAW || Colors) {
    w_setmode(w->w.win, w->bgmode);
    w_pbox(w->w.win, 0, 0, w->w.win->width, w->w.win->height);
  }
  w_setfont (w->w.win, w->fonts[0]);

  w->wd = wd;
  w->ht = ht;
  w->offs = 0;
  w->screenht = screenht;
  w->screen_size = wd * screenht;
  w->screen = malloc (w->screen_size * sizeof (vtchar_t));
  if (!w->screen)
    return -1;

  for (i = w->screen_size, vcp = w->screen; --i >= 0; ++vcp) {
    STORE_ATTRIBUTES(w, vcp);
    vcp->c = DEF_BGCHAR;
  }
  w->curx = w->savx = 0;
  w->cury = w->savy = 0;

  w->outx = w->outy = 0;
  w->outbufp = w->outbuf;

  w->outfn = putc_normal;
  return 0;
}

static void
vt_exit (vt_widget_t *w)
{
  vt_unloadfonts (w);
  if (w->fontfamily)
    free (w->fontfamily);
  if (w->screen)
    free (w->screen);

#ifdef VT_COLORS
  if (Colors) {
    int i;
    for (i = 0; i < Colors; i++) {
      /* mapped to own color instead of a shared one? */
      if (ColorMapping[i] < 0) {
        w_freeColor(w->w.win, w->color[i]);
      }
    }
  }
#endif
}

/**************************** widget code **************************/

static long vt_query_geometry (widget_t *, long *, long *, long *, long *);

static long
vt_init (void)
{
#ifdef VT_COLORS
  const char *color;
  char var[] = "vt_color0";
  long rgb;

  if ((1 << wt_global.screen_bits) >= MAX_COLORS) {

    for (Colors = 0; Colors < MAX_COLORS; Colors++) {
      var[sizeof(var)-2] = '0' + Colors;

      if (!(color = wt_variable(var))) {
        if (Colors >= wt_global.screen_shared) {
	  /* no valid color mapping */
	  break;
	}
        continue;
      }
      rgb = strtol(color, NULL, 0);
      RGB[Colors][0] = (rgb >> 16) & 0xff;
      RGB[Colors][1] = (rgb >> 8) & 0xff;
      RGB[Colors][2] = rgb & 0xff;

      /* shared color mapping won't be used for this color */
      ColorMapping[Colors] = -1;
    }
  }
#endif
  return 0;
}

static widget_t *
vt_create (widget_class_t *cp)
{
  vt_widget_t *wp = calloc (1, sizeof (vt_widget_t));
  if (!wp)
    return NULL;
  wp->w.class = wt_vt_class;
  wp->seltimer = -1;
  wp->bgmode = M_CLEAR;
  wp->reversed = 0;
  return (widget_t *)wp;
}

static long
vt_delete (widget_t *_w)
{
  vt_widget_t *w = (vt_widget_t *)_w;

  vt_exit (w);
  if (w->is_realized)
    w_delete (w->w.win);
  free (w);
  return 0;
}

static long
vt_close (widget_t *_w)
{
  vt_widget_t *w = (vt_widget_t *)_w;

  if (w->is_realized && w->is_open) {
    w_close (w->w.win);
    w->is_open = 0;
  }
  return 0;
}

static long
vt_open (widget_t *_w)
{
  vt_widget_t *w = (vt_widget_t *)_w;

  if (w->is_realized && !w->is_open) {
    w_open (w->w.win, w->w.x, w->w.y);
    w->is_open = 1;
  }
  return 0;
}

static long
vt_addchild (widget_t *parent, widget_t *w)
{
  return -1;
}

static long
vt_delchild (widget_t *parent, widget_t *w)
{
  return -1;
}

static long
vt_realize (widget_t *_w, WWIN *parent)
{
  vt_widget_t *w = (vt_widget_t *)_w;
  long x, y, wd, ht;

  if (w->is_realized)
    return -1;

  if (!w->fontwd && vt_loadfonts (w, w->fontfamily, w->fontht) < 0)
    return -1;

  vt_query_geometry (_w, &x, &y, &wd, &ht);
  w->w.w = wd;
  w->w.h = ht;

  x = wd / w->fontwd;
  y = ht / w->fontht;

  w->w.win = wt_create_window (parent, wd, ht,
	  W_NOBORDER|W_MOVE|EV_MOUSE, _w);
  if (!w->w.win)
    return -1;
  w->w.win->user_val = (long)w;

  /* alloc history, set colors, modes, font... */
  if (vt_initialize (w, x, y, y + w->histsize))
    return -1;

  vt_cursor_show (w);
  w_open (w->w.win, w->w.x, w->w.y);
  w->is_realized = 1;
  w->is_open = 1;
  return 0;
}

static long
vt_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
  vt_widget_t *w = (vt_widget_t *)_w;

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
vt_query_minsize (widget_t *_w, long *wdp, long *htp)
{
  vt_widget_t *w = (vt_widget_t *)_w;

  if (!w->fontwd)
    vt_loadfonts (w, w->fontfamily, w->fontht);

  if (!w->is_realized) {
    /* first time */
    *wdp = (w->usrcols > 0 ? w->usrcols : DEF_COLS) * w->fontwd;
    *htp = (w->usrrows > 0 ? w->usrrows : DEF_ROWS) * w->fontht;
  } else {
    *wdp = 10 * w->fontwd;
    *htp = 10 * w->fontht;
  }
  *wdp = MAX (*wdp, w->usrwd);
  *htp = MAX (*htp, w->usrht);
  return 0;
}

static long
vt_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
  vt_widget_t *w = (vt_widget_t *)_w;
  int ret = 0;

  if (x != w->w.x || y != w->w.y) {
    if (w->is_realized) {
      w_move (w->w.win, x, y);
    }
    w->w.x = x;
    w->w.y = y;
    ret = 1;
  }
  if (wd != w->w.w || ht != w->w.h) {
    wd /= w->fontwd;
    ht /= w->fontht;
    if (w->is_realized) {
      vt_resize (w, wd, ht, ht + w->histsize);
      w->w.w = w->wd * w->fontwd;
      w->w.h = w->ht * w->fontht;
    } else {
      w->w.w = wd * w->fontwd;
      w->w.h = ht * w->fontht;
    }
    ret = 1;
  }
  return ret;
}

static long
vt_setopt (widget_t *_w, long key, void *val)
{
  vt_widget_t *w = (vt_widget_t *)_w;
  short mask = 0;

  switch (key) {
    case WT_XPOS:
      if (vt_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_YPOS:
      if (vt_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
        mask |= WT_CHANGED_POS;
      break;

    case WT_WIDTH:
      w->usrwd = MAX (0, *(long *)val);
      if (w->is_realized) {
        if (vt_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
          mask |= WT_CHANGED_SIZE;
      }
      break;

    case WT_HEIGHT:
      w->usrht = MAX (0, *(long *)val);
      if (w->is_realized) {
        if (vt_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
          mask |= WT_CHANGED_SIZE;
      }
      break;

    case WT_VT_WIDTH:
      w->usrcols = *(long *)val;
      if (w->is_realized) {
        if (vt_reshape (_w, w->w.x, w->w.y, w->usrcols * w->fontwd, w->w.h))
	  mask |= WT_CHANGED_SIZE;
      }
      break;
      
    case WT_VT_HEIGHT:
      w->usrrows = *(long *)val;
      if (w->is_realized) {
        if (vt_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrrows * w->fontht))
	  mask |= WT_CHANGED_SIZE;
      }
      break;
      
    case WT_VT_HISTSIZE:
      if (!w->is_realized)
        w->histsize = *(long *)val;
      break;

    case WT_FONTSIZE:
      /* can't yet change font after realization :-( */
      if (w->is_realized)
        return -1;
      w->fontht = *(long *)val;
      break;

    case WT_FONT:
      /* can't yet change font after realization :-( */
      if (w->is_realized)
        return -1;
      if (w->fontfamily) {
	free(w->fontfamily);
	w->fontfamily = NULL;
      }
      if ((char *)val) {
        w->fontfamily = strdup((char *)val);
      }
      break;

    case WT_VT_STRING:
      if (!w->is_realized)
        return -1;
      vt_output (w, ((wt_opaque_t *)val)->cp, ((wt_opaque_t *)val)->len);
      break;

    case WT_VT_HISTPOS:
      if (!w->is_realized)
        return -1;
      vt_vmove (w, *(long *)val);
      break;

    case WT_VT_VISBELL:
      w->visbell = *(long *)val;
      break;

    case WT_VT_BLINK:
      w->curblink = *(long *)val;
      break;

    case WT_VT_REVERSE:
      w->reversed = *(long *)val;
      vt_change_bgmode (w, w->reversed);
      break;

    case WT_ACTION_CB:
      w->hist_cb = (void *)val;
      break;

    default:
      return -1;
  }
  if (mask && w->is_realized)
    wt_change_notify (_w, mask);
  return 0;
}

static long
vt_getopt (widget_t *_w, long key, void *val)
{
  vt_widget_t *w = (vt_widget_t *)_w;

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

    case WT_COLORS:
      *(long *)val = Colors;

    case WT_VT_WIDTH:
      *(long *)val = w->wd ? w->wd : w->usrcols;
      break;

    case WT_VT_HEIGHT:
      *(long *)val = w->ht ? w->ht : w->usrrows;
      break;

    default:
      return -1;
  }
  return 0;
}

static void
select_timer_cb (long arg)
{
  vt_widget_t *w = (vt_widget_t *)arg;
  short x, y;

  w_querymousepos (w->w.win, &x, &y);
  vt_select_drag (w, x, y);
  w->seltimer = wt_addtimeout (100, select_timer_cb, arg);
}

static WEVENT *
vt_event (widget_t *_w, WEVENT *ev)
{
  vt_widget_t *w = (vt_widget_t *)_w;
  wt_opaque_t pasted;

#ifdef XTERM_EMU
  /* if mode set and main app accepts, send mouse event sequences instead of
   * using the builtin widget copy / paste.
   */
  if (w->xtermemu && w->hist_cb) {
    char buf[8], button = ' ';
    pasted.cp = buf;
    pasted.len = 6;

    switch(ev->type) {

      case EVENT_MPRESS:
	/* xtermemu = 0, no emulation
	 * xtermemu = 1, x10 mode (report only presses)
	 * xtermemu = 2, x11 mode (report also releases)
	 */
	switch (ev->key) {
	  case BUTTON_RIGHT:
	    button++;
	  case BUTTON_MID:
	    button++;
	  case BUTTON_LEFT:
	    sprintf(buf, "\e[M%c%c%c", button,
		    '!' + ev->x / w->fontwd, '!' + ev->y / w->fontht);
	    break;
	}
	(*w->hist_cb) (_w, -1, -1, -1, -1, &pasted);
	break;

      case EVENT_MRELEASE:
	if (w->xtermemu > 1) {
	  sprintf(buf, "\e[M#%c%c",
		  '!' + ev->x / w->fontwd, '!' + ev->y / w->fontht);
	  (*w->hist_cb) (_w, -1, -1, -1, -1, &pasted);
	}
	break;

      default:
	return ev;
    }
  } else
#endif /* XTERM_EMU */
  {
    switch(ev->type) {
      case EVENT_MPRESS:
	if (ev->key & BUTTON_LEFT) {

	  if (ev->time > w->clicktime) {
	    w->clicktime = ev->time + DEF_CLICKTIME;
	    w->nclicks = 0;
	  }
	  switch (++w->nclicks) {
	    case 1:
	      vt_select_start (w, ev->x, ev->y);
	      select_timer_cb ((long)w);
	      break;

	    case 2:
	      vt_select_word (w, ev->x, ev->y);
	      break;

	    case 3:
	      vt_select_line (w, ev->x, ev->y);
	      w->nclicks = 0;
	      break;
	  }
	} else if (ev->key & BUTTON_RIGHT) {
	  w_selection_t *sel = w_getselection (W_SEL_TEXT);
	  if (sel) {
	    pasted.cp  = sel->data;
	    pasted.len = sel->len;
	    if (w->hist_cb)
	      (*w->hist_cb) (_w, -1, -1, -1, -1, &pasted);
	    w_freeselection (sel);
	  } else {
	    w_beep ();
	  }
	}
	break;

      case EVENT_MRELEASE:
	if (ev->key & BUTTON_LEFT) {
	  if (w->nclicks == 1) {
	    if (ev->x >= 0 && ev->y >= 0)
	      vt_select_drag (w, ev->x, ev->y);
	    vt_select_end (w);
	  }
	}
	break;

      default:
	return ev;
    }
  }
  return NULL;
}

static long
vt_changes (widget_t *w, widget_t *w2, short changes)
{
  return 0;
}

static widget_class_t _wt_vt_class = {
  "vt", 0,
  vt_init,
  vt_create,
  vt_delete,
  vt_close,
  vt_open,
  vt_addchild,
  vt_delchild,
  vt_realize,
  vt_query_geometry,
  vt_query_minsize,
  vt_reshape,
  vt_setopt,
  vt_getopt,
  vt_event,
  vt_changes,
  vt_changes
};

widget_class_t *wt_vt_class = &_wt_vt_class;
