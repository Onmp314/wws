/*
 * bitblk.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- blit parts of a window to another place (on another window) 
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


short w_bitblk(WWIN *win, short x0, short y0, short width, short height,
	       short x1, short y1)
{
  const char *cptr;
  BITBLKP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_bitblk(%p,%i,%i,%i,%i,%i,%i) -> %s\n",\
		win, x0, y0, width, height, x1, y1, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(BITBLKP));
  paket->type = htons(PAK_BITBLK);
  paket->handle = htons(win->handle);

  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->width = htons(width);
  paket->height = htons(height);
  paket->x1 = htons(x1);
  paket->y1 = htons(y1);

  TRACEPRINT(("w_bitblk(%p,%i,%i,%i,%i,%i,%i)\n",\
	      win, x0, y0, width, height, x1, y1));

  TRACEEND();
  return 0;
}


short w_bitblk2(WWIN *swin, short x0, short y0, short width, short height,
		WWIN *dwin, short x1, short y1)
{
  const char *cptr;
  BITBLKP *paket;

  TRACESTART();

  if ((cptr = _check_window(swin))) {
    TRACEPRINT(("w_bitblk2(%p,%i,%i,%i,%i,%p,%i,%i) -> source: %s\n",\
		swin, x0, y0, width, height, dwin, x1, y1, cptr));
    TRACEEND();
    return -1;
  }

  if ((cptr = _check_window(dwin))) {
    TRACEPRINT(("w_bitblk2(%p,%i,%i,%i,%i,%p,%i,%i) -> dest: %s\n",\
		swin, x0, y0, width, height, dwin, x1, y1, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(BITBLKP));
  paket->type = htons(PAK_BITBLK2);
  paket->handle = htons(swin->handle);

  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->width = htons(width);
  paket->height = htons(height);
  paket->dhandle = htons(dwin->handle);
  paket->x1 = htons(x1);
  paket->y1 = htons(y1);

  TRACEPRINT(("w_bitblk2(%p,%i,%i,%i,%i,%p,%i,%i)\n",\
	      swin, x0, y0, width, height, dwin, x1, y1));
  TRACEEND();
  return 0;
}


short w_vscroll(WWIN *win, short x0, short y0,
		short width, short height, short y1)
{
  const char *cptr;
  VSCROLLP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_vscroll(%p,%i,%i,%i,%i,%i) -> %s\n",\
		win, x0, y0, width, height, y1, cptr));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_vscroll(%p,%i,%i,%i,%i,%i)\n",
	      win, x0, y0, width, height, y1));

  paket = _wreservep(sizeof(VSCROLLP));
  paket->type = htons(PAK_VSCROLL);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->width = htons(width);
  paket->height = htons(height);
  paket->y1 = htons(y1);

  TRACEEND();
  return 0;
}
