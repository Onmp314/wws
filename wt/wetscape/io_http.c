/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * handler for 'http:' urls.
 *
 * $Id: io_http.c,v 1.4 2009-08-23 20:27:36 eero Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "url.h"
#include "io.h"
#include "mime.h"
#include "util.h"
#include "proxy.h"

/*
 * http header options we know about
 */
typedef enum {
	OptIgnore, ContLength, ContType, ContEncoding, ContTransEncoding,
	LastModified, Location
} option_t;

/*
 * internal state of the http reply parser
 */
typedef enum {
	InStatus, InOption, InValue, InEOL, InBody
} state_t;

/*
 * http reply parser record.
 */
typedef struct {
	state_t  state;
	option_t option;
	char	 buf[1024];

	long	 contentlen;
	short	 status;
	char*	 reason;
	char*	 newurl;

	long	 percent;
} http_info_t;

/************************ http_info_t support *************************/

static http_info_t *
http_info_alloc (void)
{
	http_info_t *http = malloc (sizeof (http_info_t));
	if (!http)
		return NULL;

	memset (http, 0, sizeof (http_info_t));
	http->state = InStatus;

	return http;
}

static void
http_info_free (http_info_t *http)
{
	if (http->reason)
		free (http->reason);
	if (http->newurl)
		free (http->newurl);
	free (http);
}

/************************ HTTP reply parser ***************************/

static char *
skipword (char *cp)
{
	while (*cp && !isspace (*cp))
		++cp;
	while (*cp && isspace (*cp))
		++cp;
	return cp;
}

static char *
skipspace (char *cp)
{
	while (*cp && isspace (*cp))
		++cp;
	return cp;
}

static int
getline (io_t *iop, int endc)
{
	http_info_t *http = iop->hhook;
	char *cp;
	int c;

	cp = http->buf + strlen (http->buf);
	while ((c = io_getc (iop)) != EOF && c != endc) {
		if (cp - http->buf < sizeof (http->buf)-1)
			*cp++ = c;
	}
	*cp = 0;
	return c == EOF;
}

/*
 * remove HTTP comments from 'str'
 */
static void
kill_comments (char *str)
{
	char *cp, *cp2;

	cp = str;
	while ((cp = strchr (cp, '('))) {
		cp2 = strchr (cp, ')');
		if (!cp2) {
			/*
			 * Hmm. unclosed comment.
			 */
			break;
		}
		memmove (cp, cp2, strlen(cp2)+1);
	}
}

/*
 * get the next token
 */
static char *
gettoken (char *str, int *len)
{
	char *cp, *cp2;

	cp = skipspace (str);
	switch (*cp) {
	case '"':
		++cp;
		cp2 = strchr (cp, '"');
		if (!cp2) {
			/*
			 * Hmm. unclosed quoted string
			 */
			cp2 += strlen (cp);
		}
		break;

	case '<':
		++cp;
		cp2 = strchr (cp, '>');
		if (!cp2) {
			/*
			 * Hmm. unclosed quoted string
			 */
			cp2 += strlen (cp);
		}
		break;

	default:
		*len = strcspn (cp, " \n\t");
		return cp;
	}
	*len = cp2 - cp;
	return cp;
}

/*
 * parse HTTP reply status line
 */
static void
parse_status (io_t *iop)
{
	http_info_t *http = iop->hhook;
	char *cp;

	/*
	 * skip http version
	 */
	cp = skipword (http->buf);
	/*
	 * get status code
	 */
	http->status = atoi (cp);
	cp = skipword (cp);
	/*
	 * get reason
	 */
	http->reason = strdup (cp);
}

/*
 * parse option name
 */
static void
parse_option (io_t *iop)
{
	http_info_t *http = iop->hhook;

	if (!strcasecmp (http->buf, "Content-Length")) {
		http->option = ContLength;
	} else if (!strcasecmp (http->buf, "Content-Type")) {
		http->option = ContType;
	} else if (!strcasecmp (http->buf, "Content-Encoding")) {
		http->option = ContEncoding;
	} else if (!strcasecmp (http->buf, "Location")) {
		http->option = Location;
	} else {
		http->option = OptIgnore;
	}
}

