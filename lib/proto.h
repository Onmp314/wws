/*
 * proto.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- prototypes for internal Wlib functions and variables
 */

#ifndef __W_PROTO_H
#define __W_PROTO_H

#include <stdio.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "../server/config.h"
#include "../server/pakets.h"


/* in util.c */
#ifdef TRACE
extern int _wtrace;
extern int _traceIndent;
extern void _wspaces(void);
#define TRACESTART() { _traceIndent += 2; }
#define TRACEPRINT(args) if (_wtrace) { _wspaces(); printf args ; fflush(stdout); }
#define TRACEEND() { _traceIndent -= 2; }
#else
#define TRACESTART()
#define TRACEPRINT(args)
#define TRACEEND()
#endif


/* in socket.c */
extern WSERVER	_wserver;
extern long	_winitialize (void);
extern void	_wexit(void);
extern const char *_check_window (WWIN *ptr);
extern void	*_wreservep (int size);
extern PAKET	*_wait4paket (short type);
extern void	_wremove_events (WWIN *win);
extern WEVENT	*_wait4event (fd_set *rfd, fd_set *wfd, fd_set *xfd,
			    long timeout, int *numReady);


/* in convert.c */
extern void	_wget_options(uchar **graymap, int *expand);
extern uchar	*_winit_mapping(BITMAP *bm, int row);


#endif /* __W_PROTO */
