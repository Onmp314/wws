/*
 * svgalib.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Torsten Will and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a graphics/mouse driver for the (x86) Linux-System with SVGAlib
 *
 * CHANGES
 * ++eero, 11/97:
 * - Rewrote the whole thing to use both DIRECT8 and BMONO drivers.
 *   BMONO 640x480x2 is the default resolution as that's the largest
 *   standard VGA (ie most cards should support it) resolution
 *   which can be accessed linearly.
 * ++eero, 4/03:
 * - moved all initializations and event code to their respective backends
 *
 * NOTES:
 * - Due to SVGAlib bug, with BMONO this needs to set a couple of vga
 *   registers here.  These aren't restored after changing to another
 *   virtual console and as result you'll get white-on-white. :-(.
 * - If you want a driver that support gfx-card accelerations, write
 *   a kernel driver for the GGI project.
 */

#if !defined(i386) || !defined(linux) || !defined(SVGALIB)
#error "this is a *x86-linux* *SVGAlib* graphics driver"
#endif

#if !defined(BMONO) && !defined(DIRECT8)
#error "you have to define at least one of BMONO or DIRECT8 graphics drivers!"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <vga.h>
#include <vgamouse.h>

#include "../window.h"
#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "gstructs.h"
#include "backend.h"


/*
 *
 */

#define GRA_I   0x3CE		/* Graphics Controller Index */
#define GRA_D   0x3CF		/* Graphics Controller Data Register */

/* set some vga registers and hope svgalib restores them at exit... */
static __inline__ void port_out(int value, int port)
{
	__asm__ volatile ("outb %0,%1"
	      ::"a" ((unsigned char) value), "d"((unsigned short) port));
}

/*
 * the big init functions
 */

static int svgalib_screen_init(BITMAP *bm, int forcemono)
{
	vga_modeinfo *modeinfo;
	int switch_to_linear = 0;	   /* 1 if neccessary */
	int svgamode, i;
#ifdef BMONO
	int tried_mono = 0;
#endif

	vga_init();

	/* get mode to switch to */

	/* from $GSVGAMODE environment variable.  Another alternative would
	 * be to present user with a menu like many other SVGAlib programs
	 * do...
	 */
	svgamode = vga_getdefaultmode();
#ifdef BMONO
	if (svgamode < 0 || forcemono) {
		/* try standard mono VGA resolution */
		svgamode = G640x480x2;
		tried_mono = 1;
	}
#else
	if (svgamode < 0) {
		svgamode = G320x200x256;
	}
#endif
	modeinfo = vga_getmodeinfo(svgamode);

	/* mode is linear one with 1 byte per pixel? */
	if ((modeinfo->flags & CAPABLE_LINEAR) && modeinfo->bytesperpixel == 1) {
		/* force switch to linear */
		switch_to_linear = 1;
	} else {
#ifdef BMONO
		printf ("wserver: Mode not capable of 1bpp linear adressing. Using 640x480x2!\r\n");
		svgamode = G640x480x2;
		tried_mono = 1;
#else
		printf ("wserver: Mode not capable of 1bpp linear adressing. Using 320x200x256!\r\n");
		svgamode = G320x200x256;
#endif
	}

	/* switch to mode */
	vga_setmode(svgamode);
	if (switch_to_linear) {
		if (vga_setlinearaddressing() < 0) {
#ifdef BMONO
			if (tried_mono) {
				return -1;
			}
			printf ("wserver: Could not set linear adressing. Using 640x480x2!\r\n");
			svgamode = G640x480x2;
			vga_setmode(svgamode);
#else
			printf ("wserver: Could not set linear adressing!\r\n");
			return -1;
#endif
		}
	}

	modeinfo = vga_getmodeinfo(svgamode);

	bm->data	= vga_getgraphmem();
	bm->width	= modeinfo->width;
	bm->height	= modeinfo->height;

	if (modeinfo->bytesperpixel != 1) {
		if (bm->width & 31) {
			fprintf(stderr, "warning: monochrome screen width not a multiple of 32 pixels,\r\n");
			fprintf(stderr, "         can't use this graphic mode!\r\n");
			return -1;
		}
		bm->type	= BM_PACKEDMONO;
		bm->upl		= bm->width >> 5;
		bm->planes	= 1;
		bm->unitsize	= 4;

		/* set palette registers correctly */
		vga_setpalette(0, 63, 63, 63);
		for (i = 255; i > 0; i--) {
			vga_setpalette(i, 0, 0, 0);
		}
		/* disable Set/Reset Register */
		port_out(0x01, GRA_I);
		port_out(0x00, GRA_D);
	} else {
		/* 8-bit linear */
		bm->type	= BM_DIRECT8;
		bm->upl		= bm->width; 
		bm->planes	= 8;
		bm->unitsize	= 1;
	}

	printf ("wserver: %ix%i SVGAlib screen with %i colors.\r\n",
		bm->width, bm->height, 1 << bm->planes);

	return 0;
}


#ifdef DIRECT8

/*
 * set color palette
 */
static short svgaPutCmap(COLORTABLE *colTab, short index)
{
	static int *svga_cmap = NULL;
	int *cmap, colors;
	uchar *r, *g, *b;

	if (!svga_cmap) {
		colors = 1 << theScreen->bm.planes;
		svga_cmap = malloc(3 * sizeof(*cmap) * colors);
		if (!svga_cmap) {
			return -1;
		}
	}

	if (index >= 0 ) {
		/* set just one color */
		vga_setpalette(index, colTab->red[index] >> 2,
				colTab->green[index] >> 2,
				colTab->blue[index] >> 2);
		return 0;
	}

	cmap = svga_cmap;
	r = colTab->red;
	g = colTab->green;
	b = colTab->blue;
	colors = colTab->colors;

	while (--colors >= 0) {
		*cmap++ = *r++ >> 2;
		*cmap++ = *g++ >> 2;
		*cmap++ = *b++ >> 2;
	}

	vga_setpalvec(0, colTab->colors, svga_cmap);
	return 0;
}

