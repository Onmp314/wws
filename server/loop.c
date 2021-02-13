/*
 * loop.c, a part of the W Window System
 *
 * Copyright (C) 1994-1999 by Torsten Scherer, Eero Tamminen
 * and Jan Paul Schmidt
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- server main loop and stuff belonging to it
 *
 * CHANGES
 * ++jps, 2/98:
 * - support for opaque window moving (REALTIME_MOVING)
 * ++eero, 6/98:
 * - timestamps to server events
 * ++oddie, 4/99:
 * - timestamps work on MacMiNT (1.12) using the sysvar struct
 * ++eero, 5/99:
 * - send MMOVE events also to clients
 * ++eero, 3/03:
 * - SDL/GGI redraw hacks
 */

#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"
#include "rect.h"

#if defined(__MINT__) && defined(MAC)
#include "xconout2/mint/locore.h"
#endif

/*
 *	some constants
 */

#define	EFILNF		-33
#define	FLAG_NONE	0x0
#define	FLAG_EXITED	0x1


/*
 * global variables
 */

WINDOW *glob_leftmousepressed = NULL;
WINDOW *glob_rightmousepressed = NULL;

CLIENT *glob_clients = NULL;
long glob_bytes = 0, glob_pakets = 0;
short glob_pakettype;
fd_set glob_crfd;
long glob_evtime;

CLIENT *saveclient = NULL;
static long savetime;

#ifdef REALTIME_MOVING
/* Used in mouse.c to set the mouse pointer automatically. */
short glob_loopmove = 0;

/* Window, which is moved, if loop_move */
static WINDOW *loop_move_window;
#endif


/*
 * place a rectangle somewhere on the screen
 *
 * x0 and y0 should hold the previous position, if there was any, and 'UNDEF'
 * (-32768) if there wasn't any. the mouse will keep its relative position to
 * the rectangle so that if you click on an already existing window and release
 * the button without moving the mouse, the mousepointer won't move to the
 * upper left edge of the window. if you discard the move or the rectangle is
 * bigger than the screen both x0 and y0 will contain 'UNDEF' and something
 * != 0 is returned.
 */

int get_rectangle(short width, short height,
		  short *x0, short *y0,
		  char pressed, char released)
{
  const WEVENT *ev = NULL;
  short mdx, mdy;
  short wx, wy, oldwx, oldwy;
  short end;

#ifdef CHILDS_INSIDE_PARENT
  if ((width > glob_screen->bm.width) || (height > glob_screen->bm.height)) {
    *x0 = UNDEF;
    *y0 = UNDEF;
    return -1;
  }
#endif

  if ((*x0 == UNDEF) || (*y0 == UNDEF)) {
    wx = glob_mouse.real.x0;
    wy = glob_mouse.real.y0;
  } else {
    wx = *x0;
    wy = *y0;
  }

#ifdef CHILDS_INSIDE_PARENTS
  if (wx + width > glob_screen->bm.width)
    wx = glob_screen->bm.width - width;
  if (wy + height > glob_screen->bm.height)
    wy = glob_screen->bm.height - height;
#endif

  mdx = glob_mouse.real.x0 - wx;
  mdy = glob_mouse.real.y0 - wy;

  mouse_hide();


  gc0 = glob_inversgc;
  glob_clip0 = &glob_rootwindow->pos;

  (*glob_screen->box)(&glob_screen->bm, wx, wy, width, height);
  (*glob_screen->box)(&glob_screen->bm, wx+1, wy+1, width-2, height-2);
#ifdef SCREEN_REFRESH
  {
    REC *r = rect_create(wx, wy, width, height);
    if (r) {
      screen_update(r);
      free(r);
    }
  }
#endif

  end = 0;
  while (!end) {
    if ((ev = event_mouse())) {

      oldwx = wx;
      oldwy = wy;

      if (ev->type == EVENT_MMOVE) {
#ifdef CHILDS_INSIDE_PARENTS
	wx += ev->x;
	if (wx < 0) {
	  wx = 0;
	} else if (wx + width > glob_screen->bm.width) {
	  wx = glob_screen->bm.width - width;
	}
	wy += ev->y;
	if (wy < 0) {
	  wy = 0;
	} else if (wy + height > glob_screen->bm.height) {
	  wy = glob_screen->bm.height - height;
	}
#else
	/*
	 * only make sure the mouse stays inside the screen
	 * NOTE: we must ensure (otherwise SIGSEGV):
	 *  0 <= mouse.x < glob_screen->bm.width
	 *  0 <= mouse.y < glob_screen->bm.height
	 */
	wx += ev->x;
	if (wx + mdx < 0) {
	  wx = -mdx;
	} else if (wx + mdx >= glob_screen->bm.width) {
	  wx = glob_screen->bm.width - mdx - 1;
	}
	wy += ev->y;
	if (wy + mdy < 0) {
	  wy = -mdy;
	} else if (wy + mdy >= glob_screen->bm.height) {
	  wy = glob_screen->bm.height - mdy - 1;
	}
        /* Absolute pointing devices (like touch panels) need this
           updated in order to properly generate relative events */
        glob_mouse.real.x0 = wx;
        glob_mouse.real.y0 = wy;

#endif
      } else {
	if ((ev->reserved[0] & pressed) || (ev->reserved[1] & released)) {
	  end = 1;
	}
      }

      if ((wx != oldwx) || (wy != oldwy)) {
	(*glob_screen->box)(&glob_screen->bm,
			    oldwx, oldwy, width, height);
	(*glob_screen->box)(&glob_screen->bm,
			    oldwx+1, oldwy+1, width-2, height-2);
	(*glob_screen->box)(&glob_screen->bm, wx, wy, width, height);
	(*glob_screen->box)(&glob_screen->bm,
			    wx+1, wy+1, width-2, height-2);
#ifdef SCREEN_REFRESH
	{
	  int x = MIN(wx, oldwx);
	  int y = MIN(wy, oldwy);
	  REC *r;
	  r = rect_create(x, y,
			  width  + MAX(wx, oldwx) - x,
			  height + MAX(wy, oldwy) - y);
	  if (r) {
	    screen_update(r);
	    free(r);
	  }
	}
#endif
      }
    }
  }

  (*glob_screen->box)(&glob_screen->bm, wx, wy, width, height);
  (*glob_screen->box)(&glob_screen->bm, wx+1, wy+1, width-2, height-2);
#ifdef SCREEN_REFRESH
  {
    REC *r = rect_create(wx, wy, width, height);
    if (r) {
      screen_update(r);
      free(r);
    }
  }
#endif

  /* right button = discard window? */
  if (ev->reserved[0] & BUTTON_RIGHT) {
    *x0 = UNDEF;
    *y0 = UNDEF;
#if defined(MAC) &&  defined(__MINT__)
    /* this is necessary so that W and MacOS have same idea of
       where the mouse is */
    glob_mouse.real.x1 = (glob_mouse.real.x0 = wx + mdx) + 16;
    glob_mouse.real.y1 = (glob_mouse.real.y0 = wy + mdy) + 16;
#endif
    return -1;
  }

  *x0 = wx;
  *y0 = wy;

  glob_mouse.real.x1 = (glob_mouse.real.x0 = wx + mdx) + 16;
  glob_mouse.real.y1 = (glob_mouse.real.y0 = wy + mdy) + 16;
  mouse_show();

  glob_leftmousepressed = NULL;
  glob_rightmousepressed = NULL;

  return 0;
}


