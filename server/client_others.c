/*
 * client_others.c, a part of the W Window System
 *
 * Copyright (C) 1994-1998 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- client functions for (attribute) settings
 *
 *++eero, 1/98:
 * - moved all the setting (mode, color etc) functions from client_noret.c
 * to here as that file was getting a bit big. Added line width and
 * pattern data setting.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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


short client_status(CLIENT *cptr, CLIENT *clients, short index, STATUS *status)
{
  short i;
  CLIENT *ptr;

  if (index < 0) {

    i = 0;
    ptr = clients;
    while (ptr) {
      i++;
      ptr = ptr->next;
    }

    status->ip_addr = ntohl(0);
    status->pakets = ntohl(glob_pakets);
    status->bytes = ntohl(glob_bytes);
    status->openWin = ntohs(globOpenWindows);
    status->totalWin = ntohs(globTotalWindows);

    return i;
  }

  /* try to get a specific client */

  ptr = clients;
  while (ptr && (index-- > 0)) {
    ptr = ptr->next;
  }

  if (!ptr) {
    return -1;
  }

  status->ip_addr = ntohl(ptr->raddr);
  status->pakets = ntohl(ptr->pakets);
  status->bytes = ntohl(ptr->bytes);
  status->openWin = ntohs(ptr->openWindows);
  status->totalWin = ntohs(ptr->totalWindows);

  return 0;
}


void client_setmousepointer (CLIENT *cptr, ushort handle, short type,
                             short xoff, short yoff, ushort *mask, ushort *icon)
{
  WINDOW *win;

  if (!(win = windowLookup (handle))) {
    return;
  }

  mouse_hide ();

  if (win->mdata) {
    free(win->mdata);
    win->mtype = MOUSE_ARROW;
  }

  if (type == MOUSE_USER) {
    if (!(win->mdata = malloc(sizeof(WMOUSE)))) {
      return;
    }
    win->mdata->xDrawOffset = xoff;
    win->mdata->yDrawOffset = yoff;
    memcpy(win->mdata->mask, mask, sizeof(win->mdata->mask));
    memcpy(win->mdata->icon, icon, sizeof(win->mdata->icon));
  } else {
    win->mdata = NULL;
  }

  win->mtype = type;
  mouse_show ();
}


short client_getmousepointer (CLIENT *cptr, ushort handle)
{
  WINDOW *win;

  if (!(win = windowLookup (handle))) {
    return -1;
  }

  return win->mtype;
}


void client_settitle(CLIENT *cptr, ushort handle, const char *s)
{
  short x0, y0, w, h;
  WINDOW *win;

  if (!(win = windowLookup (handle))) {
    return;
  }
  if (!(win->flags & W_TITLE)) {
    return;
  }
  strncpy (win->title, s, MAXTITLE-1);
  win->title[MAXTITLE-1] = '\0';

  if (win->flags & W_CONTAINER) {
    return;
  }

  x0 = win->area[AREA_TITLE].x0;
  y0 = win->area[AREA_TITLE].y0;
  w = win->area[AREA_TITLE].w;
  h = win->area[AREA_TITLE].h;

  gc0 = glob_drawgc;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {

    x0 += win->pos.x0;
    y0 += win->pos.y0;
    if (mouse_rcintersect(x0, y0, w, h)) {
      mouse_hide();
    }
    glob_clip0 = &win->pos;
    (*glob_screen->dpbox)(&glob_screen->bm, x0, y0, w, h);
    (*glob_screen->prints)(&glob_screen->bm, x0, y0, s);
    win->is_dirty = 1;

  } else
#endif
  {
    glob_clip0 = &win->area[AREA_TITLE];
    (*glob_screen->dpbox)(&win->bitmap, x0, y0, w, h);
    (*glob_screen->prints)(&win->bitmap, x0, y0, s);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0, y0, w, h);
    }
  }

  win->has_title = 1;
}


void client_beep(CLIENT *cptr)
{
  fputc('\007', stdout);
  fflush(stdout);
}


void client_setmode(CLIENT *cptr, ushort handle, ushort mode)
{
  WINDOW *win;

  if ((win = windowLookup(handle))) {
    win->gc.drawmode = mode;
  }
}


void client_setlinewidth(CLIENT *cptr, ushort handle, short width)
{
  WINDOW *win;

  if ((win = windowLookup(handle))) {
    win->gc.linewidth = width;
  }
}


