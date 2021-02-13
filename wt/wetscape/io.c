/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * input/output support routines.
 *
 * $Id: io.c,v 1.2 2009-08-23 20:27:36 eero Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#include <Wlib.h>
#include <Wt.h>

#include "url.h"
#include "io.h"
#include "mime.h"
#include "util.h"

extern io_handler_t file_io_handler, http_io_handler;

static io_handler_t *io_handlers[] = {
	&http_io_handler,
	&file_io_handler,
	NULL
};

static io_handler_t *
io_handler_lookup (int scheme)
{
	io_handler_t *handler;

	if (scheme > URL_MAX)
		return NULL;

	handler = io_handlers[scheme];
	if (handler && handler->scheme_id != scheme) {
		printf ("Wetscape bug: %s, %d\n", __FILE__, __LINE__);
		return NULL;
	}
	return handler;
}

static io_t *
io_alloc (void)
{
	io_t *iop;

	iop = malloc (sizeof (io_t));
	if (!iop)
		return NULL;

	memset (iop, 0, sizeof (io_t));
	iop->refcnt = 1;
	return iop;
}

static void
io_free (io_t *iop)
{
	if (iop->ibuf)
		free (iop->ibuf);
	if (iop->obuf)
		free (iop->obuf);
	if (iop->handler)
		(*iop->handler->free) (iop);
	if (iop->decoder)
		(*iop->decoder->free) (iop);
	free (iop);
}

static void
io_delete (io_t *iop)
{
	if (iop->refcnt <= 0) {
		if (iop->handler_id >= 0) {
			wt_delinput (iop->handler_id);
			iop->handler_id = 0;
		}
		io_free (iop);
	}
}

void
io_ref (io_t *iop)
{
	++iop->refcnt;
}

void
io_deref (io_t *iop)
{
	if (--iop->refcnt <= 0)
		io_delete (iop);
}

int
io_alloc_ibuf (io_t *iop, long size)
{
	iop->ibuf = malloc (size+10);
	if (!iop->ibuf)
		return -1;

	iop->ibuflen = size;
	iop->ibufused = 0;
	iop->ibufoffset = 0;
	iop->iptr = 0;
	return 0;
}

int
io_alloc_obuf (io_t *iop, long size)
{
	iop->obuf = malloc (size+10);
	if (!iop->obuf)
		return -1;

	iop->obuflen = size;
	iop->obufused = 0;
	iop->obufoffset = 0;
	iop->optr = 0;
	return 0;
}

void
io_eat_input (io_t *iop, long size)
{
	if (iop->ibufused < size)
		size = iop->ibufused;
	if (size > 0) {
		iop->ibufoffset += size;
		if (size < iop->ibufused) {
			memmove (iop->ibuf, &iop->ibuf[size],
				 iop->ibufused - size);
		}
		iop->ibufused -= size;
	}
}

void
io_eat_output (io_t *iop, long size)
{
	if (iop->obufused < size)
		size = iop->obufused;
	if (size > 0) {
		iop->obufoffset += size;
		if (size < iop->obufused) {
			memmove (iop->obuf, &iop->obuf[size],
				 iop->obufused - size);
		}
		iop->obufused -= size;
	}
}

inline int
io_getc (io_t *iop)
{
	int c;

	if (iop->iptr >= iop->ibufused)
		return EOF;

	c = iop->ibuf[iop->iptr];
	if (c == '\r') {
		if (iop->iptr+1 >= iop->ibufused)
			/*
			 * must wait for next character to arrive to
			 * see if its LF or not.
			 */
			return EOF;
		if (iop->ibuf[++iop->iptr] == '\n')
			++iop->iptr;
		return '\n';
	}
	++iop->iptr;
	return c;
}

char *
io_gets (io_t *iop, char *_buf, int buflen)
{
	char *buf = _buf;
	int i, c;

	for (i=0; i < buflen-1; ++i) {
		c = io_getc (iop);
		if (c == EOF)
			break;
		*buf++ = c;
	}
	*buf = 0;
	return _buf;
}

int
io_poll (io_t *iop)
{
	fd_set rfds, wfds, efds;
	int r;

	FD_ZERO (&rfds);
	FD_ZERO (&wfds);
	FD_ZERO (&efds);

	if (iop->flags & IO_RDONLY)
		FD_SET (iop->fh, &rfds);
	if (iop->flags & IO_WRONLY)
		FD_SET (iop->fh, &wfds);
	if (iop->flags & IO_EXCEPT)
		FD_SET (iop->fh, &efds);

	do {
		r = select (FD_SETSIZE, &rfds, &wfds, &efds, NULL);
	} while ((r < 0 && errno == EINTR) || !r);

	if (r < 0)
		return -1;

	(*iop->handler->handler) ((long)iop, &rfds, &wfds, &efds);
	return 0;
}

io_t *
io_download (url_t *url, int (*ihandler) (io_t *),
	void (*done) (io_t *, int error),
	void (*redir) (io_t *, url_t *newurl))
{
	io_handler_t *handler;
	io_t *iop;
	fd_set fds;

	iop = io_alloc ();
	if (!iop)
		return NULL;

	iop->flags = IO_RDONLY;
	iop->url = url;

	iop->input = ihandler;
	iop->done  = done;
	iop->redirect = redir;

	handler = io_handler_lookup (url->scheme_id);
	if (!handler)
		goto bad;

	if ((*handler->create) (iop))
		goto bad;

	iop->handler = handler;

	FD_ZERO (&fds);
	FD_SET (iop->fh, &fds);

	iop->handler_id = wt_addinput (
				(iop->flags & IO_RDONLY) ? &fds : NULL,
				(iop->flags & IO_WRONLY) ? &fds : NULL,
				(iop->flags & IO_EXCEPT) ? &fds : NULL,
				iop->handler->handler, (long)iop);
	if (iop->handler_id < 0)
		goto bad;

	return iop;

bad:
	io_free (iop);
	return NULL;
}

int
io_process_input (io_t *iop)
{
	int r;

	io_ref (iop);
	r = (*iop->input) (iop);
	io_deref (iop);
	return r;
}

char *
io_strerror (io_t *iop, int err)
{
	static char buf[200];

	switch (err) {
	case IO_OK:
		strcpy (buf, "everything ok");
		return buf;

	case IO_CANCEL:
		strcpy (buf, "download cancelled");
		return buf;

	case IO_ERR_CONNECT:
		strcpy (buf, "error while connecting");
		break;

	case IO_ERR_RECV:
		strcpy (buf, "error while receiving data");
		break;

	case IO_ERR_SEND:
		strcpy (buf, "error while sending data");
		break;

	case IO_ERR_DECODE:
		strcpy (buf, "error while decoding data");
		break;

	default:
	case IO_ERR_INTERN:
		strcpy (buf, "internal error");
		break;
	}
	if (iop->errcode) {
		char *cp = buf + strlen (buf);
		sprintf (cp, " (%s)", xstrerror (iop->errcode));		
	}
	return buf;
}
