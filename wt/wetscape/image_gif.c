/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * GIF reader.
 *
 * The Graphics Interchange Format(c) is the Copyright property of CompuServe
 * Incorporated. GIF(sm) is a Service Mark property of CompuServer
 * Incorporated.
 *
 * The LZW compression/decompression method is a patent of Unisys Corporation.
 *
 * $Id: image_gif.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "io.h"
#include "url.h"
#include "mime.h"
#include "image.h"
#include "util.h"
#include "dither.h"

typedef enum {
	InHeader, InCmap, InSelect, InExt, InImageDesc, InImage, Skip, InError
} state_t;

typedef struct {
	image_t* img;			/* must be first */

	state_t	state;

	uchar*	cmap;			/* colormap */
	short	cmapsz;			/* # entries in colormap */
	uchar	maxval;			/* max value in colormap */
	short	planes;			/* # of bitplanes */
	short	interlaced;		/* is this an interlaced img? */

	int	(*errbuf)[][2];		/* buffer for fs-dithering */

	uchar	pixels;
	uchar	mask;

	short	badcodes;		/* bad code count */

	short	curr_size;		/* The current code size */
	short	clear;			/* Value for a clear code */
	short	ending;			/* Value for a ending code */
	short	newcodes;		/* First available code */
	short	top_slot;		/* Highest code for current size */
	short	slot;			/* Last read code */

	short	navail_bytes;		/* # bytes left in block */
	short	nbits_left;		/* # bits left in current byte */
	uchar	b1;			/* Current byte */
	uchar	byte_buff[257];		/* Current block */
	uchar*	pbytes;			/* Pointer to next byte in block */

	uchar*	dstack;			/* Stack for storing pixels */
	uchar*	suffix;			/* Suffix table */
	ushort*	prefix;			/* Prefix linked list */

	short	dec_fc;			/* internal state of the decoder */
	short	dec_oc;
	short	dec_size;
	short	dec_clearing;
	short	dec_x;
} gif_info_t;

/************************* gif_info_t support ***************************/

static gif_info_t *
gif_info_alloc (void)
{
	gif_info_t *gif = malloc (sizeof (gif_info_t));
	if (!gif)
		return NULL;
	memset (gif, 0, sizeof (gif_info_t));
	gif->state = InHeader;
	return gif;
}

static void
gif_info_free (gif_info_t *gif)
{
	/*
	 * the image gif->img is not freed here.
	 */
	if (gif->errbuf)
		free (gif->errbuf);
	if (gif->cmap)
		free (gif->cmap);
	if (gif->dstack)
		free (gif->dstack);
	if (gif->suffix)
		free (gif->suffix);
	if (gif->prefix)
		free (gif->prefix);
	free (gif);
}

/**************************** lzw decoder *****************************/

#define MAX_CODES	4095

static const long code_mask[13] = {
	0,
	0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F,
	0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF
};

/*
 * This function initializes the decoder for reading a new image.
 */
static short
decoder_init (io_t *iop)
{
	gif_info_t *gif = iop->ihook;
	short size;

	/*
	 * Initialize for decoding a new image...
	 */
	size = iop->ibuf[iop->iptr++] & 0xff;
	if (size < 2 || 9 < size)
		return -1;

	gif->curr_size = size + 1;
	gif->top_slot = 1 << gif->curr_size;
	gif->clear = 1 << size;
	gif->ending = gif->clear + 1;
	gif->slot =
	gif->newcodes = gif->ending + 1;
	gif->navail_bytes =
	gif->nbits_left = 0;

	gif->dstack = malloc ((MAX_CODES + 1)*sizeof(uchar));
	gif->suffix = malloc ((MAX_CODES + 1)*sizeof(uchar));
	gif->prefix = malloc ((MAX_CODES + 1)*sizeof(ushort));

	if (!gif->dstack || !gif->suffix || !gif->prefix)
		return -1;

	/*
	 * Initialize in case they forgot to put in a clear code.
	 * (This shouldn't happen, but we'll try and decode it anyway...)
	 */
	gif->dec_oc = gif->dec_fc = 0;
	gif->badcodes = 0;

	gif->dec_size = size;
	gif->dec_x = 0;

	gif->pixels = 0;
	gif->mask = 0x80;

	return 0;
}

