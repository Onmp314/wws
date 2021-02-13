/*
 * event.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * --  w_queryevent() function and Xlib window message (redraw) management
 *
 * TODO:
 * - X window manager event to WEVENT mapping into _event_handler.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include "Wlib.h"
#include "proto.h"

/* 
#define DEBUG
*/

#ifdef DEBUG
static const char *events[] =
{
  "---",
  "---",
  "KeyPress",
  "KeyRelease",
  "ButtonPress",
  "ButtonRelease",
  "MotionNotify",
  "EnterNotify",
  "LeaveNotify",
  "FocusIn",
  "FocusOut",
  "KeymapNotify",
  "Expose",
  "GraphicsExpose",
  "NoExpose",
  "VisibilityNotify",
  "CreateNotify",
  "DestroyNotify",
  "UnmapNotify",
  "MapNotify",
  "MapRequest",
  "ReparentNotify",
  "ConfigureNotify",
  "ConfigureRequest",
  "GravityNotify",
  "ResizeRequest",
  "CirculateNotify",
  "CirculateRequest",
  "PropertyNotify",
  "SelectionClear",
  "SelectionRequest",
  "SelectionNotify",
  "ColormapNotify",
  "ClientMessage",
  "MappingNotify",
  "LASTEvent"
};
#endif /* DEBUG */


/* swap any objects which can be XORed */
#define	SWAP(a,b)	((a)=(a)^((b)=(b)^((a)=(a)^(b))))


/* local functions */

/* returns the char or zero if more than one */
static long process_key(XKeyEvent *kev)
{
#define BUF_SIZE 3
  char text[BUF_SIZE];
  KeySym mykey;		/* for precise id */

  /* translates XEvent string into a char string, returns lenght */
  if(XLookupString(kev, text, BUF_SIZE, &mykey, 0) == 1) {
    switch(mykey) {
      case XK_F1:	return WKEY_F1;
      case XK_F2:	return WKEY_F2;
      case XK_F3:	return WKEY_F3;
      case XK_F4:	return WKEY_F4;
      case XK_F5:	return WKEY_F5;
      case XK_F6:	return WKEY_F6;
      case XK_F7:	return WKEY_F7;
      case XK_F8:	return WKEY_F8;
      case XK_F9:	return WKEY_F9;
      case XK_F10:	return WKEY_F10;

      case XK_Up:	return WKEY_UP;
      case XK_Down:	return WKEY_DOWN;
      case XK_Left:	return WKEY_LEFT;
      case XK_Right:	return WKEY_RIGHT;

      case XK_End:	return WKEY_END;
      case XK_Begin:	return WKEY_HOME;
      case XK_Prior:	return WKEY_PGUP;
      case XK_Next:	return WKEY_PGDOWN;

      case XK_Delete:	return WKEY_DEL;
      case XK_Insert:	return WKEY_INS;
    }
    return text[0];
  }
  return 0;
#undef BUF_SIZE
}

/* internal library utilities */

