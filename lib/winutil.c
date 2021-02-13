/*
 * winutil.c, a part of the W Window System
 *
 * Copyright (C) 1996-1997 by Torsten Scherer and
 * Copyright (C) 1998 by Eero Tamminen
 * Copyright (C) 1998 by Jan Paul Schmidt
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- functions for window handle passing -> struct reconstruction
 * and window move / resize.
 *
 * CHANGES
 * ++eero, 2/98:
 * - added window construction from ID (win->handle has a value).  These
 *   windows can't be moved, resized, deleted nor receive events, just be
 *   inherited.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


ulong w_winID(WWIN *win)
{
	return win->handle;
}


WWIN *w_winFromID(ulong id)
{
	WWIN *win;

	if (!(win = calloc(1, sizeof(WWIN)))) {
		return NULL;
	}
	win->magic = MAGIC_W;
	win->type = WWIN_FAKE;
	win->handle = id;

	w_querywinsize(win, 0, &win->width, &win->height);
	return win;
}


short w_move(WWIN *win, short x0, short y0)
{
  const char *cptr;
  MOVEP *paket;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_move(%p,%i,%i) -> %s\n", win, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  if (win->type == WWIN_FAKE) {
    TRACEPRINT(("w_move(%p,%i,%i) -> fake window\n", win, x0, y0));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(MOVEP));
  paket->type   = htons(PAK_MOVE);
  paket->handle = htons(win->handle);

  paket->x0 = htons(x0);
  paket->y0 = htons(y0);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);

  TRACEPRINT(("w_move(%p,%i,%i) -> %i\n", win, x0, y0, ret));
  TRACEEND();
  return ret;
}


short w_resize (WWIN *win, short width, short height)
{
  const char *cptr;
  RESIZEP *paket;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_resize(%p,%i,%i) -> %s\n", win, width, height, cptr));
    TRACEEND();
    return -1;
  }

  if (win->type == WWIN_FAKE) {
    TRACEPRINT(("w_resize(%p,%i,%i) -> fake window\n", win, width, height));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(RESIZEP));
  paket->type   = htons(PAK_RESIZE);
  paket->handle = htons(win->handle);
  paket->width  = htons(width);
  paket->height = htons(height);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);
  if (!ret) {
    /* resize succeeded */
    win->width = width;
    win->height = height;
  }

  TRACEPRINT(("w_resize(%p,%i,%i) -> %i\n", win, width, height, ret));
  TRACEEND();
  return ret;
}

