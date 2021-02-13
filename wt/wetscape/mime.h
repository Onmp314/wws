/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * MIME related stuff.
 *
 * $Id: mime.h,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#ifndef _MIME_H
#define _MIME_H

#define MIME_DEFAULT	"text/plain"

struct _io_t;

typedef struct _decoder_t {
	int  (*create) (struct _io_t *);
	int  (*decode) (struct _io_t *);
	void (*free) (struct _io_t *);
} decoder_t;

extern const char*	mime_get_type (const char *ext);
extern decoder_t*	mime_get_decoder (const char *type);
extern const char*	mime_get_command (const char *type);
extern int		mime_input_handler (struct _io_t *);

#endif
