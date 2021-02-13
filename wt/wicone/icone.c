/*
 * A bit more complete icon editor than my earlier one. I added:
 * - different icon sizes.
 * - fileselector for saving.
 * - pbm loading.
 *
 * This uses iconedit widget for editing and PBM image format for
 * loading / saving + tests my getblock fixes...
 *
 * As iconedit widget doesn't support colors, this does neither...
 *
 * (w) 1997 by Eero Tamminen
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

BITMAP *Bitmap;		/* loaded icon if any */
widget_t *Icon;		/* editor widget */

/* use pixel testing instead of w_getblock routine, it certainly works on
 * all resolutions
 */
/* #define PIXEL_TESTING */


#ifdef PIXEL_TESTING

/* test icon pixels and set them in a BITMAP being composed */
static BITMAP *
get_icon(WWIN *win, int x, int y, int w, int h)
{
	ulong *datap, bit;
	int xx, yy, dlongs;
	static BITMAP bm;

	bm.width = w;
	bm.height = h;
	bm.type = BM_PACKEDMONO;
	bm.unitsize = 4;
	dlongs = (w + 31) / 32;
	bm.upl = dlongs;
	if (!(bm.data = calloc(1, dlongs * 4 * h))) {
		return NULL;
	}
	datap = bm.data;
	for (yy = 0; yy < h; yy++) {
		bit = 0x80000000UL;
		for (xx = 0; xx < w; xx++) {
			if (w_test(win, x + xx, y + yy)) {
				*datap |= bit;
			}
			bit >>= 1;
			if (!bit) {
				bit = 0x80000000UL;
				datap++;
			}
		}
		if (bit != 0x80000000UL) {
			datap++;
		}
	}
	return &bm;
}

#else

static BITMAP *
get_icon(WWIN *win, int x, int y, int w, int h)
{
	BITMAP *bm, *tmp;
	if (!(bm = w_getblock(win, x, y, w, h))) {
		return NULL;
	}
	if (bm->type != BM_PACKEDMONO) {
		/* color 0 should be background */
		rgb_t *rgb = bm->palette;
		rgb->red = rgb->green = rgb->blue = 255;
		memset(++rgb, 0, (bm->colors - 1) * sizeof(rgb_t));

		if (!(tmp = w_convertBitmap(bm, BM_PACKEDMONO, 2))) {
			w_freebm(bm);
			return NULL;
		}
		if (bm != tmp) {
			w_freebm(bm);
			bm = tmp;
		}
	}
	return bm;
}

#endif

static void
save_cb (widget_t * button, int down)
{
	long w, h, x, y;
	BITMAP *bm;
	char *path;

	if(down) {
		return;
	}
	wt_getopt(Icon,
		WT_ICON_XPOS, &x,
		WT_ICON_YPOS, &y,
		WT_ICON_WIDTH, &w,
		WT_ICON_HEIGHT, &h,
		WT_EOL);

	path = wt_fileselect(NULL, " Save icon: ", "", "*.icon", "");
	if (!path) {
		return;
	}
	bm = get_icon(wt_widget2win(Icon), x, y, w, h);
	if (!bm || w_writepbm(path, bm) < 0) {
		wt_dialog(NULL, "Unable to save icon!", WT_DIAL_WARN,
			" Warning... ", "Cancel", NULL);
	}
	if (bm) {
		w_freebm(bm);
	}
	free(path);
}

static void
icon_mode(int mode)
{
	long x, y, w, h;
	WWIN *win;
	int old;

	wt_getopt(Icon,
		WT_ICON_XPOS, &x,
		WT_ICON_YPOS, &y,
		WT_ICON_WIDTH, &w,
		WT_ICON_HEIGHT, &h,
		WT_EOL);

	win = wt_widget2win(Icon);
	old = w_setmode(win, mode);
	w_pbox(win, x, y, w, h);
	w_setmode(win, old);

	wt_setopt(Icon, WT_REFRESH, NULL, WT_EOL);
}

static void
clear_cb (widget_t *wp, int down)
{
	if (!down) {
		icon_mode(M_CLEAR);
	}
}

static void
fill_cb (widget_t *wp, int down)
{
	if (!down) {
		icon_mode(M_DRAW);
	}
}

