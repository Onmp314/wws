/*
 * util.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- TRACE debugging utility functions
 *
 * CHANGES
 * ++eero, 5/98:
 * - defined only if TRACE is set.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"


#ifdef TRACE
int _wtrace = 0, _traceIndent = -2;

/* for TRACEPRINT macro */
void _wspaces(void)
{
	int spaces = _traceIndent;
	while(spaces-- > 0) {
		putchar(' ');	/* fputc(' ',stdout); */
	}
}
#endif

void w_trace(short flag)
{
#ifdef TRACE
	_wtrace = !!flag;

	TRACESTART();
	TRACEPRINT(("w_trace(%i)\n", flag));
	TRACEEND();
#else
	fprintf(stderr, "w_trace: tracing not implemented\n");
#endif
}

