/*
 * settings.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib attribute setting function mappings to Xlib
 */

#include <stdio.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


short w_settextstyle(WWIN *win, ushort flags)
{
  const char *cptr;
  short old;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_settextstyle(%p,%i) -> %s\n", win, flags, cptr));
    TRACEEND();
    return -1;
  }

  old = win->textstyle;
  win->textstyle = flags & F_STYLEMASK;

  TRACEPRINT(("w_settextstyle(%p,%i)\n", win, flags));
  TRACEEND();
  return old;
}


short w_setsaver(short seconds)
{
  TRACESTART();
  TRACEPRINT(("w_setsaver(%i) -> 0\n", seconds));
  TRACEEND();

  return 0;
}


ushort w_setpatterndata(WWIN *win, ushort *data)
{
  W2XWin *ptr = (W2XWin *)win;
  ushort old;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_setpatterndata(%p,%p) -> %s\n", win, data, cptr));
    TRACEEND();
    return 0;
  }

  if (ptr->stipple)
    XFreePixmap(_Display, ptr->stipple);

  /* create bitmap for pattern and set it to window */
  ptr->stipple = XCreateBitmapFromData(_Display, ptr->wid,
		 (char*)data, 16, 16);
  XSetStipple(_Display, ptr->fillgc, ptr->stipple);

  old = win->pattern;
  win->pattern = W_USERPATTERN;

  TRACEPRINT(("w_setpatterndata(%p,%p) -> 0x%04x\n", win, data, old));
  TRACEEND();
  return old;
}


ushort w_setpattern(WWIN *win, ushort id)
{
  static const ushort DM64[8][8] = {		/* courtesy of Kay... */
    {  0, 32,  8, 40,  2, 34, 10, 42 },
    { 48, 16, 56, 24, 50, 18, 58, 26 },
    { 12, 44,  4, 36, 14, 46,  6, 38 },
    { 60, 28, 52, 20, 62, 30, 54, 22 },
    {  3, 35, 11, 43,  1, 33,  9, 41 },
    { 51, 19, 59, 27, 49, 17, 57, 25 },
    { 15, 47,  7, 39, 13, 45,  5, 37 },
    { 63, 31, 55, 23, 61, 29, 53, 21 }
  };
  const ushort *intensity;
  static ushort buffer[16];
  ushort old, patt, *patbuf = buffer;
  const char *cptr;
  int i;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_setpattern(%p,0x%04x) -> %s\n",
		win, (unsigned int)id, cptr));
    TRACEEND();
    return 0;
  }

  if(id <= MAX_GRAYSCALES)
  {
    /* grayscale patterns */
    for (i=0; i < 8; i++)
    {
      patt = 0;
      intensity = DM64[i];
      /* matrix values [0-63] -> id 0=black, 64=white */
      if(id <= *intensity++) patt  = 0x0101;
      if(id <= *intensity++) patt |= 0x0202;
      if(id <= *intensity++) patt |= 0x0404;
      if(id <= *intensity++) patt |= 0x0808;
      if(id <= *intensity++) patt |= 0x1010;
      if(id <= *intensity++) patt |= 0x2020;
      if(id <= *intensity++) patt |= 0x4040;
      if(id <= *intensity++) patt |= 0x8080;
      patbuf[8] = patt;
      *patbuf++ = patt;
    }
  }
  else if(id >= 0xff)
  {
    memset(patbuf, 0, sizeof(buffer));

    /* roll right */
    if (id & 0x8100) {
      for(i = 0; i < 16; i++) {
	*patbuf++ |= id;
	id = ((id & 1) << 15) | (id >> 1);
      }
      patbuf -= 16;
    }
    /* roll left */
    if (id & 0x300) {
      for(i = 0; i < 16; i++) {
	*patbuf++ |= id;
	id = ((id & 0x8000) >> 15) | (id << 1);
      }
    }
    /* pattern is a hatch if 1st upper byte bit is set */
  }
  else
  {
    /* default 50% gray */
    for (i = 0; i < 8; i++)
    {
      *patbuf++ = 0xaaaau;
      *patbuf++ = 0x5555u;
    }
  }

  old = w_setpatterndata(win, buffer);
  win->pattern = id;

  TRACEPRINT(("w_setpattern(%p,0x%04x) -> 0x%04x\n",
	      win, (unsigned int)id, old));
  TRACEEND();
  return old;
}


#define GC_MODE (GCForeground | GCBackground | GCFunction)

short w_setmode(WWIN *win, ushort mode)
{
  XGCValues gv;
  const char *cptr;
  short old;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_setmode(%p,%i) -> %s\n", win, mode, cptr));
    TRACEEND();
    return -1;
  }

  if(mode == M_CLEAR)
  {
    gv.foreground = _White;
    gv.background = _White;
    gv.fill_style = FillStippled;
    gv.function = GXcopy;
  }
  else
  {
    switch(mode)
    {
      case M_INVERS:
	gv.foreground = _Black^_White;
	gv.background = _White^_Black;
	gv.fill_style = FillStippled;
	gv.function = GXxor;
        break;
      case M_TRANSP:
	gv.foreground = _Black;
	gv.background = _Black;
	gv.fill_style = FillStippled;
	gv.function = GXor;
        break;
      default:
	gv.foreground = _Black;
	gv.background = _White;
	gv.fill_style = FillOpaqueStippled;
	gv.function = GXcopy;
    }
  }
  XChangeGC(_Display, ((W2XWin *)win)->gc, GC_MODE, &gv);
  XChangeGC(_Display, ((W2XWin *)win)->fillgc, GC_MODE | GCFillStyle, &gv);

  old = win->drawmode;
  win->drawmode = mode;

  TRACEPRINT(("w_setmode(%p,%i) -> %i\n", win, mode, old));
  TRACEEND();

  return old;
}


short w_setlinewidth(WWIN *win, short width)
{
  XGCValues gv;
  const char *cptr;
  short old;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_setlinewidth(%p,%i) -> %s\n", win, width, cptr));
    TRACEEND();
    return -1;
  }

  if (width < 2) {
    /* use fast lines */
    gv.line_width = 0;
    width = 1;
  } else {
    gv.line_width = width;
  }
  XChangeGC(_Display, ((W2XWin *)win)->gc, GCLineWidth, &gv);
  XChangeGC(_Display, ((W2XWin *)win)->fillgc, GCLineWidth, &gv);

  old = win->linewidth;
  win->linewidth = width;

  TRACEPRINT(("w_setlinewidth(%p,%i) -> %i\n", win, width, old));
  TRACEEND();
  return old;
}
