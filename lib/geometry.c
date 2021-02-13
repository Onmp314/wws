/*
 * geometry.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- how to scan geometry values from program argument?
 *
 * 11/97 ++eero:
 * - added limit2screen function which was earlier included into
 *   couple of W apps.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Wlib.h"
#include "proto.h"


void scan_geometry(const char *geometry, short *col, short *lin,
		   short *xp, short *yp)
{
  TRACESTART();

  TRACEPRINT(("scan_geometry(%s,%p,%p,%p,%p)\n",\
	      geometry, col, lin, xp, yp));

  if (col)
    *col = UNDEF;
  if (lin)
    *lin = UNDEF;
  if (xp)
    *xp = UNDEF;
  if (yp)
    *yp = UNDEF;

  if (!geometry) {
    TRACEEND();
    return;
  }

  if (col)
    *col = atoi(geometry);

  if (!(geometry = strchr(geometry, ','))) {
    TRACEEND();
    return;
  }

  if (lin)
    *lin = atoi(++geometry);
  else
    geometry++;

  if (!(geometry = strchr(geometry, ','))) {
    TRACEEND();
    return;
  }

  if (xp) {
    *xp = atoi(++geometry);
    if (*geometry == '-') {
      *xp -= 1;
    }
  }
  else
    geometry++;

  if (!(geometry = strchr(geometry, ','))) {
    TRACEEND();
    return;
  }

  if (yp) {
    *yp = atoi(++geometry);
    if (*geometry == '-') {
      *yp -= 1;
    }
  }

  TRACEEND();
}

void limit2screen(WWIN *win, short *xp, short *yp)
{
  short ew, eh;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("limit2screen(%p,%p,%p) -> %s\n",\
		win, xp, yp, cptr));
    TRACEEND();
    return;
  }

  if (!win || !(xp || yp)) {
    TRACEPRINT(("limit2screen(%p,%p,%p) -> argument error\n",\
		win, xp, yp));
    TRACEEND();
    return;
  }

  w_querywinsize(win, 1, &ew, &eh);

  if (xp) {
    if (*xp != UNDEF && *xp < 0) {
      /* add 1 to account for subtracted 1 in scan_geometry */
      *xp = _wserver.width - ew + *xp + 1;
    } else {
      ew -= _wserver.width;
      if (ew <= 0 && ew + *xp > 0) {
	*xp = -ew;
      }
    }
  }
  if (yp) {
    if (*yp != UNDEF && *yp < 0) {
      *yp = _wserver.height - eh + *yp + 1;
    } else {
      eh -= _wserver.height;
      if (eh <= 0 && eh + *yp > 0) {
	*yp = -eh;
      }
    }
  }

  TRACEPRINT(("limit2screen(%p,%p,%p) -> (%d,%d)\n",\
	      win, xp, yp, *xp, *yp));

  TRACEEND();
}
