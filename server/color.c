/*
 * color.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- functions for maintaining color tables
 *
 * CHANGES
 * ++eero, 5/98:
 * - removed from allocColor a check for whether there already was such a
 *   color and then using it.  User might want to change the other
 *   instance(s) later and making it change all of them would be a surprise.
 * - top level windows inherit colors now from glob_colortable instead
 *   of the background window. This will allow changing colors for the
 *   latter without it being inherited to others.
 * ++eero, 6/98:
 * - background window shared colors can't be changed.
 * - colTab->colors tells now the range of used colors for palette changing.
 * - calling colorFree with negative value frees all non-shared colors and
 *   returns how many of them were.
 *
 * NOTES:
 * - Program wanting to define new values also for color indeces inhabited
 *   by the shared server colors, must do w_freeColor() on colors handles
 *   from 'wserver->sharedcolors-1' downwards.
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


COLORTABLE *glob_colortable;	/* global (default/shared) colors */
int glob_sharedcolors;

static ulong *colorRefs;	/* how many times palette index used */
static short colorCount;	/* how many colors */


/*
 * Manipulate bitfield of used colors
 */

#define SETBIT(ptr,bit) (ptr[(bit)>>3] |= 0x80>>((bit)&7))
#define CLRBIT(ptr,bit) (ptr[(bit)>>3] &= ~(0x80>>((bit)&7)))
#define TSTBIT(ptr,bit) (ptr[(bit)>>3] & 0x80>>((bit)&7))

/*
 * This is fast loop for:
 *	if (TSTBIT(used, color)) {
 *	  colorRefs[i] += add;
 *	}
 * In case when very few or most of the entries are used.
 */
static void changeCounts(WINDOW *win, int add)
{
  int bit, color, top = win->colTab->colors;
  uchar *used = win->colTab->used;
  ulong *count = colorRefs;

  for (color = 0; color < top; color += 8, used++) {
    switch(*used) {

    case 0x00:	      /* none of the 8 colors used */
      count += 8;
      break;

    case 0xff:	      /* all of the 8 colors used */
      bit = 8;
      while(--bit >= 0) {
	*count += add;
	count++;
      }
      break;

    default:
      bit = 0x80;
      while(bit > 0) {
	if (bit & *used) {
	  *count += add;
	}
	bit >>= 1;
	count++;
      }
    }
  }
}

/*
 * needed for PACKEDCOLOR driver
 */

inline void colorGetMask (short color, ushort *mask)
{
  register short i = glob_screen->bm.planes;
  register ushort bit = 0x0001;

  while (--i >= 0) {
    if (color & bit) {
      *mask++ = 0xffff;
    } else {
      *mask++ = 0x0000;
    }
    bit <<= 1;
  }
}


/*
 * initialize a the basic color table.  the colors used are 0=white
 * (background) and 1=black (foreground).  others are set some exemplary
 * colors, don't expect them to be anything definitive until you have set
 * them yourself.
 */

short colorInit (void)
{
  short size;

  colorCount = 1 << glob_screen->bm.planes;

  if ((colorCount < 2) || (colorCount > 256)) {
    fprintf(stderr, "error: can't support %i-color screens\r\n", colorCount);
    return -1;
  }

  if (!(glob_colortable = malloc (sizeof (COLORTABLE)))) {
    fprintf(stderr, "error: not enough memory for system color table\r\n");
    return -1;
  }

  size = (colorCount+7) >> 3;
  /* bitfield of used colors */
  if (!(glob_colortable->used = calloc (1, size))) {
    fprintf(stderr, "error: not enough memory for system color table\r\n");
    free(glob_colortable);
    glob_colortable = NULL;
    return -1;
  }
 
  size = colorCount * sizeof (ulong);
  if (!(colorRefs = calloc (1, size))) {
    fprintf(stderr, "error: not enough memory for system color table\r\n");
    free(glob_colortable->used);
    free(glob_colortable);
    glob_colortable = NULL;
    return -1;
  }

  glob_colortable->red   = malloc (colorCount);
  glob_colortable->green = malloc (colorCount);
  glob_colortable->blue  = malloc (colorCount);
  if (!glob_colortable->red ||
      !glob_colortable->green ||
      !glob_colortable->blue) {
    fprintf(stderr, "error: not enough memory for system color table\r\n");
    return -1;
  }

  glob_colortable->win = NULL;

  /* only the first two colors are set/reserved as default */
  glob_colortable->red[FGCOL_INDEX]   = 0;
  glob_colortable->green[FGCOL_INDEX] = 0;
  glob_colortable->blue[FGCOL_INDEX]  = 0;

  glob_colortable->red[BGCOL_INDEX]   = 0xff;
  glob_colortable->green[BGCOL_INDEX] = 0xff;
  glob_colortable->blue[BGCOL_INDEX]  = 0xff;

  /* this will help us to use these colors only if absolutely necessary
   */
  SETBIT(glob_colortable->used, FGCOL_INDEX);
  SETBIT(glob_colortable->used, BGCOL_INDEX);
  colorRefs[FGCOL_INDEX] = 0xf0000000;
  colorRefs[BGCOL_INDEX] = 0xf0000000;

  /* this many colors used
   */
  glob_colortable->colors = 2;
  glob_sharedcolors = 2;

  return 0;
}

