/*
 * hatch.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routine returning requested hatch / line pattern ID
 */

#include <stdio.h>
#include "Wlib.h"
#include "proto.h"


/* patterns for vertical and horizontal lines, rolled
 * left (& 0x0300), right (& 0x8100) or both (& 0x0100).
 */
ushort w_hatch(int type, int width, int times)
{
	ushort id = 0;
	TRACESTART();

	if (type > 2 || width > 8 || times > 3) {
		TRACEPRINT(("w_hatch(%d,%d,%d) -> index out of range\n",\
			type, width, times));
		TRACEEND();
		return 0;
	}

	while (--width >= 0) {
		switch (type) {
			case P_LINE_R:
				id = (id >> 1) | 0x8000;
				break;
			case P_LINE_L:
				id = (id << 1) | 0x200;
				break;
			/* P_HATCH */
			default:
				id = (id >> 1) | 0x100;
		}
	}

	if (times > 1) {
		if (type == P_HATCH) {
			id |= id << 8;
		}
		id |= id >> 8;
		if (times > 2) {
			if (type == P_LINE_R) {
				id |= id >> 4;
			} else {
				id |= id << 4;
			}
		}
	}

	TRACEPRINT(("w_hatch(%d,%d,%d) -> 0x%x\n", type, width, times, id));
	TRACEEND();
	return id;
}
