/*
 * init.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- global variables and Wlib init / exit function mappings to Xlib
 *
 * TODO:
 * - implement w_null(), w_beeb().
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "Wlib.h"
#include "proto.h"


/*
 * all the global variables
 */

WWIN *WROOT = NULL;
WSERVER _wserver;

/* X Stuff */
int _Screen;			/* screen number */
Display *_Display;		/* X connection id */
GC _Rootgc;			/* default graphics context */
Window _Rootwin;		/* root (desktop) window id */
unsigned long _Black;		/* display independent colors */
unsigned long _White;
unsigned long _Depth;
Pixmap _GrayPattern;		/* default stipple pattern */
Atom _WMprotocols;		/* window manager client protocols */
Atom _WMdelete_window;		/* WM close button event id */

static const char pattern[] =
{
  0xaa, 0x55
};


WSERVER *w_init(void)
{
  static W2XWin wroot;
  XGCValues gv;

  TRACESTART();

  /* Initialization */
  if(!(_Display = XOpenDisplay("")))	/* connect to $DISPLAY */
  {
    fprintf(stderr, "Unable to open %s\n", getenv("DISPLAY"));
    TRACEPRINT(("w_init() -> NULL\n"));
    TRACEEND();
    return NULL;
  }

  _Screen  = DefaultScreen(_Display);
  _Rootwin = DefaultRootWindow(_Display);

  /* Device independent colors */
  _White = WhitePixel(_Display, _Screen);
  _Black = BlackPixel(_Display, _Screen);
  _Depth = DisplayPlanes(_Display, _Screen);

  /* create a standard graphical context */
  gv.foreground = _Black;
  gv.background = _White;
  gv.function = GXcopy;
  gv.line_width = 0;
  _Rootgc = XCreateGC(_Display, _Rootwin, GMASK, &gv);

  /* create bitmap for standard pattern */
  _GrayPattern = XCreateBitmapFromData(_Display, _Rootwin, pattern, 8, 2);

  _wserver.vmaj = _WMAJ;
  _wserver.vmin = _WMIN;
  _wserver.pl   = _WPL;
  _wserver.type = BM_PACKEDMONO;
  _wserver.width  = DisplayWidth(_Display, _Screen);
  _wserver.height = DisplayHeight(_Display, _Screen);
  _wserver.planes = 1;
  _wserver.flags  = 0;
  _wserver.fname  = X_FONT;

  _WMprotocols = XInternAtom(_Display, "WM_PROTOCOLS", 0);
  _WMdelete_window = XInternAtom(_Display, "WM_DELETE_WINDOW", 0);

  wroot.win.magic = MAGIC_W;
  wroot.win.handle = 0;
  wroot.win.width = _wserver.width;
  wroot.win.height = _wserver.height;
  wroot.win.colors = _wserver.sharedcolors;

  /* several different programs may write to ROOT window so you can't trust
   * it to have any certain value!
   */
  wroot.win.fg = UNDEF;
  wroot.win.bg = UNDEF;
  wroot.win.drawmode = UNDEF;
  wroot.win.linewidth = UNDEF;
  wroot.win.textstyle = UNDEF;
  wroot.win.pattern = 0xffff;

  wroot.wid = _Rootwin;
  wroot.fillgc = _Rootgc;
  wroot.gc = _Rootgc;
  WROOT = &wroot.win;

  TRACEPRINT(("w_init() -> 0\n"));
  TRACEEND();

  return &_wserver;
}


void w_exit(void)
{
  TRACESTART();

  if (_Display) {
    TRACEPRINT(("w_exit()\n"));
    XCloseDisplay(_Display);
  } else {
    TRACEPRINT(("w_exit() -> no Display\n"));
  }
  TRACEEND();
}


short w_null(void)
{
  /* null roundabout msg for timing etc. */
  return 0;
}


short w_beep(void)
{
  /* make server say 'beeb' */
  return 0;
}