/*
 * parse option value.
 *
 */
static void
parse_value (io_t *iop)
{
	http_info_t *http = iop->hhook;
	char *cp;
	int len;

	kill_comments (http->buf);
	switch (http->option) {
	case ContLength:
		http->contentlen = atol (gettoken (http->buf, &len));
		break;

	case ContType:
		/*
		 * ignore parameters after semicolon.
		 */
		cp = gettoken (http->buf, &len);
		iop->mimetype = xstrndup (cp, len);
		cp = xstrfind (cp, " \t\n;");
		if (cp) {
			*cp = '\0';
		}
		break;

	case Location:
		if ((cp = gettoken (http->buf, &len))) {
			http->newurl = xstrndup (cp, len);
		}
		break;

	default:
		break;
	}
}

/*
 * parse HTTP reply. this is implemented as a simple inite state machine
 * with the following states:
 *	InStatus	-- reading reply status line
 *	InOption	-- reading an option name
 *	InValue		-- reading an option value
 *	InEOL		-- at end of line after InValue
 *	InBody		-- reading body
 */
static void
parse_http_reply (io_t *iop)
{
	http_info_t *http = iop->hhook;
	int c, end = 0;
	long iptr;

	while (!end) switch (http->state) {
	case InStatus:
		if (iop->ibufoffset == 0) {
			/*
			 * we are at the beginning of the reply.
			 * try to figure out whether its a 0.9 or 1.0
			 * reply.
			 */
			if (iop->ibufused < 4) {
				end = 1;
				break;
			}
			if (strncasecmp ("HTTP", iop->ibuf, 4)) {
				/*
				 * HTTP/0.9 reply
				 */
				http->state = InBody;
				end = 1;
				break;
			}
		}
		if (getline (iop, '\n')) {
			end = 1;
			break;
		}
		parse_status (iop);
		http->state = InOption;
		http->buf[0] = 0;
		break;

	case InOption:
		if (getline (iop, ':')) {
			end = 1;
			break;
		}
		parse_option (iop);
		http->state = InValue;
		http->buf[0] = 0;
		break;

	case InValue:
		if (getline (iop, '\n')) {
			end = 1;
			break;
		}
		http->state = InEOL;
		break;

	case InEOL:
		iptr = iop->iptr;
		c = io_getc (iop);
		if (c == '\n') {
			/*
			 * empty line seen: body follows.
			 */
			parse_value (iop);
			http->buf[0] = 0;
			http->state = InBody;
			end = 1;
		} else if (isspace (c)) {
			/*
			 * space at beginning of line: value
			 * from last line continues.
			 * unget c and goto InValue.
			 */
			iop->iptr = iptr;
			http->state = InValue;
		} else if (c != EOF) {
			/*
			 * nonspace character at beginning of line.
			 * new option starts. unget c, eval value
			 * and goto InOption.
			 */
			iop->iptr = iptr;
			parse_value (iop);
			http->buf[0] = 0;
			http->state = InOption;
		} else {
			/*
			 * no more input available.
			 */
			end = 1;
		}
		break;

	case InBody:
		end = 1;
		break;
	}
	io_eat_input (iop, iop->iptr);
	iop->iptr = 0;
	if (http->state == InBody) {
		iop->ibufoffset = 0;
	}
}

/*************************** Download Status ************************/

static void
update_status (io_t *iop)
{
	http_info_t *http = iop->hhook;
	long percent;

	if (http->contentlen > 0) {
		percent = iop->ibufoffset*100/http->contentlen;
		if (percent - http->percent >= 5) {
			status_set ("received %ld%s of %ld bytes.",
				percent, "%", http->contentlen);
			http->percent = percent;
		}
	} else if (iop->ibufoffset > 0) {
		percent = (iop->ibufoffset-http->percent)*100/iop->ibufoffset;
		if (percent >= 5) {
			status_set ("received %ld bytes.", iop->ibufoffset);
			http->percent = iop->ibufoffset;
		}
	}
}

/*************************** HTTP IO Handler ************************/

