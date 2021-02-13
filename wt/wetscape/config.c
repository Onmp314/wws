/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * config file reader.
 *
 * $Id: config.c,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "url.h"
#include "defaults.h"
#include "proxy.h"

/*
 * max line length
 */
#define INPUT_LINE_MAX        1024


static int line = 0;
static const char *file = "<argv>";

typedef struct {
	const char *name;
	void (*fn) (char *val);
} option_t;

/************************************************************************/

char	*glob_bookmarkfile;
url_t	*glob_bookmarkurl;

url_t   *glob_homeurl;

servaddr_t *glob_http_proxy;

/************************************************************************/

static void
parse_http_proxy (char *value)
{
  char *cp;
  short port = 80;

  cp = strchr (value, ':');
  if (cp) {
    port = atol (cp+1);
    *cp = 0;
  }
  glob_http_proxy = servaddr_alloc (NULL, value, port);
}

static void
parse_home_page (char *value)
{
	if (glob_homeurl)
		url_free (glob_homeurl);

	glob_homeurl = url_parse (value);
	if (!glob_homeurl) {
		fprintf (stderr, "%s:%d: invalid home page `%s'\n",
			file, line, value);
	}
}

static void
parse_bookmark_file (char *value)
{
	char buf[256];

	if (glob_bookmarkurl)
		url_free (glob_bookmarkurl);
	if (glob_bookmarkfile)
		free (glob_bookmarkfile);

	sprintf (buf, "file:%s", value);
	glob_bookmarkurl = url_parse (buf);
	glob_bookmarkfile = strdup (value);
}

static option_t opttab[] = {
	{ "http-proxy",		parse_http_proxy },
	{ "home-page",		parse_home_page },
	{ "bookmark-file",	parse_bookmark_file },
	{ NULL,			NULL }
};

/************************************************************************/

static int
parse_option (char *option, char *value)
{
	option_t *op;
	char *cp;

	cp = value;
	if (*cp == '"') {
		/*
		 * a string
		 */
		++value;
		for ( ++cp; *cp && *cp != '"'; ++cp)
			;
		if (!*cp) {
			fprintf (stderr, "%s:%d: unterminated string\n",
				file, line);
			return -1;
		}
		*cp = '\0';
	}
	for (op = opttab; op->name; ++op) {
		if (!strcasecmp (option, op->name))
			break;
	}
	if (!op->name) {
		fprintf (stderr, "%s:%d: unknown option `%s'\n",
			file, line, option);
		return -1;
	}
	(*op->fn) (value);
	return 0;
}

static int
read_config (const char *fname)
{
	FILE *fp;
	char buf[INPUT_LINE_MAX];
	char *cp, *opt, *val;

	file = fname;
	line = 1;
	fp = fopen (file, "r");
	if (!fp) {
		fprintf (stderr, "cannot open config file '%s'\n", file);
		return -1;
	}
	for ( ; fgets (buf, INPUT_LINE_MAX, fp); ++line) {
		if (strlen (buf) > INPUT_LINE_MAX-10) {
			fprintf (stderr, "%s:%d: skipping too long line\n",
				file, line);
			continue;
		}
		/*
		 * remove comments
		 */
		if ((cp = strchr (buf, '#')))
			*cp = '\0';
		/*
		 * skip leading white space
		 */
		for (cp = buf; *cp && isspace (*cp); ++cp)
			;
		if (!*cp)
			continue;
		/*
		 * first string of chars is option name
		 */
		opt = cp;
		for ( ; *cp && !isspace (*cp); ++cp)
			;
		*cp++ = '\0';
		/*
		 * skip white space between option name and value
		 */
		for ( ; *cp && isspace (*cp); ++cp)
			;
		val = cp;
		parse_option (opt, val);
	}
	if (ferror(fp)) {
		fprintf (stderr, "%s:%d: error reading config file\n",
			file, line);
		fclose (fp);
		return -1;
	}
	fclose (fp);
	return 0;
}

static int
make_config_dir (const char *home, char *buf)
{
	FILE *fp;

	sprintf (buf, "%s/.wetscape", home);
	if (mkdir (buf, 0755) < 0) {
		fprintf (stderr, "cannot make config dir `%s'\n", buf);
		return -1;
	}

	sprintf (buf, "%s/.wetscape/config", home);
	fp = fopen (buf, "w");
	if (!fp) {
		fprintf (stderr, "cannot create config file `%s'\n", buf);
		return -1;
	}
	fwrite (DEF_CONFIG_FILE, strlen (DEF_CONFIG_FILE), 1, fp);
	fclose (fp);

	sprintf (buf, "%s/.wetscape/bookmarks.html", home);
	fp = fopen (buf, "w");
	if (!fp) {
		fprintf (stderr, "cannot create bookmark file `%s'\n", buf);
		return -1;
	}
	fwrite (DEF_BOOKMARK_FILE, strlen (DEF_BOOKMARK_FILE), 1, fp);
	fclose (fp);

	return 0;
}

int
config (void)
{
	const char *home;
	char buf[256];

	home = getenv ("HOME");
	if (!home) {
		fprintf (stderr, "$HOME is not set!?\n");
		return -1;
	}
	sprintf (buf, "%s/.wetscape", home);
	if (access (buf, R_OK|X_OK)) {
		fprintf (stderr, "You are running WetScape the first time.\n"
			"Creating config directory `%s' ...\n", buf);
		if (make_config_dir (home, buf))
			return -1;
	}
	sprintf (buf, "%s/.wetscape/bookmarks.html", home);
	parse_bookmark_file (buf);

	sprintf (buf, "%s/.wetscape/config", home);
	return read_config (buf);
}
