/*
 * linux.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a graphic driver for the Linuxes with the framebuffer device
 *
 * x86-linux NOTES, ++eero:
 * - MAP_PRIVATE seems to throw bus error.  If it would work as I guess it
 *   should, we wouldn't need to be concerned about VT switches...
 * - mapping from the end of virtual fb screen seems to throw core too...
 *
 * CHANGES
 * ++eero 12/98:
 * - separated atari.c to mint.c and linux.c as x86-linux has now FBs too
 * - use FBIOPAN_DISPLAY ioctl() instead of the generic FBIOPUT_VSCREENINFO
 *   as with that vesafb doesn't seem to do panning.  Hopefully this still
 *   works with m68k-linux...
 * - fb width should be gotten from line_length as it might be different
 *   for xres(_virtual).
 * ++eero 4/03:
 * - moved input (kbd / mouse) to graph dir and added an exit function
 */

#ifndef linux
#error "this is a *linux* framebuffer graphics driver"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>

#include "../config.h"
#include "../types.h"
#include "../pakets.h"	/* for proto.h */
#include "../proto.h"
#include "gstructs.h"
#include "backend.h"


#define FB_DEV	"/dev/fb0"

static int fh;
static size_t vsize;
static volatile int sigVt;


static void sigvtswitch(int sig);


static void linuxVtSwitch(void)
{
  static ushort *origScreen = NULL;
  struct sigaction sa;

  /* this routine will be called at vt-switch by the main loop when all
   * graphics has been done, therefore we needn't any kind of semaphore
   * for the screen, just copy it...
   */

  switch(sigVt) {

    case SIGUSR1:
      if (!origScreen) {
	origScreen = theScreen->bm.data;
	if ((theScreen->bm.data = malloc(vsize))) {
	  memcpy(theScreen->bm.data, origScreen, vsize);
	  ioctl(0, VT_RELDISP, 1);
	}
      }
      break;

    case SIGUSR2:
      if (origScreen) {
	memcpy(origScreen, theScreen->bm.data, vsize);
	free(theScreen->bm.data);
	theScreen->bm.data = origScreen;
	origScreen = NULL;
	ioctl(0, VT_ACKACQ, 1);
      }
      break;

    default:
      fprintf(stderr, "unknown signal received\r\n");
      exit(-1);
  }

  /* ready for further signals */

  theScreen->vtswitch = NULL;
  sa.sa_handler = sigvtswitch;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);
}


static void sigvtswitch(int sig)
{
  struct sigaction sa;

  if ((sig == SIGUSR1) || (sig == SIGUSR2)) {

    /* ignore further signals until this one is served */

    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    sigVt = sig;
    theScreen->vtswitch = linuxVtSwitch;
  }
}


static int linux_screen_init(BITMAP *bm)
{
  int type, bits;
  struct fb_fix_screeninfo finfo;
  struct fb_var_screeninfo vinfo;
  struct vt_mode vt;
  void *addr;

  if ((fh = open(FB_DEV, O_RDWR)) < 0) {
    perror("open() " FB_DEV);
    return -1;
  }

  if (ioctl(fh, FBIOGET_FSCREENINFO, &finfo)) {
    perror("ioctl()" FB_DEV);
    return -1;
  }

  finfo.id[15] = 0;
  switch(finfo.type) {
    case FB_TYPE_PACKED_PIXELS:
      printf("%s: packed pixels (scan=%d)\r\n", finfo.id, finfo.line_length);
      type = BM_DIRECT8;
      break;
    case FB_TYPE_PLANES:
      printf("%s: planes\r\n", finfo.id);
      /* ATM no Amiga bitplanes support except for mono */
      type = BM_PACKEDMONO;
      break;
    case FB_TYPE_INTERLEAVED_PLANES:
      printf("%s: interleaved planes\r\n", finfo.id);
      type = BM_PACKEDCOLOR;
      break;
    default:
      fprintf(stderr, "%s: unknown type\r\n", finfo.id);
      return -1;
  }

  ioctl(fh, FBIOGET_VSCREENINFO, &vinfo);
  printf("using %ix%i of %ix%i FB pixels, %i bits per pixel\r\n",
	 vinfo.xres, vinfo.yres,
	 vinfo.xres_virtual, vinfo.yres_virtual,
	 vinfo.bits_per_pixel);

  if (vinfo.bits_per_pixel > 8) {
    fprintf(stderr, "fatal: W doesn't have gfx driver for truecolor framebuffer!\r\n");
    return -1;
  }
  if (vinfo.xres != finfo.line_length || vinfo.xres != vinfo.xres_virtual) {
    fprintf(stderr, "fatal: virtual screen width differs from physical\r\n");
    return -1;
  }

  bits = (type == BM_PACKEDMONO ? 1 : vinfo.bits_per_pixel);
  vsize = (vinfo.xres * bits >> 3) * vinfo.yres;

  addr = mmap(0, vsize, PROT_READ | PROT_WRITE, MAP_SHARED, fh, 0);
  if (addr < 0) {
    perror("mmap() " FB_DEV);
    return -1;
  }
  printf("%ik videoram mapped to 0x%p\r\n", vsize >> 10, addr);

  if (!glob_debug) {
    /* tesche: sigh, I often need to disable this code because if wserver crashes
     * under Linux68k then the screen is locked in graphic mode and I've got to
     * reboot my machine... :(
     *
     * in fact I had to change this so often that it is now a runtime-option.
     */

    /* set up signal handlers for vtswitch
     */
    signal(SIGUSR1, sigvtswitch);
    signal(SIGUSR2, sigvtswitch);

    /* disable vty switches
     */
    vt.mode = VT_PROCESS;
    vt.waitv = 0;
    vt.relsig = SIGUSR1;
    vt.acqsig = SIGUSR2;
    if (ioctl(0, VT_SETMODE, &vt)) {
      perror("ioctl(VT_SETMODE)");
      return -1;
    }

    /* disable screensaver */
    ioctl(0, KDSETMODE, KD_GRAPHICS);
  }

  sleep(2);

  bm->type = type;
  bm->width = vinfo.xres;
  bm->height = vinfo.yres;
  bm->planes = bits;
  bm->data = addr;

  vinfo.xoffset = 0;
  vinfo.yoffset = 0;
  ioctl(fh, FBIOPAN_DISPLAY, &vinfo);

  return 0;
}


