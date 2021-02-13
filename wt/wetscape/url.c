/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * url parsing et al.
 *
 * $Id: url.c,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "url.h"
#include "util.h"


static int url_parse_file (url_t *, char *);
static int url_parse_http (url_t *, char *);
static int url_parse_ftp  (url_t *, char *);

static int url_print_file (url_t *, char *, int);
static int url_print_http (url_t *, char *, int);
static int url_print_ftp  (url_t *, char *, int);

static url_handler_t url_handlers[] = {
	{ URL_HTTP, "http", url_parse_http, url_print_http },
	{ URL_FILE, "file", url_parse_file, url_print_file },
	{ URL_FTP,  "ftp",  url_parse_ftp,  url_print_ftp },
	{ 0,        NULL,   NULL,           NULL }
};

static url_handler_t *
url_lookup_handler (short scheme)
{
	if (scheme > URL_MAX)
		return NULL;

	if (url_handlers[scheme].scheme_id != scheme) {
		printf ("Wetscape bug: %s, %d\n", __FILE__, __LINE__);
		return NULL;
	}
	return &url_handlers[scheme];
}

static url_t *
url_alloc (void)
{
	url_t *url = malloc (sizeof (url_t));
	if (!url)
		return NULL;
	memset (url, 0, sizeof (url_t));
	return url;
}

void
url_free (url_t *url)
{
	/*
	 * url->scheme points to statically allocated memory
	 */
	if (url->address)
		free (url->address);
	if (url->login)
		free (url->login);
	if (url->passwd)
		free (url->passwd);
	if (url->transtype)
		free (url->transtype);
	if (url->path)
		free (url->path);
	if (url->search)
		free (url->search);
	if (url->frag)
		free (url->frag);
	free (url);
}

url_t *
url_clone (url_t *url)
{
	url_t *nu = url_alloc ();

	if (!nu)
		return NULL;

	nu->scheme    = url->scheme;
	nu->scheme_id = url->scheme_id;
	nu->port      = url->port;

	if (url->address && !(nu->address = strdup (url->address)))
		goto bad;
	if (url->login && !(nu->login = strdup (url->login)))
		goto bad;
	if (url->passwd && !(nu->passwd = strdup (url->passwd)))
		goto bad;
	if (url->transtype && !(nu->transtype = strdup (url->transtype)))
		goto bad;
	if (url->path && !(nu->path = strdup (url->path)))
		goto bad;
	if (url->search && !(nu->search = strdup (url->search)))
		goto bad;
	if (url->frag && !(nu->frag = strdup (url->frag)))
		goto bad;
	return nu;

bad:
	url_free (nu);
	return NULL;
}

/*
 * return true if 'url' is a partial URL
 */
int
url_ispart (const char *url)
{
	char *slash_p, *colon_p;

	slash_p = strchr (url, '/');
	colon_p = strchr (url, ':');
	return !(colon_p && (!slash_p || colon_p < slash_p));
}

/*
 * parse an URL string into url_t
 */
url_t *
url_parse (char *cp)
{
	url_handler_t *handler;
	char *colon_p;
	url_t *url;

	url = url_alloc ();
	if (!url)
		return NULL;

	colon_p = strchr (cp, ':');
	if (!colon_p)
		goto bad;

	for (handler = url_handlers; handler->scheme; ++handler) {
		if (!strncasecmp (handler->scheme, cp, colon_p - cp))
			break;
	}
	if (!handler->scheme)
		goto bad;

	url->scheme    = handler->scheme;
	url->scheme_id = handler->scheme_id;

	if (!(*handler->parse) (url, colon_p+1))
		return url;
bad:
	url_free (url);
	return NULL;
}

/*
 * make an absolute url given the current absolute URL ('context') and
 * a (possibly partial) URL string 'cp'.
 */
