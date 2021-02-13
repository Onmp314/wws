/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * input/output support routines.
 *
 * $Id: io.h,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#ifndef _IO_H
#define _IO_H

#include <sys/types.h>

#define IO_RDONLY	1
#define IO_WRONLY	2
#define IO_RDWR		3
#define IO_EXCEPT	4

#define IO_CANCEL	(1)
#define IO_OK		 0
#define IO_ERR_CONNECT	(-1)
#define IO_ERR_RECV	(-2)
#define IO_ERR_DECODE	(-3)
#define IO_ERR_INTERN	(-4)
#define IO_ERR_SEND	(-6)

struct _io_handler_t;
struct _url_t;
struct _decoder_t;

typedef struct _io_t {
	struct _io_t *next;

	struct _url_t	*url;

	short	fh;				/* file handle */
	short	flags;				/* IO_RDONLY, ... */
	short	eof;
	short	errcode;

	const char *mimetype;			/* type of data */
	struct _decoder_t *decoder;

	long	timestamp;			/* last modified */

	char	*ibuf;				/* input buffer */
	long	ibuflen;
	long	ibufused;
	long	ibufoffset;
	long	iptr;

	char	*obuf;				/* output buffer */
	long	obuflen;
	long	obufused;
	long	obufoffset;
	long	optr;

	int	(*input) (struct _io_t *);	/* input ready */
	int	(*output) (struct _io_t *);	/* output ready */
	void	(*done) (struct _io_t *, int error); /* tranfer done */
	void	(*redirect) (struct _io_t *, struct _url_t *);

	struct _io_handler_t *handler;
	long	handler_id;

	void	*ihook;				/* hook for input handler */
	void	*ohook;				/* hook for output handler */
	void	*hhook;				/* hook for the downloader */

	int	refcnt;				/* reference counter */

	unsigned is_image : 1;			/* is a picture */
	unsigned do_save : 1;			/* save to disk */
	unsigned show_status : 1;		/* show status for this io_t */
	unsigned main_doc : 1;			/* main document */
} io_t;

typedef struct _io_handler_t {
	short	scheme_id;
	int	(*create) (io_t *);
	void	(*free) (io_t *);
	void	(*handler) (long iop, fd_set *, fd_set *, fd_set *);
} io_handler_t;


extern int	io_alloc_ibuf (io_t *, long size);
extern int	io_alloc_obuf (io_t *, long size);

extern void	io_eat_input (io_t *, long size);
extern void	io_eat_output (io_t *, long size);

extern int	io_getc (io_t *);
extern char*	io_gets (io_t *, char *buf, int buflen);

extern void	io_ref (io_t *);
extern void	io_deref (io_t *);

extern int	io_poll (io_t *);
extern io_t*	io_download (struct _url_t *,
			int (*ihandler) (io_t *),
			void (*done) (io_t *, int error),
			void (*redir) (io_t *, struct _url_t *));

extern int	io_process_input (io_t *);
extern char*	io_strerror (io_t *, int err);

#endif
