/*
 * cracks.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- WSaver module: 'lightning flashes'
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <Wlib.h>
#include "wsaver.h"

#define LENGTH	56		/* line segment lenght */
#define BRANCH	30		/* 1/x possibility for split */


static void draw_cracks(int ox, int oy, int count)
{
	int len, nx, ny;

	/* branching divides with 'count', so has to be > 0 */
	while (--count > 0) {

		len = random() % LENGTH + 1;
		nx = ox + random() % (2*len) - len;
		ny = oy + random() % len;

		if (nx < 0 || nx >= swidth || ny >= sheight) {
			return;
		}
		w_line(win, ox, oy, nx, ny);

		ox = nx;
		oy = ny;
		if (random() % BRANCH == 0) {
			int c = count + random() % count/2 - count/4;
			draw_cracks(ox, oy, c);
		}
	}
}


void save_cracks(void)
{
	int count, ox = 0, oy = 0, idx = 0;
	long wait = timeout;
	time_t seed;
	WEVENT *ev;

	count = sheight/16;
	seed = time(NULL);
	for (;;) {

		idx = (idx + 1) % 5;
		switch (idx) {
			case 0:
				/* clear cracks */
				w_setmode(win, M_DRAW);
				w_pbox(win, 0, 0, swidth, sheight);
				wait = random() % 4001 + 400;
				seed = time(NULL);
				break;
			case 1:
				/* draw first crack */
				w_setmode(win, M_CLEAR);
				wait = timeout * 5;
				ox = random() % swidth/2 + swidth/4;
				oy = 0;
				break;
			case 2:
				/* draw second crack */
				w_setmode(win, M_INVERS);
				ox++;
				oy++;
				break;
			case 3:
				/* clear second crack */
				break;
			case 4:
				/* redraw second crack */
				w_setmode(win, M_CLEAR);
				break;
		}

		if (idx) {
			srandom(seed);
			draw_cracks(ox, oy, count);
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
