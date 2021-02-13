/*
 * init.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- top level function for graphics initialization
 */

#include <stdio.h>
#include <string.h>
#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "backend.h"
#include "generic/generic.h"

/*
 * exported global variables
 */

REC *glob_clip0, *glob_clip1;
REC glob_screen_clip;
GCONTEXT *gc0;

ushort BackGround[16] = {
  0xEEEE, 0xBBBB, 0xDDDD, 0x7777, 0xEEEE, 0xBBBB, 0xDDDD, 0x7777,
  0xEEEE, 0xBBBB, 0xDDDD, 0x7777, 0xEEEE, 0xBBBB, 0xDDDD, 0x7777
};
ushort DefaultPattern[16] = {
  0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555,
  0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555
};
/* other patterns? */

/* 
 * These are private to graphics library
 */

SCREEN *theScreen;	/* same as glob_screen */

int FirstPoint = 1;	/* whether line function should draw the first point */

/*
 * a small temporary bitmap + REC for miscanellous operations.
 */

#define TMP_W	64
#define TMP_H	100

REC clipTmp = { 0, 0, TMP_W, TMP_H, TMP_W - 1, TMP_H - 1, NULL};
BITMAP bitmapTmp;


/*
 *
 */

SCREEN *screen_init(int forcemono)
{
  static GCONTEXT gc = { M_DRAW, F_NORMAL, 1, NULL, BackGround,
			 FGCOL_INDEX, BGCOL_INDEX,
			 { 0xffff, 0xffff, 0xffff, 0xffff,
			   0xffff, 0xffff, 0xffff, 0xffff },
			 { 0x0, 0x0, 0x0, 0x0,
			   0x0, 0x0, 0x0, 0x0 }
		       };

#if defined(SDL)
  theScreen = sdl_init(forcemono);
#elif defined(GGI)
  theScreen = ggi_init(forcemono);
#elif defined(SVGALIB)
  theScreen = svgalib_init(forcemono);
#elif defined(linux)
  theScreen = linux_init(forcemono);
#elif defined(__MINT__)
# ifdef MAC
  theScreen = macmint_init(forcemono);
# else
  theScreen = mint_init(forcemono);
# endif
#elif defined(sun)
  theScreen = sun_init(forcemono);
#elif defined(__NetBSD__)
  theScreen = netbsd_amiga_init(forcemono);
#endif

  if (theScreen) {
    if (!(*theScreen->createbm)(&bitmapTmp, TMP_W, TMP_H, 1)) {
      return NULL;
    }

    /* clear the memory first in case dpbox writes only on plane */
    memset(theScreen->bm.data, 0,
	   theScreen->bm.upl * theScreen->bm.unitsize * theScreen->bm.height);

    glob_screen_clip.x0 = glob_screen_clip.y0 = 0;
    glob_screen_clip.x1 = (glob_screen_clip.w = theScreen->bm.width) - 1;
    glob_screen_clip.y1 = (glob_screen_clip.h = theScreen->bm.height) - 1;

    gc0 = &gc;
    glob_clip0 = &glob_screen_clip;
    generic_dpbox (&theScreen->bm, 0, 0, theScreen->bm.width, theScreen->bm.height);

#ifdef SCREEN_REFRESH
    screen_update(&glob_screen_clip);
#endif
  }

  /* return whatever we've got */
  return theScreen;
}


void screen_exit(void)
{
  #if defined(SDL)
  sdl_exit();
#elif defined(GGI)
  ggi_exit();
#elif defined(SVGALIB)
  svgalib_exit();
#elif defined(linux)
  linux_exit();
#elif defined(__MINT__)
# ifdef MAC
  macmint_exit();
# else
  mint_exit();
# endif
#elif defined(sun)
  sun_exit();
#elif defined(__NetBSD__)
  netbsd_amiga_exit();
#endif
}
