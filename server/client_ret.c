/*
 * client_ret.c, a part of the W Window System
 *
 * Copyright (C) 1994-1998 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- client functions that need to return something
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"
#include "rect.h"


/*
 * functions dealing with windows
 */

/* createBitmap():  calculate window gadget rects and create the bitmap.
 * called from window create and resize.
 */
static int createBitmap (WINDOW *win, short wwidth, short wheight,
			 short x, short y)
{
  short border = 0, ewidth = wwidth, eheight = wheight;
  int wx0 = 0, wy0 = 0, ax = 0, awidth = 0;
  int theight = 0;
  ushort flags = win->flags;
  BITMAP *bm;

  if (!(flags & W_NOBORDER)) {
    /* add the window border */
    ewidth += BORDERWIDTH << 1;
    eheight += BORDERWIDTH << 1;
    wx0 = wy0 = border = BORDERWIDTH;
  }

  if (flags & W_TITLE) {
    theight = glob_font[TITLEFONT].hdr.height;
    eheight += 4 + theight;
    wy0 += 4 + theight;
    ax = BORDERWIDTH;
    awidth = wwidth;
  }

  /* w->pos holds physical position and size of the whole window. since the
   * window is currently closed the position is defined to be 0,0. this will be
   * set to the real physical values in window_open() later on.
   */
  win->pos.x0 = x;
  win->pos.y0 = y;
  win->pos.x1 = x + (win->pos.w = ewidth) - 1;
  win->pos.y1 = y + (win->pos.h = eheight) - 1;

  /* win->area[AREA_WORK] holds position and size of the working area of the
   * window relative to the whole window. win->work holds the same in physical
   * coordinates (also set by window_open()).
   */
  win->area[AREA_WORK].x1 = (win->area[AREA_WORK].x0 = wx0) + (win->area[AREA_WORK].w = wwidth) - 1;
  win->area[AREA_WORK].y1 = (win->area[AREA_WORK].y0 = wy0) + (win->area[AREA_WORK].h = wheight) - 1;
  win->work = win->area[AREA_WORK];
  win->work.x0 += win->pos.x0;
  win->work.y0 += win->pos.y0;
  win->work.x1 += win->pos.x0;
  win->work.y1 += win->pos.y0;

  /* now set up the other areas, if there're any - otherwise the fields
   * will remain set to zero
   */
  if (flags & W_CLOSE) {
    win->area[AREA_CLOSE].x0 = ax;
    win->area[AREA_CLOSE].y0 = BORDERWIDTH;
    win->area[AREA_CLOSE].w = MIN(theight, awidth);
    win->area[AREA_CLOSE].h = theight;
    win->area[AREA_CLOSE].x1 = win->area[AREA_CLOSE].x0 + win->area[AREA_CLOSE].w - 1;
    win->area[AREA_CLOSE].y1 = win->area[AREA_CLOSE].y0 + theight - 1;
    ax += win->area[AREA_CLOSE].w + BORDERWIDTH;
    awidth -= win->area[AREA_CLOSE].w + BORDERWIDTH;
  }

  if (flags & W_ICON) {
    win->area[AREA_ICON].x0 = ax;
    win->area[AREA_ICON].y0 = BORDERWIDTH;
    win->area[AREA_ICON].w = MIN(theight, awidth);
    win->area[AREA_ICON].h = theight;
    win->area[AREA_ICON].x1 = win->area[AREA_ICON].x0 + win->area[AREA_ICON].w - 1;
    win->area[AREA_ICON].y1 = win->area[AREA_ICON].y0 + theight - 1;
    ax += win->area[AREA_ICON].w + BORDERWIDTH;
    awidth -= win->area[AREA_ICON].w + BORDERWIDTH;
  }

  if (flags & W_TITLE) {
    win->area[AREA_TITLE].x0 = ax;
    win->area[AREA_TITLE].y0 = BORDERWIDTH;
    win->area[AREA_TITLE].w = awidth;
    win->area[AREA_TITLE].h = theight;
    win->area[AREA_TITLE].x1 = win->area[AREA_TITLE].x0 + awidth - 1;
    win->area[AREA_TITLE].y1 = win->area[AREA_TITLE].y0 + theight - 1;
  }

  /* set up the window bitmap, aligning the width to what seems appropriate
   * and allocing data if !W_CONTAINER.
   */
  if (!(bm = (*glob_screen->createbm)(&win->bitmap,
	      ewidth, eheight, !(flags & W_CONTAINER)))) {
    return -1;
  }
  if (bm->data) {
      memset (bm->data, 0, bm->unitsize * bm->upl * eheight);
  }

  win->has_frame = 0;
  win->has_title = 0;

  return 0;
}


