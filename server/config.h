/*
 * config.h, a part of the W Window System
 *
 * Copyright (C) 1994-1998 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- W server configuration defines
 */

#ifndef __W_CONFIG_H
#define __W_CONFIG_H


/* default font offered for clients */
#define WFONT_EXTENSION	"wfnt"
#define DEF_WFONTFAMILY	"fixed"
#define DEF_WFONTSIZE	10

/* fonts that server uses itself by default */
#define DEF_WTITLEFONT	"fixed8.wfnt"
#ifdef WTINYSCREEN
#define DEF_WMENUFONT "fixed8.wfnt"
#else
#define DEF_WMENUFONT	"lucidat11.wfnt"
#endif


/* window border and default palette color indeces.  These should be
 * the first two (but you can swap them :)) as server shared colors
 * are the first ones (and therefore programs wanting to free them
 * need only to know how many of them there are).
 */
#define FGCOL_INDEX	1
#define BGCOL_INDEX	0

/* for compatibility default window mode would better be M_INVERS */
#define DEFAULT_GMODE	M_INVERS


/* some configuration options affecting W server functionality:
 * - Create only 'local' socket
 * - Use 'opaque' window moving
 * - Restrict child windows inside their parents
 * - Draw always to window backup
 */
#define AF_UNIX_ONLY 1
#define REALTIME_MOVING 1
#undef CHILDS_INSIDE_PARENTS
#if defined(SDL) || defined(GGI)
/* window updates work much better with this if underlying graphics
 * library double buffers the output automatically like SDL
 * and GGI do
 */
# define REFRESH 1
#else
# undef REFRESH
#endif


/* the hardcoded window border width (ATM this is just informational,
 * later on it should be configurable).
*/
#define BORDERWIDTH 4

/* mouse acceleration types */
enum {	ACCEL_NONE = 0,
	ACCEL_DOUBLE,		/* 2*delta */
	ACCEL_POWER		/* delta^2 */
};

/* this doesn't work yet */
#undef LAZYMOUSE


/* some restrictions */
#define MAX_LINELEN 256		/* max. config file line lenght */
#define MAXFAMILYNAME 16	/* font family name lenght, divisable by 4 */
#define MAXPRINTS 80		/* printstring packet string size, -"- */
#define MAXPOLYPOINTS 64	/* max. w_poly() points, divisable with 2 */
#define MAXTITLE 64		/* max. window title packet string lenght */
#define MAXFONTS 40		/* max. number of fonts loaded at same time */
#define MAXITEMS 16		/* max. number of root menu items */


/* network options */

#define	SERVERPORT 7777
#define MAXLISTENQUEUE 4
#define UNIXNAME "/tmp/wserver"


/* do _not_ change any of these! */

#define	BUTTON_ALL	7
#define	BUTTON_LEFT	4
#define	BUTTON_MID	2
#define	BUTTON_RIGHT	1
#define	BUTTON_NONE	0

/* client socket input buffer size for loop.c, has to be larger than
 * the largest request packet.
 */
#define	LARGEBUF	4096


/* copyright symbol, for --version option */
/* #define _W_COPYRIGHT_MARK_ "(C)" */
#define _W_COPYRIGHT_MARK_ "\251"

#endif /* __W_CONFIG_H */
