/*
 * W window system interface for John Beale's HF-Lab v0.90.
 *
 * Replaces the 'x_iface.c' file implementing X user interface.
 *
 * HF-Lab is a really nice command line heightfield creator with a X11
 * heightfield previewer which can do light shading.  If you'll replace the
 * x_iface.c file with this, you can then do previews under W before
 * raytracing the heightfield...
 *
 * When using HF-Lab 'view' with monochrome W server, either set 'contrast
 * maximation' ON or use s2 option (slope shading). I myself prefer the
 * p2 drawing density (p1 = no skip).
 *
 * (w) 1997 by Eero Tamminen
 *
 * NOTES:
 *   X_PutImagemap data should be long aligned (evenly divisable by four),
 *   so don't use any funny preview window widths.
 *
 * CHANGES
 * ++eero, 6/98:
 * - Added color functionality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>

#define MAX_COLORS	256
#define WIN_PROPERTIES	(W_MOVE)

static WSERVER *wserver;

static WWIN *win;
static int mapped;
static rgb_t *gray;
static uchar *colmap;


/* If this succeeds (returns 0), it's not supposed to be called again with
 * different values...
 */
int init_graph(int wx, int wy, int ncols)
{
	int value;

	if (!wserver) {
		if (!(wserver = w_init())) {
			fprintf(stderr, "unable to connect W server\n");
			return -1;
		}
	}
	if (!win) {
		if (!(win = w_create(wx, wy, WIN_PROPERTIES))) {
			fprintf(stderr, "unable to create W window\n");
			return -1;
		}
	}

	if (!gray) {
		if (ncols > MAX_COLORS) {
			ncols = MAX_COLORS;
		}
		if (!(gray = malloc(sizeof(rgb_t) * ncols))) {
			fprintf(stderr, "palette alloc failed\n");
			return -1;
		}

		mapped = ncols - 1;
		while (--ncols >= 0) {

			value = 255L * ncols / mapped;
			gray[ncols].red   = value;
			gray[ncols].green = value;
			gray[ncols].blue  = value;
		}
		mapped++;

		colmap = w_allocMap(win, mapped, gray, NULL);
	}

	/* clear to black */
	w_setmode(win, M_DRAW);
	w_setForegroundColor(win, 1);
	w_pbox(win, 0, 0, win->width, win->height);

	if(w_open(win, UNDEF, UNDEF) < 0) {
		fprintf(stderr, "unable to open W window\n");
		return -1;
	}

	printf("using %dx%d window in %d colors.\n", wx, wy, mapped);
	return 0;
}


void close_graph(void)
{
	if (win) {
		w_close(win);
	}
}

int GetMaxX(void)
{
	if (win) {
		return win->width;
	}
	if (wserver) {
		return wserver->width;
	}
	return 0;
}

int GetMaxY(void)
{
	if (win) {
		return win->height;
	}
	if (wserver) {
		return wserver->height;
	}
	return 0;
}

void Gflush(void)
{
	w_flush();
}

static void assert(WWIN *win)
{
	if (!win) {
		fprintf(stderr, "no window to draw to, exiting...\n");
		exit(-1);
	}
}

void Clear(void)
{
	assert(win);
	/* clear to black */
	if (colmap) {
		w_setForegroundColor(win, colmap[0]);
	}
	w_pbox(win, 0, 0, win->width, win->height);
}

void X_Point(int x, int y)
{
	assert(win);
	w_plot(win, x, y);
}

void X_FillPoly(int n, short *points, int linecol, int fillcol)
{
	assert(win);

	if (colmap) {
		/* use colors */

		w_setForegroundColor(win, colmap[fillcol]);
		w_ppoly(win, n, points);
		if (!fillcol) {
			w_setForegroundColor(win, colmap[linecol]);
			w_poly(win, n, points);
		}
	} else {
		/* use patterns */

		w_setpattern(win, MAX_GRAYSCALES * fillcol / mapped);
		w_dppoly(win, n, points);
		if (!fillcol) {
			w_setpattern(win, MAX_GRAYSCALES * linecol / mapped);
			w_dpoly(win, n, points);
		}
	}
}

void X_PutImagemap(uchar *data, int w, int h, int x, int y)
{
	BITMAP bm, *hf = NULL;

	assert(win);

	bm.type = BM_DIRECT8;
	bm.width = w;
	bm.height = h;
	bm.planes = 8;
	bm.unitsize = 1;
	bm.upl = w;

	bm.colors = mapped;
	bm.palette = gray;
	bm.data = data;

	if (colmap) {
		/* map colors to ones we allocated at startup
		 */
		w_mapData(&bm, colmap);
	} else {
		/* if we have to do color conversions, monochrome gets best
		 * results.
		 */
		hf = w_convertBitmap(&bm, BM_PACKEDMONO, 2);
	}
	if (!hf) {
		hf = &bm;
	}
	w_putblock(hf, win, x, y);
	w_flush();
	if (hf != &bm) {
		w_freebm(hf);
	}
}
