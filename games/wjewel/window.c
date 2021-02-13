/*
 * window.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- wjewel GUI specific functions (bitmap, window and event handling)
 */

#include <sys/time.h>		/* gettimeofday() */
#include <unistd.h>
#include <string.h>		/* memset() */
#include <stdio.h>
#include <Wlib.h>		/* graphics stuff */
#include "jewel.h"		/* defines and prototypes */


#define PROPERTIES	(W_MOVE | W_TITLE | W_CLOSE | W_ICON | EV_KEYS | EV_ACTIVE)
#define ICON_PROPERTIES	(W_MOVE | EV_MOUSE)
#define NEXT_PROPERTIES	(W_MOVE)

/* local globals */
static WWIN *IconWin, *PlayWin, *NextWin, *Pieces;

/* the play area size in blocks and block size */
static int Border, Width, Height, BLKWidth, BLKHeight, Iconified, Mode;


/* local prototypes */
static int process_event(WEVENT *ev);
static void de_iconify(void);


void debugging_on(void)
{
  w_trace(1);
}

/* connects W server, initializes the windows
 * (play area, bitmap backup, scores, icon etc.)
 * returns zero for error.
 */
const char *init_win(int width, int height, int options, Blocks *blocks)
{
  WSERVER *wserver;
  BITMAP pieces;
  int idx;

  /* block / game area sizes */
  BLKWidth  = blocks->blk_w;
  BLKHeight = blocks->blk_h;

  /* account for the border blocks */
  if(options & OPT_BORDER)
    Border = 1;
  Width   = (width  += 2*Border);
  Height  = (height += Border);
  width   = Width  * BLKWidth;
  height  = Height * BLKHeight;

  if(!(wserver = w_init()))
    return "can't connect W server";

  if(width >= wserver->width || height >= wserver->height)
    return "window doesn't fit into screen";

  if(!(PlayWin = w_create(width, height, PROPERTIES)))
    return "unable to create game window";

  if(!(IconWin = w_create(BLKWidth, BLKHeight, ICON_PROPERTIES)))
    return "unable to create icon window";

  /* hidden bitmap window */
  if(!(Pieces = w_create(BLOCKS * BLKWidth, BLKHeight, 0)))
    return "unable to create a window";

  if(options & OPT_NEXT)
  {
    if(!(NextWin = w_create(BLKWidth, COLUMN * BLKHeight, NEXT_PROPERTIES)))
      return "unable to create next column window";
    w_settitle(NextWin, "NEXT");
    if(w_open(NextWin, UNDEF, UNDEF) < 0)
      return "unable to open next window";
  }

  /* transmit the bitmaps to the bitmap window */
  memset(&pieces, 0, sizeof(pieces));
  pieces.type    = BM_PACKEDMONO;
  pieces.width   = BLOCKS * BLKWidth;
  pieces.upl     = (BLOCKS * BLKWidth + 31) / 32;
  pieces.unitsize= 4;
  pieces.planes  = 1;
  pieces.height  = BLKHeight;
  pieces.data    = blocks->data;

  if(w_putblock(&pieces, Pieces, 0, 0) < 0)
    return "unable to transmit graphics";

  /* set icon */
  w_bitblk2(Pieces, BLK_JEWEL * BLKWidth, 0, BLKWidth, BLKHeight, IconWin, 0, 0);

  /* draw the borders into the game window
   * (note: show_block() offsets to play area)
   */
  if(Border)
  {
    for(idx = 0; idx < Height; idx++)
    {
      show_block(BLK_BORDER, -1, idx);
      show_block(BLK_BORDER, Width-2, idx);
    }
    for(idx = 0; idx < Width-2; idx++)
      show_block(BLK_BORDER, idx, Height-1);
  }

  /* during the game title will show the score */
  w_settitle(PlayWin, WJEWEL);

  if(w_open(PlayWin, UNDEF, UNDEF) < 0)
    return "unable to open game window";

  return NULL;
}

/* processes W events.
 * Returns a character if available, otherwise zero
 * Timeout is in milliseconds.
 */
int check_input(long timeout)
{
  WEVENT *ev;
  if(!(ev = w_queryevent(NULL, NULL, NULL, timeout)))
    return 0;

  return process_event(ev);
}