#endif


static int svgalib_input_init(void)
{
	/* has to be called before vga_setmode */
	vga_setmousesupport (1);
	glob_mouse.fh = 0;
	return 0;
}


/*
 * the real init function
 */

SCREEN *svgalib_init(int forcemono)
{
	BITMAP bm;
	
	if (svgalib_input_init()) {
		return NULL;
	}

	if (svgalib_screen_init(&bm, forcemono)) {
		fprintf(stderr, "fatal: unknown or unsupported graphics mode requested!\r\n");
		return NULL;
	}
#ifdef BMONO
	if (bm.type == BM_PACKEDMONO) {
		bmono_screen.bm = bm;
		return &bmono_screen;
	}
#endif
#ifdef DIRECT8
	if (bm.type == BM_DIRECT8) {
		direct8_screen.bm = bm;
		direct8_screen.changePalette = svgaPutCmap;
		return &direct8_screen;
	}
#endif
	fprintf(stderr, "fatal: SCREEN struct for mode missing!\n");
	return NULL;
}


void svgalib_exit(void)
{
  vga_setmode(TEXT);
}


/* ******************** event handling ************************ */


long get_eventmask(long timeout, fd_set *retrfd)
{
  fd_set rfd;
  struct timeval tv, *tvp;
  long events = 0;
  int vga_events;

  do {

    /* set up file descriptors */
    rfd = glob_crfd;

    FD_SET(glob_unixh, &rfd);

#ifndef AF_UNIX_ONLY
    if (glob_ineth >= 0) {
      FD_SET(glob_ineth, &rfd);
    }
#endif

    /* set up timeout */
    if (timeout < 0) {
      tvp = NULL;
    } else {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout - 1000 * tv.tv_sec;
      tvp = &tv;
    }

    vga_events = VGA_KEYEVENT | VGA_MOUSEEVENT;

    /* select the descriptors */
    if ((vga_events = vga_waitevent(vga_events, &rfd, NULL, NULL, tvp)) < 0) {
      return 0;
    }

    /* what have we got? */
    if (vga_events & VGA_MOUSEEVENT) {
      events |= EV_MOUSE;
    }

    if (vga_events & VGA_KEYEVENT) {
      events |= EV_KEYS;
    }

    if (FD_ISSET(glob_unixh, &rfd)) {
      events |= EV_UCONN;
    }

#ifndef AF_UNIX_ONLY
    if (glob_ineth >= 0 && FD_ISSET(glob_ineth, &rfd)) {
      events |= EV_ICONN;
    }
#endif

    /* what remains? */
    events |= EV_CLIENT;
    if (retrfd) {
      *retrfd = rfd;
    }
    
  } while (!timeout && !events);

  return events;
}


const WEVENT *event_mouse(void)
{
  static uchar lastbutton = (BUTTON_LEFT | BUTTON_MID | BUTTON_RIGHT);
  static WEVENT someevent;
  short dx, dy;
  uchar button;
  short down;

  /* wait until there's an event */
  vga_waitevent(VGA_MOUSEEVENT, NULL, NULL, NULL, NULL);

  dx = mouse_getx() - glob_mouse.real.x0;
  dy = mouse_gety() - glob_mouse.real.y0;
  
  if(dx || dy) {

    someevent.x = dx;
    someevent.y = dy;

    /* mouse position changed */
    someevent.type = EVENT_MMOVE;
    return &someevent;
  }

  button = (BUTTON_LEFT | BUTTON_MID | BUTTON_RIGHT);
  down = mouse_getbutton();

  if (down & MOUSE_LEFTBUTTON)
    button ^= BUTTON_LEFT;
  if (down & MOUSE_RIGHTBUTTON)
    button ^= BUTTON_RIGHT;
  if (down & MOUSE_MIDDLEBUTTON)
    button ^= BUTTON_MID;
 
  if(lastbutton != button) {
    /* button status changed */

    someevent.reserved[0] = lastbutton & ~button;
    someevent.reserved[1] = ~lastbutton & button;
    lastbutton = button;

    someevent.x = glob_mouse.real.x0;
    someevent.y = glob_mouse.real.y0;

    /* or EVENT_MRELEASE, we don't care here. */
    someevent.type = EVENT_MPRESS;
    return &someevent;
  }

  /* obviously nothing happened */
  return NULL;
}


/* trust that terminal programs can interpret the key sequences */
# define kbd_translate(k) (k)


void event_key(void)
{
  static EVENTP paket[MAXKEYS];
  uchar	c[MAXKEYS];
  int max, i;

  if (ioctl(glob_kbd, FIONREAD, &max)) {
    /* how could this happen?
     */
    return;
  }
  if (max > MAXKEYS) {
    max = MAXKEYS;
  }
  if ((max = read(glob_kbd, c, max)) < 1) {
    /* how could this happen?
     */
    return;
  }
  if ((glob_activewindow == glob_backgroundwin) ||
      !(glob_activewindow->flags & EV_KEYS)) {
    /* want no keys
     */
    return;
  }

  i = max;
  while (--i >= 0) {
    paket[i].len = htons(sizeof(EVENTP));
    paket[i].type = htons(PAK_EVENT);
    paket[i].event.type = htons(EVENT_KEY);
    paket[i].event.time = htonl(glob_evtime);
    paket[i].event.key = htonl(kbd_translate(c[i]));
    paket[i].event.win = glob_activewindow->libPtr;   /* no change */
  }

  write(glob_activewindow->cptr->sh, paket, max * sizeof(EVENTP));
}
