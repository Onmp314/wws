/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * handler for 'file:' urls.
 *
 * $Id: io_file.c,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "url.h"
#include "io.h"
#include "mime.h"
#include "util.h"


static int
io_file_create (io_t *iop)
{
	struct stat st;
	long size;
	const char *ext;

	if (strcasecmp ("localhost", iop->url->address)) {
		status_set ("document is unreachable.");
		return -1;
	}
	if (stat (iop->url->path, &st) < 0) {
		status_set ("cannot stat file.");
		return -1;
	}

	size = MIN (8000, st.st_size);
	if (io_alloc_ibuf (iop, size))
		return -1;

	iop->fh = open (iop->url->path, O_RDONLY);
	if (iop->fh < 0) {
		status_set ("cannot open file.");
		iop->fh = 0;
		return -1;
	}

	/*
	 * try to find out what is in the file
	 */
	ext = strrchr (iop->url->path, '.');
	iop->mimetype = ext ? mime_get_type (ext+1) : MIME_DEFAULT;
	iop->timestamp = st.st_mtime;
	return 0;
}

static void
io_file_delete (io_t *iop)
{
	if (iop->fh) {
		close (iop->fh);
		iop->fh = 0;
	}
}

static void
io_file_handler (long _iop, fd_set *rfds, fd_set *wfds, fd_set *efds)
{
	io_t *iop = (io_t *)_iop;
	int r;

	if (!FD_ISSET (iop->fh, rfds))
		return;

	while (!iop->eof) {
		r = read (iop->fh, &iop->ibuf[iop->ibufused],
			iop->ibuflen - iop->ibufused);
		if (r < 0) {
			iop->errcode = errno;
			(*iop->done) (iop, IO_ERR_RECV);
			break;
		}
		if (r == 0) {
			iop->eof = 1;
		}
		iop->ibufused += r;
		if (io_process_input (iop) < 0)
			break;
	}
}


io_handler_t file_io_handler = {
	URL_FILE, io_file_create, io_file_delete, io_file_handler
};