/*
 * resize_rectangle(): based upon an existing rectangle, allow some of the
 * coordinates to follow the mouse pointer until the(?) mouse button is
 * released. which coordinates can be changed depends on the given mask. of
 * course some simple rules apply in any case.
 */

static void resize_rectangle (short *x0Ptr, short *y0Ptr, short *x1Ptr, short *y1Ptr, int mask)
{
  short mx, my, x0 = *x0Ptr, y0 = *y0Ptr, x1 = *x1Ptr, y1 = *y1Ptr, width, height, end;
  const WEVENT *ev = NULL;

  mouse_hide ();
  mx = glob_mouse.real.x0;
  my = glob_mouse.real.y0;

  gc0 = glob_inversgc;
  glob_clip0 = &glob_rootwindow->pos;

  width = x1 - x0 + 1;
  height= y1 - y0 + 1;
  (*glob_screen->box)(&glob_screen->bm, x0, y0, width, height);
  (*glob_screen->box)(&glob_screen->bm, x0+1, y0+1, width-2, height-2);
#ifdef SCREEN_REFRESH
  {
    REC *r = rect_create(x0, y0, width, height);
    if (r) {
      screen_update(r);
      free(r);
    }
  }
#endif

  end = 0;
  while (!end) {

    if ((ev = event_mouse())) {

      short oldX0 = x0, oldX1 = x1, oldY0 = y0, oldY1 = y1;

      if (ev->type == EVENT_MMOVE) {

	if ((mx += ev->x) < 0)
	  mx = 0;
	if (mx > glob_screen->bm.width - 1)
	  mx = glob_screen->bm.width - 1;

	if ((my += ev->y) < 0)
	  my = 0;
	if (my > glob_screen->bm.height - 1)
	  my = glob_screen->bm.height - 1;

        /* Absolute pointing devices (like touch panels) need this
           updated in order to properly generate relative events */
        glob_mouse.real.x0 = mx;
        glob_mouse.real.y0 = my;

	/* one more place where the window frame size is hardcoded...
	 */

	if (mask & 1)
	  if ((y1 - (y0 = my)) < 7)
	    y0 = y1 - 7;
	if (mask & 2)
	  if (((y1 = my) - y0) < 7)
	    y1 = y0 + 7;
	if (mask & 4)
	  if ((x1 - (x0 = mx)) < 7)
	    x0 = x1 - 7;
	if (mask & 8)
	  if (((x1 = mx) - x0) < 7)
	    x1 = x0 + 7;

      } else if (ev->reserved[1]) {

	/* terminate this if *any* button is released
	 */

	end = 1;
      }

      if ((x0 != oldX0) || (y0 != oldY0) || (x1 != oldX1) || (y1 != oldY1)) {
	width = oldX1 - oldX0 + 1;
	height= oldY1 - oldY0 + 1;
	(*glob_screen->box)(&glob_screen->bm, oldX0, oldY0, width, height);
	(*glob_screen->box)(&glob_screen->bm, oldX0+1, oldY0+1, width-2, height-2);

	width = x1 - x0 + 1;
	height= y1 - y0 + 1;
	(*glob_screen->box)(&glob_screen->bm, x0, y0, width, height);
	(*glob_screen->box)(&glob_screen->bm, x0+1, y0+1, width-2, height-2);
#ifdef SCREEN_REFRESH
	{
	  int x = MIN(x0, oldX0);
	  int y = MIN(y0, oldY0);
	  REC *r;
	  r = rect_create(x, y,
			  MAX(x1, oldX1) - x + 1,
			  MAX(y1, oldY1) - y + 1);
	  if (r) {
	    screen_update(r);
	    free(r);
	  }
	}
#endif
      }
    }
  }

  width = x1 - x0 + 1;
  height= y1 - y0 + 1;
  (*glob_screen->box)(&glob_screen->bm, x0, y0, width, height);
  (*glob_screen->box)(&glob_screen->bm, x0+1, y0+1, width-2, height-2);
#ifdef SCREEN_REFRESH
  {
    REC *r = rect_create(x0, y0, width, height);
    if (r) {
      screen_update(r);
      free(r);
    }
  }
#endif

  *x0Ptr = x0;
  *y0Ptr = y0;
  *x1Ptr = x1;
  *y1Ptr = y1;

  glob_mouse.real.x1 = (glob_mouse.real.x0 = mx) + 16;
  glob_mouse.real.y1 = (glob_mouse.real.y0 = my) + 16;
  mouse_show();

  glob_leftmousepressed = NULL;
  glob_rightmousepressed = NULL;
}


/*****************************************************************************/

/*
 * some routines dealing with sockets
 */

static CLIENT *createClient(void)
{
  CLIENT *ptr;

  if (!(ptr = (CLIENT *)malloc(sizeof(CLIENT)))) {
    return NULL;
  }

  memset(ptr, 0, sizeof(CLIENT));

  if (!(ptr->buf = malloc(LARGEBUF))) {
    free(ptr);
    return NULL;
  }

  return ptr;
}


