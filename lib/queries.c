/*
 * queries.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- request window, mouse or server information
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


short w_querywinsize(WWIN *win, short effective, short *width, short *height)
{
  const char *cptr;
  QWINSZP *paket;
  S3RETP *rpaket;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_querywinsize(%p,%i,%p,%p) -> %s\n",\
		win, effective, width, height, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(QWINSZP));
  paket->type = htons(PAK_QWINSZ);
  paket->handle = htons(win->handle);
  paket->effective = htons(effective);

  rpaket = (S3RETP *)_wait4paket(PAK_S3RET);
  if (width) *width = ntohs(rpaket->ret[0]);
  if (height) *height = ntohs(rpaket->ret[1]);

  TRACEPRINT(("w_querywinsize(%p,%i,%p,%p) -> %i,%i\n",\
	      win, effective, width, height,\
	      ntohs(rpaket->ret[0]), ntohs(rpaket->ret[1])));

  TRACEEND();

  return ntohs(rpaket->ret[2]);
}


short w_querywindowpos(WWIN *win, short effective, short *x0, short *y0)
{
  const char *cptr;
  QWPOSP *paket;
  S3RETP *rpaket;
  short ret;

  TRACESTART();

  if ((cptr = _check_window(win))) {
    TRACEPRINT(("w_querywindowpos(%p,%i,%p,%p) -> %s\n",\
		win, effective, x0, y0, cptr));
    TRACEEND();
    return -1;
  }

  paket = _wreservep(sizeof(QWPOSP));
  paket->type = htons(PAK_QWPOS);
  paket->handle = htons(win->handle);
  paket->effective = htons(effective);

  rpaket = (S3RETP *)_wait4paket(PAK_S3RET);
  if (x0) {
    *x0 = ntohs(rpaket->ret[0]);
  }
  if (y0) {
    *y0 = ntohs(rpaket->ret[1]);
  }
  ret = ntohs(rpaket->ret[2]);

  TRACEPRINT(("w_querywindowpos(%p,%i,%p,%p) -> %i,%i\n",\
	      win, effective, x0, y0,\
	      ntohs(rpaket->ret[0]), ntohs(rpaket->ret[1])));
  TRACEEND();
  return ret;
}


short w_querystatus(STATUS *st, short index)
{
  QSTATUSP *paket;
  RSTATUSP *rpaket;
  short ret;

  TRACESTART();

  paket = _wreservep(sizeof(QSTATUSP));
  paket->type  = htons(PAK_QSTATUS);
  paket->index = htons(index);

  rpaket = (RSTATUSP *)_wait4paket(PAK_RSTATUS);
  if (st) {
    st->ip_addr  = ntohl(rpaket->status.ip_addr);
    st->pakets   = ntohl(rpaket->status.pakets);
    st->bytes    = ntohl(rpaket->status.bytes);
    st->totalWin = ntohs(rpaket->status.totalWin);
    st->openWin  = ntohs(rpaket->status.openWin);
  }

  ret = ntohs(rpaket->ret);

  TRACEPRINT(("w_querystatus(%p,%i) -> %i\n",\
	      st, index, ntohs(rpaket->ret)));
  TRACEEND();
  return ret;
}


short w_querymousepos(WWIN *win, short *x0, short *y0)
{
  QMPOSP *paket;
  S3RETP *rpaket;
  short ret;

  TRACESTART();

  paket = _wreservep(sizeof(QMPOSP));
  paket->type = htons(PAK_QMPOS);
  if (win) {
    paket->handle = htons(win->handle);
  } else {
    paket->handle = htons(0);
  }

  rpaket = (S3RETP *)_wait4paket(PAK_S3RET);
  if (x0)
    *x0 = ntohs(rpaket->ret[0]);
  if (y0)
    *y0 = ntohs(rpaket->ret[1]);

  ret = ntohs(rpaket->ret[2]);

  TRACEPRINT(("w_querymousepos(%p,%p,%p) -> %i,%i\n", win, x0, y0,\
	      ntohs(rpaket->ret[0]), ntohs(rpaket->ret[1])));
  TRACEEND();
  return ret;
}
