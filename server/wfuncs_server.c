/*
 * wfuncs_server.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- window top/bottoming and changing of active window
 */

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


/*
 * a `top' routine
 */

void w_topDown(WINDOW *win)
{
  if (mouse_rcintersect(win->pos.x0, win->pos.y0, win->pos.w, win->pos.h)) {
    mouse_hide();
  }

  glob_clip0 = glob_clip1 = NULL;

  /* fix window hierarchy */

  if (win->parent->childs == win) {
    window_to_bottom(win);
  } else {
    window_to_top(win);
  }
}


/*
 *
 */
#ifndef CLICK_TO_FOCUS
void w_changeActiveWindow(void)
{
  WINDOW *win, *top;
  EVENTP ev;

  if (!(win = window_find (glob_mouse.real.x0, glob_mouse.real.y0, 1))) {
    /* hmmm..??? */
    return;
  }

  /*
   * find the toplevel window the mouse is in. this is now a bit more subtle
   * as we've got W_TOP windows again and even the glob_backgroundwin has a
   * parent.
   *
   * 2/98, ++eero:  W_TOP window can have children without W_TOP flag set.
   * Check for them too.
   */
  top = win;
  if (top != glob_backgroundwin && top->parent) {
    for ( ; top->parent != glob_backgroundwin; top = top->parent) {
      if (top->flags & W_TOP)
        break;
    }
  }

  if (win != glob_activewindow) {
    memset(&ev, 0, sizeof(EVENTP));
    ev.len = htons(sizeof(EVENTP));
    ev.type = htons(PAK_EVENT);
    ev.event.time = htonl(glob_evtime);

    if (glob_activewindow->flags & EV_ACTIVE) {
      ev.event.type = htons(EVENT_INACTIVE);
      ev.event.win = glob_activewindow->libPtr;
      write(glob_activewindow->cptr->sh, &ev, sizeof(EVENTP));
    }
    glob_activewindow = win;
    if (glob_activewindow->flags & EV_ACTIVE) {
      ev.event.type = htons(EVENT_ACTIVE);
      ev.event.win = glob_activewindow->libPtr;
      write(glob_activewindow->cptr->sh, &ev, sizeof(EVENTP));
    }
    colorSetColorTable(glob_activewindow->colTab);
  }

  if (glob_activetopwin != top) {
    mouse_hide();
    windowDeactivate(glob_activetopwin);
    glob_activetopwin = top;
    windowActivate(glob_activetopwin);
  }
}
#endif

/* Kay's old version */
#if 0

void w_changeActiveWindow(void)
{
  WINDOW *win, *top;
  EVENTP ev;

  win = window_find (glob_mouse.rx, glob_mouse.ry, 1);
  if (!win || win == glob_activewindow) {
    /* hmmm..??? */
    return;
  }

  /*
   * find the toplevel window the mouse is in. if win == glob_rootwindow
   * then top == glob_rootwindow afterwards.
   */
  top = win;
  if (top->parent) {
    for ( ; top->parent != glob_rootwindow; top = top->parent)
      ;
  }

  if (glob_activewindow->flags & EV_ACTIVE) {
    ev.len = htons(sizeof(EVENTP));
    ev.type = htons(PAK_EVENT);
    ev.event.type = htons(EVENT_INACTIVE);
    ev.event.win = glob_activewindow->libPtr;
    write(glob_activewindow->cptr->sh, &ev, sizeof(EVENTP));
  }

  mouse_hide();

  /*
   * the logic to determine what windows need which changes is a bit
   * complicated. but it makes sure we do no more then two
   * window{De,}activate() calls...
   */
  if (glob_activewindow != top)
    windowDeactivate(glob_activewindow);

  if (glob_activetopwin != top &&
      glob_activetopwin != win &&
      glob_activetopwin != glob_activewindow)
    windowDeactivate(glob_activetopwin);

  if (win != glob_activetopwin)
    windowActivate(win);

  if (top != glob_activetopwin &&
      top != win)
    windowActivate(top);

  glob_activewindow = win;
  glob_activetopwin = top;

  if (glob_activewindow->flags & EV_ACTIVE) {
    ev.len = htons(sizeof(EVENTP));
    ev.type = htons(PAK_EVENT);
    ev.event.type = htons(EVENT_ACTIVE);
    ev.event.win = glob_activewindow->libPtr;
    write(glob_activewindow->cptr->sh, &ev, sizeof(EVENTP));
  }
}

#endif

#ifdef CLICK_TO_FOCUS
static void w_internalChangeActiveWindow(WINDOW *to, int enforce)
{
  WINDOW *win, *top;
  EVENTP ev;

  win = to;

  /*
   * find the toplevel window the mouse is in. this is now a bit more subtle
   * as we've got W_TOP windows again and even the glob_backgroundwin has a
   * parent.
   *
   * 2/98, ++eero:  W_TOP window can have children without W_TOP flag set.
   * Check for them too.
   */
  top = win;
  if (top != glob_backgroundwin && top->parent) {
    for ( ; top->parent != glob_backgroundwin; top = top->parent) {
      if (top->flags & W_TOP)
        break;
    }
  }

  if (! (top->flags & EV_KEYS) ) {
          if (enforce) {
                  /* printf("found nothing, backing off to root\n"); */
                  top = glob_backgroundwin;
          } else {
                  /* printf("not enforcing\n"); */
                  return;
          }
  }

  if ( top != glob_activewindow) {
    memset(&ev, 0, sizeof(EVENTP));
    ev.len = htons(sizeof(EVENTP));
    ev.type = htons(PAK_EVENT);
    ev.event.time = htonl(glob_evtime);

    if (glob_activewindow->flags & EV_ACTIVE) {
      ev.event.type = htons(EVENT_INACTIVE);
      ev.event.win = glob_activewindow->libPtr;
      write(glob_activewindow->cptr->sh, &ev, sizeof(EVENTP));
    }
    glob_activewindow = top;
    if (glob_activewindow->flags & EV_ACTIVE) {
      ev.event.type = htons(EVENT_ACTIVE);
      ev.event.win = glob_activewindow->libPtr;
      write(glob_activewindow->cptr->sh, &ev, sizeof(EVENTP));
    }
    colorSetColorTable(glob_activewindow->colTab);
  }

  if (glob_activetopwin != top) {
    mouse_hide();
    windowDeactivate(glob_activetopwin);
    glob_activetopwin = top;
    windowActivate(glob_activetopwin);
  }
}

void w_changeActiveWindowTo(WINDOW *win)
{
        w_internalChangeActiveWindow(win, 0);
}

void w_maybeActiveWindowClosing(WINDOW *win)
{
        w_internalChangeActiveWindow(win->parent, 1);
}
#endif
