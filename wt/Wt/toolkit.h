/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * $Id: toolkit.h,v 1.2 2008-08-29 19:47:09 eero Exp $
 */
#ifndef _TOOLKIT_H
#define _TOOLKIT_H

extern void read_config(const char *config);

typedef struct _input_t {
	fd_set rd, wr, ex;
	long arg;
	long id;
	short refcnt;
	void (*handler) (long arg, fd_set *r, fd_set *w, fd_set *e);
	struct _input_t *next;
} input_t;

typedef struct _timeout_t {
	long delta;
	long arg;
	long id;
	void (*handler) (long arg);
	struct _timeout_t *next;
} timeout_t;

#endif