static int process_event(WEVENT *ev)
{
  switch(ev->type)
  {
    /* user pressed  a key */
    case EVENT_KEY:
      return (ev->key & 0xFF);

    /* mouse button down */
    case EVENT_MPRESS:
      de_iconify();
      return 0;

    /* pause when mouse leaves the window */
    case EVENT_INACTIVE:
      while(check_input(-1) != EVENT_ACTIVE);
      return 0;

    case EVENT_ACTIVE:
      return EVENT_ACTIVE;

    /* handle GUI gadgets */
    case EVENT_GADGET:
      switch(ev->key)
      {
	case GADGET_EXIT:
	case GADGET_CLOSE:
	  jewel_exit(0);

	case GADGET_ICON:
	  iconify();
	  return 0;
      }
  }
  return 0;
}

/* Closes the play window and shows an icon.  Waits until it's re-opened. */
void iconify(void)
{
  short x, y;

  w_querywindowpos(PlayWin, 1, &x, &y);
  w_close(PlayWin);
  if(NextWin)
    w_close(NextWin);
  w_open(IconWin, x, y);
  Iconified = 1;
  while(Iconified)
      process_event(w_queryevent(NULL, NULL, NULL, -1));
}

/* close icon and reopen window */
static void de_iconify(void)
{
  short x, y;

  w_querywindowpos(IconWin, 1, &x, &y);
  w_close(IconWin);
  w_open(PlayWin, x, y);
  if(NextWin)
  {
   /* find a suitable position... */
   x -= BLKWidth * 2;
   if(x < 0)
     x += BLKWidth * (Width + 3);
    w_open(NextWin, x, y); 
  }
  Iconified = 0;
}

/* draws the initial stuff to windows */
void draw_level(int level)
{
  /* levels have backgrounds with different intensities (0 = black) */
  w_setpattern(PlayWin, level);
  Mode = M_DRAW;
  w_setmode(PlayWin, Mode);
  /* account for the border blocks */
  w_dpbox(PlayWin,
    Border * BLKWidth, 0,
    (Width-2*Border) * BLKWidth, (Height-Border) * BLKHeight);
}

/* draws a block at (x,y) in the play area (offset to it!!!) */
void show_block(int type, int x, int y)
{
  x += Border;
  switch(type)
  {
    case BLK_BACK:
      if(Mode != M_DRAW)
      {
	Mode = M_DRAW;
	w_setmode(PlayWin, Mode);
      }
      /* background pattern is set when the level is first drawn */
      w_dpbox(PlayWin, x * BLKWidth, y * BLKHeight, BLKWidth, BLKHeight);
      break;
    case BLK_FLASH:
      if(Mode != M_CLEAR)
      {
	Mode = M_CLEAR;
	w_setmode(PlayWin, Mode);
      }
      /* clearing mode is set when the window is initialized */
      w_pbox(PlayWin, x * BLKWidth, y * BLKHeight, BLKWidth, BLKHeight);
      break;
    default:
      if(type < BLK_BACK)
        w_bitblk2(Pieces, type * BLKWidth, 0, BLKWidth, BLKHeight,
	PlayWin, x * BLKWidth, y * BLKHeight);
  }
}

/* copies a number of blocks from one position to another */
void copy_blocks(int fox, int foy, int tox, int toy, int blocks)
{
  fox += Border; tox += Border;
  w_bitblk(PlayWin, fox * BLKWidth, foy * BLKHeight,
	   BLKWidth, blocks * BLKHeight,
	   tox * BLKWidth, toy * BLKHeight);
}

/* show the next piece (composed of blocks a,b,c) on the 'next' window */
void show_next(int *column)
{
  int idx;

  if(!NextWin)	/* to be on the safe side... */
    return;

  for(idx = 0; idx < COLUMN; idx++)
    w_bitblk2(Pieces, column[idx] * BLKWidth, 0, BLKWidth, BLKHeight,
	      NextWin, 0, idx * BLKHeight);
}

/* shows the current game score */
void show_score(long score)
{
  char title[32];

  sprintf(title, " Score: %ld ", score);
  w_settitle(PlayWin, title);
}

/* msecs got to be > 0 for setting up next time frame checkpoint
 * and 0 for requesting time left before frame checkpoint.
 */
long time_frame(int msecs)
{
  static long checkout;
  long left;

  if (msecs)
  {
    checkout = w_gettime() + msecs;
    return msecs;
  }
  left = checkout - w_gettime();
  if (left > 0)
    return left;

  return 0;
}

