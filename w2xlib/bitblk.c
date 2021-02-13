/*
 * bitblk.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib bitmap blitting function mappings to Xlib
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


short w_bitblk(WWIN *win, short x0, short y0, short width, short height,
	       short x1, short y1)
{
  W2XWin *ptr = (W2XWin *)win;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
    goto error;

  if((cptr = _flush_area(ptr, x1, y1, x1 + width, y1 + height, 0)))
    goto error;

  XCopyArea(_Display, ptr->pixmap, ptr->pixmap, _Rootgc,
    x0, y0, width, height, x1, y1);

  TRACEPRINT(("w_bitblk(%p,%i,%i,%i,%i,%i,%i)\n",\
	      win, x0, y0, width, height, x1, y1));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("w_bitblk(%p,%i,%i,%i,%i,%i,%i) -> %s\n",\
    win, x0, y0, width, height, x1, y1, cptr));
  TRACEEND();
  return -1;
}


short w_bitblk2(WWIN *swin, short x0, short y0, short width, short height,
		WWIN *dwin, short x1, short y1)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(swin)))
    goto error;

  if ((cptr = _check_window(dwin)))
    goto error;

  if(!((W2XWin *)swin)->pixmap)
  {
    cptr = "container";
    goto error;
  }

  if((cptr = _flush_area((W2XWin *)dwin, x1, y1, x1 + width - 1, y1 + height - 1, 0)))
    goto error;

  XCopyArea(_Display, ((W2XWin *)swin)->pixmap, ((W2XWin *)dwin)->pixmap,
    _Rootgc, x0, y0, width, height, x1, y1);

  TRACEPRINT(("w_bitblk2(%p,%i,%i,%i,%i,%p,%i,%i)\n",\
	      swin, x0, y0, width, height, dwin, x1, y1));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("w_bitblk2(%p,%i,%i,%i,%i,%p,%i,%i) -> source: %s\n",\
    swin, x0, y0, width, height, dwin, x1, y1, cptr));
  TRACEEND();
  return -1;
}


short w_vscroll(WWIN *win, short x0, short y0,
		short width, short height, short y1)
{
  W2XWin *ptr = (W2XWin *)win;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
    goto error;

  if((cptr = _flush_area(ptr, x0, y1, x0 + width - 1, y1 + height - 1, 0)))
    goto error;

  XCopyArea(_Display, ptr->pixmap, ptr->pixmap, _Rootgc,
    x0, y0, width, height, x0, y1);

  TRACEPRINT(("w_vscroll(%p,%i,%i,%i,%i,%i)\n",
	      win, x0, y0, width, height, y1));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("w_vscroll(%p,%i,%i,%i,%i,%i) -> %s\n",\
    win, x0, y0, width, height, y1, cptr));
  TRACEEND();
  return -1;
}
