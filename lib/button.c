/*
 * button.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- functions for drawing and checking simple UI button
 *
 * CHANGES:
 * - moved w_queryevent BUTTON event stuff here. 6/97 ++eero
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"

/* button window user_val != 0 */
#define WWIN_BUTTON 1

/*
 * some functions dealing with `buttons'
 */

WWIN *w_createButton (WWIN *parent,
		      short x0, short y0, short width, short height)
{
  const char *cptr;
  WWIN *ret;

  TRACESTART();

  if ((cptr = _check_window(parent))) {
    TRACEPRINT(("w_createChild(%p,%i,%i) -> NULL\n",\
		parent, width, height));
    TRACEEND();
    return NULL;
  }

  if (!(ret = w_createChild(parent, width, height, EV_MOUSE))) {
    TRACEPRINT(("w_createChild(%p,%i,%i) -> NULL\n",\
		parent, width, height));
    TRACEEND();
    return NULL;
  }

  ret->user_val = WWIN_BUTTON;
  ret->x0 = x0;
  ret->y0 = y0;

  TRACEPRINT(("w_createChild(%p,%i,%i) -> %p\n",\
	      parent, width, height, ret));
  TRACEEND();

  return ret;
}


short w_centerPrints (WWIN *win, WFONT *font, const char *s)
{
  const char *cptr;
  int len;
  WFONT *oldFont;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_centerPrints(%p,%p,\"%s\") -> -1\n",\
		win, font, s));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_centerPrints(%p,%p,\"%s\")...\n",\
	      win, font, s));

  len = w_strlen(font, s);

  oldFont = w_setfont(win, font);
  w_printstring(win, (win->width >> 1) - (len >> 1),
		(win->height >> 1) - (font->height >> 1), s);

  w_setfont(win, oldFont);

  TRACEPRINT(("w_centerPrints() -> 0\n"));
  TRACEEND();

  return 0;
}


short w_showButton (WWIN *button)
{
  const char *cptr;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(button))) {
    TRACEPRINT(("w_showButton(%p) -> %s\n", button, cptr));
    TRACEEND();
    return -1;
  }

  if (button->user_val != WWIN_BUTTON) {
    TRACEPRINT(("w_showButton(%p) -> not a button\n", button));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_showButton()...\n"));

  ret = w_open(button, button->x0, button->y0);

  TRACEPRINT(("w_showButton() -> %i\n", ret));
  TRACEEND();

  return ret;
}


short w_hideButton (WWIN *button)
{
  const char *cptr;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(button))) {
    TRACEPRINT(("w_hideButton(%p) -> %s\n", button, cptr));
    TRACEEND();
    return -1;
  }

  if (button->user_val != WWIN_BUTTON) {
    TRACEPRINT(("w_hideButton(%p) -> not a button\n", button));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_hideButton()...\n"));

  ret = w_close(button);

  TRACEPRINT(("w_hideButton() -> %i\n", ret));
  TRACEEND();

  return ret;
}

WEVENT *w_querybuttonevent(fd_set *rfd, fd_set *wfd, fd_set *xfd, long timeout)
{
  WEVENT *tmp;
  short oldmode;
  static WWIN *selected = NULL;
  int forceRepeat;

  TRACESTART();
  TRACEPRINT(("w_querybuttonevent(%p,%p,%p,%li) -> ?\n",\
	      rfd, wfd, xfd, timeout));

  do {

    /* normally we run through this loop only once, except when we caught a
     * button event: in this case we do it again. this may involve selecting
     * other sockets twice - quite an overhead - but seems better to me than
     * forcing a WWIN_BUTTON press and release to be served with two
     * seperate calls - the client might not like that.
     */

    forceRepeat = 0;

    if ((tmp = w_queryevent(rfd, wfd, xfd, timeout))) {

      if ((tmp->type == EVENT_MPRESS || tmp->type == EVENT_MRELEASE)
	  && tmp->win->user_val == WWIN_BUTTON)
      {
	if ((tmp->type == EVENT_MPRESS) && (tmp->key == BUTTON_LEFT)) {

	  /* graphically `select' button */
	  oldmode = w_setmode(tmp->win, M_INVERS);
	  w_pbox(tmp->win, 0, 0, tmp->win->width, tmp->win->height);
	  w_setmode(tmp->win, oldmode);

	  selected = tmp->win;

	  /* try to get the corresponding release event ASAP */
	  forceRepeat = 1;

	} else if ((tmp->type == EVENT_MRELEASE) && (tmp->key == BUTTON_LEFT)) {

	  /* graphically `unselect' button, if it doesn't exist anymore,
	   * nothing will happen
	   */
	  oldmode = w_setmode(tmp->win, M_INVERS);
	  w_pbox(tmp->win, 0, 0, tmp->win->width, tmp->win->height);
	  w_setmode(tmp->win, oldmode);

	  if ((tmp->win == selected) && (tmp->x >= 0) && (tmp->y >= 0)) {
	    tmp->type = EVENT_BUTTON;
	  } else {
	    tmp = NULL;
	    /* sleep on */
	    forceRepeat = 1;
	  }
	  selected = NULL;
	}
      }
    }
  } while (forceRepeat);

  TRACEPRINT(("w_querybuttonevent() -> %p\n", tmp));
  TRACEEND();

  return tmp;
}