void client_setfont(CLIENT *cptr, ushort handle, short fonthandle)
{
  WINDOW *win;
  LIST *font;
  int idx;

  if ((fonthandle < 0) || (fonthandle >= MAXFONTS)) {
    return;
  }

  if (glob_font[fonthandle].numused && (win = windowLookup(handle))) {
    idx = -1;
    for (font = cptr->font; font; font = font->next) {
      if (font->handle == fonthandle) {
        win->gc.font = &glob_font[fonthandle];
	break;
      }
    }
    /* else client had not loaded that font */
  }
}


void client_settextstyle(CLIENT *cptr, ushort handle, ushort flags)
{
  WINDOW *win;

  if ((win = windowLookup(handle)))
    win->gc.textstyle = flags & F_STYLEMASK;
}


void client_setpatterndata(CLIENT *cptr, ushort handle, ushort *data)
{
  WINDOW *win;

  if ((win = windowLookup(handle))) {
    memcpy(win->patbuf, data, sizeof(win->patbuf));
  }
}


/* Manufacture 16x16 pattern according to the pattern id. */

void client_setpattern(CLIENT *cptr, ushort handle, ushort id)
{
  static ushort DM64[64] = {			/* courtesy of Kay... */
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
  };
  ushort patt, *patbuf;
  WINDOW *win;
  int i;

  if (!(win = windowLookup(handle)))
    return;

  patbuf = win->patbuf;

  /* grayscale patterns */
  if(id <= MAX_GRAYSCALES)
  {
    ushort *intensity;
    intensity = DM64;

    i = 8;
    while (--i >= 0) {
      patt = 0;
      /* matrix values [0-63] -> id 0=black, 64=white */
      if(id <= *intensity++) patt  = 0x0101;
      if(id <= *intensity++) patt |= 0x0202;
      if(id <= *intensity++) patt |= 0x0404;
      if(id <= *intensity++) patt |= 0x0808;
      if(id <= *intensity++) patt |= 0x1010;
      if(id <= *intensity++) patt |= 0x2020;
      if(id <= *intensity++) patt |= 0x4040;
      if(id <= *intensity++) patt |= 0x8080;
      patbuf[8] = patt;
      *patbuf++ = patt;
    }
    return;
  }

  /* line patterns
   *
   * pattern is a hatch (rolled both ways) if 1st upper byte bit is set
   */
  if(id >= 0xff)
  {
    memset(patbuf, 0, sizeof(win->patbuf));

    /* roll right */
    if (id & 0x8100) {
      i = 16;
      while (--i >= 0) {
	*patbuf++ |= id;
	id = ((id & 1) << 15) | (id >> 1);
      }
      patbuf -= 16;
    }

    /* roll left */
    if (id & 0x300) {
      i = 16;
      while (--i >= 0) {
	*patbuf++ |= id;
	id = ((id & 0x8000) >> 15) | (id << 1);
      }
    }

    return;
  }

  /* misc patterns (eg. images) */
  switch(id)
  {
    case W_PATTERN:
      memcpy(win->patbuf, BackGround, sizeof(win->patbuf));
      break;
    default:
      memcpy(win->patbuf, DefaultPattern, sizeof(win->patbuf));
  }
}


/*
 * some color functions
 */

short clientAllocColor(CLIENT *cptr, ushort handle,
		       ushort red, ushort green, ushort blue)
{
  WINDOW *win;

  if (!(win = windowLookup(handle)))
    return -1;

  return colorAllocColor (win, red, green, blue);
}


short clientFreeColor(CLIENT *cptr, ushort handle, short color)
{
  WINDOW *win;

  if (!(win = windowLookup (handle)))
    return -1;

  return colorFreeColor (win, color);
}


short clientGetColor(CLIENT *cptr, ushort handle, short color,
		     ushort *red, ushort *green, ushort *blue)
{
  WINDOW *win;

  if (!(win = windowLookup (handle)))
    return -1;

  return colorGetColor (win, color, red, green, blue);
}


short clientChangeColor(CLIENT *cptr, ushort handle, short color,
			ushort red, ushort green, ushort blue)
{
  WINDOW *win;

  if (!(win = windowLookup (handle)))
    return - 1;

  return colorChangeColor (win, color, red, green, blue);
}


short clientSetColor (CLIENT *cptr, ushort handle, short color)
{
  WINDOW *win;

  if (!(win = windowLookup (handle)))
    return - 1;

  switch (glob_pakettype) {
    case PAK_SETFGCOL:
      return colorSetFGColor (win, color);
    case PAK_SETBGCOL:
      return colorSetBGColor (win, color);
  }
  return -1;
}
