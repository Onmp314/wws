/*
 * box.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- box drawing functions
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


static inline short box(WWIN *win, short x0, short y0, short width,
			short height, short type, const char *fname)
{
  const char *cptr;
  BOXP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%i,%i,%i,%i) -> %s\n",\
		fname, win, x0, y0, width, height, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(BOXP));
  paket->type = htons(type);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->width = htons(width);
  paket->height = htons(height);

  TRACEPRINT(("%s(%p,%i,%i,%i,%i)\n",\
	      fname, win, x0, y0, width, height));

  TRACEEND();
  return 0;
}


short w_box(WWIN *win, short x0, short y0, short width, short height)
{
  return box(win, x0, y0, width, height, PAK_BOX, "w_box");
}


short w_pbox(WWIN *win, short x0, short y0, short width, short height)
{
  return box(win, x0, y0, width, height, PAK_PBOX, "w_pbox");
}


short w_dbox(WWIN *win, short x0, short y0, short width, short height)
{
  return box(win, x0, y0, width, height, PAK_DBOX, "w_dbox");
}


short w_dpbox(WWIN *win, short x0, short y0, short width, short height)
{
  return box(win, x0, y0, width, height, PAK_DPBOX, "w_dpbox");
}
