/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * misc utility routines.
 *
 * $Id: util.h,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#ifndef _UTIL_H
#define _UTIL_H

#undef MIN
#undef MAX
#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

extern char *xstrlwr (char *);
extern char *xstrndup (const char *, int len);
extern char *xstrfind (char *, const char *);
extern char *xstrrfind (char *, const char *);
extern char *xstrerror (int err);

#endif