static void
reject_proxy (io_t *iop)
{
	status_set ("http proxy unreachable. retrying without proxy ...");
	sleep (1);

	servaddr_free (glob_http_proxy);
	glob_http_proxy = NULL;

	if (iop->fh > 0) {
		close (iop->fh);
		iop->fh = 0;
	}
}

static int
doconnect (io_t *iop)
{
	http_info_t *http;
	servaddr_t *serv;
	char *cp;
	int r;

	http = http_info_alloc ();
	if (!http)
		return -1;
	iop->hhook = http;

restart:
	if (glob_http_proxy) {
	  serv = glob_http_proxy;
	} else {
	  serv = servaddr_alloc (NULL, iop->url->address,
	                         (iop->url->port > 0) ? iop->url->port : 80);
	}
	assert (serv);
	status_set ("connecting to %s ...", serv->name);

	/*
	 * We don't want SIGPIPE... On SunOS 4.1.3 it seems impossible
	 * to catch the SIGPIPE with signal() or sigaction(). Don't know why.
	 */
	sigblock (sigmask (SIGPIPE));

	if (servaddr_resolve (serv) < 0) {
	  if (glob_http_proxy == serv) {
	    /*
	     * proxy unreachable...
	     */
	    reject_proxy (iop);
	    goto restart;
	  }
	  status_set ("cannot resolve host name.");
	  servaddr_free (serv);
	  return -1;
	}
	if (serv != glob_http_proxy && iop->url->scheme_id != URL_HTTP) {
	  status_set ("cannot do url type `%s' without proxy.",
	       	      url_scheme (iop->url->scheme_id));
	  servaddr_free (serv);
	  return -1;
	}

	/*
	 * craete socket and make it nonblocking.
	 */
	iop->fh = socket (PF_INET, SOCK_STREAM, 0);
	if (iop->fh < 0) {
		iop->fh = 0;
		if (serv != glob_http_proxy)
		  servaddr_free (serv);
		return -1;
	}
	fcntl (iop->fh, F_SETFL, fcntl (iop->fh, F_GETFL, 0) | O_NDELAY);

	/*
	 * connect to server
	 */
	r = connect (iop->fh, (struct sockaddr *)&serv->sin,
		     sizeof (serv->sin));
	if (r < 0 && errno != EINPROGRESS) {
		if (glob_http_proxy == serv) {
			reject_proxy (iop);
			goto restart;
		}
		status_set ("cannot contact host.");
		servaddr_free (serv);
		return -1;
	}

	/*
	 * WRONLY because we first select for writing until the connect()
	 * finished. Then we write the request and switch to RDONLY.
	 */
	iop->flags = IO_WRONLY;

	if (io_alloc_ibuf (iop, 8000) || io_alloc_obuf (iop, 1000)) {
	  if (glob_http_proxy != serv)
	    servaddr_free (serv);
	  return -1;
	}

	/*
	 * format the request. if not using a proxy we only send a partial
	 * URL, since some servers (ncsa) die on full URL's with 404 for
	 * some reason.
	 */
	cp = iop->obuf;
	if (glob_http_proxy == serv) {
	  sprintf (cp, "GET ");
	  cp += strlen (cp);
	  url_print (iop->url, cp, iop->obuflen - (cp - iop->obuf));
	  cp += strlen (cp);
	  sprintf (cp, " HTTP/1.0\r\n");
	} else {
	  sprintf (cp, "GET %s%s%s HTTP/1.0\r\nHost: %s\r\n",
	           iop->url->path ?: "/",
		   iop->url->search ? "?" : "",
       		   iop->url->search ?: "",
		   iop->url->address);
	}
	cp += strlen (cp);
	sprintf (cp, "User-Agent: "
		"WetScape/" WETSCAPE_VERSION "\r\n\r\n");

	iop->obufused = strlen (iop->obuf);
	if (glob_http_proxy != serv)
	  servaddr_free (serv);
	return 0;
}

