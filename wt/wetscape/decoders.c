/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * several input data decoders.
 *
 * $Id: decoders.c,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "io.h"
#include "mime.h"
#include "url.h"
#include "util.h"

/************************** HTML decoder ***************************/

static int
html_decoder_create (io_t *iop)
{
	html_clear ();
	return 0;
}

static void
html_decoder_free (io_t *iop)
{
}

static int
html_decoder_decode (io_t *iop)
{
	char *cp, backup;

	iop->ibuf[iop->ibufused] = '\0';
	if (iop->eof) {
		html_append (iop->ibuf);
		io_eat_input (iop, iop->ibufused);
		return 0;
	}
	cp = xstrrfind (iop->ibuf, "<>");
	if (cp) {
		if (*cp == '>')
			++cp;
		backup = *cp;
		*cp = 0;
		html_append (iop->ibuf);
		*cp = backup;
		io_eat_input (iop, cp - iop->ibuf);
		return 0;
	}
	if (iop->ibufused >= iop->ibuflen) {
		html_append (iop->ibuf);
		io_eat_input (iop, iop->ibufused);
		/*
		 * we append an invalid tag here. This is necessary...
		 */
		html_append ("<X>");
	}
	return 0;
}

decoder_t html_decoder = {
	html_decoder_create,  html_decoder_decode, html_decoder_free
};

/************************* Plaintext decoder ************************/

#define BUFSIZE	8000

typedef struct {
	char buf[BUFSIZE+20];
	char *cp;
} text_info_t;

static int
text_decoder_create (io_t *iop)
{
	text_info_t *tp = malloc (sizeof (text_info_t));
	if (!tp)
		return -1;
	tp->cp = tp->buf;
	iop->ihook = tp;
	html_clear ();
	return 0;
}

static void
text_decoder_free (io_t *iop)
{
	if (iop->ihook) {
		free (iop->ihook);
		iop->ihook = NULL;
	}
}

static inline void
text_flush (text_info_t *tp)
{
	if (tp->cp > tp->buf) {
		strcpy (tp->cp, "<X>");
		html_append (tp->buf);
		tp->cp = tp->buf;
	}
}

static void
text_append (text_info_t *tp, const char *cp, int len)
{
	long cando;

	while (len > 0) {
		if (len < 10) {
			/*
			 * don't break special chars
			 */
			cando = len;
		} else {
			cando = MIN (BUFSIZE - (tp->cp - tp->buf), len);
		}
		memcpy (tp->cp, cp, cando);
		tp->cp += cando;
		cp += cando;
		len -= cando;
		if (tp->cp - tp->buf >= BUFSIZE)
			text_flush (tp);
	}
}

static int
text_decoder_decode (io_t *iop)
{
	const char *cp, *cp2;

	if (iop->ibufused == 0 && !iop->eof)
		return 0;

	iop->ibuf[iop->ibufused] = '\0';
	if (iop->ibufoffset == 0) {
		/*
		 * at beginning of text
		 */
		text_append (iop->ihook, "<tt><pre>", strlen ("<tt><pre>"));
	}
	cp = iop->ibuf;
	while (*cp) {
		cp2 = xstrfind (cp, "<>\"&");
		if (!cp2) {
			text_append (iop->ihook, cp, strlen (cp));
			break;
		}
		if (cp2 > cp) {
			text_append (iop->ihook, cp, cp2 - cp);
		}
		switch (*cp2) {
		case '<':
			cp = "&lt;";
			break;
		case '>':
			cp = "&gt;";
			break;
		case '"':
			cp = "&quot;";
			break;
		case '&':
			cp = "&amp;";
			break;
		}
		text_append (iop->ihook, cp, strlen (cp));
		cp = cp2 + 1;
	}
	if (iop->eof) {
		/*
		 * at end of text
		 */
		text_append (iop->ihook, "</pre></tt>", strlen("</pre></tt>"));
		text_flush (iop->ihook);
	}
	io_eat_input (iop, iop->ibufused);
	return 0;
}

decoder_t text_decoder = {
	text_decoder_create,  text_decoder_decode, text_decoder_free
};

/********************* Save-To-Disk Decoder ************************/

static int
save_decoder_create (io_t *iop)
{
	const char *path;
	char *cp;

	if (iop->url->path) {
		path = strrchr (iop->url->path, '/');
		path = path ? path+1 : iop->url->path;
	} else {
		path = "";
	}
	download_lock (NULL);
	cp = wt_fileselect (shell, " WetScape - Save to disk ", ".", "*", path);
	download_unlock (NULL);

	if (cp) {
		FILE *fp = fopen (cp, "wb");
		if (!fp) {
			char buf[256];
			sprintf (buf, "Cannot open file `%s'.", cp);
			download_lock (NULL);
			wt_dialog (shell, buf, WT_DIAL_ERROR,
				" WetScape - Save to disk ",
				"Ok", NULL);
			download_unlock (NULL);
			free (cp);
			return -1;
		}
		free (cp);
		iop->ihook = fp;
		return 0;
	}
	return -1;
}

