/*
 * color.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib color function stubs for W2Xlib
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


short w_allocColor (WWIN *win, uchar red, uchar green, uchar blue)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_allocColor(%p,%i,%i,%i) -> %s\n",
		win, red, green, blue, cptr));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_allocColor(%p,%i,%i,%i) -> not yet implemented\n",
	      win, red, green, blue));
  TRACEEND();

  return -1;
}


short w_getColor (WWIN *win, short color, uchar *red, uchar *green, uchar *blue)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_getColor(%p,%i,%p,%p,%p) -> %s\n",
		win, color, red, green, blue, cptr));
    TRACEEND();
    return -1;
  }

  if (!(red && green && blue)) {
    TRACEPRINT(("w_getColor(%p,%i,%p,%p,%p) -> invalid argument\n",
		win, color, red, green, blue));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_allocColor(%p,%i,%i,%i,%i) -> not yet implemented\n",
	      win, color, *red, *green, *blue));
  TRACEEND();

  return -1;
}


short w_freeColor (WWIN *win, short color)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_freeColor(%p,%i) -> %s\n", win, color, cptr));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_freeColor(%p,%i) -> not yet implemented\n", win, color));
  TRACEEND();

  return -1;
}


short w_changeColor (WWIN *win, short color, uchar red, uchar green, uchar blue)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_changeColor(%p,%i,%i,%i,%i) -> %s\n",
		win, color, red, green, blue, cptr));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_changeColor(%p,%i,%i,%i,%i) -> not yet implemented\n",
	      win, color, red, green, blue));
  TRACEEND();

  return -1;
}


static inline short setColor(WWIN *win, short color, short index, const char *fname)
{
  TRACESTART();
  TRACEPRINT(("%s(%p,%i) -> not yet implemented\n", fname, win, color));
  TRACEEND();

  return -1;
}


short w_setForegroundColor(WWIN *win, short color)
{
  const char *cptr;
  short old;

  if ((cptr = _check_window(win))) {
    TRACESTART();
    TRACEPRINT(("w_setForegroundColor(%p,%i) -> %s\n", win, color, cptr));
    TRACEEND();
    return -1;
  }

  old = win->fg;
  if (old != color) {
    if (setColor(win, color, 1, "w_setForegroundColor") < 0) {
      return -1;
    }
    win->fg = color;
  }
  return old;
}


short w_setBackgroundColor(WWIN *win, short color)
{
  const char *cptr;
  short old;

  if ((cptr = _check_window(win))) {
    TRACESTART();
    TRACEPRINT(("w_setBackgroundColor(%p,%i) -> %s\n", win, color, cptr));
    TRACEEND();
    return -1;
  }

  old = win->bg;
  if (old != color) {
    if (setColor(win, color, 0, "w_setBackgroundColor") < 0) {
      return -1;
    }
    win->bg = color;
  }
  return old;
}