/*
 * free memory allocated for decoding the image
 */
static void
decoder_cleanup (gif_info_t *gif)
{
	if (gif->dstack)
		free (gif->dstack);
	if (gif->suffix)
		free (gif->suffix);
	if (gif->prefix)
		free (gif->prefix);
	gif->dstack = NULL;
	gif->suffix = NULL;
	gif->prefix = NULL;
}

/*
 * get_next_code()
 * gets the next code from the GIF file.  Returns the code, or EOF
 * if no more input data is available.
 */
static short
get_next_code (io_t *iop)
{
	gif_info_t *gif = iop->ihook;
	short i, j;
	ulong ret;
	int avail;

	j = gif->nbits_left + 8*gif->navail_bytes - gif->curr_size;
	if (j < 0) {
		/*
		 * See if there is enough input available
		 * to make the next code...
		 */
		ret = iop->iptr;
		avail = iop->ibufused - ret;
		while (j < 0) {
			if (avail < 1)
				return EOF;
			i = iop->ibuf[ret] & 0xff;
			if (avail <= i)
				return EOF;
			j += 8*i;
			ret += i+1;
			avail -= i+1;
		}
	}
	if (gif->nbits_left == 0) {
		if (gif->navail_bytes <= 0) {
			/*
			 * Out of bytes in current block, so read next block
			 */
			gif->pbytes = gif->byte_buff;
			gif->navail_bytes = i = iop->ibuf[iop->iptr] & 0xff;
			memcpy (gif->pbytes, &iop->ibuf[iop->iptr+1], i);
			iop->iptr += i+1;
		}
		gif->b1 = *gif->pbytes++;
		gif->nbits_left = 8;
		--gif->navail_bytes;
	}

	ret = gif->b1 >> (8 - gif->nbits_left);

	while (gif->curr_size > gif->nbits_left) {
		if (gif->navail_bytes <= 0) {
			/*
			 * Out of bytes in current block, so read next block
			 */
			gif->pbytes = gif->byte_buff;
			gif->navail_bytes = i = iop->ibuf[iop->iptr] & 0xff;
			memcpy (gif->pbytes, &iop->ibuf[iop->iptr+1], i);
			iop->iptr += i+1;
		}
		gif->b1 = *gif->pbytes++;
		ret |= gif->b1 << gif->nbits_left;
		gif->nbits_left += 8;
		--gif->navail_bytes;
	}
	gif->nbits_left -= gif->curr_size;
	ret &= code_mask[gif->curr_size];
	return (short)ret;
}

static inline int
dither (int x, int y, uchar col, gif_info_t *gif)
{
	if (gif->interlaced) {
		/*
		 * for interlaced images we cannot use fs-dithering...
		 */
		return dither_o (x, y, col, gif->maxval);
	} else {
		return dither_fs (x, y, col, gif->errbuf, gif->maxval);
	}
}

/*
 * This function decodes an LZW image, according to the method used
 * in the GIF spec.
 */
