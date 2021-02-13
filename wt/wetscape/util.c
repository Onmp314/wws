/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * misc utility routines.
 *
 * $Id: util.c,v 1.3 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include "util.h"

char *
xstrlwr (char *_cp)
{
	char *cp;

	for (cp = _cp; *cp; ++cp)
		*cp = tolower (*cp);
	return _cp;
}

char *
xstrndup (const char *_cp, int len)
{
	char *cp = malloc (len+1);
	if (!cp)
		return NULL;
	strncpy (cp, _cp, len);
	cp[len] = 0;
	return cp;
}

char *
xstrfind (char *cp, const char *chars)
{
	const char *cp2;

	for ( ; *cp; ++cp) {
		for (cp2 = chars; *cp2; ++cp2) {
			if (*cp == *cp2)
				return cp;
		}
	}
	return NULL;
}

char *
xstrrfind (char *_cp, const char *chars)
{
	char *cp;
	const char *cp2;

	for (cp = _cp + strlen (_cp) - 1 ; cp >= _cp; --cp) {
		for (cp2 = chars; *cp2; ++cp2) {
			if (*cp == *cp2)
				return cp;
		}
	}
	return NULL;
}

char *
xstrerror (int err)
{
#ifdef sun
	static char buf[32];
	extern char *sys_errlist[];
	extern int  sys_nerr;

	if (err >= 0 && err < sys_nerr)
		return sys_errlist[err];
	else {
		sprintf (buf, "error %d", err);
		return buf;
	}
#else
	return strerror (err);
#endif
}