static void killClient(CLIENT *cptr)
{
  CLIENT *tmp = cptr->next;
  LIST *font;

  /* shut down socket, close windows and remove client from queue */

  shutdown(cptr->sh, 2);
  close(cptr->sh);

  /* kill all windows for a particular client */
  windowKillClient(cptr);

  /* much faster than doing client_unloadfont() which traverses all windows
   * looking for GCs using the given font ++eero 2/98
   */
  while((font = cptr->font)) {
    cptr->font = font->next;
    if (--(glob_font[font->handle].numused) < 1)
      font_unloadfont (&glob_font[font->handle]);
    free(font);
  }

  /* also clear the fd_set bit */
  FD_CLR(cptr->sh, &glob_crfd);

  if (cptr->prev) {
    cptr->prev->next = tmp;
  } else {
    glob_clients = tmp;
  }

  if (tmp) {
    tmp->prev = cptr->prev;
  }

  if (cptr->bbm.data) {
    free (cptr->bbm.data);
    cptr->bbm.data = NULL;
  }

  if (saveclient == cptr) {
    saveclient = 0;
  }

  free(cptr->buf);
  free(cptr);
}


static void process_paket(CLIENT *cptr, PAKET *pptr)
{
  /* we'll need only one of these */
  static union {
    PAKET raw;
    INITRETP ir;
    SRETP sr;
    S3RETP s3r;
    RSTATUSP rs;
    LRETP lr;
    COLRETP colr;
    LFONTRETP lfontr;
  } paket;
  PAKET *rpptr = NULL;
  ushort col1, col2, col3;
  short arg1, arg2, flags = 0;
  uchar string[2];

  switch (glob_pakettype = pptr->type) {

    case PAK_NULLR:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret = htons(-1);
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_INIT:
      cptr->uid = ntohl(((INITP *)pptr)->uid);
      paket.ir.len = sizeof(INITRETP);
      paket.ir.type = htons(PAK_INITRET);
      paket.ir.vmaj = htons(_WMAJ);
      paket.ir.vmin = htons(_WMIN);
      paket.ir.pl = htons(_WPL);
      paket.ir.width = htons(glob_backgroundwin->bitmap.width);
      paket.ir.height = htons(glob_backgroundwin->bitmap.height);
      paket.ir.screenType = htons(glob_backgroundwin->bitmap.type);
      paket.ir.planes = htons(glob_backgroundwin->bitmap.planes);
      paket.ir.sharedcolors = htons(glob_sharedcolors);
#if defined(GGI) || defined(SDL) || (defined(__MINT__) && defined(MAC))
      flags |= WSERVER_KEY_MAPPING;
#endif
#if 0
      flags |= WSERVER_SHM;
#endif
      paket.ir.flags = htons(flags);
      if (glob_fontsize) {
        paket.ir.fsize = htons(glob_fontsize);
      } else {
        paket.ir.fsize = htons(DEF_WFONTSIZE);
      }
      if (glob_fontfamily) {
        strncpy(paket.ir.fname, glob_fontfamily, MAXFAMILYNAME);
      } else {
	strncpy(paket.ir.fname, DEF_WFONTFAMILY, MAXFAMILYNAME);
      }
      rpptr = (PAKET *)&paket.ir;
      break;

    case PAK_CREATE:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(client_create(cptr,
			    ntohs(((CREATEP *)pptr)->width),
			    ntohs(((CREATEP *)pptr)->height),
			    ntohs(((CREATEP *)pptr)->flags),
			    ((CREATEP *)pptr)->libPtr));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_CREATE2:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(client_create2(cptr, ntohs(((CREATEP *)pptr)->width),
			     ntohs(((CREATEP *)pptr)->height),
			     ntohs(((CREATEP *)pptr)->flags),
			     ntohs(((CREATEP *)pptr)->handle),
			     ((CREATEP *)pptr)->libPtr));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_OPEN:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(client_open(cptr, ntohs(((OPENP *)pptr)->handle),
			  ntohs(((OPENP *)pptr)->x0),
			  ntohs(((OPENP *)pptr)->y0)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_MOVE:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(client_move(cptr, ntohs(((MOVEP *)pptr)->handle),
			  ntohs(((MOVEP *)pptr)->x0),
			  ntohs(((MOVEP *)pptr)->y0)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_RESIZE:
      paket.sr.len = sizeof (SRETP);
      paket.sr.type = htons (PAK_SRET);
      paket.sr.ret
      = htons (client_resize (cptr, ntohs(((RESIZEP *)pptr)->handle),
			      ntohs(((RESIZEP *)pptr)->width),
			      ntohs(((RESIZEP *)pptr)->height)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_CLOSE:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret = htons(client_close(cptr, ntohs(((CLOSEP *)pptr)->handle)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_DELETE:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret = htons(client_delete(cptr, ntohs(((DELETEP *)pptr)->handle)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_LOADFONT:
      paket.lfontr.len = sizeof(LFONTRETP);
      paket.lfontr.type = htons(PAK_LFONTRET);
      client_loadfont(cptr, ((LOADFONTP *)pptr)->family,
			     ntohs(((LOADFONTP *)pptr)->size),
			     ntohs(((LOADFONTP *)pptr)->styles),
			     &paket.lfontr);
      rpptr = (PAKET *)&paket.lfontr;
      break;

    case PAK_UNLOADFONT:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(client_unloadfont(cptr, ntohs(((UNLOADFONTP *)pptr)->fonthandle)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_QWINSZ:
      paket.s3r.len = sizeof(S3RETP);
      paket.s3r.type = htons(PAK_S3RET);
      paket.s3r.ret[2]
      = htons(client_querywinsize(cptr, ntohs(((QWINSZP *)pptr)->handle),
				  ntohs(((QWINSZP *)pptr)->effective),
				  &arg1, &arg2));
      paket.s3r.ret[1] = htons(arg2);
      paket.s3r.ret[0] = htons(arg1);
      rpptr = (PAKET *)&paket.s3r;
      break;

    case PAK_TEST:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(client_test(cptr, htons(((TESTP *)pptr)->handle),
			  ntohs(((TESTP *)pptr)->x0),
			  ntohs(((TESTP *)pptr)->y0)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_QMPOS:
      paket.s3r.len = sizeof(S3RETP);
      paket.s3r.type = htons(PAK_S3RET);
      paket.s3r.ret[2]
      = htons(client_querymousepos(cptr, ntohs(((QMPOSP *)pptr)->handle),
				   &arg1, &arg2));
      paket.s3r.ret[1] = htons(arg2);
      paket.s3r.ret[0] = htons(arg1);
      rpptr = (PAKET *)&paket.s3r;
      break;

    case PAK_QWPOS:
      paket.s3r.len = sizeof(S3RETP);
      paket.s3r.type = htons(PAK_S3RET);
      paket.s3r.ret[2]
      = htons(client_querywindowpos(cptr, ntohs(((QWPOSP *)pptr)->handle),
				    ntohs(((QWPOSP *)pptr)->effective),
				    &arg1, &arg2));
      paket.s3r.ret[1] = htons(arg2);
      paket.s3r.ret[0] = htons(arg1);
      rpptr = (PAKET *)&paket.s3r;
      break;

    case PAK_QSTATUS:
      paket.rs.len = sizeof(RSTATUSP);
      paket.rs.type = htons(PAK_RSTATUS);
      paket.rs.ret
      = htons(client_status(cptr, glob_clients,
			    ntohs(((QSTATUSP *)pptr)->index),
			    &paket.rs.status));
      rpptr = (PAKET *)&paket.rs;
      break;

    case PAK_PUTBLKREQ:
      paket.lr.len = sizeof(LRETP);
      paket.lr.type = htons(PAK_LRET);
      paket.lr.ret
      = htonl(client_putblockreq (cptr, ntohs(((PUTBLKREQP *)pptr)->width),
				  ntohs(((PUTBLKREQP *)pptr)->height),
				  ntohs(((PUTBLKREQP *)pptr)->handle),
				  ntohs(((PUTBLKREQP *)pptr)->x1),
				  ntohs(((PUTBLKREQP *)pptr)->y1),
				  ((PUTBLKREQP *)pptr)->shmKey));
      rpptr = (PAKET *)&paket.lr;
      break;

    case PAK_SSAVER:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      if ((saveclient && (saveclient != cptr)) ||
	  (ntohs(((SSAVERP *)pptr)->seconds) < 10)) {
	paket.sr.ret = htons(-1);
      } else {
	saveclient = cptr;
	savetime = ntohs(((SSAVERP *)pptr)->seconds) * 1000L;
	paket.sr.ret = htons(0);   /* boah... */
      }
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_GETBLKREQ:
      paket.lr.len = sizeof(LRETP);
      paket.lr.type = htons(PAK_LRET);
      paket.lr.ret
      = htonl(client_getblockreq(cptr, ntohs(((GETBLKREQP *)pptr)->handle),
				  ntohs(((GETBLKREQP *)pptr)->x0),
				  ntohs(((GETBLKREQP *)pptr)->y0),
				  ntohs(((GETBLKREQP *)pptr)->width),
				  ntohs(((GETBLKREQP *)pptr)->height),
				  ((GETBLKREQP *)pptr)->shmKey));
      rpptr = (PAKET *)&paket.lr;
      break;

    case PAK_GETBLKDATA:
      paket.raw.len
      = client_getblockdata(cptr, paket.raw.data, sizeof(paket.raw.data));
      paket.raw.len += sizeof(paket.raw) - sizeof(paket.raw.data);
      paket.raw.len = htons(paket.raw.len);
      paket.raw.type = htons(PAK_RAWDATA);
      rpptr = (PAKET *)&paket.raw;
      break;

    case PAK_ALLOCCOL:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(clientAllocColor(cptr, ntohs(((ALLOCCOLP *)pptr)->handle),
			       ntohs(((ALLOCCOLP *)pptr)->red),
			       ntohs(((ALLOCCOLP *)pptr)->green),
			       ntohs(((ALLOCCOLP *)pptr)->blue)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_FREECOL:
      paket.sr.len = sizeof(SRETP);
      paket.sr.type = htons(PAK_SRET);
      paket.sr.ret
      = htons(clientFreeColor(cptr, ntohs(((FREECOLP *)pptr)->handle),
			      ntohs(((FREECOLP *)pptr)->color)));
      rpptr = (PAKET *)&paket.sr;
      break;

    case PAK_GETCOL:
      paket.colr.len = sizeof(SRETP);
      paket.colr.type = htons(PAK_SRET);
      paket.colr.ret
      = htons(clientGetColor(cptr, ntohs(((SETFGCOLP *)pptr)->handle),
			     ntohs(((SETFGCOLP *)pptr)->color),
			     &col1, &col2, &col3));
      paket.colr.red   = htons(col1);
      paket.colr.green = htons(col2);
      paket.colr.blue  = htons(col3);
      rpptr = (PAKET *)&paket.colr;
      break;

    case PAK_GMOUSE:
      paket.sr.len = sizeof (SRETP);
      paket.sr.type = htons (PAK_SRET);
      paket.sr.ret
      = htons(client_getmousepointer (cptr, ntohs (((GMOUSEP *)pptr)->handle)));
      rpptr = (PAKET *)&paket.sr;
      break;

/* pakets that don't need a return code */

    case PAK_NULL:
      break;

    case PAK_EXIT:
      cptr->flags |= FLAG_EXITED;
      break;

    case PAK_STEXTSTYLE:
      client_settextstyle(cptr, ntohs(((STEXTSTYLEP *)pptr)->handle),
			  ntohs(((STEXTSTYLEP *)pptr)->flags));
      break;

    case PAK_STITLE:
      client_settitle(cptr, ntohs(((STITLEP *)pptr)->handle),
		      ((STITLEP *)pptr)->title);
      break;

    case PAK_PLOT:
    case PAK_DPLOT:
      client_plot(cptr, ntohs(((PLOTP *)pptr)->handle),
		  ntohs(((PLOTP *)pptr)->x0), ntohs(((PLOTP *)pptr)->y0));
      break;

    case PAK_LINE:
    case PAK_DLINE:
      client_line(cptr, ntohs(((LINEP *)pptr)->handle),
		  ntohs(((LINEP *)pptr)->x0), ntohs(((LINEP *)pptr)->y0),
		  ntohs(((LINEP *)pptr)->xe), ntohs(((LINEP *)pptr)->ye));
      break;

    case PAK_HLINE:
    case PAK_DHLINE:
      client_hline(cptr, ntohs(((HVLINEP *)pptr)->handle),
		   ntohs(((HVLINEP *)pptr)->x0), ntohs(((HVLINEP *)pptr)->y0),
		   ntohs(((HVLINEP *)pptr)->e));
      break;

    case PAK_VLINE:
    case PAK_DVLINE:
      client_vline(cptr, ntohs(((HVLINEP *)pptr)->handle),
		   ntohs(((HVLINEP *)pptr)->x0), ntohs(((HVLINEP *)pptr)->y0),
		   ntohs(((HVLINEP *)pptr)->e));
      break;

    case PAK_BOX:
    case PAK_PBOX:
    case PAK_DBOX:
    case PAK_DPBOX:
      client_box(cptr, ntohs(((BOXP *)pptr)->handle),
		 ntohs(((BOXP *)pptr)->x0), ntohs(((BOXP *)pptr)->y0),
		 ntohs(((BOXP *)pptr)->width), ntohs(((BOXP *)pptr)->height));
      break;

    case PAK_BITBLK:
      client_bitblk(cptr, ntohs(((BITBLKP *)pptr)->handle),
		    ntohs(((BITBLKP *)pptr)->x0), ntohs(((BITBLKP *)pptr)->y0),
		    ntohs(((BITBLKP *)pptr)->width),
		    ntohs(((BITBLKP *)pptr)->height),
		    ntohs(((BITBLKP *)pptr)->x1), ntohs(((BITBLKP *)pptr)->y1));
      break;

    case PAK_BITBLK2:
      client_bitblk2(cptr, ntohs(((BITBLKP *)pptr)->handle),
		     ntohs(((BITBLKP *)pptr)->x0),
		     ntohs(((BITBLKP *)pptr)->y0),
		     ntohs(((BITBLKP *)pptr)->width),
		     ntohs(((BITBLKP *)pptr)->height),
		     ntohs(((BITBLKP *)pptr)->dhandle),
		     ntohs(((BITBLKP *)pptr)->x1),
		     ntohs(((BITBLKP *)pptr)->y1));
      break;

    case PAK_VSCROLL:
      client_vscroll(cptr, ntohs(((VSCROLLP *)pptr)->handle),
		     ntohs(((VSCROLLP *)pptr)->x0),
		     ntohs(((VSCROLLP *)pptr)->y0),
		     ntohs(((VSCROLLP *)pptr)->width),
		     ntohs(((VSCROLLP *)pptr)->height),
		     ntohs(((VSCROLLP *)pptr)->y1));
      break;

    case PAK_PRINTC:
      string[0] = ntohs(((PRINTCP *)pptr)->c) & 0xff;
      string[1] = '\0';
      client_prints(cptr, ntohs(((PRINTCP *)pptr)->handle),
		    ntohs(((PRINTCP *)pptr)->x0),
		    ntohs(((PRINTCP *)pptr)->y0),
		    string);
      break;

    case PAK_PRINTS:
      client_prints(cptr, ntohs(((PRINTSP *)pptr)->handle),
		    ntohs(((PRINTSP *)pptr)->x0),
		    ntohs(((PRINTSP *)pptr)->y0),
		    ((PRINTSP *)pptr)->s);
      break;

    case PAK_CIRCLE:
    case PAK_PCIRCLE:
    case PAK_DCIRCLE:
    case PAK_DPCIRCLE:
      client_circle(cptr, ntohs(((CIRCLEP *)pptr)->handle),
		    ntohs(((CIRCLEP *)pptr)->x0),
		    ntohs(((CIRCLEP *)pptr)->y0),
		    ntohs(((CIRCLEP *)pptr)->r));
      break;

    case PAK_ELLIPSE:
    case PAK_PELLIPSE:
    case PAK_DELLIPSE:
    case PAK_DPELLIPSE:
      client_ellipse(cptr, ntohs(((ELLIPSEP *)pptr)->handle),
		     ntohs(((ELLIPSEP *)pptr)->x0),
		     ntohs(((ELLIPSEP *)pptr)->y0),
		     ntohs(((ELLIPSEP *)pptr)->rx),
		     ntohs(((ELLIPSEP *)pptr)->ry));
      break;

    case PAK_ARC:
    case PAK_DARC:
    case PAK_DPIE:
    case PAK_PIE:
      client_pie(cptr, ntohs(((PIEP *)pptr)->handle),
		 ntohs(((PIEP *)pptr)->x0), ntohs(((PIEP *)pptr)->y0),
		 ntohs(((PIEP *)pptr)->rx), ntohs(((PIEP *)pptr)->ry),
		 ntohs(((PIEP *)pptr)->ax), ntohs(((PIEP *)pptr)->ay),
		 ntohs(((PIEP *)pptr)->bx), ntohs(((PIEP *)pptr)->by),
		 ntohs(((PIEP *)pptr)->adir), ntohs(((PIEP *)pptr)->bdir));
      break;

    case PAK_POLY:
    case PAK_PPOLY:
    case PAK_DPOLY:
    case PAK_DPPOLY:
      client_poly(cptr, ntohs(((POLYP *)pptr)->handle),
		  ntohs(((POLYP *)pptr)->numpoints), ((POLYP *)pptr)->points);
      break;

    case PAK_BEZIER:
    case PAK_DBEZIER:
      client_bezier(cptr, ntohs(((BEZIERP *)pptr)->handle),
		  ((BEZIERP *)pptr)->points);
      break;

    case PAK_RAWDATA:
      arg1 = pptr->len - (sizeof(*pptr) - sizeof(pptr->data));
      client_putblockdata(cptr, pptr->data, arg1);
      break;

    case PAK_BEEP:
      client_beep(cptr);
      break;

    case PAK_SMODE:
      client_setmode(cptr, ntohs(((SMODEP *)pptr)->handle),
		     ntohs(((SMODEP *)pptr)->mode));
      break;

    case PAK_SLINEWIDTH:
      client_setlinewidth(cptr, ntohs(((SMODEP *)pptr)->handle),
			  ntohs(((SMODEP *)pptr)->mode));
      break;

    case PAK_SFONT:
      client_setfont(cptr, ntohs(((SFONTP *)pptr)->handle),
		     ntohs(((SFONTP *)pptr)->fonthandle));
      break;

    case PAK_SPATTERN:
      client_setpattern(cptr, ntohs(((SPATTERNP *)pptr)->handle),
			ntohs(((SPATTERNP *)pptr)->pattern));
      break;

    case PAK_SPATTERNDATA:
      client_setpatterndata(cptr, ntohs(((SPATTERNDATAP *)pptr)->handle),
			    ((SPATTERNDATAP *)pptr)->data);
      break;

    case PAK_CHANGECOL:
      clientChangeColor(cptr, ntohs(((CHANGECOLP *)pptr)->handle),
			ntohs(((CHANGECOLP *)pptr)->color),
			ntohs(((CHANGECOLP *)pptr)->red),
			ntohs(((CHANGECOLP *)pptr)->green),
			ntohs(((CHANGECOLP *)pptr)->blue));
      break;

    case PAK_SETFGCOL:
    case PAK_SETBGCOL:
      clientSetColor(cptr, ntohs(((SETFGCOLP *)pptr)->handle),
		     ntohs(((SETFGCOLP *)pptr)->color));
      break;

    case PAK_SMOUSE:
      client_setmousepointer(cptr, ntohs(((SMOUSEP *)pptr)->handle),
			     ntohs(((SMOUSEP *)pptr)->mtype),
			     ntohs(((SMOUSEP *)pptr)->xoff),
			     ntohs(((SMOUSEP *)pptr)->yoff),
                             ((SMOUSEP *)pptr)->mask,
			     ((SMOUSEP *)pptr)->icon);
      break;

    default:
      fprintf (stderr,
	       "wserver: got paket 0x%04x from client 0x%p, killing client\r\n",
	       pptr->type, cptr);
      cptr->flags |= FLAG_EXITED;
      cptr->inbuf = 0;
  }

  if (rpptr) {
    int len = rpptr->len;
    /* we don't care if the connection is broken */
    rpptr->len = htons(rpptr->len);
    write(cptr->sh, rpptr, len);
  }
}


/*****************************************************************************/

/*
 * some routines processing the events reported by get_eventmask().
 */

static void event_connect(int sokh)
{
  int shtmp;
  socklen_t rlen;
  struct sockaddr_in raddr;
  CLIENT *cptr;

  /* it seems to be safe to expect these to stay empty for AF_UNIX accepts()
   */
  raddr.sin_addr.s_addr = 0;

  rlen = sizeof(struct sockaddr);
  if ((shtmp = accept(sokh, (struct sockaddr *)&raddr, &rlen)) < 0) {
    /* oops, I ran out of file handles? */
    return;
  }

  if ((cptr = createClient())) {
    /* ok, we can do it */
    cptr->sh = shtmp;
    if (sokh != glob_unixh) {
      cptr->raddr = raddr.sin_addr.s_addr;
    } else {
      cptr->raddr = 0;
    }
    FD_SET(shtmp, &glob_crfd);
    cptr->prev = NULL;
    cptr->next = glob_clients;
    if (glob_clients) {
      glob_clients->prev = cptr;
    }
    glob_clients = cptr;
  } else {
    /* shut it down immediately */
    shutdown(shtmp, 2);
    close(shtmp);
  }
}


static void event_client(fd_set *rfd)
{
  CLIENT *next, *cptr = glob_clients;
  int len;
  PAKET *pptr;
  long ret, offset;

  while (cptr) {
    /* keep a local copy of next client, killclient might trash cptr */
    next = cptr->next;

    if (FD_ISSET(cptr->sh, rfd)) {

      if ((ret = read(cptr->sh, cptr->buf + cptr->inbuf,
		      LARGEBUF - cptr->inbuf)) < 1) {
	cptr->flags |= FLAG_EXITED;
      } else {
	offset = 0;

	/* try to extract pakets */
	glob_bytes += ret;
	cptr->inbuf += ret;
	pptr = (PAKET *)cptr->buf;

	while ((cptr->inbuf >= 4) && (cptr->inbuf >= (len = ntohs(pptr->len)))) {
	  glob_pakets++;
	  cptr->pakets++;
	  cptr->bytes += len;
	  cptr->inbuf -= len;
	  pptr->len = len;
	  pptr->type = ntohs(pptr->type);
	  process_paket(cptr, pptr);
	  offset += len;
	  pptr = (PAKET *)(((char *)pptr) + len);
	}

	/* move remaining data down to beginning of buffer */
	if (offset && cptr->inbuf) {
	  bcopy(cptr->buf + offset, cptr->buf, cptr->inbuf);
	}
      }
    }

    if (cptr->flags & FLAG_EXITED)
      killClient (cptr);

    cptr = next;
  }
}


static void event_mousemove(const WEVENT *ev)
{
  short	oldx, oldy;

  oldx = glob_mouse.real.x0;
  oldy = glob_mouse.real.y0;

  glob_mouse.real.x0 += ev->x;
  glob_mouse.real.y0 += ev->y;

  if (glob_mouse.real.x0 < 0)
    glob_mouse.real.x0 = 0;
  if (glob_mouse.real.x0 >= glob_screen->bm.width)
    glob_mouse.real.x0 = glob_screen->bm.width - 1;
  if (glob_mouse.real.y0 < 0)
    glob_mouse.real.y0 = 0;
  if (glob_mouse.real.y0 >= glob_screen->bm.height)
    glob_mouse.real.y0 = glob_screen->bm.height - 1;

#if 0
  /* nobody actually uses the w,h,x1,y1 fields of the 'real' values...
   */
  glob_mouse.real.x1 += glob_mouse.real.x0 + 15;
  glob_mouse.real.y1 += glob_mouse.real.y0 + 15;
#endif

#ifdef REALTIME_MOVING
  if (glob_loopmove) {
    client_move (NULL, loop_move_window->id,
                       loop_move_window->pos.x0 + ev->x,
                       loop_move_window->pos.y0 + ev->y);
/* client_move hides mouse. jps */
    mouse_show ();
   return;
  }
#endif

#ifndef CLICK_TO_FOCUS
  /* watch out for active windows. this may also hide the mouse.
   */
  w_changeActiveWindow ();
#endif

  if ((glob_mouse.real.x0 != oldx) || (glob_mouse.real.y0 != oldy)) {
    /* if we don't redraw the mouse immediately it really looks stupid */
    mouse_hide ();
    mouse_show ();

    /* client wants to know that mouse moved */
    if (glob_activewindow->flags & EV_MMOVE) {
      EVENTP paket;
      paket.len = htons (sizeof(EVENTP));
      paket.type = htons (PAK_EVENT);
      paket.event.type = htons(EVENT_MMOVE);
      paket.event.time = htonl(glob_evtime);
      paket.event.win = glob_activewindow->libPtr;
      paket.event.x = htons(glob_mouse.real.x0 - oldx);
      paket.event.y = htons(glob_mouse.real.y0 - oldy);
      paket.event.key = htonl(0);
      if (write(glob_activewindow->cptr->sh, &paket,
	  sizeof(EVENTP)) != sizeof(EVENTP))
	perror ("wserver: event_mousemove(): write()");
    }
  }
}


static void event_mousebutton (const WEVENT *ev)
{
  static const char *perrorMsg = "wserver: event_mousebutton(): write()";
  short	x, y, wx, wy, pressed, released;
  EVENTP paket;
  WINDOW *win;

#ifdef REALTIME_MOVING
  if (glob_loopmove) {
    mouse_hide ();
    glob_loopmove = 0;
    mouse_show ();

    glob_leftmousepressed = NULL;
    glob_rightmousepressed = NULL;
    return;
  }
#endif

  memset(&paket, 0, sizeof(EVENTP));
  paket.len = htons (sizeof(EVENTP));
  paket.type = htons (PAK_EVENT);
  paket.event.time = htonl(glob_evtime);

  /* first deal with release events seperately, because they might go to
   * different windows...
   *
   * Removed check for backgroundwin as unnecessary from these two. ++eero
   */

  if (glob_leftmousepressed && (ev->reserved[1] & BUTTON_LEFT)) {

    paket.event.type = htons(EVENT_MRELEASE);
    paket.event.win = glob_leftmousepressed->libPtr;
    paket.event.key = htonl(BUTTON_LEFT);
    x = ev->x - glob_leftmousepressed->work.x0;
    y = ev->y - glob_leftmousepressed->work.y0;
    if ((x < 0) || (x >= glob_leftmousepressed->work.w) ||
	(y < 0) || (y >= glob_leftmousepressed->work.h)) {
      x = -1;
      y = -1;
    }
    paket.event.x = htons(x);
    paket.event.y = htons(y);
    if (write(glob_leftmousepressed->cptr->sh, &paket,
	      sizeof(EVENTP)) != sizeof(EVENTP))
      perror (perrorMsg);
    glob_leftmousepressed = NULL;
  }

  if (glob_rightmousepressed && (ev->reserved[1] & BUTTON_RIGHT)) {

    paket.event.type = htons(EVENT_MRELEASE);
    paket.event.win = glob_rightmousepressed->libPtr;
    paket.event.key = htonl(BUTTON_RIGHT);
    x = ev->x - glob_rightmousepressed->work.x0;
    y = ev->y - glob_rightmousepressed->work.y0;
    if ((x < 0) || (x >= glob_rightmousepressed->work.w) ||
	(y < 0) || (y >= glob_rightmousepressed->work.h)) {
      x = -1;
      y = -1;
    }
    paket.event.x = htons(x);
    paket.event.y = htons(y);
    if (write(glob_rightmousepressed->cptr->sh, &paket,
	      sizeof(EVENTP)) != sizeof(EVENTP))
      perror (perrorMsg);
    glob_rightmousepressed = NULL;
  }

  /* can we already break here?
   */
  if (!ev->reserved[0])
    return;

  /* there was a `press' event, deal with it...
   */
  x = ev->x;
  y = ev->y;
  if ((win = window_find (x, y, 1)) == glob_backgroundwin) {
    /* action on background
     */
    if (ev->reserved[0] & BUTTON_LEFT)
      menu_domenu();
    return;
  }

#ifdef CLICK_TO_FOCUS
  w_changeActiveWindowTo(win);
#endif

  /* action on window
   */
  paket.event.win = win->libPtr;

  if (rect_cont_point (&win->work, x, y)) {

    /* action in work area of window
     */
    if (win->flags & EV_MOUSE) {

      if (ev->reserved[0] & BUTTON_LEFT)
	glob_leftmousepressed = win;
      if (ev->reserved[0] & BUTTON_RIGHT)
	glob_rightmousepressed = win;

      /* we won't deal with the `middle' button emulation so far...
       */
      paket.event.key = htonl (ev->reserved[0] & (BUTTON_LEFT | BUTTON_RIGHT));
      paket.event.x = htons (x - win->work.x0);
      paket.event.y = htons (y - win->work.y0);
      paket.event.type = htons (EVENT_MPRESS);

      if (write(win->cptr->sh, &paket, sizeof(EVENTP)) != sizeof(EVENTP))
	perror (perrorMsg);
    }
    return;
  }

  /* window gadget action 
   */
  if (win->flags & (W_ICON | W_CLOSE)) {

    wx = x - win->pos.x0;
    wy = y - win->pos.y0;

    if (rect_cont_point (&win->area[AREA_CLOSE], wx, wy)) {

      /* close gadget clicked
       */
      paket.event.type = htons(EVENT_GADGET);
      paket.event.key = htonl(GADGET_CLOSE);

      if (write(win->cptr->sh, &paket, sizeof(EVENTP)) != sizeof(EVENTP))
	perror (perrorMsg);

      if (ev->reserved[0] & BUTTON_LEFT)
	glob_leftmousepressed = NULL;
      if (ev->reserved[0] & BUTTON_RIGHT)
	glob_rightmousepressed = NULL;

      return;
    }

    if (rect_cont_point (&win->area[AREA_ICON], wx, wy)) {

      /* icon gadget clicked
       */
      paket.event.type = htons(EVENT_GADGET);
      paket.event.key = htonl(GADGET_ICON);

      if (write(win->cptr->sh, &paket, sizeof(EVENTP)) != sizeof(EVENTP))
	perror (perrorMsg);

      if (ev->reserved[0] & BUTTON_LEFT)
	glob_leftmousepressed = NULL;
      if (ev->reserved[0] & BUTTON_RIGHT)
	glob_rightmousepressed = NULL;

      return;
    }
  }

  /* action on frame/title of window  (ie. what's left...)
   *
   * Left button resizes and moves windows.  If window isn't/can't be
   * resized, it will be moved.  Right button tops/bottoms window.
   */

#ifdef CLICK_TO_FOCUS
  window_to_top(win);
#endif

  /* for moving */
  pressed  = BUTTON_RIGHT;		/* check pressed */
  released = BUTTON_LEFT;		/* check release */

  if ((win->flags & W_RESIZE) &&
      (ev->reserved[0] & BUTTON_LEFT)) {

    int idx = 0;

    if (y - win->pos.y0 < 4)
      idx |= 1;   /* upper edge */

    if (win->pos.y1 - y < 4)
      idx |= 2;   /* lower edge */

    if (x - win->pos.x0 < 4)
      idx |= 4;   /* left edge */

    if (win->pos.x1 - x < 4)
      idx |= 8;   /* right edge */

    if (idx) {

      short x0 = win->pos.x0, y0 = win->pos.y0, xe = win->pos.x1, ye = win->pos.y1;
      short oldX0 = x0, oldY0 = y0, oldXe = xe, oldYe = ye;

      resize_rectangle (&x0, &y0, &xe, &ye, idx);

      if ((oldX0 != x0) || (oldY0 != y0) || (oldXe != xe) || (oldYe != ye)) {

	paket.len = htons (sizeof (EVENTP));
	paket.type = htons (PAK_EVENT);
	paket.event.type = htons (EVENT_RESIZE);
	paket.event.win = win->libPtr;
	paket.event.x = htons (x0);
	paket.event.y = htons (y0);
	/* size for work area.
	 * beware: y border depends on whether window has titlebar or not.
	 */
	paket.event.w = htons (xe-x0-(win->pos.w-win->work.w)+1);
 	paket.event.h = htons (ye-y0-(win->pos.h-win->work.h)+1);

	if (write (win->cptr->sh, &paket,
		   sizeof (EVENTP)) != sizeof (EVENTP))
	  perror (perrorMsg);

	/* resize made */
	return;
      }
      /* else try to move window until left button is pressed again */
      pressed  = BUTTON_LEFT;
      released = BUTTON_RIGHT;
    }
  }

  if ((win->flags & W_MOVE) && !(ev->reserved[0] & pressed)) {
    x = win->pos.x0;
    y = win->pos.y0;

#ifdef REALTIME_MOVING
    mouse_hide ();
    loop_move_window = win;
    glob_loopmove = 1;
    mouse_show ();
#else
    if (!get_rectangle(win->pos.w, win->pos.h, &x, &y, pressed, released))
      client_move(NULL, win->id, x, y);
#endif
  }

  if (!(win->flags & W_TOP) && (ev->reserved[0] & BUTTON_RIGHT)) {
    /* top or down the window */
    w_topDown(win);
  }
}


/*****************************************************************************/

/*
 * guess what... :-)
 */

void loop(void)
{
  long events, savecheck = 0x70000000;
  const WEVENT *ev;
  int issaving = 0;
  fd_set rfd;

  while (42) {

    events = get_eventmask(1000, &rfd);
#ifdef __MINT__
# ifdef MAC
    /* MacMiNT seems to have a rather strange idea
     * of how fast hz200 is supposed to go...
     */
    glob_evtime = sysvar->hz200 * 1000L / 50;
# else	    
    glob_evtime = clock () * 1000L / CLK_TCK;
# endif
#else
    {
      struct timeval tv;
      gettimeofday (&tv, NULL);
      glob_evtime = tv.tv_sec*1000L + tv.tv_usec/1000;
    }
#endif

/* check for new connection requests */
    if (events & EV_UCONN) {
      event_connect(glob_unixh);
    }
#ifndef AF_UNIX_ONLY
    if (events & EV_ICONN) {
      event_connect(glob_ineth);
    }
#endif

/* check for client pakets */
    if (events & EV_CLIENT) {
      event_client(&rfd);
    }

/* check for mouse events */
    if (events & EV_MOUSE) {
      savecheck = glob_evtime;
      if ((ev = event_mouse())) switch (ev->type) {
        case EVENT_MMOVE:
	  event_mousemove(ev);
	  break;
	case EVENT_MPRESS:
	  event_mousebutton(ev);
	  break;
      }
    }

/* check for keys */
    if (events & EV_KEYS) {
      savecheck = glob_evtime;
      event_key();
    }

/* update everything that may have been changed by the client or the mouse */
    windowRedrawAllIfDirty();

/* how about the saver? */
    if (saveclient) {
      EVENTP paket;

      if (issaving) {
	if (glob_evtime < savecheck + savetime) {
	  issaving = 0;
	  paket.len = htons(sizeof(EVENTP));
	  paket.type = htons(PAK_EVENT);
	  paket.event.type = htons(EVENT_SAVEOFF);
	  paket.event.time = htonl(glob_evtime);
	  if (write(saveclient->sh, &paket, sizeof(EVENTP))
	      != sizeof(EVENTP)) {
	    perror("server: disable_ssaver: write()");
	  }
	}
      } else {
        if (glob_evtime > savecheck + savetime) {
	  issaving = 1;
	  paket.len = htons(sizeof(EVENTP));
	  paket.type = htons(PAK_EVENT);
	  paket.event.type = htons(EVENT_SAVEON);
	  paket.event.time = htonl(glob_evtime);
	  if (write(saveclient->sh, &paket, sizeof(EVENTP))
	      != sizeof(EVENTP)) {
	    perror("server: enable_ssaver: write()");
	  }
	}
      }
    }

/* re-enable the mouse cursor if necessary and possible */
#ifdef LAZYMOUSE
    if ((glob_mouse.rx != glob_mouse.dx) || (glob_mouse.ry != glob_mouse.dy))
#endif
      mouse_show();

/* complete exit wanted? */
    if (is_terminating && !glob_clients) {
      /* this won't return */
      terminate(0, "");
    }

/* deal with VT switches if necessary (ATM linux only) */
    if (glob_screen->vtswitch) {
      (glob_screen->vtswitch)();
    }
  }
}
