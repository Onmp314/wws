/*
 * wreversi, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- this is my old `wreversi' program I've written over a year ago, but
 * never really got it ready.  in fact it only served as a test platform for
 * the pcircle call.  the algorithm is surely not very clever, but then I
 * wrote it out of scratch without any further knowledge of other ways of
 * doing it, i.e. I did *not* look into some other program and copied that.
 * :)
 *
 * CHANGES
 * ++TeSche, 02/96:
 * - adapted for W1R3, but still needs an awful lot of fixes I guess
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Wlib.h>

#define	BWIDTH	8
#define	BHEIGHT	8

#define	WIDTH		(BWIDTH*20)
#define	HEIGHT		(BHEIGHT*20)
#define	MAXLOOSE	(- BWIDTH * BHEIGHT)

typedef	struct {
  short field[BWIDTH][BHEIGHT];
} BOARD;

char icon[] = ".o*";
short maxdepth;


/*
 * the graphic parts...
 */

static WWIN *glob_win;
static short glob_z1, glob_z2, glob_yoffset;
static WFONT *glob_font;


/*
 *
 */

static WWIN *openwindow(short width, short height)
{
  WWIN *ret;

  if (!w_init()) {
    return NULL;
  }

  if (!(glob_font = w_loadfont("fixed", 13, 0))) {
    return NULL;
  }

  glob_z1 = 1;
  glob_z2 = 3 + glob_font->height;
  glob_yoffset = 2 * (glob_font->height + 2);

  if (!(ret = w_create(width, height + glob_yoffset,
		       W_MOVE | W_TITLE | W_CLOSE | EV_MOUSE))) {
    return NULL;
  }

  w_settitle(ret, " wreversi ");
  w_setfont(ret, glob_font);

  if (w_open(ret, UNDEF, UNDEF) < 0) {
    w_delete(ret);
    return NULL;
  }

  return ret;
}


static void showboard(BOARD *bp)
{
  int x, y;

  for (y=0; y<BHEIGHT; y++) {
    for (x=0; x<BWIDTH; x++) {
      if (bp->field[x][y]) {
	switch (bp->field[x][y]) {
	  case 1:
	    w_setmode(glob_win, M_CLEAR);
	    break;
	  case 2:
	    w_setmode(glob_win, M_DRAW);
	  }
	w_pcircle(glob_win, x*20+10, glob_yoffset+y*20+10, 8);
      }
    }
  }
}


/*
 * the reversi functions...
 */

static long checked;
static char buf[80];

static void init(BOARD *bp)
{
  int i, j;

  for (i=0; i<BWIDTH; i++) {
    for (j=0; j<BHEIGHT; j++) {
      bp->field[i][j] = 0;
    }
  }

  bp->field[BWIDTH/2-1][BHEIGHT/2-1] = 1;
  bp->field[BWIDTH/2][BHEIGHT/2] = 1;
  bp->field[BWIDTH/2][BHEIGHT/2-1] = 2;
  bp->field[BWIDTH/2-1][BHEIGHT/2] = 2;
}


static short flipdir(BOARD *fp,
		     short party,
		     short x, short y,
		     short dx, short dy)
{
  int lx, ly, count, end;

  /* first check if this would give legal flips */

  lx = x;
  ly = y;
  end = 0;
  count = 0;

  while (!end) {

    lx += dx;
    ly += dy;

    if ((lx<0) || (lx>=BWIDTH) || (ly<0) || (ly>=BHEIGHT)) {
      end = 1;
      count = 0;
    } else if (fp->field[lx][ly] == 0) {
      end = 1;
      count = 0;
    } else if (fp->field[lx][ly] == party) {
      end = 1;
    } else {
      count++;
    }
  }

  /* then do the flips */

  if (count) {

    lx = x;
    ly = y;
    end = 0;

    while (!end) {

      lx += dx;
      ly += dy;

      if (fp->field[lx][ly] == party) {
	end = 1;
      } else {
	fp->field[lx][ly] = party;
      }
    }
  }

  return count;
}


static short flips(BOARD *bp, short party, short x, short y)
{
  short count;

  if (!(++checked % 2500)) {
    w_setmode(glob_win, M_CLEAR);
    w_pbox(glob_win, 0, 0, WIDTH, glob_yoffset);
    sprintf(buf, "%li checked...", checked);
    w_printstring(glob_win, 1, glob_z2, buf);
    w_flush();
  }

  count  = flipdir(bp, party, x, y, -1, -1);
  count += flipdir(bp, party, x, y, -1,  0);
  count += flipdir(bp, party, x, y, -1, +1);
  count += flipdir(bp, party, x, y,  0, -1);
  count += flipdir(bp, party, x, y,  0, +1);
  count += flipdir(bp, party, x, y, +1, -1);
  count += flipdir(bp, party, x, y, +1,  0);
  count += flipdir(bp, party, x, y, +1, +1);

  if (count) {
    bp->field[x][y] = party;
  }

  return count;
}


