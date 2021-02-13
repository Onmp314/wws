/*
 * fontmisc.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routine to separate a font file name into font family, size and style
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"

#include "../server/config.h"	/* MAXFAMILYNAME (default=16) */

/* convert font filename into family name, size and styles */
const char *w_fonttype(const char *filename, short *size, ushort *styles)
{
	static char name[MAXFAMILYNAME + 4];
	int len, idx;

	TRACESTART();

	if(!(filename && size && styles && *filename)) {
		TRACEPRINT(("w_loadfont(%s,%p,%p) -> NULL\n",\
			filename, size, styles));
		TRACEEND();
		return NULL;
	}

	/* remove path */
	idx = len = strlen(filename);
	while (--idx >= 0 && filename[idx] != '/')
		;
	filename += ++idx;
	len -= idx;

	/* remove extension */
	idx = len;
	while(--idx > 0 && filename[idx] != '.')
		;
	if (!idx) {
		idx = len;
	}

	/* get styles */
	*styles = 0;
	for(;--idx > 0;) {
		switch(filename[idx]) {
		case 'b':
			*styles |= F_BOLD;
			break;
		case 'i':
			*styles |= F_ITALIC;
			break;
		case 'l':
			*styles |= F_LIGHT;
			break;
		case 'r':
			*styles |= F_REVERSE;
			break;
		case 'u':
			*styles |= F_UNDERLINE;
			break;
		default:
			goto out_of_loop;
		}
	}

out_of_loop:
	strncpy(name, filename, ++idx);
	name[idx] = 0;

	/* get size and the rest is family name */
	while(--idx > 0 && (name[idx] >= '0' && name[idx] <= '9'))
		;
	*size = atoi(&name[++idx]);
	name[idx] = 0;

	TRACEPRINT(("w_loadfont(%s,%p,%p) -> %s (%d,%d)\n", filename, \
			size, styles, name, *size, *styles));
	TRACEEND();

	return name;
}