/* createWindow(): create a window
 */
static long createWindow (CLIENT *cptr, short wwidth, short wheight, ushort flags,
			  WINDOW *parent, WWIN *libPtr)
{
  WINDOW *win;

  flags &= W_FLAGMASK;

  if (flags & W_TOP) {
    /* a W_TOP window (wcpyrgt, wgone & wsaver) is a dangerous thing as it can
     * lock up the screen (concerning wgone & wsaver this is exactly what it's
     * good for), so we will only allow it for local connections and if the
     * user who is requesting it is the same who started us.
     *
     * a request for a W_TOP window may still fail later on because due to
     * the same security reasons we allow only one such window to exist at a
     * time.
     */
    if (cptr->raddr || (cptr->uid != glob_uid)) {
      return 0;
    }
  }

  if (flags & W_TITLE) {
    flags |= W_CLOSE;   /* W_CLOSE is a `must' */
  } else {
    /* a window without title can't have gadgets */
    flags &= ~(W_CLOSE | W_ICON);
  }

  if (flags & W_NOBORDER) {
    /* a window without border can't have anything */
    flags &= ~(W_CLOSE | W_ICON | W_RESIZE | W_TITLE);
  }

  /* create the window (and initialize data structure to zero)
   */
  if (!(win = window_create (parent, flags)))
    return 0;

  if (createBitmap (win, wwidth, wheight, 0, 0)) {
    window_delete (win);
    return 0;
  }

  set_defaultgc(win);

  /* final set-ups
   */
  win->cptr = cptr;
  win->libPtr = libPtr;
  colorCopyVirtualTable (win, parent);   /* inherit color table from parent */

  cptr->totalWindows++;
  globTotalWindows++;

  win->mtype = MOUSE_ARROW;
  win->mdata = NULL;

  return win->id;
}


long client_create(CLIENT *cptr, short width, short height,
		   ushort flags, WWIN *libPtr)
{
  /* 'glob_backgroundwin' and the 'W_TOP' window are children of
   * 'glob_rootwindow', rest of the windows are children of
   * 'glob_backgroundwin'.
   */
  return createWindow (cptr, width, height, flags,
		       (flags & W_TOP) ? glob_rootwindow : glob_backgroundwin,
		       libPtr);
}


long client_create2(CLIENT *cptr, short width, short height,
		    ushort flags, ushort handle, WWIN *libPtr)
{
  WINDOW *parent;

  if (handle < 2)
    /* you cannot append to the root or background window this way */
    return 0;

  if (!(parent = windowLookup(handle)))
    return 0;

  return createWindow (cptr, width, height, flags, parent, libPtr);
}


