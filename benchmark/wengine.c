/*
 * W engine, a part of the W Window System
 *
 * Copyright (C) 1996 by Kay Römer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- A whimsical W server benchmarking program
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "Wlib.h"

WWIN *win, *bm;

#define RAD	30
#define CYLWD	40
#define COLHT	30
#define BORDER	20
#define PLLEN	(2*RAD+COLHT/2+BORDER)
#define CYLHT	(RAD+PLLEN+COLHT/2)

#define WD	(2*(RAD+BORDER)+4)
#define HT	(RAD+CYLHT+2*BORDER+4)

#define CX	(WD/2)
#define CY	(HT-RAD-BORDER-2)

static void draw_engine (double alpha)
{
	short x2, y2, x3, y3, pts[2*4];
	double sin_a, cos_a, sin_b, cos_b, r_sin_a, r_cos_a;

	w_setmode (bm, M_DRAW);
	w_pcircle (bm, CX, CY, RAD + BORDER);
	w_pbox (bm, CX - CYLWD/2 - BORDER/2, CY - CYLHT - BORDER/2,
		CYLWD + BORDER, CYLHT + BORDER/2);

	sin_a = sin (alpha);
	cos_a = cos (alpha);
	r_sin_a = RAD*sin_a;
	r_cos_a = RAD*cos_a;

	cos_b = r_cos_a/PLLEN;
	sin_b = sqrt (1 - cos_b*cos_b);

	x2 = CX + (short)r_cos_a;
	y2 = CY - (short)r_sin_a;

	x3 = CX;
	y3 = CY - (short)(r_sin_a + PLLEN*sin_b);

	w_setmode (bm, M_INVERS);

	w_pcircle (bm, CX, CY, RAD + BORDER/2);
	w_pcircle (bm, CX, CY, 3);
	w_pcircle (bm, x2, y2, 3);
	w_pcircle (bm, x3, y3, 3);

	w_pbox (bm, x3 - CYLWD/2, y3 - COLHT/2, CYLWD, COLHT);

	alpha = acos (cos_b);

	cos_a = 8 * cos (alpha + M_PI/4);
	sin_a = 8 * sin (alpha + M_PI/4);

	cos_b = 8 * cos (alpha - M_PI/4);
	sin_b = 8 * sin (alpha - M_PI/4);

	pts[0] = x2 + (short)cos_a;
	pts[1] = y2 + (short)sin_a;

	pts[2] = x2 + (short)cos_b;
	pts[3] = y2 + (short)sin_b;

	pts[4] = x3 - (short)cos_a;
	pts[5] = y3 - (short)sin_a;

	pts[6] = x3 - (short)cos_b;
	pts[7] = y3 - (short)sin_b;

	w_ppoly (bm, 4, pts);
}

static long
gettime (void)
{
	WEVENT *ev;
	if((ev = w_queryevent(0, 0, 0, 0))) {
		if(ev->type == EVENT_GADGET &&
		   (ev->key == GADGET_EXIT || ev->key == GADGET_CLOSE)) {
			w_delete(win);
			w_delete(bm);
			exit(0);
		}
	}
	return w_gettime();
}

int
main ()
{
	double angle = 0.0;
	long i, start, stop, slen;
	char buf[100];
	WFONT *fp;

	if (!w_init ())
		return 1;
	bm = w_create (WD, HT, W_NOBORDER);
	win = w_create (WD+20, HT+40, W_TITLE|W_MOVE|W_CLOSE);
	w_open (win, UNDEF, UNDEF);
	w_settitle (win, " W Engine ");

	fp = w_loadfont ("fixed", 13, 0);
	if (!fp)
		return 1;
	w_setfont (win, fp);

	sprintf (buf, "%5d RPM", 0);
	slen = w_strlen (fp, buf);

	start = gettime ();
	for (i=0;;++i) {
		draw_engine (angle);
		w_bitblk2 (bm, 0, 0, WD, HT, win, 10, 30);
		angle += M_PI/8;
		stop = gettime ();
		if (stop - start > 5000) {
			sprintf (buf, "%5ld RPM", i*1000*60/(16*(stop-start)));
			start = stop;
			i = 0;
			w_printstring (win, (WD+20-slen)/2,
				(30-fp->height)/2, buf);
		}
	}
	return 0;
}
