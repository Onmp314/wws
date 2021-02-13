/*
 * socket.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- all low-level library functions and global variables
 *
 * CHANGES:
 * ++eero, 10/96
 * - WEVENT buffer will now discard oldest events (instead of newest ones)
 *   in case of an overflow.
 * - _remove_events() will now move events in buffer only when needed.
 * - Moved _spaces() and _check_window().
 * - BUFSIZE: 1024 -> 4096
 *
 * FIXME:
 * - needs a more dynamic event buffering scheme.
 * - send_paket and query functions might also check whether server is
 *   really connected.
 * - use sigaction() instead of signal() in event_push() when buffer
 *   overflows.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include "Wlib.h"
#include "proto.h"

#define	BUFSIZE		4096
#define	MAXWEVENTS	32


/*
 * some public global variables
 */

WWIN *WROOT = NULL;
WSERVER _wserver;


/*
 * some private variables...
 */

static int Sock = -1;
static long Count = 0;
static char Buffer[BUFSIZE];
static short Events, Ev_write, Ev_read;
static WEVENT Event[MAXWEVENTS];

/*
 *	some private functions...
 */

const char *_check_window (WWIN *ptr)
{
	if (!ptr) {
		return "no window";
	}
	if (ptr->magic != MAGIC_W) {
		return "not W window (anymore?)";
	}
	return NULL;
}


void _wexit (void)
{
	if (Sock >= 0) {
		close(Sock);
		Sock = -1;
	}
}

