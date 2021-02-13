/*
 * bounce.c, a part of the W Window System
 *
 * Copyright (C) 1997 by The GGI Project and
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- WSaver module: 'bouncing ball'
 * 
 * NOTES
 * - This is heavily modified from GPL GGI demo screen saver module.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <Wlib.h>
#include "wsaver.h"

#define RADIUS	5

void save_bounce(void)
{
	/* color indeces, screen co-ordinates and old positions */
	short idx, gray, xx, yy, ox[MAX_GRAYSCALES], oy[MAX_GRAYSCALES];
	/* position and move vector */
	float x, y, dx, dy;
	WEVENT *ev;
	long wait;

	x = xx = swidth/2;
	y = yy = 2*RADIUS;

	for (idx = 0; idx < MAX_GRAYSCALES; idx++) {
		ox[idx] = oy[idx] = RADIUS;
	}

	wait = timeout / 4;
	for (;;) {
		/* move vector (speed x: [-2,2], y: [-0.6,1.4]) */
		dx = (random()&1024) / 128.0 - 4.0;
		dy = (random()&1024) / 128.0 - 1.2;
		for (;;) {
			/* move  */
			x += dx;
			y += dy;
			/* bounce from walls */
			if (x < 2*RADIUS || x >= swidth-2*RADIUS) {
				dx = -dx;
				x += dx;
			}
			if (y < 2*RADIUS || y > sheight-2*RADIUS) {
				dy = -dy;
				y += dy;
			}
			/* 'gravity' pulls down... */
			if (y < sheight-4*RADIUS) {
				dy += 0.02;
			}
			/* 'frictions' slows down */
			dx *= 0.999;
			dy *= 0.999;

			/* wait a bit */
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

			/* slowed down sidewise, get new speed? */
			if (fabs(dx) < 0.1) {
				break;
			}
			/* position differs from screen co-ordinated? */
			if (xx == x && yy == y) {
				continue;
			}

			/* color index */
			idx = idx % (MAX_GRAYSCALES-2) + 1;
			
			w_setmode(win, M_DRAW);
			w_pcircle(win, ox[idx], oy[idx], RADIUS);
			ox[idx] = xx = x;
			oy[idx] = yy = y;

			gray = 2*idx - MAX_GRAYSCALES;
			if (gray < 0) {
				gray = -gray;
			}
			w_setpattern(win, gray);
			w_setmode(win, M_CLEAR);
			w_dpcircle(win, xx, yy, RADIUS);
		}
	}
}