static short
decoder (io_t *iop)
{
	gif_info_t *gif = iop->ihook;
	short linewidth;
	register uchar *sp;
	uchar color, pix, mask;
	register short code, fc, oc, x;
	short c, size;

	linewidth = (short)gif->img->wd;

	sp =   gif->dstack;
	fc =   gif->dec_fc;
	oc =   gif->dec_oc;
	x    = gif->dec_x;
	size = gif->dec_size;
	pix  = gif->pixels;
	mask = gif->mask;

	if (gif->dec_clearing) {
		gif->dec_clearing = 0;
		goto clearing;
	}
	gif->dec_clearing = 0;

	/*
	 * This is the main loop.  For each code we get we pass through the
	 * linked list of prefix codes, pushing the corresponding "character"
	 * for each code onto the stack.  When the list reaches a single
	 * "character" we push that on the stack too, and then start
	 * unstacking each character for output in the correct order.
	 * Special handling is included for the clear code, and the whole
	 * thing ends when we get an ending code.
	 */
	while ((c = get_next_code (iop)) != gif->ending && c != EOF) {
		/*
		 * If the code is a clear code, reinitialize all necessary
		 * items.
		 */
		if (c == gif->clear) {
			gif->curr_size = size + 1;
			gif->slot = gif->newcodes;
			gif->top_slot = 1 << gif->curr_size;

			/*
			 * Continue reading codes until we get a non-clear code
			 * (Another unlikely, but possible case...)
			 */
		clearing:
			while ((c = get_next_code (iop)) == gif->clear)
				;
			if (c == EOF) {
				gif->dec_clearing = 1;
				break;
			}

			/*
			 * If we get an ending code immediately after a clear
			 * code (Yet another unlikely case), then break out
			 * of the loop.
			 */
			if (c == gif->ending)
				break;

			/*
			 * Finally, if the code is beyond the range of already
			 * set codes, (This one had better NOT happen...  I
			 * have no idea what will result from this, but I
			 * doubt it will look good...) then set it to color
			 * zero.
			 */
			if (c >= gif->slot)
				c = 0;

			oc = fc = c;

			/*
			 * And let us not forget to put the char into the
			 * buffer...
			 */
			color = (c < gif->cmapsz) ? gif->cmap[c] : 0;
			if (!dither (x, gif->img->cur_y, color, gif))
				pix |= mask;
			if (++x >= linewidth) {
				image_addpixels (gif->img, pix);
				x = 0;
				pix = 0;
				mask = 0x80;
			} else if (!(mask >>= 1)) {
				image_addpixels (gif->img, pix);
				pix = 0;
				mask = 0x80;
			}
		} else {

			/*
			 * In this case, it's not a clear code or an ending
			 * code, so it must be a code code...  So we can
			 * now decode the code into a stack of character
			 * codes. (Clear as mud, right?)
			 */
			code = c;

			/*
			 * Here we go again with one of those off chances...
			 * If, on the off chance, the code we got is beyond
			 * the range of those already set up (Another thing
			 * which had better NOT happen...) we trick the
			 * decoder into thinking it actually got the last
			 * code read.
			 * (Hmmn... I'm not sure why this works...
			 * But it does...)
			 */
			if (code >= gif->slot) {
				if (code > gif->slot)
					++gif->badcodes;
				code = oc;
				*sp++ = (uchar) fc;
			}

			/*
			 * Here we scan back along the linked list of
			 * prefixes, pushing helpless characters (ie.
			 * suffixes) onto the stack as we do so.
			 */
			while (code >= gif->newcodes) {
				*sp++ = gif->suffix[code];
				code  = gif->prefix[code];
			}

			/*
			 * Push the last character on the stack, and set
			 * up the new prefix and suffix, and if the
			 * required slot number is greater than that
			 * allowed by the current bit size, increase the bit
			 * size.  (NOTE - If we are all full, we *don't*
			 * save the new suffix and prefix...  I'm not certain
			 * if this is correct... it might be more proper to
			 * overwrite the last code...
			 */
			*sp++ = (uchar) code;
			if (gif->slot < gif->top_slot) {
				fc = code;
				gif->suffix[gif->slot] = (uchar) fc;
				gif->prefix[gif->slot++] = oc;
				oc = c;
			}
			if (gif->slot >= gif->top_slot) {
				if (gif->curr_size < 12) {
					gif->top_slot <<= 1;
					++gif->curr_size;
				}
			}

			/*
			 * Now that we've pushed the decoded string (in
			 * reverse order) onto the stack, lets pop it off
			 * and put it into our decode buffer...
			 */
			while (sp > gif->dstack) {
				c = *--sp & 0xff;
				color = (c < gif->cmapsz) ? gif->cmap[c] : 0;
				if (!dither (x, gif->img->cur_y, color, gif))
					pix |= mask;
				if (++x >= linewidth) {
					image_addpixels (gif->img, pix);
					x = 0;
					pix = 0;
					mask = 0x80;
				} else if (!(mask >>= 1)) {
					image_addpixels (gif->img, pix);
					pix = 0;
					mask = 0x80;
				}
			}
		}
	}

	gif->dec_fc   = fc;
	gif->dec_oc   = oc;
	gif->dec_x    = x;
	gif->dec_size = size;
	gif->pixels   = pix;
	gif->mask     = mask;

	return (c == EOF ? 0 : 1);
}