void colorInitShared(char *colorspec)
{
  long rgb;

  if (glob_sharedcolors >= colorCount) {
    return;
  }
  rgb = strtol(colorspec, NULL, 0);
  glob_colortable->red[glob_sharedcolors] = (rgb >> 16) & 0xff;
  glob_colortable->green[glob_sharedcolors] = (rgb >> 8) & 0xff;
  glob_colortable->blue[glob_sharedcolors] = rgb & 0xff;

  SETBIT(glob_colortable->used, glob_sharedcolors);

  /* try to avoid allocating to these colors */
  colorRefs[glob_sharedcolors++] += 0xf000;
  glob_colortable->colors++;
}


short colorSetColorTable(COLORTABLE *colTab)
{
  int ret;
  ret = (*glob_screen->changePalette)(colTab, -1);
  return ret;
}

/*
 * private helpers
 */

static void makePrivateTable (WINDOW *win)
{
  COLORTABLE *new;
  int bitbytes;

  if (!(new = calloc (1, sizeof (COLORTABLE)))) {
    wserver_exit(-1, "Out of memory in makePrivateTable()");
  }

  new->colors = win->colTab->colors;
  new->red = malloc (colorCount);
  new->green = malloc (colorCount);
  new->blue = malloc (colorCount);
  bitbytes = (colorCount+7) >> 3;
  new->used = malloc (bitbytes);

  if (!(new->red && new->green && new->blue && new->used)) {
    wserver_exit(-1, "Out of memory in makePrivateTable()");
  }

  /* inherit colors */
  memcpy(new->red, win->colTab->red, new->colors);
  memcpy(new->green, win->colTab->green, new->colors);
  memcpy(new->blue, win->colTab->blue, new->colors);
  memcpy(new->used, win->colTab->used, bitbytes);

  new->win = win;
  win->colTab = new;
}

/*
 * some window orientated functions
 */

/* colorCopyVirtualTable(): called when creating a new window. increments
 * global counters for color usage for those colors currently in use by
 * parent, but doesn't give the child a private color table yet. that's done
 * later when it turns out to be really necessary.
 */
short colorCopyVirtualTable (WINDOW *win, WINDOW *parent)
{
  if (parent == glob_backgroundwin) {
    /* this will allow background window palette modifications without
     * inheriting modifications to all other windows created afterwards...
     */
    win->colTab = glob_colortable;
  } else {
    win->colTab = parent->colTab;
  }
  changeCounts(win, 1);

  return 0;
}

short colorFreeTable (WINDOW *win)
{
  /* always decrement global counters
   */
  changeCounts(win, -1);

  /* if this was really our table, physically remove it
   */
  if (win->colTab->win == win) {
    COLORTABLE *colTab = win->colTab;

    free (colTab->used);
    free (colTab->blue);
    free (colTab->green);
    free (colTab->red);
    free (colTab);

    /* catch errors */
    win->colTab = NULL;
  }

  return 0;
}


