/*
 * text.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib text function mappings to Xlib
 *
 * ***NOTE***
 * - Only function changing *_Rootgc* font should be w_printstring()!!!
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


short w_printchar(WWIN *win, short x0, short y0, short c)
{
  char string[2];

  TRACESTART();
  string[0] = c & 0xff; string[1] = 0;
  return w_printstring(win, x0, y0, string);
  TRACEEND();
}


short w_printstring(WWIN *win, short x0, short y0, const char *s)
{
  static Font rootfont = 0;
  W2XWin *ptr = (W2XWin *)win;
  int len, width, height, base;
  XFontStruct *xfont;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
    goto error;

  if (!win->font)
  {
    cptr = "no font set";
    goto error;
  }

  len = strlen(s);
  xfont = ((W2XFont *)(win->font))->xfont;
  width = XTextWidth(xfont, s, len);
  height = win->font->height;
  base = y0 + xfont->ascent;

  if((cptr = _flush_area(ptr, x0, y0, x0 + width - 1, y0 + height - 1, 0)))
    goto error;

  if(rootfont != xfont->fid)
  {
    rootfont = xfont->fid;
    XSetFont(_Display, _Rootgc, rootfont);
  }
  XDrawImageString(_Display, ptr->pixmap, _Rootgc, x0, base, s, len);

  if (win->textstyle & F_UNDERLINE)
  {
    base++;
    XDrawLine(_Display, ptr->pixmap, _Rootgc, x0, base, x0 + width, base);
  }
  if (win->textstyle & F_REVERSE)
  {
    short old = w_setmode(win, M_INVERS);
    XFillRectangle(_Display, ptr->pixmap, ptr->gc, x0, y0, width, height);
    w_setmode(win, old);
  }


  TRACEPRINT(("w_printstring(%p,%i,%i,\"%s\") -> 0\n", win, x0, y0, s));
  TRACEEND();
  return 0;

error:
  TRACEPRINT(("w_printstring(%p,%i,%i,\"%s\") -> %s\n", win, x0, y0, s, cptr));
  TRACEEND();
  return -1;
}


int w_strlen(WFONT *font, const char *s)
{
  const char *cptr;
  int len;

  TRACESTART();

  if (!font)
  {
    cptr = "no font";
    goto error;
  }

  if(font->magic != MAGIC_F)
  {
    cptr = "font error";
    goto error;
  }

  len = XTextWidth(((W2XFont *)font)->xfont, s, strlen(s));

  TRACEPRINT(("w_strlen(%p,\"%s\") -> %i\n", font, s, len));
  TRACEEND();
  return len;

error:
  TRACEPRINT(("w_strlen(%p,\"%s\") -> -1 (%s)\n", font, s, cptr));
  TRACEEND();
  return -1;
}
