/*
 * vhline.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- horizontal and vertical lines
 *
 * CHANGES
 * 1/98, ++eero:
 * - When line width > 1, map lines to pboxes.
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


/*
 * fast lines
 */

static inline short hvline(WWIN *win, short x0, short y0, short e,
			   short type, const char *fname)
{
  const char *cptr;
  HVLINEP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("%s(%p,%i,%i,%i) -> %s\n", fname, win, x0, y0, e, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(HVLINEP));
  paket->type = htons(type);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->e = htons(e);

  TRACEPRINT(("%s(%p,%i,%i,%i)\n", fname, win, x0, y0, e));
  TRACEEND();

  return 0;
}


short w_hline(WWIN *win, short x0, short y0, short xe)
{
  if (win && win->linewidth > 2) {
    short wd = xe - x0, th = win->linewidth >> 1;
    if (wd >= 0) {
      return w_pbox(win, x0, y0-th, wd+1, th+th+1);
    } else {
      return w_pbox(win, xe, y0-th, -wd+1, th+th+1);
    }
  } else {
    return hvline(win, x0, y0, xe, PAK_HLINE, "w_hline");
  }
}


short w_vline(WWIN *win, short x0, short y0, short ye)
{
  if (win && win->linewidth > 2) {
    short ht = ye - y0, th = win->linewidth >> 1;
    if (ht > 0) {
      return w_pbox(win, x0-th, y0, th+th+1, ht+1);
    } else {
      return w_pbox(win, x0-th, ye, th+th+1, -ht+1);
    }
  } else {
    return hvline(win, x0, y0, ye, PAK_VLINE, "w_vline");
  }
}


short w_dhline(WWIN *win, short x0, short y0, short xe)
{
  if (win && win->linewidth > 2) {
    short wd = xe - x0, th = win->linewidth >> 1;
    if (wd > 0) {
      return w_dpbox(win, x0, y0-th, wd+1, th+th+1);
    } else {
      return w_dpbox(win, xe, y0-th, -wd+1, th+th+1);
    }
  } else {
    return hvline(win, x0, y0, xe, PAK_DHLINE, "w_dhline");
  }
}


short w_dvline(WWIN *win, short x0, short y0, short ye)
{
  if (win && win->linewidth > 2) {
    short ht = ye - y0, th = win->linewidth >> 1;
    if (ht > 0) {
      return w_dpbox(win, x0-th, y0, th+th+1, ht+1);
    } else {
      return w_dpbox(win, x0-th, ye, th+th+1, -ht+1);
    }
  } else {
    return hvline(win, x0, y0, ye, PAK_DVLINE, "w_dvline");
  }
}

