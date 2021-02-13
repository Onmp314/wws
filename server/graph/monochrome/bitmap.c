/*
 * bitmap.c, a part of the W Window System
 *
 * Copyright (C) 1997 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- allocates a bitmap suitable for driver functions
 *
 * As this should appear for clients as BM_PACKEDMONO,
 * bitmap lines are long aligned.
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "bmono.h"

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
  bm->type = BM_PACKEDMONO;
  bm->upl = (width + 31) >> 5;

  if (do_alloc) {
    if (!(bm->data = malloc (height * bm->unitsize * bm->upl)))
      return NULL;
  } else {
    bm->data = NULL;
  }

  return bm;
}
