/*
 * types.h, a part of the W Window System
 *
 * Copyright (C) 1994-1998 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- definition of most (internal) data structures
 *
 * TODO
 * - Only top level windows need gadget RECs, title string and hints.
 *   Container windows won't need BITMAP, GC, pattbuf nor colortable.
 *   Maybe some of them could be done dynamically allocated (probably
 *   not worth the effort, unless you have hundreds of windows)? ++eero
 */

#ifndef __W_TYPES_H
#define __W_TYPES_H

#include "config.h"
#define __WSERVER
#include "../lib/Wlib.h"

#ifndef uchar
#define	uchar	unsigned char
#endif
#ifndef ushort
#define	ushort	unsigned short
#endif
#ifndef ulong
#define	ulong	unsigned long
#endif


/* the header of a *.wfnt font FILE(!)
 *
 * because different compilers may use different padding for different
 * types, font file struct members are ordered in decreasing type size.
 * I'm assuming that shorts are always aligned to short boundaries,
 * if not, fonts aren't interchangeble between systems. ++eero
 */
typedef struct {
  long magic;			/* font `generation' id */
  long lenght;			/* font data lenght */
  short header;			/* FONTHDR + info lenght */

/* font type */
  short height;			/* font size */
  ushort flags;			/* proportional, CHARSET etc. */
  ushort styles;		/* styles implemented in font */

/* style effect variables */
  short thicken;		/* `boldness' */
  ushort skew;			/* slanting pattern */
  ushort reserved;		/* ATM just padding */
  ushort effect_mask;		/* usable (=readable) style effects */

/* font cell information */
  short baseline;		/* baseline offset for vertical aligning */
  short maxwidth;		/* width of the widest char */

  char family[MAXFAMILYNAME];	/* font family name */
} FONTHDR;
/* followed with optional ASCII strings (included into `header' lenght)
 * character width table and the streamed font data
 */

/* how a font looks like in memory
 */
typedef struct {
  char *name;		/* the name repeated, to be able to search for it */

  FONTHDR hdr;		/* font header */
  short slant_size;	/* slanting width increase */
  short slant_offset;	/* italic correction */

  uchar widths[256];	/* character widths */
  ulong *data;		/* actual font data in longs */

  long offsets[256];	/* offsets in longs for each char */
  long numused;		/* how many times this font is used by clients */

  /* should eventually go to font references */
  ushort effects;	/* effect flags */
} FONT;


typedef struct _list {
  struct _list *next;
  short handle;
} LIST;

struct _window;

typedef struct _client {
  /* these fields implement a socket buffer */
  char *buf;
  short inbuf;

  int sh;		/* file handle for socket */

  /* some links for the linear list */
  struct _client *prev, *next;

  ushort flags;		/* various flags */
  ulong raddr;		/* remote IP adress if != 0 */
  long pakets, bytes;	/* some statistics */
  uid_t uid;		/* uid of user of this client */

  /* these fields are used for get/setblock */
  short btype, bx0, by0;
  long bsize;
  uchar *bptr;
  BITMAP bbm;
  struct _window *bwin;

  /* fonts are now on a per-client basis instead of per-window */
  LIST *font;

  /* some more statistics */
  int openWindows, totalWindows;
} CLIENT;


/* rectangle definition, used in many places like rect.c. for clipping
 * rectangles, for speed reasons we store both x1, y1 and w, h.
 * x1 = x0+w-1, y1 = y0+w-1 and w,h >= 0 is required.
 */
typedef struct _rect{
  short x0, y0, w, h;
  short x1, y1;
  struct _rect *next;
} REC;


/* a graphic context
 */
typedef struct {
  ushort drawmode;	/* M_CLEAR... */
  ushort textstyle;	/* F_NORMAL... */
  short linewidth;	/* line width */
  FONT *font;
  ushort *pattern;	/* for dashed functions */
  short fgCol, bgCol;	/* foreground and background color */
  /* helpers for PACKED driver (with up to 8 planes) */
  ushort fgColMask[8], bgColMask[8];
} GCONTEXT;


/* how the color tables look like
 *
 * 'red', 'green' and 'blue' are (1 << glob_screen->bm.planes) bytes long
 * and 'used' same lenght in bits...
 */
