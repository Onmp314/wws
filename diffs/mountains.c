/*
 * A replacement for XMountains 'X_graphics.c' file allowing one to
 * run XMountains fractal mountain generator under W.
 *
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>
#include "paint.h"

/* globals used by xmountains.c */
char *display = NULL;       /* name of display to open, NULL for default */
char *geom = NULL;          /* geometry of window, NULL for default */
int quit_xmount = 0;

#define WIN_PROPERTIES	(W_TITLE|W_MOVE|EV_MOUSE)
#define MAX_COLORS	256

/* local globals */
static WWIN *Win;
static uchar *Mapping;
static short Oldcol = -1;
static int x0, y0, y1;

/* mode (color/mono) specific functions */
static short (*SetColorFn)(WWIN*, short);
static short (*PlotFn)(WWIN*, short, short);
static short (*VlineFn)(WWIN*, short, short, short);


void zap_events(int snooze)
{
	WEVENT *ev;
	if (quit_xmount || (ev = w_queryevent(NULL, NULL, NULL, snooze))) {
		w_exit();
		exit(0);
	}
}


void finish_graphics(void)
{
	w_exit();
}


void init_graphics(int use_root, int background, int clear,
		   int *width, int *height, int ncols,
		   ushort *red, ushort *green, ushort *blue)
{
	short wd, ht, wx = UNDEF, wy = UNDEF;
	static WSERVER *wserver = NULL;
	rgb_t *palette;
	long value;

	if (!wserver && !(wserver = w_init())) {
		fprintf(stderr, "Unable to connect W server\n");
		exit(-1);
	}

	if (use_root) {
		Win = WROOT;
		wd = wserver->width;
		ht = wserver->height;
		w_setmode(Win, M_DRAW);
		w_pbox(Win, 0, 0, wd, ht);
	} else {
		if (geom) {
			scan_geometry(geom, &wd, &ht, &wx, &wy);
		} else {
			wd = wserver->width - 8;
			ht = wserver->height - 20;
		}
		if (!Win && !(Win = w_create(wd, ht, WIN_PROPERTIES))) {
			fprintf(stderr, "unable to create a W window\n");
			w_exit();
			exit(-1);
		}
		w_setmode(Win, M_DRAW);
		w_pbox(Win, 0, 0, wd, ht);
	}
	*width = wd;
	*height = ht;

	if (ncols > MAX_COLORS) {
		ncols = MAX_COLORS;
	}
	
	/* if enough colors, try mapping them
	 */
	if (ncols <= (1 << wserver->planes) &&
	    (palette = malloc(sizeof(rgb_t) * ncols))) {
		short index = ncols;

		palette += ncols;
		while (--index >= 0) {
			palette--;
			palette->red   = red[index] >> 8;
			palette->green = green[index] >> 8;
			palette->blue  = blue[index] >> 8;
		}
		Mapping = w_allocMap(Win, ncols, palette, NULL);
		free(palette);
	}

	/* has to be done after color mapping allocations so that user
	 * doesn't need to do a focus change to see correct colors....
	 */
	if(!use_root && w_open(Win, wx, wy) < 0) {
		fprintf(stderr, "unable to open a W window\n");
		w_exit();
		exit(-1);
	}

	if (Mapping) {
		SetColorFn = w_setForegroundColor;
		VlineFn = w_vline;
		PlotFn = w_plot;
		return;
	}

	if(!(Mapping = malloc(sizeof(*Mapping) * ncols))) {
		fprintf(stderr, "unable to alloc mapping\n");
		w_exit();
		exit(-1);
	}

	while (--ncols >= 0) {
		value = red[ncols] + green[ncols] + blue[ncols];
		Mapping[ncols] = MAX_GRAYSCALES*value / (3*((1<<16)-1));
	}
	SetColorFn = w_setpattern;
	VlineFn = w_dvline;
	PlotFn = w_dplot;
}


void scroll_screen(int dist)
{
	int reverse = 0;
	if (dist < 0) {
		reverse = 1;
		dist = -dist;
	}
	if (dist > Win->width) {
		dist = Win->width;
	}
	/* copy data and blank new area */
	if (reverse) {
		w_bitblk(Win, 0, 0, Win->width - dist, Win->height, dist, 0);
		w_pbox(Win, 0, 0, dist, Win->height);
	} else {
		w_bitblk(Win, dist, 0, Win->width - dist, Win->height, 0, 0);
		w_pbox(Win, Win->width - dist, 0, dist, Win->height);
	}
}


void update(void)
{
	if (y0 != y1) {
		(*VlineFn)(Win, x0, y1, y0);
	} else {
		(*PlotFn)(Win, x0, y0);
	}
}


void plot_pixel(int x, int y, short value)
{
	value = Mapping[value];

	if (value == Oldcol && x == x0) {
		y1 = y;
	} else {
		if (Oldcol >= 0) {
			update();
		}
		(*SetColorFn)(Win, value);
		Oldcol = value;
		x0 = x;
		y0 = y;
		y1 = y;
	}
}


void flush_region(int x, int y, int w, int h)
{
	/* flush outstanding plots */
	update();
	Oldcol = -1;
	w_flush();
}

