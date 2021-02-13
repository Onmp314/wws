/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * image output support routines.
 *
 * $Id: image.h,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#ifndef _IMAGE_H
#define _IMAGE_H

struct _url_t;
struct _io_t;

typedef struct {
	short start, offs;
} interlace_t;

typedef struct _image_t {
	struct _image_t	*next;
	struct _url_t	*url;

	long	x, y, wd, ht;
	long	cur_ht;
	WWIN*	win;
	WWIN*	dstwin;

	interlace_t *ilace;	

	short	bpl, pad, nlines, npasses, newlines;
	short	cur_pass, cur_y, cur_bytes, cur_lines;
	char	*buf, *cp;

	uchar	locked;
} image_t;

typedef struct {
	image_t *img;
} image_info_t;

extern image_t*	image_alloc (struct _url_t *);
extern void	image_free (image_t *);
extern void	image_free_list (image_t *);
extern int	image_getwin (image_t *, WWIN *dstwin,
			int wd, int ht, interlace_t *ilace, int npasses);
extern void	image_place (image_t *, int x, int y);
extern image_t*	image_get (struct _io_t *);
extern void	image_lock_list (image_t *);

extern void	image_flush (image_t *);
extern void	image_done (image_t *);


static inline void
image_addpixels (image_t *img, char eight_pixels)
{
	*img->cp++ = eight_pixels;
	if (++img->cur_bytes >= img->bpl) {
		img->cur_bytes = 0;
		img->cp += img->pad;
		if (++img->cur_lines < img->nlines &&
		    img->cur_y < img->ht-1) {
			++img->cur_y;
		} else {
			image_flush (img);
		}
	}
}

#endif
