/*
 * font.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- font loading, setting and unloading
 *
 * CHANGES
 * 11/97 ++eero:
 * - new fontscheme where we'll request font by attributes instead of it's
 *   filename.
 * - font structure will be freed and unlinked regardless of what server
 *   returns in w_unloadfont().
 * - w_fonttype() which will convert fontname to corresponding attributes.
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

WFONT *w_loadfont(const char *family, short size, ushort stylemask)
{
  WFONT *fptr;
  LOADFONTP *paket;
  LFONTRETP *lfontretp;
  const char *checkname;
  short checksize;

  TRACESTART();


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
  while (fptr) {
    if (!strcmp(fptr->family, checkname) &&
        fptr->height == checksize && fptr->styles == stylemask) {

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
  if (!(fptr = calloc(1, sizeof(WFONT)))) {
    TRACEPRINT(("NULL (out of memory)\n"));
    TRACEEND();
    return NULL;
  }

  paket = _wreservep(sizeof(LOADFONTP));
  paket->type = htons(PAK_LOADFONT);
  paket->size = htons(size);
  paket->styles = htons(stylemask);

  if (family && *family) {
    strncpy(paket->family, family, sizeof(paket->family));
    paket->family[sizeof(paket->family)-1] = 0;
  } else {
    paket->family[0] = 0;
  }

  lfontretp = (LFONTRETP *)_wait4paket(PAK_LFONTRET);

  if ((fptr->handle = ntohs(lfontretp->handle)) < 0) {
    TRACEPRINT(("NULL\n"));
    TRACEEND();
    free(fptr);
    return NULL;
  }

  fptr->magic = MAGIC_F;

  /* checksize would be better as font existence is checked against
   * that above...
   */
  fptr->height = ntohs(lfontretp->height);
  fptr->flags  = ntohs(lfontretp->flags);
  fptr->styles = ntohs(lfontretp->styles);
  fptr->maxwidth = ntohs(lfontretp->maxwidth);
  fptr->baseline = ntohs(lfontretp->baseline);

  lfontretp->family[sizeof(lfontretp->family)-1] = '\0';
  fptr->family = strdup(lfontretp->family);

  memcpy(fptr->widths, lfontretp->widths, 256);

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
  UNLOADFONTP *paket;
  short ret;

  TRACESTART();
  TRACEPRINT(("w_unloadfont(%p)... ", font));

  if (!font) {
    TRACEPRINT(("zero pointer\n"));
    TRACEEND();
    return -1;
  }

  if (font->magic != MAGIC_F) {
    TRACEPRINT(("not a WFONT pointer\n"));
    TRACEEND();
    return -1;
  }

  /* first see if we really need to unload it
   */
  if (--font->used > 0) {
    /* no, it's still needed somewhere
     */
    TRACEPRINT(("0 (ok, still used)\n"));
    TRACEEND();

    return 0;
  }

  paket = _wreservep(sizeof(UNLOADFONTP));
  paket->type = htons(PAK_UNLOADFONT);
  paket->fonthandle = htons(font->handle);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);

  /* we should unlink this in any case ++eero */
  font->magic = 0;
  if (font->prev)
    font->prev->next = font->next;
  else
    WFonts = font->next;
  if (font->next)
    font->next->prev = font->prev;
  free(font->family);
  free(font);

  if (ret) {
    TRACEPRINT(("server unloading failed\n"));
  } else {
    TRACEPRINT(("%i\n", ret));
  }

  TRACEEND();
  return ret;
}


WFONT *w_setfont(WWIN *win, WFONT *font)
{
  const char *cptr;
  SFONTP *paket;
  WFONT *oldfont;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_setfont(%p,%p) -> %s\n", win, font, cptr));
    TRACEEND();
    return NULL;
  }

  if (!font || font->magic != MAGIC_F) {
    TRACEPRINT(("w_setfont(%p,%p) -> font error\n", win, font));
    TRACEEND();
    return NULL;
  }

  paket = _wreservep(sizeof(SFONTP));
  paket->type = htons(PAK_SFONT);
  paket->handle = htons(win->handle);

  oldfont = win->font;
  if (font) {
    paket->fonthandle = htons((win->font = font) -> handle);
  } else {
    paket->fonthandle = htons(-1);
  }

  TRACEPRINT(("w_setfont(%p,%p) -> %p\n", win, font, oldfont));
  TRACEEND();
  return oldfont;
}

