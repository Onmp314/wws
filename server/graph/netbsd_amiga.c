/*
 * netbsd_amiga.c, a part of the W Window System
 *
 * Copyright (C) 01/96-06/96 Phx (Frank Wille) frank@phoenix.owl.de
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- init / exit code for NetBSD Amiga graphics devices
 *
 * CHANGES:
 * ++(02-Feb-96):
 * - included support for NetBSD-Amiga
 * - Adapted for W1R2PL3
 * ++(08-Feb-96):
 * - Adapted for W1R3a (bm.wpl -> bm.upl)
 * ++eero 12/98:
 * - changed code to look like other initialization code, uses now gstructs.h
 */


#ifndef __NetBSD__
#error "this is an (Amiga) *NetBSD* framebuffer graphics driver"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <amiga/dev/grfioctl.h>
#include <amiga/dev/iteioctl.h>
#include <amiga/dev/kbdmap.h>
#include <amiga/dev/kbdreg.h>

#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "gstructs.h"
#include "backend.h"

#define DEVICE "/dev/grf"
#define ROMSIZE 0x10000

/*
 * Support for all Amiga graphics devices (DIRECT8), except /dev/grf0 (=Amiga
 * Custom Chipset) and /dev/grf4 (=A2024 Monitor). Currently tested for
 * /dev/grf5 (Cybervision 64/Trio64) and /dev/grf6 (Merlin/Tseng-compat.) only.
 */
static int Supported[8] = { 0, 1, 1, 1, 0, 1, 1, 1 };


static int fh;

static int netbsd_amiga_screen_init(BITMAP *bm)
{
  size_t size;
  caddr_t addr;
  volatile int vmode = GRF_VMODE;
  struct grfvideo_mode gv;
  char grfdev[10];

  sprintf(grfdev, "%s%d", DEVICE, NETBSD_GRF);
  if (!Supported[NETBSD_GRF]) {
    fprintf(stderr, "%s not supported!\n", grfdev);
    return -1;
  }

  if ((fh = open(grfdev, O_RDWR)) < 0) {
    perror("open()");
    return -1;
  }

  /* Init graphics - set requested Video Mode */
  if (ioctl(fh, GRFSETVMODE, &vmode)) {
    perror("ioctl()");
    close(fh);
    return -1;
  }
  ioctl(fh, GRFIOCON);
  gv.mode_num = vmode;
  ioctl(fh, GRFGETVMODE, &gv);

  printf("%d x %d pixels, %d bits per pixel\n",
	 gv.disp_width, gv.disp_height, gv.depth);

  size = (size_t)(gv.disp_width * gv.disp_height * gv.depth) >> 3;
  if ((addr = mmap(0, size,
		   PROT_READ | PROT_WRITE, MAP_SHARED,
		   fh, ROMSIZE)) == (caddr_t) -1) {
    perror("mmap()");
    close(fh);
    return -1;
  }

  printf("%dK mapped to 0x%08x\n", size >> 10, (unsigned int)addr);

  bm->data	= (ushort *)addr;
  bm->width	= gv.disp_width;
  bm->height	= gv.disp_height;
  bm->upl	= gv.disp_width;
  bm->type	= BM_DIRECT8;
  bm->planes	= gv.depth;  /* must be 8 */
  bm->unitsize	= 1;

  return 0;
}


static short grfPutCmap(COLORTABLE *colTab, short index)
{
  struct grf_colormap cm;

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

  ioctl(fh, GRFIOCPUTCMAP, &cm);
  return 0;
}


SCREEN *netbsd_amiga_init(int forcemono)
{
  BITMAP bm;

  if (unix_input_init("/dev/kbd", "/dev/mouse") < 0) {
    return NULL;
  }
  
  if (netbsd_amiga_screen_init(&bm)) {
    fprintf(stderr, "fatal: graphics device opening failed!\r\n");
    return NULL;
  }
  direct8_screen.bm = bm;
  direct8_screen.changePalette = grfPutCmap;
  return &direct8_screen;
}

void netbsd_amiga_exit(void)
{
	unix_input_exit();
}