short client_open (CLIENT *cptr, ushort handle, short x0, short y0)
{
  WINDOW *win, *parent;
  short px0, py0;

  if (!(win = windowLookup(handle)))
    return -9999;

  if (win->is_open)
    return 1;

  /* draw icons/title of window if it doesn't yet have one (if !W_CONTAINER)
   */
  if (!(win->flags & W_CONTAINER)) {
    REC cl;

    gc0 = glob_drawgc;

    /*
     * cannot use win->pos as clip rectangle any longer, because
     * win->pos.{x0,y0} may be != 0 now.
     */
    cl.x0 = cl.y0 = 0;
    cl.x1 = (cl.w = win->pos.w) - 1;
    cl.y1 = (cl.h = win->pos.h) - 1;
    glob_clip0 = &cl;

    if (!win->has_frame) {

      win->has_frame = 1;

      if (win->flags & W_CLOSE) {

	(*glob_screen->line)(&win->bitmap,
			     win->area[AREA_CLOSE].x0,
			     win->area[AREA_CLOSE].y0,
			     win->area[AREA_CLOSE].x1,
			     win->area[AREA_CLOSE].y1);
	(*glob_screen->line)(&win->bitmap,
			     win->area[AREA_CLOSE].x0,
			     win->area[AREA_CLOSE].y1,
			     win->area[AREA_CLOSE].x1,
			     win->area[AREA_CLOSE].y0);
      }

      if (win->flags & W_ICON) {

	(*glob_screen->pbox)(&win->bitmap,
			     win->area[AREA_ICON].x0 + (win->area[AREA_ICON].w >> 1) - 1,
			     win->area[AREA_ICON].y0 + (win->area[AREA_ICON].h >> 1) - 1,
			     2, 2);
      }
    }

    if (!win->has_title && (win->flags & W_TITLE))
      client_settitle (NULL, win->id, win->title);
  }

  /* positions are parent work area relative
   */
  parent = win->parent;
  px0 = parent->work.x0;
  py0 = parent->work.y0;

  /* huh, w_open(-1,-1) doesn`t work anymore, because windows can very well
   * be at position (-1,-1). kay.
   * well, so then we use something different, eh? TeSche
   */
  if ((x0 == UNDEF) || (y0 == UNDEF)) {
    if (get_rectangle(win->pos.w, win->pos.h,
		      &x0, &y0, BUTTON_LEFT | BUTTON_RIGHT, BUTTON_NONE))
      return -1;
  } else {
    /* absolute coordinates */
    x0 += px0;
    y0 += py0;
  }

#ifdef CHILDS_INSIDE_PARENT
  if (x0 < 0) {
    x0 = 0;
  }
  if (y0 < 0) {
    y0 = 0;
  }
  if (x0 + win->pos.w > glob_screen->bm.width) {
    x0 = glob_screen->bm.width - win->pos.w;
  }
  if (y0 + win->pos.h > glob_screen->bm.height) {
    y0 = glob_screen->bm.height - win->pos.h;
  }
#endif
  /*
   * must disable mouse anyway, because window_save_contents()
   * may copy from any window... The mouse_hide() should
   * better be moved into window_save_contents() ?
   *
   * ++kay
   */
  mouse_hide ();

  /* don't care for clipping here */
  glob_clip0 = NULL;
  glob_clip1 = NULL;

  if (window_open (win, x0, y0) < 0)
    return -1;

  /* otherwise we would have no frame at all
   */
  windowDeactivate(win);

  /* am I now the active window?
   */
#ifdef CLICK_TO_FOCUS
  w_changeActiveWindowTo(win);
#else
  w_changeActiveWindow();
#endif

  return 0;
}


/*
 * this will be called from window_open() when really opening a window (ie
 * it has no closed ancestors).
 *
 * kay.
 */

void client_do_open (WINDOW *win)
{
  if (win->flags & W_NOMOUSE) {
    glob_mouse.disabled++;
  }
  win->cptr->openWindows++;
  globOpenWindows++;
}


short client_move (CLIENT *cptr, ushort handle, short x0, short y0)
{
  WINDOW *win, *parent;
  short width, height, ret;

  if (!(win = windowLookup (handle)))
    return -9999;

  if (!win->is_open)
    return -1;

  if (!(win->flags & W_MOVE))
    return -1;

#ifdef CHILDS_INSIDE_PARENT
  if ((x0 < 0) || (y0 < 0))
    return -1;
#endif

  parent = win->parent;

  /* cptr == NULL means we're called by a mouse move and have absolute
   * coordinates, otherwise they're relative to the work area of the parent
   * window.
   */
  if (cptr) {
    x0 += parent->work.x0;
    y0 += parent->work.y0;
  }

  width = win->pos.w;
  height = win->pos.h;

#ifdef CHILDS_INSIDE_PARENT
  /* temporarily: a window may only lay inside the work area of its parent */
  if (x0 < parent->work.x0) {
    x0 = parent->work.x0;
  }
  if (x0 + width > parent->work.x0 + parent->work.w) {
    x0 = parent->work.x0 + parent->work.w - width;
  }
  if (y0 < parent->work.y0) {
    y0 = parent->work.y0;
  }
  if (y0 + height > parent->work.y0 + parent->work.h) {
    y0 = parent->work.y0 + parent->work.h - height;
  }
#endif

  /* disable mouse always */
  mouse_hide();

  /* don't care for clipping here */
  glob_clip0 = NULL;
  glob_clip1 = NULL;

  ret = window_move(win, x0, y0);

#ifndef CLICK_TO_FOCUS
  /* did the active window change? */
  w_changeActiveWindow();
#endif

  return ret;
}