typedef struct _colortable {
  struct _window *win;		/* pointer to window owning this stuff */
  short colors;			/* the highest used color index */
  uchar *red, *green, *blue;	/* the RGB values */
  uchar *used;			/* bitfield of the really used entries */
} COLORTABLE;


#if 0
/* hints to be used when dealing with a window
 */
typedef struct {
  short minWidth, minHeight;
  short maxWidth, maxHeight;
  short widthInc, heightInc;
} HINTS;
#endif


/* maximum number of 'logical' areas a window might have
 */
#define AREA_MAXAREAS 4

/* some indices to the area array for each window, if *.width is < 1 the
 * area is not used in this window, e.g. it hasn't got the specific gadget.
 * of course all windows should have the work area...
 */
#define AREA_WORK 0
#define AREA_CLOSE 1
#define AREA_ICON 2
#define AREA_TITLE 3

/* this is the window struct
 */
typedef struct _window {

  /* client this window belongs to
   */
  CLIENT *cptr;

  /* window handle that is passed to the clients
   */
  long id;

  /* client window pointer returned in events
   */
  WWIN *libPtr;

  /* `is_open' is nonzero if the window is opened (ie. mapped to the screen).
   */
  unsigned is_open:1;

  /* `is_hidden' is zero if the window is totally visible, ie is not hidden
   * by other windows.
   */
  unsigned is_hidden:1;
  unsigned is_hidden_backup:1;

  /* `is_dirty' must be set by the drawing routines to some value != 0 if the
   * window on the screen is more up to date than the backing bitmap.
   *
   * the window manager routines use it in the following way when opening,
   * closing, moving, resizing or restacking windows:
   *
   * if (is_dirty) {
   *	save parts of the window on the screen to the backing bitmap
   *    because they will be obscured by other windows soon.
   * }
   */
  unsigned is_dirty:1;

  /* whether the window already has a frame or not. this one's used at
   * opening time to check if this is the first open of the window and
   * therefore a frame must be drawn.
   */
  unsigned has_frame:1;

  /* whether the window already has a title or not. see above.
   */
  unsigned has_title:1;

  /* each window has a pointer to its parent (only for the root window the
   * `parent' pointer is NULL).
   *
   * `childs' points to a doubly linked list of childs of that window. If
   * there are no childs then `childs' is NULL.
   *
   * the childlist is constructed via the `next' and `prev' pointers. the
   * topmost window is the first (leftmost) element of the list, the window
   * at the bottom is the last (rightmost) element of the list. for the top
   * window the `prev' pointer is NULL, for the bottom window `next' is NULL.
   */
  struct _window *next, *prev;
  struct _window *parent, *childs;

  /* a pointer to a list of rectangles that make up the visible area of the
   * window. All rectangles are stored with absolute (ie not window relative
   * coordinates). both pointers are NULL if the window is closed (ie when
   * is_open == 0).
   */
  REC *vis_recs;
  REC *vis_recs_backup;

  /* `pos' is a rectangle that makes up the whole area the window covers,
   * including the border (in absolute coords). `work' is a rectangle that
   * makes up the working area of the window, excluding the border (in
   * absolute coords). `clip' will become the user-settable clipping areas,
   * clip[0] in absolute and clip[1] in relative coordinates - but they're
   * not yet implemented.
   */
  REC pos;
  REC work;
  REC clip[2];

  /* `area' is an array of rectangles describing several logical areas of the
   * window (in coordinates relative to `pos'), like the working area (again),
   * close and icon gadget and so on...
   */
  REC area[AREA_MAXAREAS];

  /* functions for performing backing storage and redrawing for this window.
   * the rectangle is in absolute (ie not window relative) coords and is
   * already clipped to the visibile area of the window.
   */
  int (*save) (struct _window *, REC *);
  int (*redraw) (struct _window *, REC *);

  /* bitblitting routine for moving windows fast. */
  int (*bitblk) (REC *, REC *);

  ushort flags;   /* window creation flags */
  char title[MAXTITLE];   /* title text */

  /* the backing store bitmap */
  BITMAP bitmap;

  /* patterns */
  ushort patbuf[16]; /* buffer for patterns */

  /* the graphic context */
  GCONTEXT gc;

  /* rectangle that must be redrawn */
  REC dirty;

  /* W1R4 extensions */
  struct _colortable *colTab;
#if 0
  HINTS hints;
#endif

  short mtype;
  WMOUSE *mdata;
} WINDOW;