/* process / convert X events */
WEVENT *_event_handler(XEvent *xevent)
{
  static WEVENT wevent;		/* pointed by return value */
  W2XWin *ptr;
  int key;

#ifdef DEBUG
  printf("XEvent type: '%s'\n", events[xevent->type]);
#endif

  if (!(ptr = _find_window(xevent->xany.window)))
  {
    /* (silently) ignore X events for non-existing W windows */
#ifdef DEBUG
    printf("XEvent for non-existing W window!\n");
#endif
    return NULL;
  }

  memset(&wevent, 0, sizeof(WEVENT));
  wevent.time = w_gettime();

  switch(xevent->type)
  {
    /* Redraw 're-surfaced' window contents */
    case Expose:
      if(ptr->pixmap)
      {
	for(;;)
	{
	  if(_flush_area(ptr, xevent->xexpose.x, xevent->xexpose.y,
	      xevent->xexpose.x + xevent->xexpose.width  - 1,
	      xevent->xexpose.y + xevent->xexpose.height - 1, 0))
	    key = 1;
	  /* these should AFAIK come all to same window... */
	  if(!xevent->xexpose.count)
	    break;
	  XNextEvent(_Display, xevent);
	}
	_redraw(ptr);
      }
      else
      {
        /* pixmap or container */
	while(xevent->xexpose.count)
	  XNextEvent(_Display, xevent);
        XClearWindow(_Display, ptr->wid);
      }
      break;


    /* keyboard mapping changed */
    case MappingNotify:
      XRefreshKeyboardMapping(&xevent->xmapping);
      break;


    /* user closed window */
    case ClientMessage:
      if (xevent->xclient.message_type != _WMprotocols ||
          xevent->xclient.data.l[0] != _WMdelete_window)
	break;
    /* I don't know whether I (can) get this too, but it shouldn't harm
     * anything
     */
    case DestroyNotify:
      wevent.type = EVENT_GADGET;
      wevent.key = GADGET_CLOSE;
      wevent.win = &ptr->win;
      return &wevent;


    case ConfigureNotify:
      wevent.w = xevent->xconfigure.width;
      wevent.h = xevent->xconfigure.height;
      if (wevent.w == ptr->win.width && wevent.h == ptr->win.height)
        break;
      w_querywindowpos(&ptr->win, 0, &wevent.x, &wevent.y);
      wevent.type = EVENT_RESIZE;
      wevent.win = &ptr->win;
      return &wevent;


    /* a user keypress */
    case KeyRelease:
      if((key = process_key(&xevent->xkey)))
      {
        wevent.type = EVENT_KEY;
        wevent.key = key;
        wevent.win = &ptr->win;
        return &wevent;
      }
      break;


    case ButtonPress:
    case ButtonRelease:

      if(xevent->type == ButtonPress)
        wevent.type = EVENT_MPRESS;
      else
        wevent.type = EVENT_MRELEASE;

      wevent.key = 0;
#if 0
      /* previous state */
      if(xevent->xbutton.state & Button1Mask)
        wevent.key |= BUTTON_LEFT;
      if(xevent->xbutton.state & Button2Mask)
        wevent.key |= BUTTON_MID;
      if(xevent->xbutton.state & Button3Mask)
        wevent.key |= BUTTON_RIGHT;
#endif
      /* changes */
      if(xevent->xbutton.button == Button1)
        wevent.key = BUTTON_LEFT;
      else if(xevent->xbutton.button == Button2)
        wevent.key = BUTTON_MID;
      else if(xevent->xbutton.button == Button3)
        wevent.key = BUTTON_RIGHT;

      wevent.x = xevent->xbutton.x;
      wevent.y = xevent->xbutton.y;
      wevent.win = &ptr->win;
      return &wevent;


    case EnterNotify:
    case LeaveNotify:

      if(xevent->type == LeaveNotify)
        wevent.type = EVENT_INACTIVE;
      else
        wevent.type = EVENT_ACTIVE;

      wevent.x = xevent->xcrossing.x;
      wevent.y = xevent->xcrossing.y;
      wevent.win = &ptr->win;
      return &wevent;
  }
  return NULL;
}

/* check that there's a backup where to draw and 
 * store the maximum of co-ordinates of area to refresh
 */
const char *_flush_area(W2XWin *ptr, int x1, int y1, int x2, int y2, int width)
{
  if(!ptr->pixmap)
    return "container";

  /* order co-ordinates */
  if(x1 > x2)
    SWAP(x1, x2);
  if(y1 > y2)
    SWAP(y1, y2);

  if (width > 1) {
    width >>= 1;
    x1 -= width;
    y1 -= width;
    x2 += width;
    y2 += width;
  }

  /* not in the window */
  if(x2 < 0 || y2 < 0 || x1 >= ptr->win.width || y1 >= ptr->win.height)
    return NULL;

  /* store the maximum co-ordinates */
  if(ptr->need_flush)
  {
    if(ptr->wxy[0] > x1)
      ptr->wxy[0] = x1;
    if(ptr->wxy[1] > y1)
      ptr->wxy[1] = y1;
    if(ptr->wxy[2] < x2)
      ptr->wxy[2] = x2;
    if(ptr->wxy[3] < y2)
      ptr->wxy[3] = y2;
  }
  else
  {
    ptr->need_flush = 1;
    ptr->wxy[0] = x1;
    ptr->wxy[1] = y1;
    ptr->wxy[2] = x2;
    ptr->wxy[3] = y2;
  }
  return NULL;
}

/* refresh / redraw the window contents from the backup */
void _redraw(W2XWin *ptr)
{
  if(ptr->wid && ptr->need_flush)
  {
    XCopyArea(_Display, ptr->pixmap, ptr->wid, _Rootgc,
      ptr->wxy[0], ptr->wxy[1],
      ptr->wxy[2] - ptr->wxy[0] + 1, ptr->wxy[3] - ptr->wxy[1] + 1,
      ptr->wxy[0], ptr->wxy[1]);
    ptr->need_flush = 0;
  }
}

