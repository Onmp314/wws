/*-----------------------------------------------------------------------
 * Warp-GGI
 *	
 *	General Graphics Interface demo: Realtime picture 'gooing'
 *
 *	written by Emmanuel Marty <emarty@mirus.fr> November 1st, 1997
 *
 *	Released under GNU Public License
 *
 * CHANGES
 *
 *	Eero Tamminen 3/98:
 *	- Converted for Wlib PBM functions, graycale and the W window
 *	  system.  Changed initialization calculations from floats to
 *	  shorts (takes half the memory and is *much* faster if you
 *        don't have FPU).
 *	Eero Tamminen 6/98:
 *	- Added color mapping for color W servers.
 *-----------------------------------------------------------------------
 */

#include <stdio.h>
#include <stddef.h>		/* offsetof() */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>

#define FSDITHER_MONO	/* much nicer for monochrome servers */
#undef  SET_BG		/* single background color OR 'rubber walls' */

#define WIN_FLAGS	(W_MOVE | W_TITLE | W_CLOSE | EV_KEYS)

typedef uchar pix_t;

struct warp {
	int width, height, depth;
	int srclinelen, destlinelen;
	pix_t **offstable;
	short *disttable;
	pix_t *source;
	pix_t *framebuf;
	short ctable [1024];
	short sintable [1024+256];
};


static void initSinTable (struct warp *w)
{
	short	*tptr, *tsinptr;
	float	i;

	tsinptr = tptr = w->sintable;

	for (i = 0; i < 1024; i++) {
		*tptr++ = qsin (i*180 / 512) * 32763;
	}
	for (i = 0; i < 256; i++) {
		*tptr++ = *tsinptr++;
	}
}

static void initOffsTable (struct warp *w)
{
	int	height, len, y;
	pix_t	*source, **offptr;

	offptr = w->offstable;
	height = w->height;
	source = w->source;
	len    = w->srclinelen;

	for (y = 0; y < height; y++) {
		*offptr++ = source;
		source += len;
	}
}

static void initDistTable (struct warp *w)
{
	int	halfw, halfh, x, yy, y, m;
	short	*distptr;

	halfw = w->width >> 1;
	halfh = w->height >> 1;

	distptr = w->disttable;

	m = isqrt(halfw*halfw + halfh*halfh);

	for (y = -halfh; y < halfh; y++) {
		yy = y * y;
		for (x = -halfw; x < halfw; x++) {
			*distptr++ = ((isqrt(x*x+yy) << 9) / m) << 1;
		}
	}
}

static struct warp *
initWarp (int width, int height, int depth, int srclinelen,
	pix_t *source, pix_t *destination)
{
	int		bytespp;
	struct warp	*w;

	if ( (w = (struct warp *) malloc (sizeof (struct warp))) ) {
		if ( (w->offstable = malloc (height * sizeof (pix_t *))) ) {
			bytespp = (depth == 8) ? 1 : 4;

			if ( (w->disttable = malloc (width *
                                                     height *
						     sizeof (short))) ) {
				w->width = width;
				w->height = height;
				w->depth = depth;
				w->framebuf = destination;
				w->srclinelen = srclinelen;
				w->source = source;

				initSinTable (w);
				initOffsTable (w);
				initDistTable (w);

				return (w);
			}
			free (w->offstable);
		}
		free (w);
	}
	return (NULL);
}

static void disposeWarp (struct warp *w)
{
	if (w) {
		free (w->disttable);
		free (w->offstable);
		free (w);
	}
}

static void doWarp8bpp (struct warp *w, int xw, int yw, int cw, int bg)
{
	register short *ctable, *ctptr, *distptr;
	register pix_t *destptr, **offstable;
	register int x,y, dx,dy, maxx, maxy;
	short *sintable;

	ctptr = ctable = &(w->ctable[0]);
	sintable = &(w->sintable[0]);

	for (dx = 0, x = 0; x < 512; x++) {
		y = (dx >> 3) & 0x3FF;
		*ctptr++ = ((sintable[y] * yw) >> 15);
		*ctptr++ = ((sintable[y+256] * xw) >> 15);
		dx += cw;
	}

	maxx = w->width;
	maxy = w->height;
	offstable = w->offstable;
	destptr = w->framebuf + maxx * maxy;
	distptr = w->disttable;

	y = maxy;
 	while (--y >= 0) {
		x = maxx;
		while (--x >= 0) {
			ctptr = ctable + *distptr++;
			dy = y - *ctptr++;
			dx = x - *ctptr;

#ifdef SET_BG
			if (dx >= 0 && dy >= 0 && dx < maxx && dy < maxy) {
				*--destptr = offstable[dy][dx];
			} else {
				/* set 'out of range' color index */
				*--destptr = bg;
			}
#else
			if (dx < 0) {
				dx = 0;
			} else if (dx >= maxx) {
				dx = maxx-1;
			}
			if (dy < 0) {
				dy = 0;
			} else if (dy >= maxy) {
				dy = maxy-1;
			}
			*--destptr = offstable[dy][dx];
#endif
		}
        }
}