static void
save_decoder_free (io_t *iop)
{
	FILE *fp = iop->ihook;

	if (fp) {
		fclose (fp);
		iop->ihook = NULL;
	}
}

static int
save_decoder_decode (io_t *iop)
{
	FILE *fp = iop->ihook;

	if (fp && iop->ibufused) {
		if (fwrite (iop->ibuf, iop->ibufused, 1, fp) != 1) {
			download_lock (NULL);
			wt_dialog (shell, "Error while writing to disk",
				WT_DIAL_ERROR,
				" WetScape - Save to disk ",
				"Ok", NULL);
			download_unlock (NULL);
			return -1;
		}
		io_eat_input (iop, iop->ibufused);
	}
	return 0;
}

decoder_t save_decoder = {
	save_decoder_create,  save_decoder_decode, save_decoder_free
};

/**************** Execute-External-Viewer Decoder ******************/

typedef struct _exec_info_t {
	struct _exec_info_t *next;
	char *fname;
	FILE *fp;
	pid_t pid;
} exec_info_t;

static exec_info_t *glob_exec_list;

static void
exec_info_free (exec_info_t *ep)
{
	exec_info_t *ep2, **prev;

	if (ep) {
		prev = &glob_exec_list;
		for (ep2 = *prev; ep2; prev = &ep2->next, ep2 = *prev) {
			if (ep2 == ep) {
				*prev = ep2->next;
				break;
			}
		}
		if (ep->fp) {
			fclose (ep->fp);
		}
		if (ep->fname) {
			unlink (ep->fname);
			free (ep->fname);
		}
		free (ep);
	}
}

static void
exec_sigchld (int sig)
{
	exec_info_t *ep;
	pid_t pid;
	int status;

	pid = waitpid (-1, &status, WNOHANG);
	if (pid > 0) {
		for (ep = glob_exec_list; ep; ep = ep->next) {
			if (pid == ep->pid) {
				exec_info_free (ep);
				break;
			}
		}
	}
	signal (SIGCHLD, exec_sigchld);
}

static int
exec_decoder_create (io_t *iop)
{
	exec_info_t *ep = malloc (sizeof (exec_info_t));
	if (!ep)
		return -1;
	memset (ep, 0, sizeof (exec_info_t));

	ep->fname = strdup (tmpnam (NULL));
	if (!ep->fname) {
		exec_info_free (ep);
		return -1;
	}
#ifdef __MINT__
	{
		char *cp;
		for (cp = ep->fname; *cp; ++cp) {
			if (*cp == '\\')
				*cp = '/';
		}
	}
#endif
	ep->fp = fopen (ep->fname, "wb");
	if (!ep->fp) {
		exec_info_free (ep);
		return -1;
	}
	ep->next = glob_exec_list;
	glob_exec_list = ep;

	iop->ihook = ep;
	signal (SIGCHLD, exec_sigchld);
	return 0;
}

static void
exec_decoder_free (io_t *iop)
{
	exec_info_t *ep = iop->ihook;

	if (ep && ep->pid <= 0) {
		exec_info_free (ep);
		iop->ihook = NULL;
	}
}

static int
exec_decoder_decode (io_t *iop)
{
	exec_info_t *ep = iop->ihook;
	FILE *fp;
	pid_t pid;

	if (!ep || !(fp = ep->fp))
		return -1;

	if (iop->ibufused) {
		if (fwrite (iop->ibuf, iop->ibufused, 1, fp) != 1) {
			download_lock (NULL);
			wt_dialog (shell, "Error while writing to disk",
				WT_DIAL_ERROR,
				" WetScape - Exec ",
				"Ok", NULL);
			download_unlock (NULL);
			return -1;
		}
		io_eat_input (iop, iop->ibufused);
	}
	if (iop->eof && ep->pid <= 0) {
		fclose (fp);
		ep->fp = NULL;

		pid = fork ();
		if (pid < 0) {
			download_lock (NULL);
			wt_dialog (shell, "Cannot execute child",
				WT_DIAL_ERROR,
				" WetScape - Exec ",
				"Ok", NULL);
			download_unlock (NULL);
			return -1;
		}
		if (pid > 0) {
			ep->pid = pid;
		} else {
			char buf[512];
			const char *command = mime_get_command (iop->mimetype);

			sprintf (buf, command, ep->fname);
			execl ("/bin/sh", "/bin/sh", "-c", buf, NULL);
			exit (1);
		}
	}
	return 0;
}

decoder_t exec_decoder = {
	exec_decoder_create,  exec_decoder_decode, exec_decoder_free
};