static int
doredirect (io_t *iop)
{
	http_info_t *http = iop->hhook;
	fd_set wrset;
	url_t *url;
	int r;

	if (!http->newurl || (http->status != 302 && http->status != 303))
		return 0;

	url = url_make (iop->url, http->newurl);
	if (!url)
		return 0;

	if (iop->redirect)
		(*iop->redirect) (iop, url);

	url_free (iop->url);
	iop->url = url;

	http_info_free (iop->hhook);
	iop->hhook = NULL;

	free (iop->ibuf);
	iop->ibuf = NULL;

	free (iop->obuf);
	iop->obuf = NULL;

	iop->eof = 0;

	if (iop->fh > 0) {
		close (iop->fh);
		iop->fh = 0;
	}
	if (iop->handler_id >= 0) {
		wt_delinput (iop->handler_id);
		iop->handler_id = -1;
	}

	r = doconnect (iop);
	if (r < 0) {
		(*iop->done) (iop, IO_ERR_CONNECT);
		return -1;
	}

	FD_ZERO (&wrset);
	FD_SET (iop->fh, &wrset);
	iop->handler_id = wt_addinput (NULL, &wrset, NULL,
				iop->handler->handler, (long)iop);
	if (iop->handler_id < 0) {
		(*iop->done) (iop, IO_ERR_INTERN);
		return -1;
	}
	return 1;
}

static int
io_http_create (io_t *iop)
{
	return doconnect (iop);
}

static void
io_http_delete (io_t *iop)
{
	if (iop->fh > 0) {
		close (iop->fh);
		iop->fh = 0;
	}
	if (iop->hhook) {
		http_info_free (iop->hhook);
		iop->hhook = NULL;
	}
}

static void
io_http_handler (long _iop, fd_set *rfds, fd_set *wfds, fd_set *efds)
{
	io_t *iop = (io_t *)_iop;
	http_info_t *http = iop->hhook;
	int r;

	if (FD_ISSET (iop->fh, wfds)) {
		/*
		 * the connect either succeeded and we can send the request
		 * now or the connect failed and we get an error.
		 */
		r = write (iop->fh, iop->obuf, iop->obufused);
		if (r < 0 && errno != EWOULDBLOCK) {
			iop->errcode = errno;
			r = iop->obufoffset > 0 ? IO_ERR_SEND : IO_ERR_CONNECT;
			(*iop->done) (iop, r);
			return;
		}
		if (r > 0) {
			io_eat_output (iop, r);
		}
		if (iop->obufused <= 0) {
			/*
			 * request sent. change input handler to just wait
			 * for reading from now on.
			 */
			fd_set rdset;

			FD_ZERO (&rdset);
			FD_SET (iop->fh, &rdset);
			wt_chginput (iop->handler_id, &rdset, NULL, NULL);
			iop->flags = IO_RDONLY;

			status_set ("host contacted. waiting for reply.");
		}
	}
	if (FD_ISSET (iop->fh, rfds)) {
		r = read (iop->fh, &iop->ibuf[iop->ibufused],
			iop->ibuflen - iop->ibufused);
		if (r < 0 && errno != EWOULDBLOCK) {
			iop->errcode = errno;
			(*iop->done) (iop, IO_ERR_RECV);
			return;
		}
		if (r >= 0) {
#ifdef __MINT__
			u_long avail;
#endif
			if (r == 0 && !iop->eof
#ifdef __MINT__
			    /*
			     * under MiNT, 0 is returned if currently
			     * no data is available, i.e. it does not
			     * imply EOF.
			     */
			    && !ioctl (iop->fh, FIONREAD, &avail)
			    && avail > 100000L
#endif
			) {
				iop->eof = 1;
				close (iop->fh);
				iop->fh = 0;
				wt_delinput (iop->handler_id);
				iop->handler_id = -1;
			}
			iop->ibufused += r;

			if (http->state != InBody) {
				parse_http_reply (iop);
				if (http->state == InBody && doredirect (iop))
					return;
			}
			if (http->state == InBody) {
				if (!iop->mimetype) {
					/*
					 * somehow the http header had no
					 * Content-Type in it...
					 */
					iop->mimetype = strdup ("text/plain");
				}
				if (iop->show_status)
					update_status (iop);
				io_process_input (iop);
			}
		}
	}
}


io_handler_t http_io_handler = {
	URL_HTTP, io_http_create, io_http_delete, io_http_handler
};
