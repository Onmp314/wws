/*
 * text.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- some functions dealing with text
 *
 * CHANGES:
 * ++TeSche 23.01.96:
 * - w_printstring() can now print strings of any size.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


short w_printchar(WWIN *win, short x0, short y0, short c)
{
  const char *cptr;
  PRINTCP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_printchar(%p,%i,%i,%i) -> %s\n", win, x0, y0, c, cptr));
    TRACEEND();
    return -1;
  }

  if (!win->font) {
    TRACEPRINT(("w_printchar(%p,%i,%i,%i) -> no font\n", win, x0, y0, c));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(PRINTCP));
  paket->type = htons(PAK_PRINTC);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);
  paket->c = htons(c);

  TRACEPRINT(("w_printchar(%p,%i,%i,%i) -> 0\n", win, x0, y0, c));
  TRACEEND();
  return 0;
}


short w_printstring(WWIN *win, short x0, short y0, const char *s)
{
  const char *cptr;
  PRINTSP *paket;
  WFONT *font;
  int len, todo;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_printstring(%p,%i,%i,\"%s\") -> %s\n",\
		win, x0, y0, s, cptr));
    TRACEEND();
    return -1;
  }

  if (!(font = win->font)) {
    TRACEPRINT(("w_printstring(%p,%i,%i,\"%s\") -> no font\n",\
		win, x0, y0, s));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_printstring(%p,%i,%i,\"%s\")...\n",\
	      win, x0, y0, s));

  len = strlen(s);
  cptr = s;

  while (len > 0) {
    todo = MIN(len, MAXPRINTS-1);

    paket = _wreservep(offsetof(PRINTSP, s) + ((todo+1 + 3) & ~3));
    paket->type = htons(PAK_PRINTS);
    paket->handle = htons(win->handle);
    paket->x0 = htons(x0);
    paket->y0 = htons(y0);

    strncpy(paket->s, cptr, todo);
    paket->s[todo] = 0;

    cptr += todo;
    len -= todo;
    if (len > 0) {
      x0 += w_strlen(font, paket->s);
    }
  }

  TRACEPRINT(("w_printstring() -> 0 (ok)\n"));
  TRACEEND();
  return 0;
}


int w_strlen(WFONT *font, const char *s)
{
  int len;
  const char *cptr = s;

  TRACESTART();

  if (!font) {
    TRACEPRINT(("w_strlen(%p,\"%s\") -> -1 (no font)\n", font, s));
    TRACEEND();
    return -1;
  }

  if (font->magic != MAGIC_F) {
    TRACEPRINT(("w_strlen(%p,\"%s\") -> -1 (not W font)\n", font, s));
    TRACEEND();
    return -1;
  }

  len = 0;
  while (*cptr) {
    len += font->widths[*cptr++ & 0xff];
  }

  TRACEPRINT(("w_strlen(%p,\"%s\") -> %i\n", font, s, len));
  TRACEEND();
  return len;
}
