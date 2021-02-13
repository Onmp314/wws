/*
 * window.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib window function mappings to Xlib
 *
 * CHANGES:
 * - added window construction from ID (win->handle has a value).  These
 *   windows can't be drawn, moved, resized, deleted nor receive events,
 *   just be inherited.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"
#include "window.h"

static const char *WName = "W-Application";

static W2XWin *W2XRoot;		/* the 'head' for the window list */



ulong w_winID(WWIN *win)
{
  /* better if this would be a container */
  return ((W2XWin *)win)->wid;
}


WWIN *w_winFromID(ulong id)
{
  W2XWin *ptr;
  WWIN *win;

  if (!(ptr = (W2XWin*)calloc(1, sizeof(W2XWin))))
    return NULL;

  ptr->win.magic = MAGIC_W;
  ptr->win.handle = 1;		/* for normal window it's zero */
  ptr->flags = W_CONTAINER;	/* can't be drawn into */
  ptr->wid = id;

  win = &ptr->win;
  w_querywinsize(win, 0, &win->width, &win->height);
  return win;
}


/* find the window from the list */
W2XWin *_find_window(Window win)
{
  W2XWin *ptr = W2XRoot;

  while(ptr)
  {
    if(ptr->wid == win)
      return ptr;
    ptr = ptr->next;
  }
  return NULL;
}

const char *_check_window(WWIN *ptr)
{
  if(!ptr)
    return "no window";

  if(ptr->magic != MAGIC_W)
    return "not a W window (anymore?)";

  return NULL;
}


static WWIN *create_win(Window parent, short width, short height,
	short flags, const char *fname)
{
  int border;
  unsigned long mask;
  XSetWindowAttributes attrib;
  XGCValues gv;
  W2XWin *ptr;
  const char *cptr = NULL;

  if (!(ptr = (W2XWin*)calloc(1, sizeof(W2XWin))))
  {
    cptr = "unable to allocate W2XWin structure";
    goto error;
  }
  if(!(flags & W_CONTAINER))
  {
    /* create a pixmap for backing up the window drawing for redrawing */
    ptr->pixmap = XCreatePixmap(_Display, _Rootwin, width, height, _Depth);

    /* Create a graphic context */
    gv.function = GXcopy;
    gv.foreground = _White;
    ptr->gc = XCreateGC(_Display, ptr->pixmap, GCFunction | GCForeground, &gv);

    if(!(ptr->pixmap && ptr->gc))
    {
      cptr = "unable to allocate window Pixmap or GC";
      goto error;
    }
    /* clear the window backup */
    XFillRectangle(_Display, ptr->pixmap, ptr->gc, 0, 0, width, height);

    /* copy default attributes */
    XCopyGC(_Display, _Rootgc, GMASK, ptr->gc);

    /* windows start with invert mode */
    gv.function = GXxor;
    gv.foreground = _Black^_White;
    XChangeGC(_Display, ptr->gc, GCForeground | GCFunction, &gv);

    /* set fill pattern */
    XGetGCValues(_Display, ptr->gc, GMASK, &gv);
    gv.fill_style = FillStippled;
    gv.stipple = _GrayPattern;
    ptr->fillgc = XCreateGC(_Display, ptr->pixmap,
                  GMASK | GCFillStyle | GCStipple, &gv);
  }

  /* same as with the root window */
  set_defgc(&ptr->win, 0, width, height);

  ptr->flags = flags;
  ptr->title = strdup(WName);

  if(flags & W_NOBORDER)
    border = 0;		/* containers don't have borders */
  else
    border = 2;		/* so that e.g.subwindows are seen... */

  mask = CWBackPixel | CWOverrideRedirect | CWEventMask;
  attrib.background_pixel = _White;
  attrib.override_redirect = False;

  /* window manager flags (W_ICON etc.) are ignored */
  attrib.event_mask = StructureNotifyMask | ExposureMask;
  if(flags & EV_KEYS)
    attrib.event_mask |= KeyReleaseMask; /* KeyPressMask; */
  if(flags & EV_MOUSE)
    attrib.event_mask |= ButtonPressMask | ButtonReleaseMask;
  if(flags & EV_ACTIVE)
  {
    attrib.event_mask |= EnterWindowMask;
    attrib.event_mask |= LeaveWindowMask;
  }

  /* display, parent, position, size, border width, depth, class, visual,
   * attribute mask, attribute struct
   */
  if(!(ptr->wid = XCreateWindow(_Display, parent, 0, 0, width, height, border,
     CopyFromParent, CopyFromParent, CopyFromParent, mask, &attrib)))
  {
    cptr = "unable to create X window";
    goto error;
  }
  /* client accepts delete events from the *window manager* */
  XSetWMProtocols(_Display, ptr->wid, &_WMdelete_window, 1);

  TRACEPRINT(("%s(%i,%i,0x%04x) -> %p\n",\
	      fname, width, height, (unsigned int)flags, ptr));

  ptr->prev = NULL;
  ptr->next = W2XRoot;
  if(W2XRoot)
    W2XRoot->prev = ptr;
  W2XRoot = ptr;

  return &ptr->win;


error:
  /* error exit */
  w_delete(&ptr->win);
  TRACEPRINT(("%s(%i,%i,0x%04x) -> %s\n",\
	      fname, width, height, (unsigned int)flags, cptr));
  return NULL;
}