url_t *
url_make (url_t *context, char *cp)
{
	char strurl[256];
	char *slash, *cp2;
	url_t *newurl;
	int n, l;

	if (!context || !url_ispart (cp))
		return (url_parse (cp));

	switch (*cp) {
	case '/':
		/*
		 * count the number N of leading slashes in the partial url.
		 * then find the first occurence of exactly N consecutive
		 * slashes in 'context' that has no more than N consecutive
		 * slashes to its right and remove everything after that
		 * occurence including the N slashes.
		 * Prepend the result to the partial url.
		 */
		if (url_print (context, strurl, sizeof (strurl)))
			return NULL;

		n = strspn (cp, "/");
		cp2 = strchr (strurl, '/');
		for (slash = NULL; cp2 && *cp2; cp2 = strchr (cp2, '/')) {
			l = strspn (cp2, "/");
			if (l == n) {
				if (!slash)
					slash = cp2;
			} else if (l > n) {
				slash = NULL;
			}
			cp2 += l;
		}
		if (!slash)
			return NULL;
		strcpy (slash, cp);
		break;

	case '#':
		/*
		 * just a new fragment id.
		 */
		newurl = url_clone (context);
		if (!newurl)
			return NULL;
		if (newurl->frag)
			free (newurl->frag);
		newurl->frag = strdup (cp+1);
		return newurl;

	default:
		/*
		 * remove everything after the last slash in 'context'
		 * and prepend the result to the partial url.
		 */
		if (url_print (context, strurl, sizeof (strurl)))
			return NULL;

		slash = xstrrfind (strurl, "/:");
		if (!slash) {
			return NULL;
		} else {
			++slash;
			strcpy (slash, cp);
		}
		break;
	}

	/*
	 * convert:
	 *	'/xxx/../'	-> /
	 *	'/./'		-> /
	 */
	for (slash = cp = strurl; *cp; ) {
		if (!strncmp ("/../", cp, 4)) {
			strcpy (slash, &cp[4]);
			slash = cp = strurl;
		} else if (!strncmp ("/./", cp, 3)) {
			strcpy (&cp[1], &cp[3]);
			slash = cp = strurl;
		} else if (*cp == '/') {
			slash = ++cp;
		} else {
			++cp;
		}
	}
	return url_parse (strurl);
}

/*
 * return !=0 if url1 and url2 refer to the same document (ie fragment ids
 * may differ).
 */
static inline int
compare_strings (const char *s1, const char *s2)
{
	if (!!s1 ^ !!s2)
		return 0;
	if (s1 && strcmp (s1, s2))
		return 0;
	return 1;
}

int
url_same (url_t *url1, url_t *url2)
{
	int port1 = url1->port > 0 ? url1->port : 80;
	int port2 = url2->port > 0 ? url2->port : 80;
	
	return (url1->scheme_id == url2->scheme_id &&
		compare_strings (url1->address, url2->address) &&
		compare_strings (url1->path, url2->path) &&
		compare_strings (url1->search, url2->search) &&
		compare_strings (url1->login, url2->login) &&
		compare_strings (url1->passwd, url2->passwd) &&
		port1 == port2);
}

int
url_print (url_t *url, char *buf, int buflen)
{
	url_handler_t *handler = url_lookup_handler (url->scheme_id);
	if (!handler)
		return -1;
	return (*handler->print) (url, buf, buflen);
}

const char *
url_scheme (short scheme_id)
{
  url_handler_t *handler = url_lookup_handler (scheme_id);
  return handler ? handler->scheme : "(invalid scheme)";
}

/********************** linked list handling *************************/

void
url_insert_after (url_t *url, url_t *new)
{
	new->prev = url;
	new->next = NULL;

	if (url) {
		new->next = url->next;
		url->next = new;
		if (new->next)
			new->next->prev = new;
	}
}

void
url_insert_before (url_t *url, url_t *new)
{
	new->next = url;
	new->prev = NULL;

	if (url) {
		new->prev = url->prev;
		url->prev = new;
		if (new->prev)
			new->prev->next = url;
	}
}

void
url_remove (url_t *url)
{
	if (url->prev)
		url->prev->next = url->next;
	if (url->next)
		url->next->prev = url->prev;

	url->prev = url->next = NULL;
}

/******************** Scheme specific parsers ************************/

