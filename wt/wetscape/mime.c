/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * MIME related stuff.
 *
 * $Id: mime.c,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mime.h"
#include "io.h"


#define MIME_NEXT	4

typedef struct {
	const char	*type;
	decoder_t	*decoder;
	const char	*command;
	const char	*ext[MIME_NEXT];
} mime_t;

extern decoder_t html_decoder, text_decoder, save_decoder, exec_decoder;
extern decoder_t xbm_decoder, gif_decoder, jpeg_decoder;

static mime_t mime_tab[] = {
 { "application/postscript",
   &exec_decoder, "ghostscript %s",
   { "ai", "eps", "ps" } },

 { "application/rtf",
   NULL, NULL,
   { "rtf" } },

 { "application/x-tex",
   NULL, NULL,
   { "tex" } },

 { "application/x-texinfo",
   NULL, NULL,
   { "texinfo", "texi" } },

 { "application/x-troff",
   NULL, NULL,
   { "t", "tr", "roff" } },

 { "audio/basic",
   &exec_decoder, "cat %s > /dev/audio",
   { "au", "snd" } },

 { "audio/x-aiff",
   NULL, NULL,
   { "aif", "aiff", "aifc" } },

 { "audio/x-wav",
   NULL, NULL,
   { "wav" } },

 { "image/gif",
   &gif_decoder, NULL,
   { "gif" } },

 { "image/ief",
   NULL, NULL,
   { "ief" } },

 { "image/jpeg",
   &jpeg_decoder, NULL,
   { "jpeg", "jpg", "jpe" } },

 { "image/tiff",
   NULL, NULL,
   { "tiff", "tif" } },

 { "image/x-xwindowdump",
   NULL, NULL,
   { "xwd" } },

 { "image/x-xbitmap",
   &xbm_decoder, NULL,
   { "xbm" } },

 { "text/html",
   &html_decoder, NULL,
   { "html", "htm" } },

 { "text/plain",
   &text_decoder, NULL,
   { "txt", "c", "cc", "h" } },

 { "video/mpeg",
   NULL, NULL,
   { "mpeg", "mpg", "mpe" } },

 { "video/quicktime",
   NULL, NULL,
   { "qt", "mov" } },

 { "video/x-msvideo",
   NULL, NULL,
   { "avi" } },

 { "video/x-sgi-movie",
   NULL, NULL,
   { "movie" } },

 { NULL, NULL, NULL, { NULL } }
};

/*
 * find the mime type for a file extension
 */
const char *
mime_get_type (const char *ext)
{
	mime_t *mp;
	int i;

	for (mp = mime_tab; mp->type; ++mp) {
		for (i = 0; i < MIME_NEXT && mp->ext[i]; ++i) {
			if (!strcasecmp (ext, mp->ext[i]))
				return mp->type;
		}
	}
	return MIME_DEFAULT;
}

/*
 * find decoder function for a mime type
 */
decoder_t *
mime_get_decoder (const char *type)
{
	mime_t *mp;

	for (mp = mime_tab; mp->type; ++mp) {
		if (!strcasecmp (type, mp->type))
			return mp->decoder ?: &save_decoder;
	}
	return &save_decoder;
}

/*
 * find command function for a mime type
 */
const char *
mime_get_command (const char *type)
{
	mime_t *mp;

	for (mp = mime_tab; mp->type; ++mp) {
		if (!strcasecmp (type, mp->type))
			return mp->command;
	}
	return NULL;
}

/*
 * default input handler for downloaded data. Waits until the downloader
 * (file, http, ftp, ...) has figured out the mime type, gets the
 * decoder function for this type and calls this decoder on the input
 * data. Does various error checking and cancels the download when something
 * goes wrong.
 */
int
mime_input_handler (io_t *iop)
{
	decoder_t *decoder;
	int r;

	if (!iop->mimetype)
		return 0;

	if (!iop->decoder) {
		if (iop->do_save) {
			/*
			 * save to disk
			 */
			decoder = &save_decoder;
		} else {
			decoder = mime_get_decoder (iop->mimetype);
			if (!decoder) {
				printf ("no handler for %s. cancel.\n",
					iop->mimetype);
				(*iop->done) (iop, IO_ERR_INTERN);
				return -1;
			}
		}
		r = (*decoder->create) (iop);
		if (r < 0) {
			printf ("error creating decoder for %s. cancel.\n",
				iop->mimetype);
			(*iop->done) (iop, r);
			return -1;
		}
		iop->decoder = decoder;
	}
	r = (*iop->decoder->decode) (iop);
	if (r < 0) {
		printf ("error while decoding. cancel.\n");
		(*iop->done) (iop, IO_ERR_DECODE);
		return -1;
	}
	if (iop->eof > 0) {
		(*iop->done) (iop, IO_OK);
		return -1;
	}
	return 0;
}