short client_resize (CLIENT *cptr, ushort handle, short width, short height)
{
  WINDOW *win, oldWin;
  int wasOpen = 0;
  short x0, y0, ret = 0;

  if (!(win = windowLookup (handle)))
    return -9999;

  /*
   * window positions are relative to work area of parent (ie. to
   * win->parent->work, not win->parent->pos).
   */
  x0 = win->pos.x0 - win->parent->work.x0;
  y0 = win->pos.y0 - win->parent->work.y0;
  wasOpen = !client_close (NULL, win->id);

  oldWin = *win;

  /*
   * make sure win->pos.x0 and win->pos.y0 do not change during the
   * resize, because window_open() relies on it (otherwise subwindows
   * of the resized window are shifted to wrong positions).
   * therefore two new params to createBitmap()...
   * also do not try to copy for W_CONTAINER windows...
   * kay.
   */
  if (!createBitmap (win, width, height, win->pos.x0, win->pos.y0)) {
    if (!(win->flags & W_CONTAINER)) {
      /* ok, copy as much of the bitmap as possible
       */
      short wMin = MIN (oldWin.area[AREA_WORK].w, win->area[AREA_WORK].w);
      short hMin = MIN (oldWin.area[AREA_WORK].h, win->area[AREA_WORK].h);

      glob_clip0 = NULL;
      glob_clip1 = NULL;
      (*glob_screen->bitblk) (&oldWin.bitmap,
			      oldWin.area[AREA_WORK].x0, oldWin.area[AREA_WORK].y0,
			      wMin, hMin,
			      &win->bitmap, win->area[AREA_WORK].x0, win->area[AREA_WORK].y0);

      free (oldWin.bitmap.data);
    }
  } else {

    /* error! keep old state.
     */
    *win = oldWin;
    ret = -1;
  }

  if (wasOpen)
    client_open (NULL, win->id, x0, y0);

  return ret;
}


short client_close(CLIENT *cptr, ushort handle)
{
  WINDOW *win;

  if (handle < 2) {
    /* you cannot close root or background window */
    return -1;
  }
  if (!(win = windowLookup(handle))) {
    return -9999;
  }
  if (!win->is_open) {
    return -1;
  }

  /* disable mouse always */
  mouse_hide();

  /* don't care for clipping here */
  glob_clip0 = NULL;
  glob_clip1 = NULL;

  window_close(win);

  /* was I the active window? */
#ifdef CLICK_TO_FOCUS
  w_maybeActiveWindowClosing(win);
#else
  w_changeActiveWindow();
#endif

  return 0;
}


/*
 * this will be called from window_close() just closing a window.
 *
 * we need such a function because closing a window causes closing all
 * its subwindows as well.
 * kay.
 */

void client_do_close (WINDOW *win)
{
  if (win->flags & W_NOMOUSE) {
    glob_mouse.disabled--;
  }

  /* if there were any mouse events pending, ignore them */
  if (glob_leftmousepressed == win)
    glob_leftmousepressed = NULL;
  if (glob_rightmousepressed == win)
    glob_rightmousepressed = NULL;

  win->cptr->openWindows--;
  globOpenWindows--;
}


short client_delete(CLIENT *cptr, ushort handle)
{
  WINDOW *win;

  if (handle < 2) {
    /* you cannot close root or background window */
    return -1;
  }
  if (!(win = windowLookup(handle))) {
    return -9999;
  }
  if (!win->cptr) {
    return -1;
  }

  if (win->is_open) {
    client_close(cptr, handle);
  }
  window_delete(win);
  return 0;
}


/*
 * this will be called from window_delete() just before deleting a window.
 * it deletes the window pointer, backing bitmap and color palette and
 * updates the window counters.
 *
 * we need such a function because deleting a window causes deleting all
 * its subwindows as well.
 * kay.
 */

