/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * $Id: wetscape.h,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#ifndef _WETSCAPE_H
#define _WETSCAPE_H

#include <assert.h>
#include "version.h"
#include "proxy.h"

struct _image_t;
struct _url_t;
struct _io_t;

extern widget_t *shell;

extern void status_set (const char *, ...);

extern void html_clear (void);
extern void html_resize (long wd, long ht);
extern void html_set (const char *);
extern void html_append (const char *);
extern void html_setpic (struct _url_t *);
extern void html_goto (const char *);
extern WWIN* html_getwin (void);

extern void urltext_set (const char *);

extern void image_add (struct _image_t *);
extern int  image_del (struct _image_t *);

extern void download_lock (struct _io_t *);
extern void download_unlock (struct _io_t *);

/*
 * config vars
 */

extern int config (void);

extern servaddr_t *glob_http_proxy;

extern struct _url_t *glob_homeurl;

extern struct _url_t *glob_bookmarkurl;
extern char *glob_bookmarkfile;

#endif
