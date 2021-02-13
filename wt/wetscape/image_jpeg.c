/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * JPEG reader using the IJG's jpeg library.
 *
 * $Id: image_jpeg.c,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "io.h"
#include "url.h"
#include "mime.h"
#include "image.h"
#include "util.h"
#include "dither.h"
#include "jpeglib.h"


typedef enum {
	InHeader, InStart, InBody, InFinish, InError, Skip
} state_t;


/*
 * our own error manager
 */
typedef struct {
	struct jpeg_error_mgr pub;
	jmp_buf jmpbuf;
} my_error_mgr_t;

/*
 * our own decompression info
 */
struct _jpeg_info_t;

typedef struct {
	struct jpeg_decompress_struct pub;
	struct _jpeg_info_t *jp;
} my_decomp_t;

typedef struct _jpeg_info_t {
	image_t* 	img;		/* must be first */
	state_t		state;		/* decoder state */
	int		(*errbuf)[][2];	/* buffer for fs-dithering */

	my_error_mgr_t	cerr;		/* error handler */
	my_decomp_t	cinfo;		/* decompression info */
	JSAMPARRAY	cbuf;		/* buffer for one row of samples */
	long		cbufstride;	/* bytes per pixel in cbuf */
	long		skip;		/* # bytes to skip in input data */
} jpeg_info_t;

/************************* jpeg_info_t support ***************************/

static jpeg_info_t *
jpeg_info_alloc (void)
{
	jpeg_info_t *jpeg = malloc (sizeof (jpeg_info_t));
	if (!jpeg)
		return NULL;
	memset (jpeg, 0, sizeof (jpeg_info_t));
	jpeg->state = InHeader;
	return jpeg;
}

static void
jpeg_info_free (jpeg_info_t *jpeg)
{
	if (jpeg->errbuf)
		free (jpeg->errbuf);
	/*
	 * the image jpeg->img is not freed here.
	 */
	free (jpeg);
}

/******************* libjpeg replacement error handler ******************/

static void
my_error_exit (j_common_ptr cinfo)
{
	my_error_mgr_t *myerr = (my_error_mgr_t *)cinfo->err;

	(*cinfo->err->output_message) (cinfo);
	longjmp (myerr->jmpbuf, 1);
}

static void
my_output_message (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message) (cinfo, buffer);
	status_set (buffer);
}

static struct jpeg_error_mgr *
my_jpeg_error (struct jpeg_error_mgr *err)
{
	jpeg_std_error (err);
	err->error_exit =     my_error_exit;
	err->output_message = my_output_message;

	return err;
}

/******************* libjpeg replacement data source *******************/

static void
my_init_source (j_decompress_ptr cinfo)
{
}

static boolean
my_fill_input_buffer (j_decompress_ptr cinfo)
{
	return 0;
}

static void
my_skip_input_data (j_decompress_ptr cinfo, long nbytes)
{
	jpeg_info_t *jpeg = ((my_decomp_t *)cinfo)->jp;
	long canskip;

	canskip = MIN (cinfo->src->bytes_in_buffer, nbytes);

	cinfo->src->bytes_in_buffer -= canskip;
	cinfo->src->next_input_byte += canskip;

	jpeg->skip = nbytes - canskip;
}

static void
my_term_source (j_decompress_ptr cinfo)
{
}

static void
my_jpeg_source (j_decompress_ptr cinfo)
{
	cinfo->src = (struct jpeg_source_mgr *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT,
				      sizeof (struct jpeg_source_mgr));

	cinfo->src->init_source       = my_init_source;
	cinfo->src->fill_input_buffer = my_fill_input_buffer;
	cinfo->src->skip_input_data   = my_skip_input_data;
	cinfo->src->resync_to_restart = jpeg_resync_to_restart;
	cinfo->src->term_source       = my_term_source;
	cinfo->src->bytes_in_buffer   = 0;
	cinfo->src->next_input_byte   = NULL;
}

/*********************** some utility funcs *****************************/

static int
jpeg_getimage (io_t *iop)
{
	jpeg_info_t *jpeg = iop->ihook;
	j_decompress_ptr cinfo = &jpeg->cinfo.pub;
	int wd, ht;

	jpeg->img = image_alloc (iop->url);
	if (!jpeg->img)
		return -1;

	wd = cinfo->output_width;
	ht = cinfo->output_height;

	jpeg->cbufstride = wd * cinfo->output_components;
	/*
	 * this will throw an error if fails. memory stays until
	 * jpeg_destroy_decompress().
	 */
	jpeg->cbuf = (*cinfo->mem->alloc_sarray)
		((j_common_ptr)cinfo, JPOOL_IMAGE, jpeg->cbufstride, 1);

	/*
	 * get buffer for fs-error-diffusion.
	 */
	jpeg->errbuf = malloc (sizeof (int) * 2 * (wd+4));
	if (!jpeg->errbuf) {
		image_free (jpeg->img);
		jpeg->img = NULL;
		return -1;
	}
	memset (jpeg->errbuf, 0, sizeof (int) * 2 * (wd+4));

	/*
	 * allocate a bitmap for the image.
	 */
	if (image_getwin (jpeg->img, html_getwin (), wd, ht, NULL, 0) < 0) {
		image_free (jpeg->img);
		jpeg->img = NULL;
		free (jpeg->errbuf);
		jpeg->errbuf = NULL;
		return -1;
	}
	if (iop->main_doc) {
		html_clear ();
		image_add (jpeg->img);
		html_setpic (iop->url);	
	} else {
		/*
		 * link into the chain of images (must be after html_clear())
		 */
		image_add (jpeg->img);
	}
	return 0;
}

