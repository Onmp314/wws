/*
 * ants.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- WSaver module : 'Ant-walk'
 *
 * NOTES
 * If you want to know how different seed values behave, you can test them
 * out with either 'want' or (preferably) the W Toolkit test program wt_ant.
 *
 * CHANGES
 * ++eero, 6/98:
 * - use colors when number of shared colors is larger than sequence lenght.
 */

#include <Wlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include "wsaver.h"

#define BLK_SIZE	8	/* size of a square */
#define SEED		38	/* move sequence bits (=rllrrl) */
#define LENGHT		6	/* bits in the sequence */


void save_ants(void)
{
  static int cols, rows;	/* walking area for the ant */
  static uchar **grid;		/* 2-dim. array	*/

  int antx, anty, heading = 0;
  uchar value, prev;
  ulong move = 0;

  WEVENT *ev;

  /* screen size shouldn't change between calls, so we'll keep these
   * after they have been initialized
   */
  if (!grid) {

    cols = swidth / BLK_SIZE;
    rows = sheight / BLK_SIZE;

    if(!(grid = (uchar **)malloc(sizeof(uchar*) * cols))) {
      fprintf(stderr, "wsaver: malloc failed\n");
      w_exit();
      exit(-1);
    }

    /* create and initialize the grid array */
    for (antx = 0; antx < cols; antx++) {

      if(!(grid[antx] = (uchar *)calloc(1, sizeof(uchar) * rows))) {
	fprintf(stderr, "wsaver: calloc failed\n");
	w_exit();
	exit(-1);
      }
    }
  }

  antx = cols >> 1;				/* initial position	*/
  anty = rows >> 1;				/* at center		*/
  w_setmode(win, M_DRAW);

  for (;;) {
    prev = grid[antx][anty];			/* previous grid value	*/
    value = (prev + 1) % LENGHT;		/* new grid value	*/
    grid[antx][anty] = value;			/* store		*/

    /* display ant at (antx,anty) with 'color' according to value */
    if (scolors >= LENGHT) {
      w_setForegroundColor(win, scolors - 1 - value);
      w_pbox(win, antx*BLK_SIZE+1, anty*BLK_SIZE+1, BLK_SIZE-2, BLK_SIZE-2);
    } else {
      w_setpattern(win, MAX_GRAYSCALES * value / (LENGHT-1));
      w_dpbox(win, antx*BLK_SIZE+1, anty*BLK_SIZE+1, BLK_SIZE-2, BLK_SIZE-2);
    }

    /* turn according to seed sequence & (previous) grid value */
    if (SEED & (1 << prev)) {
      heading++;
    } else {
      heading--;
    }

    /* new ant position */
    switch (heading) {

      case 4:
	heading = 0;
      case 0:
	anty++;		/* down		*/
	break;
      case 1:
	antx--;		/* left		*/
	break;
      case 2:
	anty--;		/* up		*/
	break;
      case -1:
	heading = 3;
      case 3:
	antx++;		/* right	*/
	break;
    }

    /* check boundaries */
    if (antx >= cols) {
      antx = 0;
    }
    if (anty >= rows) {
      anty = 0;
    }
    if (antx < 0) {
      antx = cols - 1;
    }
    if (anty < 0) {
      anty = rows - 1;
    }

    /* flush buffer only every 16th move */
    if (!(++move & 15)) {
      if ((ev = w_queryevent(NULL, NULL, NULL, timeout))) {
	switch (ev->type) {

	  case EVENT_GADGET:
	    if (ev->key == GADGET_EXIT) {
	      w_exit();
	      exit(0);
	    }
	    break;

	  default:
	    return;
	}
      }
    }
  }
}
