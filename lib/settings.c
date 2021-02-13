/*
 * settings.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- change window (or server) settings
 *
 * CHANGES
 * ++eero, 1/98:
 * - added w_setpatterndata(), w_setlinewidth()
 * ++eero, 6/98:
 * - send setting to server only if it's value differs from the current one.
 */

#include <stdio.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


short w_settextstyle(WWIN *win, ushort flags)
{
  const char *cptr;
  STEXTSTYLEP *paket;
  ushort old;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_settextstyle(%p,%i) -> %s\n", win, flags, cptr));
    TRACEEND();
    return -1;
  }

  old = win->textstyle;
  if (old != flags) {
    paket = _wreservep(sizeof(STEXTSTYLEP));
    paket->type = htons(PAK_STEXTSTYLE);
    paket->handle = htons(win->handle);
    paket->flags = htons(win->textstyle = flags & F_STYLEMASK);
  }

  TRACEPRINT(("w_settextstyle(%p,%i)\n", win, flags));
  TRACEEND();
  return old;
}


short w_setsaver(short seconds)
{
  SSAVERP *paket;
  short ret;

  TRACESTART();

  paket = _wreservep(sizeof(SSAVERP));
  paket->type = htons(PAK_SSAVER);
  paket->seconds = htons(seconds);

  ret =  ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);

  TRACEPRINT(("w_setsaver(%i) -> %i\n", seconds, ret));
  TRACEEND();
  return ret;
}


ushort w_setpattern(WWIN *win, ushort pattern)
{
  const char *cptr;
  SPATTERNP *paket;
  ushort old;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_setpattern(%p,0x%04x) -> %s\n",
		win, (unsigned int)pattern, cptr));
    TRACEEND();
    return 0;
  }

  old = win->pattern;
  if (old != pattern) {
    paket = _wreservep(sizeof(SPATTERNP));
    paket->type = htons(PAK_SPATTERN);
    paket->handle = htons(win->handle);
    paket->pattern = htons(win->pattern = pattern);   /* signed vs. unsigned? */
  }

  TRACEPRINT(("w_setpattern(%p,0x%04x) -> 0x%04x\n",
	      win, (unsigned int)pattern, old));
  TRACEEND();
  return old;
}


ushort w_setpatterndata(WWIN *win, ushort *data)
{
  const char *cptr;
  SPATTERNDATAP *paket;
  ushort old;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_setpatterndata(%p,%p) -> %s\n", win, data, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(SPATTERNDATAP));
  paket->type = htons(PAK_SPATTERNDATA);
  paket->handle = htons(win->handle);
  memcpy(paket->data, data, sizeof(paket->data));

  old = win->pattern;
  win->pattern = W_USERPATTERN;

  TRACEPRINT(("w_setpatterndata(%p,%p) -> 0x%04x\n", win, data, old));
  TRACEEND();
  return old;
}


short w_setmode(WWIN *win, ushort mode)
{
  const char *cptr;
  SMODEP *paket;
  ushort old;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_setmode(%p,%i) -> %s\n", win, mode, cptr));
    TRACEEND();
    return -1;
  }

  old = win->drawmode;
  if (old != mode) {
    paket = _wreservep(sizeof(SMODEP));
    paket->type = htons(PAK_SMODE);
    paket->handle = htons(win->handle);
    paket->mode = htons(win->drawmode = mode);
  }

  TRACEPRINT(("w_setmode(%p,%i) -> %i\n", win, mode, old));
  TRACEEND();
  return old;
}


short w_setlinewidth(WWIN *win, short width)
{
  const char *cptr;
  SMODEP *paket;
  short old;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_setlinewidth(%p,%i) -> %s\n", win, width, cptr));
    TRACEEND();
    return -1;
  }

  if (width < 0) {
    TRACEPRINT(("w_setlinewidth(%p,%i) -> invalid value\n", win, width));
    TRACEEND();
    return win->linewidth;
  }

  old = win->linewidth;
  if (old != width) {
    paket = _wreservep(sizeof(SMODEP));
    paket->type = htons(PAK_SLINEWIDTH);
    paket->handle = htons(win->handle);
    paket->mode = htons(win->linewidth = width);
  }

  TRACEPRINT(("w_setlinewidth(%p,%i) -> %i\n", win, width, old));
  TRACEEND();
  return old;
}