typedef struct {
  short	fh;   /* filehandle */
  short	visible;   /* is it currently visible? */
  short	disabled;   /* whether is it disabled or not */
  REC real;   /* where it is really */
  REC drawn;   /* where it is drawn */
} MOUSE;


typedef struct {
  const char *pattern;
  short xDrawOffset, yDrawOffset;
  ushort mask[16];
  ushort icon[16];
} MOUSEPOINTER;


typedef struct {
  const char *title;
  const char *command;
} ITEM;


typedef struct {
  short	items;
  ITEM item[MAXITEMS];
} MENU;


typedef struct {
  long x0, y0;
  long rx, ry;
  long ax, ay;
  long bx, by;
  short adir, bdir;
} PIZZASLICE;


/* the interface to the actual graphic driver
 */
typedef struct {
  BITMAP bm;
  short (*changePalette)(COLORTABLE *colTab, short index);
  void (*vtswitch)(void);
  void (*mouseShow)(MOUSEPOINTER *mptr);
  void (*mouseHide)(void);
  BITMAP * (*createbm)(BITMAP *to_fill, short width, short height, short alloc_data);
  void (*bitblk) (BITMAP *bm, long x0, long y0, long width, long height, BITMAP *bm1, long x1, long y1);
  void (*scroll) (BITMAP *bm, long x0, long y0, long width, long height, long y1);
  void (*prints) (BITMAP *bm, long x0, long y0, const uchar *s);
  long (*test)   (BITMAP *bm, long x0, long y0);
  void (*plot)   (BITMAP *bm, long x0, long y0);
  void (*line)   (BITMAP *bm, long x0, long y0, long xe, long ye);
  void (*hline)  (BITMAP *bm, long x0, long y0, long xe);
  void (*vline)  (BITMAP *bm, long x0, long y0, long ye);
  void (*box)    (BITMAP *bm, long x0, long y0, long width, long height);
  void (*pbox)   (BITMAP *bm, long x0, long y0, long width, long height);
  void (*circ)   (BITMAP *bm, long x0, long y0, long r);
  void (*pcirc)  (BITMAP *bm, long x0, long y0, long r);
  void (*ell)    (BITMAP *bm, long x0, long y0, long rx, long ry);
  void (*pell)   (BITMAP *bm, long x0, long y0, long rx, long ry);
  void (*arc)    (BITMAP *bm, PIZZASLICE *data);
  void (*pie)    (BITMAP *bm, PIZZASLICE *data);
  void (*poly)   (BITMAP *bm, long npts, long *pts);
  void (*ppoly)  (BITMAP *bm, long npts, long *pts);
  void (*bez)    (BITMAP *bm, long *controls);
  void (*dplot)  (BITMAP *bm, long x0, long y0);
  void (*dline)  (BITMAP *bm, long x0, long y0, long xe, long ye);
  void (*dvline) (BITMAP *bm, long x0, long y0, long ye);
  void (*dhline) (BITMAP *bm, long x0, long y0, long xe);
  void (*dbox)   (BITMAP *bm, long x0, long y0, long width, long height);
  void (*dpbox)  (BITMAP *bm, long x0, long y0, long width, long height);
  void (*dcirc)  (BITMAP *bm, long x0, long y0, long r);
  void (*dpcirc) (BITMAP *bm, long x0, long y0, long r);
  void (*dell)   (BITMAP *bm, long x0, long y0, long rx, long ry);
  void (*dpell)  (BITMAP *bm, long x0, long y0, long rx, long ry);
  void (*darc)   (BITMAP *bm, PIZZASLICE *data);
  void (*dpie)   (BITMAP *bm, PIZZASLICE *data);
  void (*dpoly)  (BITMAP *bm, long npts, long *pts);
  void (*dppoly) (BITMAP *bm, long npts, long *pts);
  void (*dbez)   (BITMAP *bm, long *controls);
} SCREEN;

#endif /* __W_TYPES_H */
