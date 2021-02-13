/*
 * init.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- initialize connection to W server and couple of misc functions
 *
 * NOTES
 * the values set to WWIN struct in _wset_defgc() should correspond to
 * values used by W server!
 *
 * CHANGES
 * ++eero, 12/97:
 * - fixed re-initializing bug.
 * ++eero, 5/98:
 * - Added new struct members.
 * ++eero, 6/98:
 * - w_resize() updates WWIN width/height members correctly.
 * ++oddie, 4/99:
 * - gets sysvar cookie when running on MacMiNT (needed for time functions)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


/* MacMiNT timer stuff */
#if defined(__MINT__) && defined(MAC)
#include <osbind.h>
#include "../server/xconout2/mint/locore.h"

#define SIG_SVAR 0x53564152 /*  'SVAR' */

struct sysvars  *sysvar;

static void _fetchsvar(void)
{
	long *cookie;
	long ssp;

	ssp = (long)Super(0L);
	cookie = *(long **)0x5a0L;
	while (cookie[0]) {
		if (cookie[0] == SIG_SVAR)
			sysvar = (struct sysvars *)cookie[1];
		cookie += 2;
	}
	if (sysvar == NULL) {
		fprintf(stderr, "Wlib: can't find SVAR cookie\n");
		exit(1);
	}
	(void)Super((void *)ssp);
}
#endif


WSERVER *w_init(void)
{
  INITP *paket;
  INITRETP *irpaket;
  static WWIN wroot;

  if (getenv("WTRACE")) {
    w_trace(1);
  }

  TRACESTART();

#if defined(__MINT__) && defined(MAC)
  /* get timer from cookies */
  _fetchsvar();
#endif
  
  if (_winitialize() < 0) {
    TRACEPRINT(("w_init() -> NULL\n"));
    TRACEEND();
    return NULL;
  }

  paket = _wreservep(sizeof(INITP));
  paket->type = htons(PAK_INIT);
  paket->uid  = getuid();	/* could be short or long */
  paket->uid  = htonl(paket->uid);

  irpaket = (INITRETP *)_wait4paket(PAK_INITRET);

  _wserver.vmaj   = htons(irpaket->vmaj);
  _wserver.vmin   = htons(irpaket->vmin);
  _wserver.pl     = htons(irpaket->pl);
  _wserver.type   = htons(irpaket->screenType);
  _wserver.width  = htons(irpaket->width);
  _wserver.height = htons(irpaket->height);
  _wserver.planes = htons(irpaket->planes);
  _wserver.flags  = htons(irpaket->flags);
  _wserver.fsize  = htons(irpaket->fsize);
  _wserver.fname  = strdup(irpaket->fname);
  _wserver.sharedcolors = htons(irpaket->sharedcolors);

  if ((_wserver.vmaj != _WMAJ) || (_wserver.vmin != _WMIN)) {
    fprintf(stderr, "Wlib: this binary was compiled for W%iR%i, but\n",
	    _WMAJ, _WMIN);
    fprintf(stderr, "      the server claims to be W%iR%i, update your binary!\n",
	    _wserver.vmaj, _wserver.vmin);
    exit(-99);
  }

  TRACEPRINT(("w_init() -> %p\n", irpaket));

  wroot.magic  = MAGIC_W;
  wroot.handle = 0;
  wroot.width  = _wserver.width;
  wroot.height = _wserver.height;
  wroot.colors = _wserver.sharedcolors;

  /* several different programs may write to ROOT window so you can't trust
   * it to have any certain value!
   */
  wroot.fg = UNDEF;
  wroot.bg = UNDEF;
  wroot.drawmode  = UNDEF;
  wroot.linewidth = UNDEF;
  wroot.textstyle = UNDEF;
  wroot.pattern   = 0xffff;
  WROOT = &wroot;

  TRACEEND();
  return &_wserver;
}


void w_exit(void)
{
  EXITP	*expaket = _wreservep(sizeof(EXITP));

  TRACESTART();
  TRACEPRINT(("w_exit()\n"));

  expaket->type = htons(PAK_EXIT);

  /* this is needed for programs which run other W programs */
  _wexit();

  TRACEEND();
}


short w_null(void)
{
  EXITP *paket = _wreservep(sizeof(EXITP));

  TRACESTART();
  TRACEPRINT(("w_null()\n"));

  paket->type = htons(PAK_NULL);

  TRACEEND();
  return 0;
}


short w_beep(void)
{
  EXITP *paket = _wreservep(sizeof(EXITP));

  TRACESTART();
  TRACEPRINT(("w_beep()\n"));

  paket->type = htons(PAK_BEEP);

  TRACEEND();
  return 0;
}