long _winitialize (void)
{
	const char *wdisplay;

	if (Sock >= 0) {
		fprintf(stderr, "Wlib: server is already connected.\r\n");
		return -1;
	}

	if ((wdisplay = getenv("WDISPLAY"))) {

		struct hostent *he;
		struct sockaddr_in raddr;

		if (!(he = gethostbyname(wdisplay))) {
			perror(wdisplay);
			fprintf(stderr,	"Wlib: can't resolve address for server `%s'.\r\n", wdisplay);
			return -1;
		}

		if ((Sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			fprintf(stderr, "Wlib: can't create AF_INET socket.\r\n");
			return -1;
		}

		raddr.sin_family = AF_INET;
		raddr.sin_addr = *(struct in_addr *)he->h_addr_list[0];
		raddr.sin_port = htons(SERVERPORT);
		if (connect(Sock, (struct sockaddr *)&raddr, sizeof(struct sockaddr))) {
			fprintf(stderr, "Wlib: can't connect to server `%s' (%s).\r\n",
			wdisplay, inet_ntoa(raddr.sin_addr));
			close(Sock);
			return -1;
		}

	} else {

		struct sockaddr raddr;

		if ((Sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
			fprintf(stderr, "Wlib: can't create AF_UNIX socket.\r\n");
			return -1;
		}

		raddr.sa_family = AF_UNIX;
		strcpy(raddr.sa_data, "/tmp/wserver");
		if (connect(Sock, &raddr, sizeof(struct sockaddr))) {
			fprintf(stderr, "Wlib: can't connect to local server.\r\n");
			close(Sock);
			return -1;
		}
	}

	Events = Ev_write = Ev_read = 0;

	return 0;
}


void w_flush (void)
{
	long	ret;
	char	*ptr = Buffer;

	/*
	 * shit, on the one hand side this is TCP and therefore
	 * guaranteed to only return if either all data is written
	 * or the connection is broken, but on the other hand side
	 * this involves an OS call which may be interrupted by a
	 * signal, so what can we do here?
	 */

	while (Count > 0) {

		if ((ret = write(Sock, ptr, Count)) < 0) {
#ifdef __NetBSD__
			Count = 0;
			break;
#else
			perror("Wlib: w_flush() socket write failed");
			exit(-99);
#endif
		}
		Count -= ret;
		ptr += ret;
	}

	/* Count should now be zero */
}

/* reserve space in socket buffer for a packet of given lenght, flushing the
 * buffer if necessary.  returns pointer to beginning of the reserved area.
 *
 * if Wlib would thread, this would set a lock on socket buffer and another
 * functions would free it when buffer has been written to.
 */

void *_wreservep (int size)
{
	PAKET *packet;

	if (size <= 0 || size & 3) {
		fprintf(stderr, "Wlib: bogus packet size (%d)!\r\n", size);
		exit(-1);
	}

	if (Count + size > BUFSIZE) {
		if (size > BUFSIZE) {
			fprintf(stderr, "Wlib: packet size larger than buffer!\r\n");
			exit(-1);
		}
		w_flush();
	}

	packet = (PAKET *)(Buffer + Count);
	packet->len = htons(size);
	Count += size;
	return packet;
}


/*
 * some stuff dealing with buffered WEVENTs, the push routine
 * also deals with net2host byte order problems and converting
 * between the internal and external event types
 *
 * Buffer is circular and will overwrite older events in case of
 * an overflow.
 */

static WEVENT *event_pop (void)
{
	WEVENT *ptr = NULL;

	if (Events) {
		ptr = &Event[Ev_read];
		if (++Ev_read == MAXWEVENTS) {
			Ev_read = 0;
		}
		Events--;
	}

	return ptr;
}


static void event_push (WEVENT *ptr)
{
	Event[Ev_write].win  = ptr->win;
	Event[Ev_write].type = ntohs(ptr->type);
	Event[Ev_write].time = ntohl(ptr->time);
	Event[Ev_write].x    = ntohs(ptr->x);
	Event[Ev_write].y    = ntohs(ptr->y);
	Event[Ev_write].w    = ntohs(ptr->w);
	Event[Ev_write].h    = ntohs(ptr->h);
	Event[Ev_write].key  = ntohl(ptr->key);

	if (++Ev_write == MAXWEVENTS) {
		Ev_write = 0;
	}

	if (Events >= MAXWEVENTS) {
#ifdef SIGTTOU
		void (*old)();
		/* Better to allow background write as overflow isn't that
		 * uncommon (eg.  if program has been suspended while user
		 * gave input to it's window), and stopping the program
		 * might confuse the user...
		 */
		old = signal(SIGTTOU, SIG_IGN);
		fprintf(stderr, "Wlib: event buffer overflow.\r\n");
		signal(SIGTTOU, old);
#else
		fprintf(stderr, "Wlib: event buffer overflow.\r\n");
#endif
		event_pop();
	}
	Events++;
}


/*
 * routine for reading the socket and buffering event pakets. if called with
 * wantEvent==0 it means caller is looking for a real paket. in this case it
 * returns when the first complete paket is received. the fd_sets should be
 * NULL to not disturb selecting the socket. if called with wantEvent==1 it
 * means caller wants events. in this case it returns when either an event
 * is/becomes available or the timeout or the supplied fd_sets suggest to
 * return.
 */

static PAKET *scanSocket (fd_set *rfd, fd_set *wfd, fd_set *xfd, long timeout,
			  int *numReady, int wantEvent)
{
	static long inbuf = 0;
	static char ibuf[BUFSIZE];
	static PAKET paket;
	long this;
	PAKET *pptr, *ret = NULL;
	short len, type, idx;
	fd_set myRfd;
	struct timeval tv, *tvp;
	int fds = 0;

	while (42) {
		/*
		 * prepare for select()
		 */

		if (wantEvent && Events) {
			/* can return immediately */   	
			timeout = 0;
		}
		/* read from W socket */
		if (!wantEvent || !Events) {
			if (!rfd) {
				FD_ZERO (&myRfd);
				rfd = &myRfd;
			}
			FD_SET (Sock, rfd);
		}

		tvp = &tv;
		if (timeout < 0) {
			tvp = NULL;
		} else {
			tv.tv_sec = timeout / 1000;
			tv.tv_usec = 1000 * (timeout - 1000 * tv.tv_sec);
		}

		if ((fds = select (FD_SETSIZE, rfd, wfd, xfd, tvp)) < 0) {
			fprintf (stderr, "Wlib: select() failed in w_queryevent().\r\n");
			fds = 0;
		}

		/*
		 * refill input buffer from socket, if possible
		 */

		if (rfd && FD_ISSET (Sock, rfd)) {
			fds--;
			this = read (Sock, ibuf + inbuf, BUFSIZE - inbuf);
			if (this < 1) {
				fprintf (stderr, "Wlib: connection broken, sending SIGPIPE to my own.\r\n");
				kill (getpid(), SIGPIPE);
				/* not reached?
				 */
				fprintf (stderr, "Wlib: connection broken, how did we get here? DIE!!\r\n");
				_exit (-999);
			}
			FD_CLR(Sock, rfd);
			inbuf += this;
		}

		/*
		 * analyze input buffer
		 */

		idx = 0;
		while (inbuf >= 4) {

			/* check if there's a complete paket
			 */
			pptr = (PAKET *)&ibuf[idx];
			len = ntohs(pptr->len);
			type = ntohs(pptr->type);
			if (inbuf < len) {
				break;
			}
			inbuf -= len;
			idx += len;

			if (type == PAK_EVENT) {
				event_push ((WEVENT *)&((EVENTP *)pptr)->event);
				continue;
			}

			/* we've got a non-event paket
			 */
			memcpy (&paket, pptr, len);
			paket.len = len;
			paket.type = type;
			ret = &paket;
			break;
		}

		if (idx) {
			/* move unprocessed data to buffer start
			 */
#ifdef sun
			bcopy (&ibuf[idx], ibuf, inbuf);
#else
			memmove (ibuf, &ibuf[idx], inbuf);
#endif
		}

		if (ret || (wantEvent && Events > 0) || fds > 0 || timeout >= 0) {
			/* until we get either:
			 * - return paket
			 * - event(s)
			 * - FD(s) set
			 * - timeout
			 */
			break;
		}
	}

	if (numReady) {
		*numReady = fds;
	}
	if (!fds) {
		/* event queued or timeout */
		if(rfd) {
			FD_ZERO(rfd);
		}
		if(wfd) {
			FD_ZERO(wfd);
		}
		if(xfd) {
			FD_ZERO(xfd);
		}
	}
	return ret;
}


/* Called by w_delete() to remove buffered events for window in question
 */
void _wremove_events (WWIN *win)
{
	short nr, nw;

	if(!Events) {
		return;
	}
	while (Event[Ev_read].win == win) {
		if (++Ev_read == MAXWEVENTS) {	/* skip events */
			Ev_read = 0;
		}
		Events--;
		if (!Events) {			/* no events? */
			return;
		}
	}

	/* first non-removable event */
	nr = Ev_read;			/* at least one event to preserve */
	do {
		if (++nr == MAXWEVENTS) {
			nr = 0;
		}
		if (nr == Ev_write) {		/* all preserved? */
			return;
		}
	} while (Event[nr].win != win);

	/* first removable event after it */
	nw = nr;				/* first empty place holder */
	for(;;) {
		if (++nr == MAXWEVENTS) {
			nr = 0;
		}
		if (nr == Ev_write) {
			break;
		}
		if (Event[nr].win != win) {	/* move valid event? */
			Event[nw] = Event[nr];
			if (++nw == MAXWEVENTS) {
				nw = 0;
			}
			Events--;
		}
	}
	/* first empty event place */
	Ev_write = nw;
}


/*
 * next are two routines for listening on the socket: one for waiting for
 * pakets (return codes) and one for waiting for events
 */

PAKET *_wait4paket (short type)
{
	PAKET *pptr;

	w_flush ();

	if (!(pptr = scanSocket (NULL, NULL, NULL, -1, NULL, 0))) {
		fprintf (stderr, "Wlib: socket communication garbled, aborting\r\n");
		fprintf (stderr, "      expected 0x%04x paket, received NULL\r\n", type);
		w_exit ();
		_exit (-1);
	}

	if (type != pptr->type) {
		fprintf (stderr, "Wlib: socket communication garbled, aborting\r\n");
		fprintf (stderr, "      expected 0x%04x paket, received 0x%04x\r\n", type, pptr->type);
		w_exit ();
		_exit (-1);
	}

	return pptr;
}

WEVENT *w_queryevent (fd_set *rfd,
		      fd_set *wfd,
		      fd_set *xfd,
		      long timeout)
{
	int *numReady = NULL;		/* ATM not used */
	PAKET *pptr;

	w_flush ();

	if ((pptr = scanSocket (rfd, wfd, xfd, timeout, numReady, 1))) {
		fprintf (stderr, "Wlib: socket communication garbled, aborting\r\n");
		fprintf (stderr, "      expected event paket, received 0x%04x\r\n", pptr->type);
		w_exit ();
		_exit (-1);
	}

	return event_pop ();
}
