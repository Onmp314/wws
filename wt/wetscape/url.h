/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * url parsing et al.
 *
 * $Id: url.h,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#ifndef _URL_H
#define _URL_H

#define URL_HTTP	0
#define URL_FILE	1
#define URL_FTP		2
#define URL_MAX		2

typedef struct _url_t {
	struct _url_t *next, *prev;

	short		scheme_id;
	const char	*scheme;
	char		*address;
	char		*login;
	char		*passwd;
	char		*transtype;
	int		port;
	char		*path;
	char		*search;
	char		*frag;
} url_t;

typedef struct {
	short		scheme_id;
	const char	*scheme;
	int		(*parse) (url_t *, char *url_rest);
	int		(*print) (url_t *, char *buf, int buflen);
} url_handler_t;

extern void	url_free   (url_t *);
extern url_t*	url_clone  (url_t *);
extern int	url_ispart (const char *url_string);
extern url_t*	url_parse  (char *url_string);
extern url_t*	url_make   (url_t *context_url, char *url_string);
extern int	url_print  (url_t *, char *print_buf, int buflen);
extern int	url_same   (url_t *, url_t *);
extern const char*	url_scheme (short id);

extern void	url_insert_before (url_t *url, url_t *new);
extern void	url_insert_after (url_t *url, url_t *new);
extern void	url_remove (url_t *url);

#endif