void client_do_delete (WINDOW *win)
{
  if (win->mdata) {
    free(win->mdata);
    win->mdata =NULL;
  }

  if (win->bitmap.data) {
    free(win->bitmap.data);
    win->bitmap.data = NULL;
  }

  /* the next two if's make sure we do not deactivate a window
   * that has been deleted already. this shouldn't happen because
   * currently windows are closed (and thus deactivated) before
   * deleting, but better safe than sorry.
   */
  if (win == glob_activewindow)
    glob_activewindow = glob_backgroundwin;
  if (win == glob_activetopwin)
    glob_activetopwin = glob_backgroundwin;

  /* since all childs of a window are deleted before the window itself it's
   * safe to simply free the color table here because it will never be used.
   * well, in fact the routine will check if it really has to free it
   * physically.
   */
  colorFreeTable(win);

  win->cptr->totalWindows--;
  globTotalWindows--;
}


/*
 * these four load and unload fonts
 */

static char *compose_name(const char *family, short size, ushort styles)
{
  static char fname[MAXFAMILYNAME + 5 + 3 + 1 + sizeof(WFONT_EXTENSION) + 1];
  short idx = 0;

  /* family name */
  if (!*family) {
    if (glob_fontfamily) {
      family = glob_fontfamily;
    } else {
      family = DEF_WFONTFAMILY;
    }
  }
  /* only aplphanumeric characters valid for family names */
  while(isalnum(*family)) {
    fname[idx++] = *family++;
    if (idx >= MAXFAMILYNAME)
      break;
  }

  /* size */
  if (!size) {
    if (glob_fontsize > 0) {
      size = glob_fontsize;
    } else {
      size = DEF_WFONTSIZE;
    }
  }
  if (size / 100) {
    fname[idx++] = '0' + size / 100;
    size %= 100;
  }
  if (size / 10) {
    fname[idx++] = '0' + size / 10;
    size %= 10;
  }
  fname[idx++] = '0' + size;

  /* styles */
  if (styles & F_BOLD) {
    fname[idx++] = 'b';
  }
  if (styles & F_ITALIC) {
    fname[idx++] = 'i';
  }
  if (styles & F_LIGHT) {
    fname[idx++] = 'l';
  }
  /* underline and reverse should be generally done with effects */
  if (styles & F_REVERSE) {
    fname[idx++] = 'r';
  }
  if (styles & F_UNDERLINE) {
    fname[idx++] = 'u';
  }

  /* extension */
  fname[idx++] = '.';
  family = WFONT_EXTENSION;
  while(*family && idx < sizeof(fname)) {
    fname[idx++] = *family++;
  }
  fname[idx] = '\0';

  return fname;
}

void client_loadfont (CLIENT *cptr, const char *family, short size, short styles, LFONTRETP *frpaket)
{
  LIST *list = NULL;
  short idx = 0;
  char *fname;
  FONT *font;

  /* compose font filename */
  fname = compose_name(family, size, styles);

  /*
   * is it already loaded?
   */
  for (idx = 0; idx < MAXFONTS; idx++) {
    if (glob_font[idx].numused && !strcmp(glob_font[idx].name, fname)) {
      for (list = cptr->font; list; list = list->next) {
	if (list->handle == idx) {
	  break;
	}
      }
      glob_font[idx].numused++;
      break;
    }
  }

  if (idx == MAXFONTS) {
    /* looks like we've got to load it manually */
    if ((idx = font_loadfont(fname)) < 0) {
      frpaket->handle = htons(-1);
      return;
    }
  }

  if (list) {
    glob_font[idx].numused--;
  } else {  
    /* add handle to client font list start */
    if (!(list = malloc(sizeof(LIST)))) {
      glob_font[idx].numused--;
      frpaket->handle = htons(-1);
    }
    if (cptr->font) {
      list->next = cptr->font;
    } else {
      list->next = NULL;
    }
    cptr->font = list;
    list->handle = idx;
  }

  font = &(glob_font[idx]);
  frpaket->handle = htons(idx);

  frpaket->height = htons(font->hdr.height);
  frpaket->flags  = htons(font->hdr.flags);
  frpaket->styles = htons(font->hdr.styles);
  frpaket->maxwidth = htons(font->hdr.maxwidth);
  frpaket->baseline = htons(font->hdr.baseline);

  memcpy(frpaket->family, font->hdr.family, MAXFAMILYNAME);
  memcpy(frpaket->widths, font->widths, 256);
}


