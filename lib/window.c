/*
 * window.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- window manipulation functions (create, open, delete, settitle)
 *
 * CHANGES
 * ++eero, 6/98:
 * - added set_defgc().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"
#include "window.h"


WWIN *w_create (short width, short height, ushort flags)
{
  WWIN *ptr;
  CREATEP *paket;
  ushort ret;

  TRACESTART();

  if (!(ptr = calloc (1, sizeof (WWIN)))) {
    TRACEPRINT(("w_create(%i,%i,0x%04x) -> NULL\n",\
		width, height, (unsigned int)flags));
    TRACEEND();
    return NULL;
  }

  paket = _wreservep(sizeof(CREATEP));
  paket->type   = htons(PAK_CREATE);
  paket->width  = htons(width);
  paket->height = htons(height);
  paket->flags  = htons(flags);
  paket->libPtr = ptr;

  if (!(ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret))) {
    free(ptr);
    TRACEPRINT(("w_create(%i,%i,0x%04x) -> NULL\n",\
		width, height, (unsigned int)flags));
    TRACEEND();

    return NULL;
  }

  /* set default graphics context */
  set_defgc(ptr, ret, width, height);

  TRACEPRINT(("w_create(%i,%i,0x%04x) -> %p\n",\
	      width, height, (unsigned int)flags, ptr));
  TRACEEND();
  return ptr;
}


WWIN *w_createChild(WWIN *parent, short width, short height, ushort flags)
{
  WWIN *ptr;
  CREATEP *paket;
  ushort ret;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(parent))) {
    TRACEPRINT(("w_createChild(%p,%i,%i,0x%04x) -> NULL\n",\
		parent, width, height, (unsigned int)flags));
    TRACEEND();
    return NULL;
  }

  if (!(ptr = calloc(1, sizeof(WWIN)))) {
    TRACEPRINT(("w_createChild(%p,%i,%i,0x%04x) -> NULL\n",\
		parent, width, height, (unsigned int)flags));
    TRACEEND();
    return NULL;
  }

  paket = _wreservep(sizeof(CREATEP));
  paket->type   = htons(PAK_CREATE2);
  paket->width  = htons(width);
  paket->height = htons(height);
  paket->flags  = htons(flags);
  paket->handle = htons(parent->handle);
  paket->libPtr = ptr;

  if (!(ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret))) {
    free(ptr);
    TRACEPRINT(("w_createChild(%p,%i,%i,0x%04x) -> NULL\n",\
		parent, width, height, (unsigned int)flags));
    TRACEEND();

    return NULL;
  }

  /* set default graphics context */
  set_defgc(ptr, ret, width, height);
  ptr->colors = parent->colors;
  ptr->type = WWIN_SUB;

  TRACEPRINT(("w_createChild(%p,%i,%i,0x%04x) -> %p\n",\
	      parent, width, height, (unsigned int)flags, ptr));
  TRACEEND();
  return ptr;
}


short w_settitle(WWIN *win, const char *s)
{
  const char *cptr;
  STITLEP *paket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_settitle(%p,%s) -> %s\n", win, s, cptr));
    TRACEEND();
    return -1;
  }

  TRACEPRINT(("w_settitle(%p,%s)\n", win, s));

  paket = _wreservep(sizeof(STITLEP));
  paket->type   = htons(PAK_STITLE);
  paket->handle = htons(win->handle);

  strncpy(paket->title, s, sizeof(paket->title)-1);
  paket->title[sizeof(paket->title)-1] = 0;

  TRACEEND();
  return 0;
}


short w_open(WWIN *win, short x0, short y0)
{
  const char *cptr;
  OPENP *paket;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_open(%p,%i,%i) -> %s\n", win, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(OPENP));
  paket->type   = htons(PAK_OPEN);
  paket->handle = htons(win->handle);
  paket->x0 = htons(x0);
  paket->y0 = htons(y0);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);

  TRACEPRINT(("w_open(%p,%i,%i) -> %i\n", win, x0, y0, ret));
  TRACEEND();
  return ret;
}


short w_close(WWIN *win)
{
  const char *cptr;
  CLOSEP *paket;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_close(%p) -> %s\n", win, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(CLOSEP));
  paket->type   = htons(PAK_CLOSE);
  paket->handle = htons(win->handle);

  ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);

  TRACEPRINT(("w_close(%p) -> %i\n", win, ret));
  TRACEEND();
  return ret;
}


short w_delete(WWIN *win)
{
  DELETEP *paket;
  short ret;
  const char *cptr;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_delete(%p) -> %s\n", win, cptr));
    TRACEEND();
    return -1;
  }

  if (win->type == WWIN_FAKE) {
    win->magic = 0;
    free(win);

    TRACEPRINT(("w_delete(%p)\n", win));
    TRACEEND();
    return 0;
  }

  paket = _wreservep(sizeof(DELETEP));
  paket->type   = htons(PAK_DELETE);
  paket->handle = htons(win->handle);

  if (!(ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret))) {

    _wremove_events(win);
    win->magic = 0;
    free(win);
    TRACEPRINT(("w_delete(%p)\n", win));
    TRACEEND();

    return 0;
  }

  TRACEPRINT(("w_delete(%p) -> %i\n", win, ret));
  TRACEEND();
  return ret;
}
