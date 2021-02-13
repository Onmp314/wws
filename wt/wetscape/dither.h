/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * dithering routines.
 *
 * $Id: dither.h,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#ifndef _DITHER_H
#define _DITHER_H

#include <sys/types.h>

/*
 * floyd-steinberg dithering
 */
static inline u_char
dither_fs (int x, int y, u_char col, int (*buf)[][2], int maxval)
{
	int err, val, lum;

	y &= 1;
	lum = col + ((*buf)[x+1][y] >> 4);
	if (lum >= maxval/2) {
		val = 1;
		err = lum - maxval;
	} else {
		val = 0;
		err = lum;
	}
	(*buf)[x+1][y  ]  = 0;
	(*buf)[x+2][y  ] += 7*err;
	(*buf)[x+2][y^1] += 1*err;
	(*buf)[x+1][y^1] += 5*err;
	(*buf)[x  ][y^1] += 3*err;

	return val;
}

extern u_char dither8[16][16];

/*
 * ordered dithering
 */
static inline u_char
dither_o (int x, int y, u_char col, int maxval)
{
	return ((int)dither8[x&15][y&15]*maxval <= (int)col*256);
}

/*
 * color to grayscale conversion
 */
static inline u_char
col2grey (u_char red, u_char green, u_char blue)
{
	return (307*(u_long)red + 599*(u_long)green + 118*(u_long)blue) >> 10;
}

#endif
 
