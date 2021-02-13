/*
 * proto.h, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- W2Xlib structure, function and variable declarations
 */

#ifndef __W_PROTO_H
#define __W_PROTO_H

/* standard includes */
#include <stdio.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <netinet/in.h>
#include "../server/config.h"


/* in util.c */
#ifdef TRACE
extern int _wtrace;
extern int _traceIndent;
extern void _wspaces(void);
#define TRACESTART() { _traceIndent += 2; }
#define TRACEPRINT(args) if (_wtrace) { _wspaces(); printf args ; fflush(stdout); }
#define TRACEEND() { _traceIndent -= 2; }
#else
#define TRACESTART()
#define TRACEPRINT(args)
#define TRACEEND()
#endif


/* default context settings and font */
#define GMASK (GCForeground | GCBackground | GCFunction | GCLineWidth)

/* default X font (font alias, not a full name, see font.c) */
#define X_FONT	"fixed"


/* Wlib internal window structure */
typedef struct _W2XWin
{
  WWIN win;			/* Wlib window struct */
  struct _W2XWin *prev;		/* list for event mapping */
  struct _W2XWin *next;

  Window wid;			/* identifier for X window */
  int flags;			/* W window flags */
  char *title;			/* window title */

  /* not W_CONTAINER */
  GC  gc;			/* graphics context */
  GC fillgc;			/* -"- for dashed / filled primitives */
  Pixmap pixmap;		/* backup of window contents */
  Pixmap stipple;		/* window pattern */
  int need_flush;		/* whether window contents need flushing */
  short wxy[4];			/* window area that needs flushing */
} W2XWin;


typedef struct
{
  WFONT font;
  XFontStruct *xfont;
  ushort req_styles;		/* what we mapped font into */
  short req_size;
} W2XFont;


/* X11 Stuff */
extern int _Screen;			/* screen number */
extern Display *_Display;		/* X connection id */
extern GC _Rootgc;			/* default graphics context */
extern Window _Rootwin;			/* root (desktop) window id */
extern unsigned long _Black;		/* display independent colors */
extern unsigned long _White;
extern unsigned long _Depth;		/* display bit depth */
extern Pixmap _GrayPattern;		/* default stipple pattern */

/* X11 window manager stuff */
extern Atom _WMprotocols;		/* to get events... */
extern Atom _WMdelete_window;		/* ...like delete window */

/* Wlib stuff */
extern WSERVER _wserver;
extern WWIN *WROOT;

/* in convert.c */
extern void	_wget_options(uchar **graymap, int *expand);
extern uchar	*_winit_mapping(BITMAP *bm, int row);

/* W2Xlib internal utilities */
extern W2XWin	*_find_window(Window win);
extern WEVENT	*_event_handler(XEvent *xevent);
extern const char *_check_window(WWIN *ptr);
extern const char *_flush_area(W2XWin *ptr, int x1, int y1, int x2, int y2, int width);
extern void	_set_last(W2XWin *ptr);
extern void	_redraw(W2XWin *ptr);

#endif	/* __W_PROTO_H */
