/*
 * color.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- color allocation, changing and freeing
 *
 * Color changing functions should get a value from server indicating
 * whether the operation succeeded.  This might not be worth the effort
 * though (client should check return values from allocColor() and those
 * should then work fine).
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


short w_allocColor (WWIN *win, uchar red, uchar green, uchar blue)
{
  ALLOCCOLP *paket;
  const char *cptr;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_allocColor(%p,%i,%i,%i) -> %s\n",
		win, red, green, blue, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(ALLOCCOLP));
  paket->type   = htons(PAK_ALLOCCOL);
  paket->handle = htons(win->handle);

  paket->red   = htons(red);
  paket->green = htons(green);
  paket->blue  = htons(blue);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);
  if (ret >= 0) {
    win->colors++;
  }

  TRACEPRINT(("w_allocColor(%p,%i,%i,%i) -> %i\n",
	      win, red, green, blue, ret));
  TRACEEND();
  return ret;
}


short w_getColor (WWIN *win, short color, uchar *red, uchar *green, uchar *blue)
{
  SETFGCOLP *paket;
  COLRETP *retp;
  const char *cptr;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_allocColor(%p,%i,%p,%p,%p) -> %s\n",
		win, color, red, green, blue, cptr));
    TRACEEND();
    return -1;
  }

  if (!(red && green && blue)) {
    TRACEPRINT(("w_allocColor(%p,%i,%p,%p,%p) -> invalid argument(s)\n",
		win, color, red, green, blue));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(SETFGCOLP));
  paket->type = htons(PAK_GETCOL);
  paket->handle = htons(win->handle);
  paket->color = htons(color);

  retp = (COLRETP *)_wait4paket(PAK_SRET);
  ret = ntohs(retp->ret);
  if (ret) {
    TRACEPRINT(("w_allocColor(%p,%i,%i,%i,%i) -> %i\n",
		win, color, *red, *green, *blue, ret));
    TRACEEND();
    return ret;
  }

  *red = ntohs(retp->red);
  *green = ntohs(retp->green);
  *blue = ntohs(retp->blue);

  TRACEPRINT(("w_allocColor(%p,%i,%i,%i,%i) -> ok\n",
	      win, color, *red, *green, *blue));
  TRACEEND();
  return 0;
}


short w_freeColor (WWIN *win, short color)
{
  FREECOLP *paket;
  const char *cptr;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_freeColor(%p,%i) -> %s\n", win, color, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(FREECOLP));
  paket->type = htons(PAK_FREECOL);
  paket->handle = htons(win->handle);
  paket->color = htons(color);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);
  if (ret > 0) {
    /* adapt color counter */
    win->colors -= ret;
  }

  TRACEPRINT(("w_freeColor(%p,%i) -> %d\n", win, color, ret));
  TRACEEND();
  return ret;
}


short w_changeColor (WWIN *win, short color, uchar red, uchar green, uchar blue)
{
  CHANGECOLP *paket;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_changeColor(%p,%i,%i,%i,%i) -> %s\n",
		win, color, red, green, blue, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(CHANGECOLP));
  paket->type   = htons(PAK_CHANGECOL);
  paket->handle = htons(win->handle);

  paket->color = htons(color);
  paket->red   = htons(red);
  paket->green = htons(green);
  paket->blue  = htons(blue);

  TRACEPRINT(("w_changeColor(%p,%i,%i,%i,%i) -> ok\n",
	      win, color, red, green, blue));
  TRACEEND();
  return 0;
}


static inline short setColor(WWIN *win, short color, short type, const char *fname)
{
  SETFGCOLP *paket;

  TRACESTART();

  if (color < 0 || color >= (1 << _wserver.planes)) {
    TRACEPRINT(("%s(%p,%i) -> index out of range\n", fname, win, color));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(SETFGCOLP));
  paket->type = htons(type);
  paket->handle = htons(win->handle);
  paket->color = htons(color);

  TRACEPRINT(("%s(%p,%i) -> ok\n", fname, win, color));
  TRACEEND();
  return 0;
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
    if (setColor(win, color, PAK_SETFGCOL, "w_setForegroundColor") < 0) {
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
    if (setColor(win, color, PAK_SETBGCOL, "w_setBackgroundColor") < 0) {
      return -1;
    }
    win->bg = color;
  }
  return old;
}