short colorAllocColor (WINDOW *win, ushort red, ushort green, ushort blue)
{
  COLORTABLE *colTab = win->colTab;
  short i, index;
  uchar *used;
  ulong min;

  /* we search the least used color in the global table which is still
   * unused in our table.
   */
  index = -1;
  min = 0xffffffff;
  used = win->colTab->used;
  i = colorCount;
  while (--i >= 0) {
    if (!TSTBIT(used, i) && (colorRefs[i] < min)) {
      min = colorRefs[i];
      index = i;
    }
  }
  /* no colors left?
   */
  if (index < 0) {
    return -1;
  }

  /* do we've have to install a private colortable?
   */
  if (colTab->win != win) {
    makePrivateTable(win);
  }
  colTab = win->colTab;

  /* set color and increase counters
   */
  colTab->red[index] = red;
  colTab->green[index] = green;
  colTab->blue[index] = blue;
  SETBIT(colTab->used, index);
  colorRefs[index]++;

  /* set top */
  if (index >= colTab->colors) {
    colTab->colors = index + 1;
  }

  return index;
}

short colorFreeColor (WINDOW *win, short index)
{
  uchar *used;

  if (index < 0) {
    /* no inherited colors to free? */
    if (win->colTab->colors <= glob_sharedcolors)
      return 0;
  } else {
    /* not in range or used? */
    if (index >= colorCount || !TSTBIT(win->colTab->used, index))
      return -1;
  }

  /* colortable may still be virtual if this was called to free
   * shrared/inherited colors directly after creating a new window.
   */
  if (win->colTab->win != win) {
    makePrivateTable(win);
  }
  used = win->colTab->used;

  /* free all non-shared colors?
   */
  if (index < 0) {
    short freed = 0;

    index = win->colTab->colors;
    while (--index >= glob_sharedcolors) {
	if (TSTBIT(used, index)) {
	  CLRBIT(used, index);
	  freed++;
        }
    }
    win->colTab->colors = glob_sharedcolors;
    return freed;
  }

  /* free just the given index
   */
  CLRBIT(used, index);
  colorRefs[index]--;

  /* set top */
  if (index == win->colTab->colors - 1) {
    while (--index > 0) {
      if (TSTBIT(used, index))
	break;
    }
    win->colTab->colors = index + 1;
  }

  return 1;
}


short colorGetColor (WINDOW *win, short index,
		     ushort *red, ushort *green, ushort *blue)
{
  COLORTABLE *colTab = win->colTab;

  if ((index < 0) || (index >= colorCount)) {
    return -1;
  }
  if (!TSTBIT(colTab->used, index)) {
    return -1;
  }

  *red   = colTab->red[index];
  *green = colTab->green[index];
  *blue  = colTab->blue[index];
  return 0;
}


short colorChangeColor (WINDOW *win, short index,
			ushort red, ushort green, ushort blue)
{
  COLORTABLE *colTab;

  if ((index < 0) || (index >= colorCount)) {
    return -1;
  }
  if (win == glob_backgroundwin && index < glob_sharedcolors) {
    return -1;
  }
  if (!TSTBIT(win->colTab->used, index)) {
    return -1;
  }

  /* this can also be the first table modifying call
   */
  if (win->colTab->win != win) {
    makePrivateTable (win);
  }

  colTab = win->colTab;
  colTab->red[index] = red;
  colTab->green[index] = green;
  colTab->blue[index] = blue;

  if (win == glob_activewindow) {
    (*glob_screen->changePalette)(colTab, index);
  }
  return 0;
}


short colorSetFGColor (WINDOW *win, short index)
{
  if ((index < 0) || (index >= colorCount)) {
    return -1;
  }
  if (!TSTBIT(win->colTab->used, index)) {
    return -1;
  }
  colorGetMask(index, win->gc.fgColMask);
  win->gc.fgCol = index;
  return 0;
}


short colorSetBGColor (WINDOW *win, short index)
{
  if ((index < 0) || (index >= colorCount)) {
    return -1;
  }
  if (!TSTBIT(win->colTab->used, index)) {
    return -1;
  }
  colorGetMask(index, win->gc.bgColMask);
  win->gc.bgCol = index;
  return 0;
}