#if defined(DIRECT8) || defined(PCOLOR) || defined(PCOLORMONO)

/*
 * set palette
 */
static short linuxPutCmap(COLORTABLE *colTab, short index)
{
  unsigned char *sr, *sg, *sb;
  unsigned short *dr, *dg, *db;
  /* static variables are always NULL at start */
  static struct fb_cmap cmap;
  int colors;

  /* sadly enough, linux uses 'ushort' as RGB values - allthough I heavily
   * doubt there's any need for this, but it forces us to copy the color table
   * to a different format. we use a static struct here because we assume that
   * all color tables will have the same number of colors - this may be legal
   * for a beginning, but may change.
   */
  if (!cmap.len) {
    colors = 1 << theScreen->bm.planes;
    cmap.red = malloc(colors * 3 * sizeof(ushort));
    if (!cmap.red) {
      return -1;
    }
    cmap.green = &cmap.red[colors];
    cmap.blue = &cmap.green[colors];
    cmap.transp = 0;
    cmap.start = 0;
  }

  colors = colTab->colors;
  cmap.len = colors;

  dr = cmap.red + colors;
  dg = cmap.green + colors;
  db = cmap.blue + colors;
  sr = colTab->red + colors;
  sg = colTab->green + colors;
  sb = colTab->blue + colors;

  while (--colors >= 0) {
    *--dr = (unsigned short)*--sr << 8;
    *--dg = (unsigned short)*--sg << 8;
    *--db = (unsigned short)*--sb << 8;
  }
  ioctl(fh, FBIOPUTCMAP, &cmap);

  return 0;
}

#endif


SCREEN *linux_init (int forceMono)
{
  BITMAP bm;

  if (unix_input_init(NULL, "/dev/mouse")) {
    return NULL;
  }

  if (linux_screen_init(&bm)) {
    fprintf(stderr, "fatal: unknown graphics or system is not supported!\r\n");
    return NULL;
  }

#if defined(PMONO) || defined(BMONO)
  if (bm.planes == 1) {
    if (bm.width & 31) {
      fprintf(stderr, "fatal: monochrome screen width not a multiple of 32 pixels!\r\n");
      return NULL;
    }
    bm.type = BM_PACKEDMONO;
    bm.upl = bm.width >> 5;
    bm.unitsize = 4;
#ifdef PMONO
    /* faster, so use this if possible (only for m68k) */
    packed_mono_screen.bm = bm;
    return &packed_mono_screen;
#else
    bmono_screen.bm = bm;
    return &bmono_screen;
#endif
  }
#endif

#ifdef DIRECT8
  if (bm.type == BM_DIRECT8) {
    if (bm.planes != 8) {
      fprintf(stderr, "fatal: DIRECT8 screen bit-depth != 8!\r\n");
      return NULL;
    }
    bm.upl = bm.width;
    bm.unitsize	= 1;

    direct8_screen.bm = bm;
    direct8_screen.changePalette = linuxPutCmap;
    return &direct8_screen;
  }
#endif

#if defined(PCOLOR) || defined(PCOLORMONO)
  if (bm.type == BM_PACKEDCOLOR) {
    /* warn in case somebody's compiling this on Amiga Linux... */
#warning Atari interleaved bitmap format support

    bm.unitsize = 2;
    bm.upl = (bm.width * bm.planes) >> 4;

#ifdef PCOLORMONO
    if (forceMono) {
      bm.type = BM_PACKEDCOLORMONO;
      packed_colormono_screen.bm = bm;
      packed_colormono_screen.changePalette = linuxPutCmap;
      return &packed_colormono_screen;
    }
#endif

#ifdef PCOLOR
    bm.type = BM_PACKEDCOLOR;
    packed_color_screen.bm = bm;
    packed_color_screen.changePalette = linuxPutCmap;
    return &packed_color_screen;
#endif
  }
#endif

  fprintf(stderr, "fatal: you didn't configure graphic driver for this mode?!\r\n");

  return NULL;
}

void linux_exit(void)
{
  ioctl(0, KDSETMODE, KD_TEXT);

  unix_input_exit();
}