WWIN *w_create(short width, short height, ushort flags)
{
  WWIN *ptr;

  TRACESTART();
  ptr = create_win(_Rootwin, width, height, flags, "w_create");
  TRACEEND();
  return ptr;
}


WWIN *w_createChild(WWIN *parent, short width, short height, ushort flags)
{
  W2XWin *ptr = (W2XWin *)parent;
  WWIN *win;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(parent)))
    goto error;

  if(!ptr->flags & W_CONTAINER)
  {
    cptr = "not a container";
    goto error;
  }

  win = create_win(ptr->wid, width, height, flags, "w_createChild");
  win->colors = parent->colors;
  win->type = WWIN_SUB;

  TRACEEND();
  return win;

error:
  TRACEPRINT(("w_createChild(%p,%i,%i,0x%04x) -> NULL (%s)\n",\
    parent, width, height, (unsigned int)flags, cptr));
  TRACEEND();
  return NULL;
}


short w_settitle(WWIN *win, const char *s)
{
  W2XWin *ptr = (W2XWin *)win;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_settitle(%p,%s) -> %s\n", win, s, cptr));
    TRACEEND();
    return -1;
  }

  if (win->type != WWIN_SUB)
  {
    if (ptr->wid)
      XSetStandardProperties(_Display, ptr->wid, s, s, 0, NULL, 0, NULL);

    if (ptr->title)
      free(ptr->title);
    ptr->title = strdup(s);
  }
  TRACEPRINT(("w_settitle(%p,%s)\n", win, s));
  TRACEEND();
  return 0;
}


short w_open(WWIN *win, short x0, short y0)
{
  XSizeHints hint;
  W2XWin *ptr = (W2XWin *)win;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_open(%p,%i,%i) -> %s\n", win, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  XMoveWindow(_Display, ptr->wid, x0, y0);

  if (win->type != WWIN_SUB)
  {
    int width = win->width;
    int height = win->height;

    /* set the hints for the window manager */
    hint.flags = 0;

    /* x, y, width, height hints are needed only for older wm's */
    hint.width = width;
    hint.height = height;

    /* I use user position here too as most WMs seem as default to ignore
     * program position
     */
    if(x0 == UNDEF)
      hint.x = 0;
    else
    {
      hint.flags |= USPosition;
      hint.x = x0;
    }
    if(y0 == UNDEF)
      hint.y = 0;
    else
    {
      hint.flags |= USPosition;
      hint.y = y0;
    }
    if(!(ptr->flags & W_RESIZE))
    {
      hint.min_width  = width;
      hint.min_height = height;
      hint.max_width  = width;
      hint.max_height = height;
      hint.flags = PMinSize | PMaxSize;
    }

    /* announce the window (struct, name, icon, pos, size...)  to the
     * other apps
     */
    XSetStandardProperties(_Display, ptr->wid, ptr->title, ptr->title,
	  (Pixmap)NULL, NULL, 0, &hint);
  }

  /* raise (draw) the window on screen (results in expose event) */
  XMapRaised(_Display, ptr->wid);

  TRACEPRINT(("w_open(%p,%i,%i) -> 0\n", win, x0, y0));
  TRACEEND();
  return 0;
}


