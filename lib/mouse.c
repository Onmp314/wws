/*
 * winutil.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Jan Paul Schmidt
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- set and query window mouse pointer (shape)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


short w_getmousepointer (WWIN *win)
{
	const char *cptr;
	GMOUSEP *paket;
	short ret;

	TRACESTART ();

	if ((cptr = _check_window(win))) {
		TRACEPRINT(("w_getmousepointer(%p) -> NULL\n", win));
		TRACEEND();
		return -1;
	}

	paket = _wreservep(sizeof(GMOUSEP));
	paket->type = htons(PAK_GMOUSE);
	paket->handle = htons(win->handle);

	ret = ntohs(((SRETP *)_wait4paket(PAK_SRET))->ret);

	TRACEPRINT(("w_getmousepointer(%p) -> %i\n", win, ret));
	TRACEEND();
	return ret;
}


short w_setmousepointer (WWIN *win, short type, WMOUSE *data)
{
	const char *cptr;
	SMOUSEP *paket;

	TRACESTART ();

	if ((cptr = _check_window(win))) {
		TRACEPRINT(("w_setmousepointer(%p,%i,%p) -> %s\n",
			win, type, data, cptr));
		TRACEEND();
		return -1;
	}

	paket = _wreservep(sizeof(SMOUSEP));
	paket->type = htons(PAK_SMOUSE);
	paket->handle = htons(win->handle);
	paket->mtype = htons(type);

	/* not a predefined one? */
	if (type >= MOUSE_USER) {
		if (!data) {
			TRACEPRINT(("w_setmousepointer(%p,%i,%p) -> no data\n",\
				win, type, data));
			TRACEEND();
			return -1;
		}
		paket->xoff = htons(data->xDrawOffset);
		paket->yoff = htons(data->yDrawOffset);
		memcpy(paket->mask, data->mask, sizeof(paket->mask));
		memcpy(paket->icon, data->icon, sizeof(paket->icon));
	}

	TRACEPRINT(("w_setmousepointer(%p,%i,%p) -> 0\n", win, type, data));
	TRACEEND();

	/* Must flush so mouse pointer will change immediately.
	 */
	w_flush();

	return 0;
}
