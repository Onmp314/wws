/*
 * mint.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a graphic driver for the MiNT on Atari ST/TT
 *
 * CHANGES
 * ++eero 5/98:
 * - added support for BMONO and DIRECT8 drivers
 * ++eero 12/98:
 * - moved linux specific code to linux.c
 */

#ifndef __MINT__
#error "this is an *Atari/MiNT* graphics driver"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <osbind.h>
#include <linea.h>

#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "gstructs.h"
#include "backend.h"


static int mint_screen_init(BITMAP *bm)
{
  linea0();

  bm->width = V_X_MAX;
  bm->height = V_Y_MAX;
  bm->planes = VPLANES;

  bm->data = Logbase();

  if (bm->planes == 1) {
    printf("Mint monochrome, %i * %i pixel\r\n", bm->width, bm->height);
  } else {
    printf("Mint color, %i * %i pixel\r\n", bm->width, bm->height);
  }

  return 0;
}


#if defined(DIRECT8) || defined(PCOLOR) || defined(PCOLORMONO)

/* !!! this should be same as the define in ../color.c !!! */
#define TSTBIT(ptr,bit) (ptr[(bit)>>3] & 0x80>>((bit)&7))

/*
 * set palette (this should work at least for ST/STe)
 */
static short mintPutCmap(COLORTABLE *colTab, short index)
{
  uchar *used, *r, *g, *b;
  short rgb, colors;

  r = colTab->red;
  g = colTab->green;
  b = colTab->blue;

  if (index >= 0) {
    r += index;
    g += index;
    b += index;
    /* 3 MSB ST bits + 1 STe bit */
    rgb  = (short)((*r >> 5) | (*r & 8)) << 8;
    rgb |= (short)((*g >> 5) | (*g & 8)) << 4;
    rgb |= (short)((*b >> 5) | (*b & 8));
    Setcolor(index, rgb);
    return 0;
  }

  used = colTab->used;
  colors = colTab->colors;

  for (index = 0; index < colors; index++) {

    if (TSTBIT(used, index)) {

      rgb  = (short)((*r >> 5) | (*r & 8)) << 8;
      rgb |= (short)((*g >> 5) | (*g & 8)) << 4;
      rgb |= (short)((*b >> 5) | (*b & 8));
      Setcolor(index, rgb);
    }
    r++;  g++;  b++;
  }

  return 0;
}

#endif


/*
 * the real init function
 */

SCREEN *mint_init (int forceMono)
{
  BITMAP bm;

  if (unix_input_init(NULL, "/dev/mouse")) {
    return NULL;
  }

  if (mint_screen_init(&bm)) {
    fprintf(stderr, "fatal: unknown graphics or system is not supported!\r\n");
    return NULL;
  }

#if defined(PMONO) || defined(BMONO)
  if (bm.planes == 1) {
    if (bm.width & 31) {
      fprintf(stderr, "warning: monochrome screen width not a multiple of 32 pixels,\r\n");
      fprintf(stderr, "         can't use this graphic mode!\r\n");
    } else {
      bm.type = BM_PACKEDMONO;
      bm.unitsize = 4;
      bm.upl = bm.width >> 5;
#ifdef PMONO
      /* faster, so use this if possible */
      packed_mono_screen.bm = bm;
      return &packed_mono_screen;
#else
#warning On Atari PMONO driver is faster...
      bmono_screen.bm = bm;
      return &bmono_screen;
#endif
    }
  }
#endif

#ifdef DIRECT8
#warning DIRECT8 mode is not tested on Ataris
  bm.type	= BM_DIRECT8;
  bm.upl	= bm.width;
  bm.unitsize	= 1;
  bm.planes	= 8;
  direct8_screen.bm = bm;
  return &direct8_screen;
#endif

#if defined(PCOLOR) || defined(PCOLORMONO)
  bm.unitsize = 2;
  bm.upl = (bm.width * bm.planes) >> 4;

#ifdef PCOLORMONO
  if (forceMono) {
    bm.type = BM_PACKEDCOLORMONO;
    packed_colormono_screen.bm = bm;
    packed_colormono_screen.changePalette = mintPutCmap;
    return &packed_colormono_screen;
  }
#endif

#ifdef PCOLOR
  bm.type = BM_PACKEDCOLOR;
  packed_color_screen.bm = bm;
  packed_color_screen.changePalette = mintPutCmap;
  return &packed_color_screen;
#endif

#endif

  fprintf(stderr, "fatal: you didn't configure graphic driver for this mode?!\r\n");

  return NULL;
}

void mint_exit(void)
{
	unix_input_exit();
}
