/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * XBM reader.
 *
 * $Id: image_xbm.c,v 1.1.1.1 1998/11/01 19:15:04 eero Exp $
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

typedef enum {
	InHeader, InBody, InError
} state_t;

typedef struct {
	image_t* img;	/* must be first */

	state_t	state;
	short	comment;
	char*	comment_cp;
} xbm_info_t;


/************************* xbm_info_t support ***************************/

static xbm_info_t *
xbm_info_alloc (void)
{
	xbm_info_t *xbm = malloc (sizeof (xbm_info_t));
	if (!xbm)
		return NULL;
	memset (xbm, 0, sizeof (xbm_info_t));
	xbm->state = InHeader;
	return xbm;
}

static void
xbm_info_free (xbm_info_t *xbm)
{
	/*
	 * the image xbm->img is not freed here.
	 */
	free (xbm);
}

/*********************** XBM parser stuff *****************************/

static void
kill_comments (io_t *iop)
{
	xbm_info_t *xbm = iop->ihook;
	char *cp, *cp_o, *cp_c;
	int l;

	cp = xbm->comment_cp;
	while (*cp) {
		if (xbm->comment > 0) {
			cp_c = strstr (cp, "*/");
			cp_o = strstr (cp, "/*");
			if (cp_c && (!cp_o || cp_c < cp_o)) {
				/*
				 * comment closed
				 */
				--xbm->comment;
				cp_c += 2;
				strcpy (xbm->comment_cp, cp_c);
				cp = xbm->comment_cp;

				iop->ibufused -= cp_c - xbm->comment_cp;
			} else if (cp_o && (!cp_c || cp_o < cp_c)) {
				/*
				 * comment opened
				 */
				cp_o[1] = ' ';
				++xbm->comment;
				cp = cp_o + 2;
			} else {
				/*
				 * everything up to the end of the string
				 * except the last character in it is inside
				 * a comment
				 */
				l = strlen (xbm->comment_cp);
				xbm->comment_cp[0] = xbm->comment_cp[l-1];
				xbm->comment_cp[1] = 0;

				iop->ibufused -= l-1;
				break;
			}
		} else {
			cp_o = strstr (cp, "/*");
			if (cp_o) {
				cp_o[1] = ' ';
				++xbm->comment;
				xbm->comment_cp = cp_o;
				cp = cp_o + 2;
			} else {
				/*
				 * everything up to the end of the string
				 * except the last character in it is outside
				 * a comment
				 */
				xbm->comment_cp = &cp[strlen (cp) - 1];
				break;
			}
		}
	}
}

static char *
getdefine (char *cp)
{
	cp = strstr (cp, "define");
	if (!cp)
		return NULL;
	cp += strlen ("define");

	while (*cp && isspace (*cp))
		++cp;
	while (*cp && !isspace (*cp))
		++cp;
	while (*cp && isspace (*cp))
		++cp;
	return cp;
}

static int
xbm_parse_header (io_t *iop)
{
	xbm_info_t *xbm = iop->ihook;
	char *cp;
	int wd, ht;

	/*
	 * get width of the picture
	 */
	cp = getdefine (iop->ibuf);
	if (!cp || !*cp)
		return -1;
	wd = strtol (cp, NULL, 0);

	/*
	 * get height of the picture
	 */
	cp = getdefine (cp);
	if (!cp || !*cp)
		return -1;
	ht = strtol (cp, NULL, 0);

	if (wd <= 0 || ht <= 0)
		return -1;

	xbm->img = image_alloc (iop->url);
	if (!xbm->img)
		return -1;

	/*
	 * allocate a bitmap for the image.
	 */
	if (image_getwin (xbm->img, html_getwin (), wd, ht, NULL, 0) < 0) {
		image_free (xbm->img);
		xbm->img = NULL;
		return -1;
	}
	if (iop->main_doc) {
		html_clear ();
		image_add (xbm->img);
		html_setpic (iop->url);
	} else {
		/*
		 * link into the chain of images (must be after html_clear())
		 */
		image_add (xbm->img);
	}
	return 0;
}

#define issep(c)	(isspace(c) || (c) == ',' || (c) == '}' || (c) == ';')

/*
 * assume no comments in image data
 */
static inline int
getbyte (io_t *iop)
{
	char *cp, *cp2;
	int c;

	cp = &iop->ibuf[iop->iptr];

	while ((c = *cp) && issep (c))
		++cp;

	cp2 = cp;
	while ((c = *cp2) && !issep (c))
		++cp2;

	if (!c) {
		/*
		 * part of the number might be still missing
		 */
		return EOF;
	}
	iop->iptr = cp2 - iop->ibuf;

	c = strtol (cp, NULL, 0) & 0xff;
	/*
	 * reverse bitorder of `c'.
	 */
	c = ((c & 0xf0) >> 4) | ((c & 0x0f) << 4);
	c = ((c & 0xcc) >> 2) | ((c & 0x33) << 2);
	c = ((c & 0xaa) >> 1) | ((c & 0x55) << 1);

	return c;
}


/**************************** XBM decoder *****************************/

static int
xbm_decoder_create (io_t *iop)
{
	xbm_info_t *xbm = xbm_info_alloc ();

	iop->ihook = xbm;
	if (!iop->ihook)
		return -1;
	iop->is_image = 1;
	xbm->comment_cp = iop->ibuf;
	return 0;
}

static void
xbm_decoder_free (io_t *iop)
{
	if (iop->ihook) {
		xbm_info_free (iop->ihook);
		iop->ihook = NULL;
	}
}

static int
xbm_decoder_decode (io_t *iop)
{
	xbm_info_t *xbm = iop->ihook;
	int c, end = 0;
	char *cp = NULL;

	while (!end) switch (xbm->state) {
	case InHeader:
		iop->ibuf[iop->ibufused] = 0;
		kill_comments (iop);
		if (xbm->comment > 0 || !(cp = strchr (iop->ibuf, '{'))) {
			end = 1;
			break;
		}
		if (xbm_parse_header (iop)) {
			/*
			 * bad header
			 */
			end = -1;
			xbm->state = InError;
			break;
		}
		io_eat_input (iop, (cp+1) - iop->ibuf);
		xbm->state = InBody;
		break;

	case InBody:
		iop->ibuf[iop->ibufused] = 0;
		iop->iptr = 0;
		while ((c = getbyte (iop)) != EOF) {
			image_addpixels (xbm->img, c);
		}
		io_eat_input (iop, iop->iptr);
		iop->iptr = 0;
		end = 1;
		break;

	case InError:
		io_eat_input (iop, iop->ibufused);
		end = -1;
		break;
	}
	if (iop->eof && xbm->img) {
		image_flush (xbm->img);
		image_done (xbm->img);
	}
	return end > 0 ? 0 : -1;
}

decoder_t xbm_decoder = {
	xbm_decoder_create,  xbm_decoder_decode, xbm_decoder_free
};