static short best(BOARD *bp, short party, short depth,
		  short *zugx, short *zugy)
{
  short	i, j, max, new, mx = 0, my = 0;
  BOARD	tmp;

  /* the termination condition */

  if (depth > maxdepth) {
    return 0;
  }

  max = MAXLOOSE;
  for (i=0; i<BWIDTH; i++) {
    for (j=0; j<BHEIGHT; j++) {
      if (!bp->field[i][j]) {
	tmp = *bp;
	tmp.field[i][j] = party;

	/* is this step legal? */
	if ((new = flips(&tmp, party, i, j)) > 0) {

	  if (depth & 1) {
	    new += best(&tmp, 3-party, depth+1, zugx, zugy);
	  } else {
	    new -= best(&tmp, 3-party, depth+1, zugx, zugy);
	  }

	  if (new > max) {
	    mx = i;
	    my = j;
	    max = new;
	  }
	}
      }
    }
  }

  if (max != MAXLOOSE) {
    bp->field[mx][my] = party;
    flips(bp, party, mx, my);
    *zugx = mx;
    *zugy = my;
  }

  return max;
}


/*
 *
 */

int main(int argc, char *argv[])
{
  BOARD board, tmp;
  short party, i, j, weiss, schwarz;
  short zugx, zugy;
  short flipped, user = 1;
  WEVENT *wevent;

  maxdepth = 1;
  if (argc == 2) {
    if ((maxdepth = atoi(argv[1])) < 1) {
      maxdepth = 1;
    }
  }

  if (!(glob_win = openwindow(WIDTH, HEIGHT))) {
    fprintf(stderr, "error: wreversi can't open a window\n");
    return -1;
  }

  w_dpbox(glob_win, 0, glob_yoffset, WIDTH, HEIGHT);
  w_setmode(glob_win, M_DRAW);
  for (j=0; j<HEIGHT; j+=20) {
    for (i=0; i<WIDTH; i+=20) {
      w_box(glob_win, i, glob_yoffset+j, 20, 20);
    }
  }

  init(&board);
  showboard(&board);

  party = 1;

  while (42) {

    /* just check if the next party could move at all */

    checked = 0;
    tmp = board;

    if (best(&tmp, party, 1, &zugx, &zugy) == MAXLOOSE) {

      w_setmode(glob_win, M_CLEAR);
      w_pbox(glob_win, 0, 0, WIDTH, glob_yoffset);

      sprintf(buf, "`%c' can't move!", icon[party]);
      w_printstring(glob_win, 1, glob_z1, buf);

      weiss = 0;
      schwarz = 0;
      for (i=0; i<BWIDTH; i++) {
	for (j=0; j<BHEIGHT; j++) {
	  if (board.field[i][j] == 1) {
	    weiss++;
	  } else if (board.field[i][j] == 2) {
	    schwarz++;
	  }
	}
      }
      sprintf(buf, "result: o=%i, *=%i", weiss, schwarz);
      w_printstring(glob_win, 1, glob_z2, buf);

w_flush();
sleep(5);

      w_queryevent(NULL, NULL, NULL, -1);

      w_close(glob_win);
      w_delete(glob_win);

      return -1;
    }

    if (party == user) {

      w_setmode(glob_win, M_CLEAR);
      w_pbox(glob_win, 0, 0, WIDTH, glob_yoffset);
      sprintf(buf, "it's your turn...");
      w_printstring(glob_win, 1, glob_z1, buf);

      while (w_queryevent(NULL, NULL, NULL, 0))
	;

      flipped = 0;
      do {
	if ((wevent = w_queryevent(NULL, NULL, NULL, -1))) {
	  switch (wevent->type) {
	    case EVENT_GADGET:
	      if ((wevent->key == GADGET_EXIT) ||
		  (wevent->key == GADGET_CLOSE)) {
		return -1;
	      }
	      break;
	    case EVENT_MPRESS:
	      if (wevent->y > glob_yoffset) {
		tmp = board;
		zugx = wevent->x / 20;
		zugy = (wevent->y - glob_yoffset) / 20;
		flipped = flips(&tmp, party, zugx, zugy);
	      }
	  }
	}
      } while (!flipped);

    } else {

      sprintf(buf, "it's my turn...");
      w_printstring(glob_win, 1, glob_z1, buf);
      w_flush();

      checked = 0;
      tmp = board;
      best(&tmp, party, 1, &zugx, &zugy);

      w_setmode(glob_win, M_CLEAR);
      w_pbox(glob_win, 0, 0, WIDTH, glob_yoffset);
      sprintf(buf, "%li checked...", checked);
      w_printstring(glob_win, 1, glob_z2, buf);
    }

    for (i=0; i<10; i++) {
      if (i & 1) {
	w_setmode(glob_win, M_DRAW);
      } else {
	w_setmode(glob_win, M_CLEAR);
      }
      w_pcircle(glob_win, zugx*20+10, glob_yoffset+zugy*20+10, 8);
      w_flush();
      usleep(50000);
    }

    board = tmp;
    showboard(&board);

    party = 3 - party;
  }
}
