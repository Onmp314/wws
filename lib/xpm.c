/*
 * xpm.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- read image info, palette and data from XPM style strings and convert
 * them to a DIRECT8 BITMAP
 *
 * CHANGES
 * ++eero, 10/97:
 * - Generalized TeSche's w_xpm2bm() routine in w_cpyrgt.c a bit and
 *   moved it here.
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


/*
 * routine to copy a XPM pixmap like string array to a W BITMAP.
 */

BITMAP *w_xpm2bm(const char **xpm)
{
  int width, height, colors, bpc, twidth, idx;
  uchar *dptr, *ldptr;
  const char *line;
  char id[256];
  ulong color;
  BITMAP *bm;

  TRACESTART();
  if (!xpm) {
    TRACEPRINT(("w_xpm2bm(%p) -> invalid XPM\n", xpm));
    TRACEEND();
    return NULL;
  }

  sscanf(*xpm++, "%i %i %i %i", &width, &height, &colors, &bpc);

  if ((colors < 2) || (colors > 255) || (width < 1) || (height < 1)) {
    TRACEPRINT(("w_xpm2bm(%p) -> illegal XPM header\n", xpm));
    TRACEEND();
    return NULL;
  }
  if (!(bm = w_allocbm (width, height, BM_DIRECT8, colors))) {
    TRACEPRINT(("w_xpm2bm(%p) -> unable to allocate a bitmap\n", xpm));
    TRACEEND();
    return NULL;

    return NULL;
  }
  for(idx = 0; idx < colors; idx++, xpm++) {
    sscanf(*xpm, "%c c #%lx", &id[idx], &color);
    if (bm->palette) {
      bm->palette[idx].red   = (color >> 16) & 0xff;
      bm->palette[idx].green = (color >> 8) & 0xff;
      bm->palette[idx].blue  = color & 0xff;
    }
  }

  dptr = bm->data;

  while (--height >= 0) {
    line = *xpm++;
    ldptr = dptr;
    twidth = width;
    while (--twidth >= 0) {
      for(idx = 0; idx < colors && *line != id[idx]; idx++)
        ;
      *ldptr++ = idx;
      line++;
    }
    dptr += bm->upl;
  }

  TRACEPRINT(("w_xpm2bm(%p) -> %p\n", xpm, bm));
  TRACEEND();
  return bm;
}
