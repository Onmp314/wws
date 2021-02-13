/*
 * stars.c, a part of the W Window System
 *
 * Copyright (C) 1997 by The GGI Project and
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- WSaver module: 'starfield'
 * 
 * NOTES
 * - This is heavily modified from GPL GGI demo screen saver module.
 * - Original code didn't have any docs so I wasn't quite sure how to get a
 *   'normalized' distance to a 'star'.  So...  If stars don't grow properly
 *   as they get 'nearer', fiddle with 'z' in the redraw loop.
 * - 40 stars is about maximum that fits into current Wlib socket buffer in
 *   one go, so I wouldn't recommend more.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <Wlib.h>
#include "wsaver.h"


#define STARS	40	/* number of stars */
#define BORDER	10

#define rando ((random()&0xfff) / (float)0x1000)

typedef struct { float x, y, z; } pix3d;
typedef struct { short x, y; unsigned moved:1; } pix2d;


void save_stars(void)
{ 
	short x, y, z, ox, oy, idx, xsize, ysize, scale;
	float da, nda, dz, ndz, cx, sx, cz, sz, z0, hlp;
	static pix2d oldpos[STARS];
	static pix3d pix[STARS];
	pix2d *old;
	pix3d *pp;
	WEVENT *ev;
	long wait;

	wait = timeout / 8;
	xsize = swidth;
	ysize = sheight;
	scale = ysize * 4;
	z0 = 2.0*ysize;

	memset(pix, 0, sizeof(pix));
	for(idx = 0; idx < STARS; idx++) {
	      pix[idx].z = -z0 + 50;
	}

	memset(oldpos, 0, sizeof(oldpos));
	da = nda = 0.0;
	dz = ndz = 0.0;
	y = 0;

	for (;;) {
		if (rando < 0.001) {
			nda = (rando-0.5) / 200.0;
		}
		da = (nda + 999.0*da) / 1000.0;
		cx = qcos(da);
		sx = qsin(da);
		if (rando < 0.001) {
			ndz = (rando-0.5) / 200.0;
		}
		dz = (ndz - 10*nda + 989.0*dz) / 1000.0;
		cz = qcos(dz/10);
		sz = qsin(dz/10);

		w_setmode(win, M_DRAW);
		/* calculate new positions, remove old */
		for(idx = 0; idx < STARS; idx++) {

			pp = &pix[idx];
			for (;;) {
				pp->z -= 5.0;

				hlp = pp->y;
				pp->y = -sx*pp->x + cx*hlp;
				pp->x =  cx*pp->x + sx*hlp;

				hlp = pp->z;
				pp->z = -sz*pp->x + cz*hlp;
				pp->x =  cz*pp->x + sz*hlp;

				hlp = z0 / (pp->z + z0);
				x = swidth/2 + pp->x*hlp;
				y = sheight/2 + pp->y*hlp;

				if (pp->z + z0 < 100.0 ||
				    x < BORDER || x >= xsize-BORDER ||
				    y < BORDER || y >= ysize-BORDER) { 
					pp->x = 2*rando*z0 - z0;
					pp->y = 2*rando*z0 - z0;
					pp->z = 10*rando*z0 + z0;
					continue;
				}
				break;
			}

			/* remove those which have moved */
			old = &oldpos[idx];
			ox = old->x;
			oy = old->y;
			if (x != ox || y != oy) {
				w_pcircle(win, ox, oy, 3);
				old->x = x;
				old->y = y;
				old->moved = 1;
			} else {
				old->moved = 0;
			}
		}

		/* redraw new ones */
		w_setmode(win, M_CLEAR);
		for(idx = 0; idx < STARS; idx++) {
			old = &oldpos[idx];
			if (old->moved) {
				z = 3 - (short)pix[idx].z / scale;
				w_pcircle(win, old->x, old->y, z);
			}
		}

		if ((ev = w_queryevent(NULL, NULL, NULL, wait))) {
			switch (ev->type) {

				case EVENT_GADGET:
					if (ev->key == GADGET_EXIT) {
						w_exit();
						exit(0);
					}
					break;

				default:
					return;
			}
		}
	}
}
