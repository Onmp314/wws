/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * image output support routines.
 *
 * $Id: image.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "url.h"
#include "io.h"
#include "image.h"
#include "util.h"

image_t *
image_alloc (url_t *url)
{
	image_t *img = malloc (sizeof (image_t));
	if (!img)
		return NULL;
	memset (img, 0, sizeof (image_t));

	if (url) {
		img->url = url_clone (url);
		if (!url) {
			free (img);
			return NULL;
		}
	}
	img->locked = 1;

	return img;
}

inline void
image_free (image_t *img)
{
	if (img->url)
		url_free (img->url);
	if (img->win)
		w_delete (img->win);
	if (img->buf)
		free (img->buf);
	free (img);
}

void
image_free_list (image_t *img)
{
	image_t *freeme;

	while (img) {
		freeme = img;
		img = img->next;
		image_free (freeme);
	}
}

static interlace_t def_ilace = { 0, 1 };

int
image_getwin (image_t *img, WWIN *dstwin, int wd, int ht,
	interlace_t *ilace, int npasses)
{
	int bpl;

	if (!img->win) {
		/*
		 * we are the first time through. allocate buffers
		 * for transferring the image to the server
		 */
		img->bpl = (wd+7) >> 3;
		img->pad = (4 - (img->bpl & 3)) & 3;
		bpl = img->bpl + img->pad;

		if (npasses == 0) {
			/*
			 * non-interlaced picture. buffer that many lines
			 * as fit into a 1k buffer, but at least one line.
			 */
			img->nlines = MAX (MIN (1024/bpl, ht), 1);

			ilace = &def_ilace;
			npasses = 1;
		} else {
			/*
			 * interlaced picture. cannot buffer more than
			 * one line.
			 */
			img->nlines = 1;
		}
		img->buf = malloc (img->nlines * bpl);
		if (!img->buf)
			return -1;

		img->ilace = ilace;
		img->npasses = npasses;

		img->newlines = 0;

		img->cur_y = ilace[0].start;
		img->cur_pass = 0;
		img->cur_lines = 0;
		img->cur_bytes = 0;
		img->cp = img->buf;
	}
	img->win = w_create (wd, ht, W_NOBORDER);
	if (!img->win) {
		free (img->buf);
		img->buf = NULL;
		return -1;
	}
	img->wd = wd;
	img->ht = ht;
	img->dstwin = dstwin;

	return 0;
}

void
image_flush (image_t *img)
{
	BITMAP bm;
	int ypos;

	if (img->cur_y >= img->ht) {
		/*
		 * putblock() fails if we put an image that is partially
		 * outside the window (which I consider a bug). So
		 * cut off lines that are outside the window
		 */
		img->cur_lines -= img->cur_y - img->ht + 1;
		img->cur_y = img->ht - 1;
	}
	if (img->cur_lines <= 0) {
		/*
		 * Check if there is at least on pixel that must be
		 * putblocke()ed.
		 */
		if (img->cur_lines < 0 || img->cur_bytes == 0)
			return;
		img->cur_lines = 1;
	}

	bm.width    = img->wd;
	bm.height   = img->cur_lines;
	bm.type     = BM_PACKEDMONO;
	bm.planes   = 1;
	bm.unitsize = 4;
	bm.upl      = (bm.width + 31) / 32;
	bm.data     = img->buf;

	ypos = img->cur_y - img->cur_lines + 1;
	w_putblock (&bm, img->win, 0, ypos);

	/*
	 * update number of lines that we have something written to
	 * the bitmap already.
	 */
	if (img->cur_y+1 > img->cur_ht)
		img->cur_ht = img->cur_y+1;

	if (!img->locked) {
		if (img->npasses <= 1) {
			/*
			 * immediately update non-interlace picture
			 */
			w_bitblk2  (img->win, 0, ypos, img->wd, img->cur_lines,
				img->dstwin, img->x, img->y + ypos);
		} else {
			img->newlines += img->ilace[img->cur_pass].offs;
			if (img->newlines > (img->ht >> 3)) {
				/*
				 * update interlaced pictures not after
				 * every line...
				 */
				w_bitblk2 (img->win, 0, 0,
					img->wd, img->cur_ht,
					img->dstwin, img->x, img->y);
				img->newlines = 0;
			}
		}
	}

	/*
	 * check if we go beyond img->ht and start the next pass if so.
	 */
	img->cur_y += img->ilace[img->cur_pass].offs;
	if (img->cur_y >= img->ht) {
		if (++img->cur_pass >= img->npasses) {
			/*
			 * there should be no more lines, really.
			 */
			img->cur_pass = 0;
		}
		img->cur_y = img->ilace[img->cur_pass].start;
	}
	img->cur_lines = 0;
	img->cur_bytes = 0;
	img->cp = img->buf;
}

void
image_done (image_t *img)
{
	if (!img->locked && img->newlines > 0) {
		w_bitblk2 (img->win, 0, 0, img->wd, img->cur_ht,
			img->dstwin, img->x, img->y);
		img->newlines = 0;
	}
	if (img->buf) {
		free (img->buf);
		img->buf = img->cp = NULL;
	}
}

void
image_place (image_t *img, int x, int y)
{
	img->x = x;
	img->y = y;
	img->locked = 0;

	if (img->cur_ht > 0) {
		w_bitblk2 (img->win, 0, 0, img->wd, img->cur_ht,
			img->dstwin, x, y);
	}
}

image_t *
image_get (io_t *iop)
{
	if (!iop->is_image)
		return NULL;
	return (((image_info_t *)iop->ihook)->img);
}

void
image_lock_list (image_t *img)
{
	while (img) {
		img->locked = 1;
		img = img->next;
	}
}
