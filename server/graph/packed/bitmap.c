/*
 * bitmap.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- allocates a bitmap suitable for driver(s) functions
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "packed.h"

BITMAP *
FUNCTION(createbm)(bm, width, height, do_alloc)
	BITMAP *bm;
	short width;
	short height;
	short do_alloc;
{
  *bm = theScreen->bm;
  bm->width = width;
  bm->height = height;

#if defined(MONO)
  bm->upl = (width + 31) >> 5;
  bm->type = BM_PACKEDMONO;
#elif defined(COLORMONO)
  /* client thinks we're BM_PACKEDMONO */
  bm->upl = ((width + 31) & ~31) >> 4;
  bm->type = BM_PACKEDMONO;
  bm->planes = 1;
#elif defined(COLOR)
  bm->upl = (((width + 15) & ~15) * bm->planes) >> 4;
  bm->type = BM_PACKEDCOLOR;
#endif /* COLOR/MONO */

  if (do_alloc) {
    if (!(bm->data = malloc (height * bm->unitsize * bm->upl)))
      return NULL;
  } else {
    bm->data = NULL;
  }

  return bm;
}