/*
 * output a row of samples. Assumes sizeof (JSAMPLE) == 1 and
 * JCS_GRAYSCALE, i.e. cinfo->output_components == 1.
 */
static inline void
jpeg_outrow (jpeg_info_t *jpeg)
{
	JSAMPLE *cp;
	uchar pixels, mask;
	int x, y, wd;

	pixels = 0;
	mask = 0x80;

	wd = jpeg->img->wd;
	y  = jpeg->img->cur_y;
	cp = jpeg->cbuf[0];

	for (x = 0; x < wd; ++x, ++cp) {
		if (!dither_fs (x, y, *cp, jpeg->errbuf, 255))
			pixels |= mask;
		if (!(mask >>= 1)) {
			image_addpixels (jpeg->img, pixels);
			pixels = 0;
			mask = 0x80;
		}
	}
	if (mask != 0x80) {
		image_addpixels (jpeg->img, pixels);
	}
}

/**************************** jpeg decoder *****************************/

static int
jpeg_decoder_create (io_t *iop)
{
	jpeg_info_t *jpeg = jpeg_info_alloc ();

	iop->ihook = jpeg;
	if (!iop->ihook)
		return -1;
	iop->is_image = 1;

	/*
	 * backlink to jpeg_info_t for the data source
	 */
	jpeg->cinfo.jp = jpeg;

	/*
	 * install error handler.
	 */
	jpeg->cinfo.pub.err = my_jpeg_error (&jpeg->cerr.pub);

	if (setjmp (jpeg->cerr.jmpbuf)) {
		/*
		 * initialization failed.
		 */
		jpeg_destroy_decompress (&jpeg->cinfo.pub);
		jpeg_info_free (iop->ihook);
		iop->ihook = NULL;
		return -1;
	}
	jpeg_create_decompress (&jpeg->cinfo.pub);
	/*
	 * initialize data source
	 */
	my_jpeg_source (&jpeg->cinfo.pub);
	return 0;
}

static void
jpeg_decoder_free (io_t *iop)
{
	jpeg_info_t *jpeg = iop->ihook;

	if (jpeg) {
		jpeg_destroy_decompress (&jpeg->cinfo.pub);
		jpeg_info_free (jpeg);
		iop->ihook = NULL;
	}
}

static int
jpeg_decoder_decode (io_t *iop)
{
	jpeg_info_t *jpeg = iop->ihook;
	j_decompress_ptr cinfo = &jpeg->cinfo.pub;
	int r, avail, end;

	if (setjmp (jpeg->cerr.jmpbuf)) {
		jpeg->state = InError;
		return -1;
	}

	avail = iop->ibufused;
	if (jpeg->skip > 0) {
		long canskip = MIN (jpeg->skip, avail);

		jpeg->skip -= canskip;
		iop->iptr  += canskip;
		avail      -= canskip;
	}
	if (avail <= 0) {
		end = 1;
	} else {
		cinfo->src->next_input_byte = &iop->ibuf[iop->iptr];
		cinfo->src->bytes_in_buffer = avail;
		end = 0;
	}
	while (!end) switch (jpeg->state) {
	case InHeader:
		r = jpeg_read_header (cinfo, 1);
		if (r == JPEG_SUSPENDED) {
			iop->iptr = (const char *)cinfo->src->next_input_byte -
				iop->ibuf;
			end = 1;
			break;
		}
		/*
		 * we want grayscale output
		 */
		cinfo->out_color_space = JCS_GRAYSCALE;
		jpeg->state = InStart;
		break;

	case InStart:
		r = jpeg_start_decompress (cinfo);
		if (!r) {
			iop->iptr = (const char *)cinfo->src->next_input_byte -
				iop->ibuf;
			end = 1;
			break;
		}
		if (cinfo->output_components != 1) {
			/*
			 * we somehow didn't get JCS_GRAYSCALE...
			 */
			jpeg->state = InError;
			end = -1;
			break;
		}
		if (jpeg_getimage (iop)) {
			jpeg->state = InError;
			end = -1;
			break;
		}
		jpeg->state = InBody;
		break;

	case InBody:
		while (cinfo->output_scanline < cinfo->output_height) {
			r = jpeg_read_scanlines (cinfo, jpeg->cbuf, 1);
			if (r == 0) {
				break;
			}
			jpeg_outrow (jpeg);
		}
		if (cinfo->output_scanline < cinfo->output_height) {
			iop->iptr = (const char *)cinfo->src->next_input_byte -
				iop->ibuf;
			end = 1;
			break;
		}
		jpeg->state = InFinish;
		break;

	case InFinish:
		r = jpeg_finish_decompress (cinfo);
		if (!r) {
			iop->iptr = (const char *)cinfo->src->next_input_byte -
				iop->ibuf;
			end = 1;
			break;			
		}
		jpeg->state = Skip;
		break;

	case InError:
		iop->iptr = iop->ibufused;
		end = -1;
		break;

	case Skip:
		iop->iptr = iop->ibufused;
		end = 1;
		break;
	}
	io_eat_input (iop, iop->iptr);
	iop->iptr = 0;
	
	if (iop->eof && jpeg->img) {
		image_flush (jpeg->img);
		image_done (jpeg->img);
	}
	return end > 0 ? 0 : -1;
}

decoder_t jpeg_decoder = {
	jpeg_decoder_create,  jpeg_decoder_decode, jpeg_decoder_free
};