/********************** helper functions *************************/

static interlace_t gif_ilace[] = {
	{ 0, 8 }, /* 1st pass, start with y=0 and increment by 8 */
	{ 4, 8 }, /* 2nd pass, start with y=4 and increment by 8 */
	{ 2, 4 }, /* 3rd pass, start with y=2 and increment by 4 */
	{ 1, 2 }  /* 4th pass, start with y=1 and increment by 2 */
};

/*
 * allocate an image_t and other stuff that is needed to decode the
 * picture.
 */
static int
getimage (io_t *iop, int wd, int ht)
{
	gif_info_t *gif = iop->ihook;
	int r;

	if (wd <= 0 || ht <= 0)
		return -1;

	gif->img = image_alloc (iop->url);
	if (!gif->img)
		return -1;

	if (!gif->interlaced) {
		/*
		 * get buffer for fs-error-diffusion.
		 */
		gif->errbuf = malloc (sizeof (int) * 2 * (wd+4));
		if (!gif->errbuf) {
			image_free (gif->img);
			gif->img = NULL;
			return -1;
		}
		memset (gif->errbuf, 0, sizeof (int) * 2 * (wd+4));

		r = image_getwin (gif->img, html_getwin (), wd, ht, NULL, 0);
	} else {
		/*
		 * for interlaced pictures we cannot use fs-dithering and
		 * therefore don't need the buffer
		 */
		r = image_getwin (gif->img, html_getwin (), wd, ht,
			gif_ilace, 4);
	}
	if (r < 0) {
		image_free (gif->img);
		gif->img = NULL;
		if (gif->errbuf) {
			free (gif->errbuf);
			gif->errbuf = NULL;
		}
		return -1;
	}
	if (iop->main_doc) {
		html_clear ();
		image_add (gif->img);
		html_setpic (iop->url);
	} else {
		/*
		 * link into the chain of images (must be after html_clear()).
		 */
		image_add (gif->img);
	}
	return 0;
}

/*
 * store colormap
 */
static int
getcmap (io_t *iop, uchar *cp)
{
	gif_info_t *gif = iop->ihook;
	uchar val;
	int i;

	if (gif->cmap)
		free (gif->cmap);
	gif->cmap = malloc (gif->cmapsz);
	if (!gif->cmap)
		return -1;

	gif->maxval = 0;
	for (i = 0; i < gif->cmapsz; ++i, cp += 3) {
		gif->cmap[i] = val = col2grey (cp[0], cp[1], cp[2]);
		if (val > gif->maxval)
			gif->maxval = val;
	}
	return 0;
}

/**************************** gif decoder *****************************/

static int
gif_decoder_create (io_t *iop)
{
	gif_info_t *gif = gif_info_alloc ();

	iop->ihook = gif;
	if (!iop->ihook)
		return -1;
	iop->is_image = 1;
	return 0;
}

static void
gif_decoder_free (io_t *iop)
{
	if (iop->ihook) {
		gif_info_free (iop->ihook);
		iop->ihook = NULL;
	}
}