static void usage(void)
{
	fprintf(stderr, "Warp by Emmanuel Marty <emarty@mirus.fr>\n");
	fprintf(stderr, "W port by Eero Tamminen 3/98\n");
	fprintf(stderr, "\nRelased under GNU Public License.\n\n");
	fprintf(stderr, "Usage: warp [-r] <PGM file>\n");
	fprintf(stderr, "       djpeg -grayscale <JPEG file> | warp [-r]\n");
	fprintf(stderr, "       djpeg -colors N <JPEG file>  | warp [-r]\n");
	fprintf(stderr, "Latter is for color W servers where N is the number\n");
	fprintf(stderr, "of colors on the server minus two.\n");
}

int main (int argc, char **argv)
{
	WWIN		*win;
	WSERVER		*server;
	BITMAP		*bm1, *bm2;
	int		idx, xoff, yoff, xw, yw, cw, tval, stride, bg, use_root;
	char		*picname;
	uchar		*colmap;
	struct warp	*warp;
#ifdef FSDITHER_MONO
	uchar		graymap[256];
	const char	*error;
#endif

	use_root = bg = idx = 0;
	while (++idx < argc) {
		if (argv[idx][0] == '-') {
			switch (argv[idx][1]) {
				case 'r':
					use_root = 1;
					break;
				case 'b':
#ifndef SET_BG
					fprintf(stderr, "SET_BG compile option not defined!\n");
#endif
					if (++idx < argc) {
						bg = atoi(argv[idx]) & 0xff;
						break;
					}
					/* error */
				default:
					usage();
					return 1;
			}
		} else {
			break;
		}
	}

	if (argc > idx) {
		picname = argv[idx];
	} else {
		picname = NULL;		/* -> stdin */
	}

	if (!(bm1 = w_readpbm(picname))) {
		fprintf(stderr, "PBM `%s'reading failed!\n", picname);
		return -1;
	}
	if (bm1->type != BM_DIRECT8) {
		fprintf(stderr, "Only 8-bit images supported!\n");
		return -1;
	}

	/* actually we would only need to copy bitmap header and palette,
	 * not the data.  It's just easier to use a readymade function...
	 */
	if (!(bm2 = w_copybm(bm1))) {
		fprintf(stderr, "BITMAP allocation failed!\n");
		return -1;
	}

	stride = bm1->upl * bm1->unitsize;
	if (!(warp = initWarp(stride, bm1->height,
			      bm1->planes, stride,
			      (pix_t *)bm1->data,
			      (pix_t *)bm2->data))) {
		fprintf(stderr, "warp alloc(s) failed!\n");
		return -1;
	}

	if (!(server = w_init())) {
		fprintf(stderr, "W intialization failed!\n");
		return -1;
	}

	if (use_root) {
		win = WROOT;
		xoff = (win->width - bm1->width) / 2;
		yoff = (win->height - bm1->height) / 2;
	} else {
		if (!(win = w_create(bm1->width, bm1->height, WIN_FLAGS))) {
			fprintf(stderr, "Unable to create a W window!\n");
			return -1;
		}
		xoff = 0;
		yoff = 0;
	}

	colmap = w_allocMap(win, bm1->colors, bm1->palette, NULL);
	if (colmap) {
		w_mapData(bm1, colmap);
		/* don't zero the pointer, it's used as a flag later on */
		free(colmap);

#ifdef FSDITHER_MONO
	} else {
		for (idx = 0; idx < 256; idx++) {
			graymap[idx] = idx;
		}
		w_ditherOptions(graymap, 0);
#endif
	}

	if (!use_root) {
		/* have to do this here because window color allocations
		 * should be done before window is opened.
		 */
		w_open(win, UNDEF, UNDEF);
	}

	tval = 0;
	do {
		xw  = (int) (qsin((tval+100)*180/128) * 30);
		yw  = (int) (qsin((tval)*180/256) * -35);
		cw  = (int) (qsin((tval-70)*180/64) * 50);
		xw += (int) (qsin((tval-10)*180/512) * 40);
		yw += (int) (qsin((tval+30)*180/512) * 40);
		tval = (tval+1) & 511;

		doWarp8bpp (warp, xw, yw, cw, bg);

#ifdef FSDITHER_MONO
		if (!colmap) {
			/* copy (most of) header from bm1 */
			memcpy(bm2, bm1, offsetof(BITMAP,data));

			/* FS-dither to monochrome *in-place* (changes header)
			 */
			fs_direct2mono(bm2, 1, &error);

			/* otherwise w_putblock() will use not-in-place
			 * ordered dithering for the monochrome server.
			 * It doesn't look as good but is slightly faster.
			 */
		}
#endif
		w_putblock(bm2, win, xoff, yoff);

	} while (!w_queryevent(NULL, NULL, NULL, 0));

	disposeWarp(warp);
	w_freebm(bm2);
	w_freebm(bm1);
	w_exit();

	return 0;
}