static int
url_parse_file (url_t *url, char *cp)
{
	char *next_cp;

	/*
	 * get the hostname
	 */
	if (!strncmp ("//", cp, 2)) {
		cp += 2;
		next_cp = xstrfind (cp, "/?#");
		if (!next_cp) {
			/*
			 * address is the whole rest of the string
			 */
			url->address = strdup (cp);
			cp += strlen (cp);
		} else {
			url->address = xstrndup (cp, next_cp - cp);
			cp = next_cp;
		}
	} else {
		/*
		 * missing address field means "localhost"
		 */
		url->address = strdup ("localhost");
	}
	if (!url->address)
		return -1;

	/*
	 * get the path
	 */
	if (*cp == '/') {
		next_cp = xstrfind (cp, "?#");
		if (!next_cp) {
			url->path = strdup (cp);
			cp += strlen (cp);
		} else {
			url->path = xstrndup (cp, next_cp - cp);
			cp = next_cp;
		}
		if (!url->path)
			return -1;
	}

	/*
	 * get search string
	 */
	if (*cp == '?') {
		++cp;
		next_cp = xstrfind (cp, "#");
		if (!next_cp) {
			url->search = strdup (cp);
			cp += strlen (cp);
		} else {
			url->search = xstrndup (cp, next_cp - cp);
			cp = next_cp;
		}
		if (!url->search)
			return -1;
	}

	/*
	 * get fragment id
	 */
	if (*cp == '#') {
		++cp;
		url->frag = strdup (cp);
		cp += strlen (cp);
		if (!url->frag)
			return -1;
	}
	return 0;
}

static int
url_parse_http (url_t *url, char *cp)
{
	char *next_cp;

	/*
	 * get the hostname
	 */
	if (strncmp ("//", cp, 2))
		return -1;

	cp += 2;
	next_cp = xstrfind (cp, "/?#:");
	if (!next_cp) {
		/*
		 * address is the whole rest of the string
		 */
		url->address = strdup (cp);
		cp += strlen (cp);
	} else {
		url->address = xstrndup (cp, next_cp - cp);
		cp = next_cp;
	}
	if (!url->address)
		return -1;

	/*
	 * get the port
	 */
	if (*cp == ':') {
		url->port = atol (++cp);
		next_cp = xstrfind (cp, "/?#");
		if (next_cp) {
			cp = next_cp;
		} else {
			cp += strlen (cp);
		}
	}
	
	/*
	 * get the path
	 */
	if (*cp == '/') {
		next_cp = xstrfind (cp, "?#");
		if (!next_cp) {
			url->path = strdup (cp);
			cp += strlen (cp);
		} else {
			url->path = xstrndup (cp, next_cp - cp);
			cp = next_cp;
		}
		if (!url->path)
			return -1;
	}

	/*
	 * get search string
	 */
	if (*cp == '?') {
		++cp;
		next_cp = xstrfind (cp, "#");
		if (!next_cp) {
			url->search = strdup (cp);
			cp += strlen (cp);
		} else {
			url->search = xstrndup (cp, next_cp - cp);
			cp = next_cp;
		}
		if (!url->search)
			return -1;
	}

	/*
	 * get fragment id
	 */
	if (*cp == '#') {
		++cp;
		url->frag = strdup (cp);
		cp += strlen (cp);
		if (!url->frag)
			return -1;
	}
	return 0;
}

static int
url_parse_ftp (url_t *url, char *cp)
{
	return 0;
}

/******************** Scheme specific printers ************************/

static int
url_print_file (url_t *url, char *buf, int buflen)
{
	sprintf (buf, "file://%s%s", url->address, url->path ?: "");
	buf += strlen (buf);
	if (url->search) {
		sprintf (buf, "?%s", url->search);
		buf += strlen (buf);
	}
	if (url->frag) {
		sprintf (buf, "#%s", url->frag);
		buf += strlen (buf);
	}
	return 0;
}

static int
url_print_http (url_t *url, char *buf, int buflen)
{
	sprintf (buf, "http://%s", url->address);
	buf += strlen (buf);
	if (url->port > 0) {
		sprintf (buf, ":%d", url->port);
		buf += strlen (buf);
	}
	if (url->path) {
		sprintf (buf, "%s", url->path);
		buf += strlen (buf);
	}
	if (url->search) {
		sprintf (buf, "?%s", url->search);
		buf += strlen (buf);
	}
	if (url->frag) {
		sprintf (buf, "#%s", url->frag);
		buf += strlen (buf);
	}
	return 0;
}

static int
url_print_ftp (url_t *url, char *buf, int buflen)
{
	return 0;
}