/*
 * unloading a font is a bit more difficult
 */

typedef struct {
  CLIENT *cptr;
  FONT *fp;
} ARGS;

static int killFont(WINDOW *win, void *arg)
{
  if (win->cptr == ((ARGS *)arg)->cptr) {
    if (win->gc.font == ((ARGS *)arg)->fp) {
      win->gc.font = NULL;
    }
    return 0;
  }
  /* skip this window tree */
  return 1;
}

short client_unloadfont(CLIENT *cptr, short fonthandle)
{
  LIST *font, *prev = NULL;
  ARGS args;

  if ((fonthandle < 0) || (fonthandle >= MAXFONTS) ||
      !glob_font[fonthandle].numused) {
    return -1;
  }

  for (font = cptr->font; font; font = font->next) {
    if (font->handle == fonthandle) {
      break;
    }
    prev = font;
  }
  if (!font) {
    return -1;
  }
  if (prev) {
    prev->next = font->next;
  } else {
    cptr->font = font->next;
  }
  free(font);

  /* unload this font from all windows of that client */

  args.cptr = cptr;
  args.fp = &glob_font[fonthandle];
  wtree_traverse_pre (glob_rootwindow, &args, killFont);

  if (--(args.fp->numused) < 1)
    font_unloadfont (args.fp);

  return 0;
}


short client_querywinsize(CLIENT *cptr, ushort handle, short effective,
			  short *width, short *height)
{
  WINDOW *win;

  if (!(win = windowLookup(handle))) {
    return -9999;
  }

  if (effective) {
    if (width)
      *width = win->pos.w;
    if (height)
      *height = win->pos.h;
  } else {
    if (width)
      *width = win->work.w;
    if (height)
      *height = win->work.h;
  }

  return 0;
}


short client_test(CLIENT *cptr, ushort handle, short x0, short y0)
{
  WINDOW *win;
  short ret;

  if (!(win = windowLookup(handle))) {
    return -9999;
  }

  if (win->flags & W_CONTAINER) {
    return -9998;
  }

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

#ifndef REFRESH
  ret = -1;
  gc0 = &win->gc;
  if (!win->is_open || win->is_hidden) {
#endif
    glob_clip0 = &win->area[AREA_WORK];
    ret = (*glob_screen->test)(&win->bitmap, x0, y0);
#ifndef REFRESH
  } else {
    if (mouse_rcintersect(win->pos.x0 + x0, win->pos.y0 + y0, 1, 1)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    ret = (*glob_screen->test)(&glob_screen->bm, win->pos.x0 + x0,
			       win->pos.y0 + y0);
  }
#endif
  return ret;
}


short client_querymousepos(CLIENT *cptr, ushort handle, short *x0, short *y0)
{
  WINDOW *win;
  short mx, my;

  /* FIXME: shouldn't this be the 'real' values?
   */
  mx = glob_mouse.drawn.x0;
  my = glob_mouse.drawn.y0;

  if (!handle) {
    if (x0)
      *x0 = mx;
    if (y0)
      *y0 = my;
    return 0;
  }

  if (!(win = windowLookup (handle)))
    return -9999;

  if (win->cptr && win->is_open) {
    mx -= win->pos.x0;
    my -= win->pos.y0;

    if (x0)
      *x0 = mx - win->area[AREA_WORK].x0;
    if (y0)
      *y0 = my - win->area[AREA_WORK].y0;

    /* is it in this window?
     */
    if (window_find (glob_mouse.drawn.x0, glob_mouse.drawn.y0, 0) != win)
      return 1;

    /* is it really in the work area of the window? */
    if (!rect_cont_point (&win->area[AREA_WORK], mx, my))
      return 1;

    return 0;
  }

  return -1;
}


short client_querywindowpos(CLIENT *cptr, ushort handle, short effective,
			    short *x0, short *y0)
{
  WINDOW *win;

  if (!(win = windowLookup(handle))) {
    return -9999;
  }

  if (win->cptr && win->is_open) {

    if (effective) {
      if (x0)
	*x0 = win->pos.x0;
      if (y0)
	*y0 = win->pos.y0;
    } else {
      if (x0)
	*x0 = win->work.x0;
      if (y0)
	*y0 = win->work.y0;
    }

    return 0;
  }

  return -1;
}

