/*
 * client_block.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- these routines implement w_getblock and w_putblock.
 *
 * they are a bit more difficult as data comes in seperate chunks.  data is
 * expected to fit to current screen type, so error checking is a bit too
 * relaxed sometimes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"
#include "rect.h"

#define BLOCK_NONE 0
#define BLOCK_GET 1
#define BLOCK_PUT 2


/*****************************************************************************/

/*
 * client_putblock() and friends
 */

static void putblock (CLIENT *cptr)
{
  REC clip;
  short x1, y1, width, height;
  WINDOW *win = cptr->bwin;

  x1 = cptr->bx0;
  y1 = cptr->by0;
  width = cptr->bbm.width;
  height = cptr->bbm.height;

  clip.x0 = clip.y0 = 0;
  clip.x1 = (clip.w = cptr->bbm.width) - 1;
  clip.y1 = (clip.h = cptr->bbm.height) - 1;
  glob_clip0 = &clip;

  x1 += win->area[AREA_WORK].x0;
  y1 += win->area[AREA_WORK].y0;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect (win->pos.x0 + x1, win->pos.y0 + y1,
			   width, height)) {
      mouse_hide ();
    }
    glob_clip1 = &win->work;
    (*glob_screen->bitblk)(&cptr->bbm, 0, 0, width, height,
			   &glob_screen->bm, win->pos.x0 + x1, win->pos.y0 + y1);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip1 = &win->area[AREA_WORK];
    (*glob_screen->bitblk)(&cptr->bbm, 0, 0, width, height, &win->bitmap, x1, y1);
    if (win->is_open) {
      rectUpdateDirty (&win->dirty, x1, y1, width, height);
    }
  }
}


long client_putblockreq (CLIENT *cptr,
			 short width, short height,
			 ushort handle, short x0, short y0,
			 long shmKey)
{
  WINDOW *win;
  BITMAP *bm = &cptr->bbm;

  /* do some checks first - they might save us a lot of data transfer
   */
  if (!(win = (cptr->bwin = windowLookup (handle)))) {
    return -1;
  }
  if (win->flags & W_CONTAINER) {
    return -2;
  }
  if (cptr->btype || (width < 1) || (height < 1)) {
    return -3;
  }

  if(!(*glob_screen->createbm)(bm, width, height, 1)) {
    cptr->btype = BLOCK_NONE;
    return -4;
  }

  /* looks like we can do it
   */
  cptr->btype = BLOCK_PUT;
  cptr->bsize = height * bm->unitsize * bm->upl;
  cptr->bx0 = x0;
  cptr->by0 = y0;
  cptr->bptr = bm->data;

  return cptr->bsize;
}


void client_putblockdata(CLIENT *cptr, uchar *rawdata, short space)
{
  long count;

  if (cptr->btype != BLOCK_PUT) {
    return;
  }
  count = cptr->bsize;
  if (count > space) {
    count = space;
  }
  memcpy (cptr->bptr, rawdata, count);
  cptr->bptr += count;

  if ((cptr->bsize -= count) < 1) {
    putblock (cptr);
    free (cptr->bbm.data);
    cptr->bbm.data = NULL;
    cptr->btype = BLOCK_NONE;
  }
}


/*****************************************************************************/

/*
 * client_getblock() and friends
 */

static void getblock (BITMAP *bm,
		      WINDOW *win,
		      short x0, short y0,
		      short width, short height)
{
  REC clip;

  /* dummy clipping...
   */
  clip.x0 = clip.y0 = 0;
  clip.x1 = (clip.w = width) - 1;
  clip.y1 = (clip.h = height) - 1;
  glob_clip1 = &clip;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect (win->pos.x0 + x0, win->pos.y0 + y0,
			   width, height)) {
      mouse_hide ();
    }
    glob_clip0 = &glob_rootwindow->area[AREA_WORK];
    (*glob_screen->bitblk)(&glob_screen->bm, win->pos.x0 + x0,
			   win->pos.y0 + y0, width, height,
			   bm, 0, 0);
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*glob_screen->bitblk)(&win->bitmap, x0, y0, width, height, bm, 0, 0);
  }
}


long client_getblockreq (CLIENT *cptr, ushort handle,
			 short x0, short y0, short width, short height,
			 long shmKey)
{
  WINDOW *win;
  BITMAP *bm = &cptr->bbm;

  if (cptr->btype || (width < 0) || (height < 0) || (x0 < 0) || (y0 < 0)) {
    return -1;
  }
  if (!(win = windowLookup (handle))) {
    return -2;
  }
  if (win->flags & W_CONTAINER) {
    return -3;
  }
  if ((x0 + width > win->area[AREA_WORK].w) ||
      (y0 + height > win->area[AREA_WORK].h)) {
    return -4;
  }

  if(!(*glob_screen->createbm)(bm, width, height, 1)) {
    return -5;
  }

  getblock (bm, win, x0, y0, width, height);

  cptr->btype = BLOCK_GET;
  cptr->bsize = height * bm->unitsize * bm->upl;
  cptr->bptr = bm->data;

  return cptr->bsize;
}


short client_getblockdata(CLIENT *cptr, uchar *rawdata, short space)
{
  long count;

  if (cptr->btype != BLOCK_GET) {
    return 0;
  }

  count = cptr->bsize;
  if (count > space) {
    count = space;
  }

  memcpy(rawdata, cptr->bptr, count);
  cptr->bptr += count;

  if ((cptr->bsize -= count) < 1) {
    free (cptr->bbm.data);
    cptr->bbm.data = NULL;
    cptr->btype = BLOCK_NONE;
  }
  return count;
}
