/*
 * queries.c, a part of the W Window System
 *
 * Copyright (C) 1996-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wlib query function mappings to Xlib
 */

#include <stdio.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


short w_querywinsize(WWIN *win, short effective, short *width, short *height)
{
  XWindowAttributes watt;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_querywinsize(%p,%i,%p,%p) -> %s\n",\
		win, effective, width, height, cptr));
    TRACEEND();
    return -1;
  }

  XGetWindowAttributes(_Display, ((W2XWin *)win)->wid, &watt);
  *height = watt.height;
  *width = watt.width;

  TRACEPRINT(("w_querywinsize(%p,%i,%p,%p) -> %i,%i\n",\
	      win, effective, width, height, *width, *height));

  TRACEEND();

  return 0;
}


short w_querywindowpos(WWIN *win, short effective, short *x0, short *y0)
{
  int x, y;
  unsigned int w, h, border, depth;
  Window root, *child, wid;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_querywindowpos(%p,%i,%p,%p) -> %s\n",\
		win, effective, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  wid = ((W2XWin *)win)->wid;
  *x0 = *y0 = 0;
  do
  {
    XGetGeometry(_Display, wid, &root, &x, &y, &w, &h, &border, &depth);
    *x0 += x;
    *y0 += y;
    if (effective)
    {
      *x0 += border;
      *y0 += border;
    }
    XQueryTree(_Display, wid, &root, &wid, &child, &border);
  }
  while(wid != root);

  TRACEPRINT(("w_querywindowpos(%p,%i,%p,%p) -> %i,%i\n",\
	      win, effective, x0, y0, *x0, *y0));
  TRACEEND();

  return 0;
}


short w_querystatus(STATUS *st, short index)
{
  TRACESTART();

  if (st)
    memset(st, 0, sizeof(STATUS));

  TRACEPRINT(("w_querystatus(%p,%i) -> 0\n", st, index));
  TRACEEND();
  return 0;
}


short w_querymousepos(WWIN *win, short *x0, short *y0)
{
  XEvent xevent;
  Window wid, child, root;
  int root_x, root_y, x, y;
  unsigned int b;

  TRACESTART();

  w_flush();

  /* process / return pending event(s) */
  while(XEventsQueued(_Display, QueuedAlready))
  {
    /* process the masked events */
    XPeekEvent(_Display, &xevent);
    if(_event_handler(&xevent))
      break;

    XNextEvent(_Display, &xevent);
  }

  if(win)
    wid = ((W2XWin *)win)->wid;
  else
    wid = _Rootwin;

  /* display, window, pointer rootwin, pointer childwin, root x, root y,
   * window x, window y, buttons
   */
  XQueryPointer(_Display, wid, &root, &child, &root_x, &root_y, &x, &y, &b);

  TRACEPRINT(("w_querymousepos(%p,%p,%p) -> %i,%i\n", win, x0, y0, *x0, *y0));
  TRACEEND();

  if(win)
  {
    *x0 = x;
    *y0 = y;
    if (x >= 0 && y >= 0 && x < win->width && y < win->height)
      return 0;
    return -1;
  }
  *x0 = root_x;
  *y0 = root_y;
  return 0;
}


WEVENT *w_queryevent(fd_set *rfd, fd_set *wfd, fd_set *xfd, long timeout)
{
  XEvent xevent;
  struct timeval *tm, tv;
  WEVENT *event = NULL;
  int X_fd, ready = 0;
  fd_set myRfd;

  TRACESTART();
  TRACEPRINT(("w_queryevent(%p,%p,%p,%li) -> ?\n",\
	      rfd, wfd, xfd, timeout));

  /* flush requests / sychronize screen */
  w_flush();

  /* process / return queued event(s) */
  while(XEventsQueued(_Display, QueuedAfterFlush))
  {
    /* process the masked events */
    XNextEvent(_Display, &xevent);
    if((event = _event_handler(&xevent)))
      break;
  }

  if (!event)
  {
    if (timeout < 0)
      tm = NULL;
    else
    {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = 1000 * (timeout - tv.tv_sec * 1000);
      tm = &tv;
    }

    if (!rfd)
    {
      FD_ZERO (&myRfd);
      rfd = &myRfd;
    }
    /* I wonder what's the story behind that 'intuitive'
     * ConnectionNumber() macro name...
     */
    X_fd = ConnectionNumber(_Display);
    FD_SET (X_fd, rfd);

    if ((ready = select(FD_SETSIZE, rfd, wfd, xfd, tm)) < 0)
    {
      /* undefined behaviour... */
      fprintf(stderr, "Wlib: select() failed in w_queryevent().\n");
      return NULL;
    }

    /* check for new X events */
    if (FD_ISSET(X_fd, rfd))
    {
      FD_CLR(X_fd, rfd);

      while(XEventsQueued(_Display, QueuedAfterFlush))
      {
	/* process the masked events */
	XNextEvent(_Display, &xevent);
	if((event = _event_handler(&xevent)))
	  break;
      }
    }
  }
   
  if(!ready)
  {
    /* event queued or timeout */
    if(rfd)
      FD_ZERO(rfd);
    if(wfd)
      FD_ZERO(wfd);
    if(xfd)
      FD_ZERO(xfd);
  }

  TRACEPRINT(("w_queryevent() -> %p\n", event));
  TRACEEND();

  return event;
}

