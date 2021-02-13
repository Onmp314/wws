/*
 * sun.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- init code for Sun Sparcstations, CG6 only so far
 */


#ifndef sun
#error "this is a *Sun* framebuffer graphics driver"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sun/fbio.h>
#include <sundev/cg6reg.h>
#include <sundev/kbio.h>

#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "gstructs.h"
#include "backend.h"


#define DEVICE "/dev/fb"
#define ROMSIZE 0x00040000


/*
 *
 */

static int fh;


static int sun_screen_init(BITMAP *bm)
{
  int size;
  caddr_t addr;
  struct fbtype fb;

  if ((fh = open(DEVICE, O_RDWR)) < 0) {
    perror("open()");
    return -1;
  }

  if (ioctl(fh, FBIOGTYPE, &fb)) {
    perror("ioctl()");
    return -1;
  }

  switch (fb.fb_type) {
    case FBTYPE_SUN4COLOR:
      printf("Sun: Memory color w/overlay\n");
      break;
    default:
      fprintf(stderr, "Sun: Unknown graphics device, aborting\n");
      close(fh);
      return -1;
  }

  printf("%i x %i pixels, %i bits per pixel\n",
	 fb.fb_width, fb.fb_height, fb.fb_depth);

  size = fb.fb_width * fb.fb_height;
  if ((addr = mmap(0, size,
		   PROT_READ | PROT_WRITE, MAP_SHARED,
		   fh, ROMSIZE)) == (caddr_t) -1) {
    perror("mmap()");
    close(fh);
    return -1;
  }

  printf("%iK mapped to 0x%08x\n", size >> 10, (unsigned int)addr);

  sleep(2);

  bm->data	= addr;
  bm->width	= fb.fb_width;
  bm->height	= fb.fb_height;
  bm->upl	= fb.fb_width;
  bm->type	= BM_DIRECT8;
  bm->planes	= 8;
  bm->unitsize	= 1;

  return 0;
}


static short sunPutCmap (COLORTABLE *colTab, short index)
{
  struct fbcmap cm;

  if (index < 0) {
    cm.index = 0;
    cm.count = colTab->colors;
    cm.red   = colTab->red;
    cm.green = colTab->green;
    cm.blue  = colTab->blue;
  } else {
    cm.index = index;
    cm.count = 1;
    cm.red   = &(colTab->red[index]);
    cm.green = &(colTab->green[index]);
    cm.blue  = &(colTab->blue[index]);
  }

  ioctl (fh, FBIOPUTCMAP, &cm);
  return 0;
}


SCREEN *sun_init (int mono)
{
  BITMAP bm;

	if (unix_input_init("/dev/kbd", "/dev/mouse") < 0) {
		return NULL;
	}

  if (sun_screen_init(&bm)) {
    fprintf(stderr, "fatal: graphics device opening failed!\r\n");
    return NULL;
  }
  direct8_screen.bm = bm;
  direct8_screen.changePalette = sunPutCmap;
  return &direct8_screen;
}


void sun_exit(void)
{
	unix_input_exit();
}