static void
invert_cb (widget_t *wp, int down)
{
	if (!down) {
		icon_mode(M_INVERS);
	}
}

static void
undo_cb (widget_t *wp, int down)
{
	long x, y;
	if (Bitmap && !down) {
		wt_getopt(Icon,	WT_ICON_XPOS, &x, WT_ICON_YPOS, &y, WT_EOL);
		w_putblock(Bitmap, wt_widget2win(Icon), x, y);
		wt_setopt(Icon, WT_REFRESH, NULL, WT_EOL);
	}
}

static int
initialize(int argc, char *argv[], long *width, long *height, long *zoom, BITMAP **bm)
{
	int size = 2, idx = 0;

	*zoom = 0;
	while(++idx < argc && argv[idx][0] == '-' && idx+1 < argc) {
		switch (argv[idx][1]) {
			case 'z':
				*zoom = atoi(argv[++idx]);
				if (*zoom < 4 || *zoom > 12) {
					return -1;
				}
				break;
			case 's':
				size = atoi(argv[++idx]);
				if (size < 1 || size > 6) {
					return -1;
				}
				break;
			default:
				return -1;
		}
	}
	/* 4/3 is a standard screen ratio */
	*width  = 4 << size;
	*height = 3 << size;

	if (idx == argc) {
		*bm = NULL;
		return 0;
	}
	if (argc - idx != 1) {
		return -1;
	}
	if ((*bm = w_readpbm(argv[idx]))) {
		*width  = (*bm)->width;
		*height = (*bm)->height;
		return 0;
	}
	fprintf(stderr, "\nIcon `%s' loading failed\n", argv[idx]);
	return -1;
}

int main(int argc, char *argv[])
{
	widget_t *top, *shell, *hpane, *vpane,
		*clear, *fill, *inv, *undo, *save;
	long width, height, zoom;

	if (initialize(argc, argv, &width, &height, &zoom, &Bitmap) < 0) {
		fprintf(stderr, "\nusage: %s [options] [<icon PBM>]\n", *argv);
		fprintf(stderr, "\t-z <zoom>	zoom can be between 4-12\n");
		fprintf(stderr, "\t-s <size>	size can be between 1-6\n");
		return -1;
	}

	top   = wt_init();
	shell = wt_create(wt_shell_class, top);
	hpane = wt_create(wt_pane_class, shell);
	vpane = wt_create(wt_pane_class, hpane);
	clear = wt_create(wt_button_class, vpane);
	fill  = wt_create(wt_button_class, vpane);
	inv   = wt_create(wt_button_class, vpane);
	undo  = wt_create(wt_button_class, vpane);	
	save  = wt_create(wt_button_class, vpane);
	Icon  = wt_create(wt_iconedit_class, hpane);
	if (!Icon) {
		return -2;
	}
	wt_setopt (shell, WT_LABEL, " Icon Editor ", WT_EOL);

	wt_setopt (clear, WT_LABEL, "Clear",  WT_ACTION_CB, clear_cb,  WT_EOL);
	wt_setopt (fill,  WT_LABEL, "Fill",   WT_ACTION_CB, fill_cb,   WT_EOL);
	wt_setopt (inv,   WT_LABEL, "Invert", WT_ACTION_CB, invert_cb, WT_EOL);
	wt_setopt (undo,  WT_LABEL, "Undo",   WT_ACTION_CB, undo_cb,   WT_EOL);
	wt_setopt (save,  WT_LABEL, "Save",   WT_ACTION_CB, save_cb,   WT_EOL);

	wt_setopt (Icon,
		WT_ICON_WIDTH, &width,
		WT_ICON_HEIGHT, &height,
		WT_EOL);

	if (zoom) {
		wt_setopt (Icon, WT_UNIT_SIZE, &zoom, WT_EOL);
	}

	width = AlignFill;
	wt_setopt(vpane, WT_ALIGNMENT, &width, WT_EOL);

	width = OrientHorz;
	height = 6;
	wt_setopt(hpane,
		WT_ORIENTATION, &width,
		WT_HDIST, &height,
		WT_EOL);

	if (wt_realize(top) < 0) {
		return -3;
	}
	/* set icon */
	undo_cb(NULL, 0);
	wt_run ();
	return 0;
}