static int
gif_decoder_decode (io_t *iop)
{
	gif_info_t *gif = iop->ihook;
	int wd, ht, i, avail, end = 0;
	uchar *cp;

	avail = iop->ibufused;

	while (!end) switch (gif->state) {
	case InHeader:
		/*
		 * gif header
		 */
		if (avail < 13) {
			end = 1;
			break;
		}
		cp = &iop->ibuf[iop->iptr];
		if (strncmp (cp, "GIF", 3) ||
		    !isdigit (cp[3]) || !isdigit (cp[4]) ||
		    !isalpha (cp[5])) {
			gif->state = InError;
			end = -1;
			break;
		}
		if (cp[10] & 0x80) {
			/*
			 * have a global colormap
			 */
			gif->planes = (cp[10] & 0x07) + 1;
			gif->cmapsz = 1 << gif->planes;
			gif->state = InCmap;
		} else {
			gif->state = InSelect;
		}
		iop->iptr += 13;
		avail -= 13;
		break;

	case InCmap:
		/*
		 * global colormap
		 */
		if (avail < 3*gif->cmapsz) {
			end = 1;
			break;
		}
		if (getcmap (iop, &iop->ibuf[iop->iptr]) < 0) {
			gif->state = InError;
			end = -1;
			break;
		}
		iop->iptr += 3*gif->cmapsz;
		avail -= 3*gif->cmapsz;
		gif->state = InSelect;
		break;

	case InSelect:
		/*
		 * block ID follows
		 */
		if (avail < 1) {
			end = 1;
			break;
		}
		cp = &iop->ibuf[iop->iptr];
		switch (*cp) {
		case ';':
			/*
			 * finished before we saw a picture?!
			 */
			end = -1;
			gif->state = InError;
			break;

		case '!':
			/*
			 * extension block
			 */
			if (avail < 2) {
				/*
				 * wait for ID
				 */
				end = 1;
				break;
			}
			iop->iptr += 2;
			avail -= 2;
			gif->state = InExt;
			break;
			
		case ',':
			/*
			 * image description
			 */
			++iop->iptr;
			--avail;
			gif->state = InImageDesc;
			break;

		default:
			gif->state = InError;
			end = -1;
			break;
		}
		break;

	case InExt:
		/*
		 * extension
		 */
		if (avail < 1) {
			end = 1;
			break;
		}
		i = iop->ibuf[iop->iptr] & 0xff;
		if (i == 0) {
			++iop->iptr;
			--avail;
			gif->state = InSelect;
			break;
		}
		if (avail < ++i) {
			/*
			 * wait for ext data
			 */
			end = 1;
			break;
		}
		iop->iptr += i;
		avail -= i;
		break;

	case InImageDesc:
		/*
		 * image description
		 */
		if (avail < 9 + 1) {
			/*
			 * wait for image desc and first data byte
			 */
			end = 1;
			break;
		}
		cp = &iop->ibuf[iop->iptr];

		gif->interlaced = !!(cp[8] & 0x40);

		wd = cp[4] | (cp[5] << 8);
		ht = cp[6] | (cp[7] << 8);

		if (cp[8] & 0x80) {
			/*
			 * local colormap.
			 */
			gif->planes = (cp[8] & 0x07) + 1;
			gif->cmapsz = 1 << gif->planes;
			if (avail < 9 + 3*gif->cmapsz + 1) {
				/*
				 * wait for colormap and first data byte
				 */
				end = 1;
				break;
			}
			if (getcmap (iop, cp+9) < 0) {
				gif->state = InError;
				end = -1;
				break;
			}
			iop->iptr += 3*gif->cmapsz;
			avail -= 3*gif->cmapsz;
		}
		iop->iptr += 9;
		avail -= 9;

		if (!gif->cmap) {
			/*
			 * Hmm. no color map.
			 */
			gif->state = InError;
			end = -1;
			break;
		}
		if (getimage (iop, wd, ht) < 0) {
			gif->state = InError;
			end = -1;
			break;
		}
		if (decoder_init (iop) < 0) {
			decoder_cleanup (gif);
			gif->state = InError;
			end = -1;
		} else {
			gif->state = InImage;
		}
		break;

	case InImage:
		/*
		 * image data
		 */
		end = 1;
		i = decoder (iop);
		if (i != 0) {
			decoder_cleanup (gif);
			gif->state = i < 0 ? InError : Skip;
			end = i;
		}
		break;

	case Skip:
		iop->iptr = iop->ibufused;
		end = 1;
		break;

	case InError:
		iop->iptr = iop->ibufused;
		end = -1;
		break;
	}
	io_eat_input (iop, iop->iptr);
	iop->iptr = 0;

	if (iop->eof && gif->img) {
		image_flush (gif->img);
		image_done (gif->img);
	}
	return end > 0 ? 0 : -1;
}

decoder_t gif_decoder = {
	gif_decoder_create,  gif_decoder_decode, gif_decoder_free
};
