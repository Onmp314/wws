/*
 * mouse.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Jan Paul Schmidt
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- mouse shape handling and mouse show / hide
 *
 * CHANGES
 * ++TeSche, 11/96:
 * - multiple mouse pointer support
 * ++jps, 3/98:
 * - new mouse shapes
 * ++eero, 04/03:
 * - SDL/GGI refresh hacks
 * - moved init code to respective backends
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"

/* global variables */
MOUSE glob_mouse;

#define mouseNormal \
"oo.............."\
"o*o............."\
"o**o............"\
"o***o..........."\
"o****o.........."\
"o*****o........."\
"o******o........"\
"o*******o......."\
"o********o......"\
"o*****ooo......."\
"o**o**o........."\
"o*o.o**o........"\
"oo..o**o........"\
".....o**o......."\
".....o**o......."\
"......ooo......."


#define mouseLeftRight \
"................"\
"................"\
"................"\
"....oo....oo...."\
"...o**o..o**o..."\
"..o**o....o**o.."\
".o**oooooooo**o."\
"o**************o"\
"o**************o"\
".o**oooooooo**o."\
"..o**o....o**o.."\
"...o**o..o**o..."\
"....oo....oo...."\
"................"\
"................"\
"................"


#define mouseUpDown \
".......oo......."\
"......o**o......"\
".....o****o....."\
"....o******o...."\
"...o**o**o**o..."\
"...o*oo**oo*o..."\
"....o.o**o.o...."\
"......o**o......"\
"......o**o......"\
"....o.o**o.o...."\
"...o*oo**oo*o..."\
"...o**o**o**o..."\
"....o******o...."\
".....o****o....."\
"......o**o......"\
".......oo......."


#define mouseUpperLeftLowerRight \
"ooooooooo......."\
"o*******o......."\
"o*******o......."\
"o****oooo......."\
"o*****o........."\
"o**o***o........"\
"o**oo***o..oooo."\
"o**o.o***o.o**o."\
"oooo..o***oo**o."\
".......o***o**o."\
"........o*****o."\
"......oooo****o."\
"......o*******o."\
"......o*******o."\
"......ooooooooo."\
"................"


#define mouseLowerLeftUpperRight \
".......ooooooooo"\
".......o*******o"\
".......o*******o"\
".......oooo****o"\
".........o*****o"\
"........o***o**o"\
".oooo..o***oo**o"\
".o**o.o***o.o**o"\
".o**oo***o..oooo"\
".o**o***o......."\
".o*****o........"\
".o****oooo......"\
".o*******o......"\
".o*******o......"\
".ooooooooo......"\
"................"

#define mouseMove \
"....ooo**o.oo..."\
"...o***oo*o**o.."\
"..o*oo*ooo*oo*o."\
"..o*ooo*ooo*oo*o"\
".o***ooo*ooo*oo*"\
"o*oo**ooo**oooo*"\
"o*oooo*oooooooo*"\
".o*oooo*oooooooo"\
".o***ooooooooooo"\
"o*oo**oooooooooo"\
"o*oooo*ooooooooo"\
".o*ooooooooooooo"\
"..o**ooooooooooo"\
"...oo***oooooooo"\
".....ooo***ooooo"\
"........oooooooo"

#define mouseBusy \
"....o*****o....."\
"....o*****o....."\
"....o*****o....."\
"...o**ooo**o...."\
"..o*ooo*ooo*o..."\
"..o*ooo*ooo*o..."\
".o*oooo*oooo*o.."\
".o*oooo***oo*o.."\
".o*ooooooooo*o.."\
"..o*ooooooo*o..."\
"..o*ooooooo*o..."\
"...o**ooo**o...."\
"....o*****o....."\
"....o*****o....."\
"....o*****o....."\
"....ooooooo....."

#define mouseUser \
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"\
"................"

#define MOUSE_POINTER_TYPES 8

static MOUSEPOINTER realPointer [MOUSE_POINTER_TYPES] = {
  {mouseNormal, 0, 0},
  {mouseUpDown, -8, -8},
  {mouseLeftRight, -8, -8},
  {mouseUpperLeftLowerRight, -8, -8},
  {mouseLowerLeftUpperRight, -8, -8},
  {mouseMove, -8, -8},
  {mouseBusy, -8, -8},
  {mouseUser, -8, -8}
};



static void decode (const char *pattern, ushort *mask, ushort *icon)
{
  short height = 16;

  while (--height >= 0) {

    ushort maskval = 0, iconval = 0, bit = 0x8000;

    while (bit) {

      switch (*pattern++) {

        case '*':
	  iconval |= bit;
	  /* fall through */

        case 'o':
	  maskval |= bit;
	  break;
      }

      bit >>= 1;
    }

    *mask++ = maskval;
    *icon++ = iconval;
  }
}


static inline MOUSEPOINTER *getPointerType (void)
{
  WINDOW *win;
  int x = glob_mouse.real.x0, y = glob_mouse.real.y0;
  short type = MOUSE_ARROW;

#ifdef REALTIME_MOVING
  if (glob_loopmove) {
    type = MOUSE_MOVE;
  }
  else
#endif
  if ((win = window_find (x, y, 1))) {
    if (intersect (win->work.x0, win->work.y0, win->work.w, win->work.h, 
                   x, y, 1, 1)) {
      type = win->mtype;
      if (type == MOUSE_USER) {
        memcpy (realPointer [type].mask, win->mdata->mask, 32);
        memcpy (realPointer [type].icon, win->mdata->icon, 32);
        realPointer [type].xDrawOffset = win->mdata->xDrawOffset;
        realPointer [type].yDrawOffset = win->mdata->yDrawOffset;
      }
    }
    else if (win->flags & W_RESIZE) {
      if (y - win->pos.y0 < 4) {
        type = MOUSE_UPDOWN;
      }

      if (x - win->pos.x0 < 4) {
        type += MOUSE_LEFTRIGHT;
      }

      if (win->pos.x1 - x < 4) {
        type += MOUSE_LEFTRIGHT + type;
      }

      if (win->pos.y1 - y < 4) {
        type += MOUSE_UPDOWN + (x - win->pos.x0 < 4);
      }
    }
  }

  return &realPointer [type];
}


/*
 * the exported functions
 */

/* SVGALIB already has a function named 'mouse_init', so I renamed this */
void wmouse_init (void)
{
  int i;

  /*  decode graphics
   */
  i = MOUSE_POINTER_TYPES;
  while (--i >= 0) {
    decode (realPointer[i].pattern, realPointer[i].mask, realPointer[i].icon);
  }
  glob_mouse.disabled = 0;
  glob_mouse.visible = 0;
}


/*
 * hardware specific routine will set 'drawn' = 'real' values
 */

void mouse_show (void)
{
  if (glob_mouse.visible || glob_mouse.disabled)
    return;

  glob_mouse.visible = 1;
  (*glob_screen->mouseShow) (getPointerType ());
#ifdef SCREEN_REFRESH
  screen_update(&(glob_mouse.drawn));
#endif
}


void mouse_hide (void)
{
  if (!glob_mouse.visible)
    return;

  glob_mouse.visible = 0;
  (*glob_screen->mouseHide) ();
#ifdef SCREEN_REFRESH
  screen_update(&(glob_mouse.drawn));
#endif
}


short mouse_rcintersect (short x0, short y0, short width, short height)
{
  if (!glob_mouse.visible)
    return 0;

  return intersect (glob_mouse.drawn.x0, glob_mouse.drawn.y0,
		    glob_mouse.drawn.w, glob_mouse.drawn.h,
		    x0, y0, width, height);
}
