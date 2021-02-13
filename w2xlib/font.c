/*
 * font.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib font function mappings to Xlib
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Wlib.h"
#include "proto.h"


/*
 * the root for the fonts list
 */

static WFONT *WFonts = NULL;


/*
 *
 */
static short get_fontwidths(uchar *widths, XFontStruct *xfont)
{
  XCharStruct *chr;
  int idx, left, defwidth = 0;

  if(xfont->per_char)
  {
    /* xfont->all_chars_exist tells whether charset has all characters */

    /* check whether there's a default char... */
    if (xfont->default_char >= xfont->min_char_or_byte2 &&
        xfont->default_char <= xfont->max_char_or_byte2)
      defwidth = xfont->per_char[xfont->default_char].width;

    left = xfont->min_char_or_byte2;;
    if(left > 255)
    {
      fprintf(stderr, "warning: empty X charset\n");
      left = 255;
    }
    idx = left;
    while(--idx >= 0)
      *widths++ = defwidth;

    idx = xfont->max_char_or_byte2 - xfont->min_char_or_byte2;
    if (idx + left >= 256)
    {
      fprintf(stderr, "warning: X charset size exceeds 256\n");
      idx = 256 - left;
    }
    left = 256 - left - idx;

    chr = xfont->per_char;
    while(--idx >= 0)
    {
      *widths++ = chr->width;
      chr++;
    }

    while(--left >= 0)
      *widths++ = defwidth;
  }
  else
  {
    /* xfont->max_bounds.rbearing - min_bounds.lbearing; */
    memset(widths, xfont->max_bounds.width, 256);
  }
  return  xfont->max_bounds.width;
}


WFONT *w_loadfont(const char *family, short size, ushort stylemask)
{
  W2XFont *wxf;
  WFONT *fptr;
  const char *checkname;
  short checksize;

  /* W to X font name mapping variables */
  char *buf = NULL, slant = 'r', pxlsize[8] = "*";
  const char *weight = "medium";


  TRACESTART();

  /* first try to see if the font is already loaded */
  if(family && *family) {
    checkname = family;
  } else {
    checkname = _wserver.fname;
  }
  if (size) {
    checksize = size;
  } else {
    checksize = _wserver.fsize;
  }

  TRACEPRINT(("w_loadfont(%s,%d,%d)... ", checkname, checksize, stylemask));

  /* first try to see if the font is already loaded */

  fptr = WFonts;
  while (fptr)
  {
    if (!strcmp(fptr->family, checkname) &&
        ((W2XFont *)fptr)->req_size == checksize &&
	((W2XFont *)fptr)->req_styles == stylemask)
    {
      /* it is, so return this one */
      fptr->used++;
      TRACEPRINT(("0x%p (cached)\n", fptr));
      TRACEEND();
      return fptr;
    }
    fptr = fptr->next;
  }

  /* it isn't, so load it
   */
  if (!(wxf = (W2XFont*)calloc(sizeof(W2XFont), 1)))
  {
    TRACEPRINT(("NULL (out of memory)\n"));
    TRACEEND();
    return NULL;
  }
  fptr = (WFONT *)wxf;

  if (!(fptr->family = strdup(checkname)))
  {
    TRACEPRINT(("NULL (out of memory)\n"));
    TRACEEND();
    free(wxf);
    return NULL;
  }

  if (family || size || stylemask)
  {
    /* construct X font name */
    if (stylemask & F_BOLD)
      weight = "bold";
    if (stylemask & F_ITALIC)
      slant = 'o';
    if (checksize)
      sprintf(pxlsize, "%d", checksize);

    /* map W font families to X font families */
    if (!strcmp(family, "cour"))
      checkname = "courier";
    else if (!strcmp(family, "lucidat"))
      checkname = "lucidatypewriter";

    if (!(buf = malloc(strlen(checkname) + 50)))
    {
      free(fptr->family);
      free(wxf);

      TRACEPRINT(("NULL (out of memory)\n"));
      TRACEEND();
      return NULL;
    }
    /* use ISO-latin fonts, because otherwise you could get also Kanji etc. */
    sprintf(buf, "-*-%s-%s-%c-*-*-%s-*-*-*-*-*-iso8859-1",
            checkname, weight, slant, pxlsize);

    family = buf;
  }
  else
    family = _wserver.fname;	/* default X font */


  /* load the font */
  if(!(wxf->xfont = XLoadQueryFont(_Display, family)))
  {
    if(!(wxf->xfont = XLoadQueryFont(_Display, X_FONT)))
    {
      if (buf)
        free(buf);
      free(fptr->family);
      free(wxf);

      printf("Unable to load '" X_FONT "' font!\n");
      TRACEPRINT(("NULL\n"));
      TRACEEND();
      return NULL;
    }
  }
  if (buf)
    free(buf);

  wxf->req_styles = stylemask;
  wxf->req_size = size;

  fptr->magic = MAGIC_F;
  fptr->handle = 0;
  fptr->flags  = 0;
  fptr->styles = 0;
  fptr->baseline = wxf->xfont->ascent;
  fptr->height   = wxf->xfont->ascent + wxf->xfont->descent;
  fptr->maxwidth = get_fontwidths(fptr->widths, wxf->xfont);

  fptr->used = 1;
  fptr->prev = NULL;
  if ((fptr->next = WFonts))
    WFonts->prev = fptr;
  WFonts = fptr;

  TRACEPRINT(("w_loadfont(%s,%d,%d) -> %p\n", fptr->family, \
	     fptr->height, fptr->styles, fptr));
  TRACEEND();

  return fptr;
}


short w_unloadfont(WFONT *font)
{
  TRACESTART();
  TRACEPRINT(("w_unloadfont(%p)... ", font));

  if (!font)
  {
    TRACEPRINT(("zero pointer\n"));
    TRACEEND();
    return -1;
  }

  if (font->magic != MAGIC_F)
  {
    TRACEPRINT(("not a WFONT pointer (anymore?)\n"));
    TRACEEND();
    return -1;
  }

  /* first see if we really need to unload it
   */
  if (--font->used > 0)
  {
    /* no, it's still needed somewhere
     */
    TRACEPRINT(("0 (ok, still used)\n"));
    TRACEEND();

    return 0;
  }

  XUnloadFont(_Display, ((W2XFont *)font)->xfont->fid);

  font->magic = 0;
  if (font->prev)
    font->prev->next = font->next;
  else
    WFonts = font->next;
  if (font->next)
    font->next->prev = font->prev;
  free(font->family);
  free(font);

  TRACEPRINT(("0\n"));
  TRACEEND();

  return 0;
}


WFONT *w_setfont(WWIN *win, WFONT *font)
{
  const char *cptr;
  WFONT *oldfont;

  TRACESTART();

  if ((cptr = _check_window(win)))
    goto error;

  if (!font || font->magic != MAGIC_F)
  {
    cptr = "not a WFONT (anymore?)";
    goto error;
  }

  oldfont = win->font;
  win->font = font;
  
  TRACEEND();
  return oldfont;

error:
  TRACEPRINT(("w_setfont(%p,%p) -> %s\n", win, font, cptr));
  TRACEEND();
  return NULL;
}