short w_move(WWIN *win, short x0, short y0)
{
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_move(%p,%i,%i) -> %s\n", win, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  if (win->handle)
  {
    TRACEPRINT(("w_move(%p,%i,%i) -> fake window\n", win, x0, y0));
    TRACEEND();
    return -1;
  }

  XMoveWindow(_Display, ((W2XWin *)win)->wid, x0, y0);

  TRACEPRINT(("w_move(%p,%i,%i) -> 0\n", win, x0, y0));
  TRACEEND();
  return 0;
}


short w_resize (WWIN *win, short width, short height)
{
  W2XWin *ptr = (W2XWin *)win;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_resize(%p,%i,%i) -> %s\n", win, width, height, cptr));
    TRACEEND();
    return -1;
  }

  if (win->handle)
  {
    TRACEPRINT(("w_resize(%p,%i,%i) -> fake window\n", win, width, height));
    TRACEEND();
    return -1;
  }

  if(ptr->pixmap)
  {
    XGCValues gv;
    GC gc;

    XFreePixmap(_Display, ptr->pixmap);
    ptr->pixmap = XCreatePixmap(_Display, _Rootwin, width, height, _Depth);
    if(!ptr->pixmap)
    {
      TRACEPRINT(("w_resize(%p,%i,%i) -> pixmap unavailable\n", win, width, height));
      TRACEEND();
      return -1;
    }

    /* Create a graphic context */
    gv.function = GXcopy;
    gv.foreground = _White;
    gc = XCreateGC(_Display, ptr->pixmap, GCFunction | GCForeground, &gv);

    if(gc)
    {
      XFillRectangle(_Display, ptr->pixmap, gc, 0, 0, width, height);
      XFreeGC(_Display, gc);
    }
  }

  win->width = width;
  win->height = height;
  _flush_area(ptr, 0, 0, width - 1, height - 1, 0);
  XResizeWindow(_Display, ptr->wid, width, height);

  TRACEPRINT(("w_resize(%p,%i,%i) -> 0\n", win, width, height));
  TRACEEND();
  return 0;
}

short w_close(WWIN *win)
{
  W2XWin *ptr = (W2XWin *)win;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win)))
  {
    TRACEPRINT(("w_close(%p) -> %s\n", win, cptr));
    TRACEEND();
    return -1;
  }

  XUnmapWindow(_Display, ptr->wid);

  TRACEPRINT(("w_close(%p) -> 0\n", win));
  TRACEEND();
  return 0;
}


short w_delete(WWIN *win)
{
  W2XWin *ptr = (W2XWin *)win;

  TRACESTART();

  if(ptr)
  {
    win->magic = 0;
    if (ptr->title)
      free(ptr->title);

    if(ptr->pixmap)
      XFreePixmap(_Display, ptr->pixmap);

    if(ptr->fillgc)
      XFreeGC(_Display, ptr->fillgc);

    if(ptr->gc)
      XFreeGC(_Display, ptr->gc);

    if(ptr->wid)
      XDestroyWindow(_Display, ptr->wid);

    if(ptr->prev)
      ptr->prev->next = ptr->next;
    else
      W2XRoot = ptr->next;
    if(ptr->next)
      ptr->next->prev = ptr->prev;

    free(ptr);
  }

  TRACEPRINT(("w_delete(%p) -> 0\n", win));
  TRACEEND();
  return 0;
}

/* go through and redraw all the windows */
void w_flush(void)
{
  W2XWin *ptr = W2XRoot;
  int flushed = 0;

  while(ptr)
  {
    if(ptr->need_flush)
    {
      _redraw(ptr);
      flushed = 1;
    }
    ptr = ptr->next;
  }

  if(flushed)
    XSync(_Display, 0);
}

