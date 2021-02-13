/*
 * ggi.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- intialization for GGI, the Generic Graphics Interface
 *
 * NOTES
 * - actually we should only set colors that are used in the palette so that
 *   other windows aren't affected so much.  If the average palette use is
 *   significantly smaller than the palette size, setting only used colors
 *   could be faster too.
 *
 * CHANGES
 * ++eero, 03/98:
 * - preliminary GGI support.  All GGI events come from same fd, whereas
 *   with others we'll check keyboard and mouse separately, therefore I
 *   had to do fairly extensive changes...
 * ++eero 1/99:
 * - Adapted to 1998 libGGI changes (doesn't offer framebuffer
 *   address anymore, have to use DirectBuffer scheme now,
 *   several functions vanished/replaced etc)...
 * - adapted libGGI stuff for libGII.  Keyboard and mouse events come
 *   now from different FD.
 * - special key mapping for libGII
 * - kbdExit() stubs
 * ++eero 5/99:
 * - GGI modifier mappings
 * ++eero, 4/03:
 * - moved all initializations and event code to their respective backends
 * ++eero, 5/2003:
 * - fixed compilation with latest GGI (v2)
 */

#if !defined(BMONO) && !defined(DIRECT8)
#error "you have to define at least one of BMONO or DIRECT8 graphics drivers!"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ggi/ggi.h>
#include <ggi/ggi-unix.h>
#include <unistd.h>
#include <netinet/in.h>
#include "../window.h"
#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "gstructs.h"
#include "backend.h"


ggi_visual_t Visual;
static const ggi_directbuffer *DB;


/* event masks */
#define WGGI_KEY_MASK (emKeyRepeat|emKeyPress|emKeyRelease)
#define WGGI_MOUSE_MASK (emPtrButtonPress|emPtrButtonRelease|emPtrRelative|emPtrAbsolute)
#define WGGI_MMOVE_MASK (emPtrRelative|emPtrAbsolute)


static int ggi_screen_init(BITMAP *bm, int mono)
{
	char *err_msg = "";
	int type, stride;
	ggi_mode info;

	if (ggiInit() || !(Visual = ggiOpen(NULL))) {
		fprintf(stderr, "wserver: GGI initialization failed!\r\n");
		return -1;
	}

#ifdef BMONO
	if (mono) {
		type = GT_1BIT;
	} else
#endif
	{
#ifdef DIRECT8
		type = GT_8BIT;
#else
		type = GT_1BIT;
#endif
	}

	/* find user-specified parameters */
	ggiParseMode ("", &info);

	info.graphtype = type;

	/* default size */
	if (info.visible.x == GT_AUTO) {
		info.visible.x = 640;
	}
	if (info.visible.y == GT_AUTO) {
		info.visible.y = 480;
	}

	info.frames = 1; /* no double buffering */
	info.virt.x = GGI_AUTO;
	info.virt.y = GGI_AUTO;
	info.dpp.x = info.dpp.y = GGI_AUTO;

	/*  force target to update the mode to something it can handle */
	ggiCheckMode(Visual, &info);

	/* we really want that type though */
	info.graphtype = type;

        if (ggiSetMode(Visual, &info)) {
		err_msg = "wserver: GGI mode setting error!\r\n";
		goto error;
       }
	/* as we don't use GGI functions have to do flushes... */
	ggiAddFlags(Visual, GGIFLAG_ASYNC);

	/* get the first & only buffer */
	DB = ggiDBGetBuffer(Visual, 0);

	/* We don't handle anything but simple pixel-linear buffers
	 * where your write to same buffer you read.
	 *
	 * Let's hope this check is enough for now...
	 */
	if(!DB || !(DB->type & GGI_DB_SIMPLE_PLB)) {
		err_msg = "wserver: GGI DirectBuffer not supported or of incompatible type!\r\n";
		goto error;
	}
	/* Acquire DirectBuffer before we use it. */
	if (ggiResourceAcquire(DB->resource, GGI_ACTYPE_WRITE|GGI_ACTYPE_READ)) {
		err_msg = "wserver: error acquiring read/write access to GGI DirectBuffer!\r\n";
		goto error;
	}

	bm->data   = DB->write;
	bm->planes = GT_DEPTH(type);
	bm->width  = info.visible.x;	/* or virt.x? */
	bm->height = info.visible.y;
	stride     = DB->buffer.plb.stride;

	if (!bm->data) {
		err_msg = "wserver: mode has no linear frame buffer!\r\n";
		goto error;
	}

	switch (bm->planes) {
#ifdef BMONO
	case 1:
		/* check for driver requirements */
		if (bm->width & 31 || stride & 3) {
			err_msg = "wserver: screen width not long aligned (32 multiple)!\r\n";
			goto error;
		}
		bm->type	= BM_PACKEDMONO;
		bm->upl		= stride >> 2;  /* bm->width >> 5; */
		bm->unitsize	= 4;
		break;
#endif
#ifdef DIRECT8
	case 8:
		/* check for driver requirements */
		if (bm->width & 3 || stride & 3) {
			err_msg = "wserver: screen width not long aligned (4 multiple)!\r\n";
			goto error;
		}
		bm->type	= BM_DIRECT8;
		bm->upl		= stride;
		bm->unitsize	= 1;
		break;
#endif
	default:
		err_msg = "wserver: GGI mode depth error!\r\n";
		goto error;
	}

	printf ("wserver: %ix%i GGI screen with %i colors.\r\n",
		bm->width, bm->height, 1 << bm->planes);

	/* all OK */
	return 0;

error:
	/* handle errors */
	if (DB) {
		ggiResourceRelease(DB->resource);
	}
	if (Visual) {
		ggiClose(Visual);
	}
	fprintf(stderr, err_msg);
	ggiExit();
	Visual = NULL;
	return -1;
}


#ifdef DIRECT8
/*
 * set color palette
 */
static short WggiPalette(COLORTABLE *colTab, short idx)
{
	static ggi_color *ggi_cmap = NULL;
	uchar *r, *g, *b;
	ggi_color *cmap;
	int colors;

	if (!ggi_cmap) {
		colors = 1 << theScreen->bm.planes;
		ggi_cmap = malloc(sizeof(ggi_color) * colors);
		if (!ggi_cmap) {
			return -1;
		}
	}

	cmap = ggi_cmap;
	r = colTab->red;
	g = colTab->green;
	b = colTab->blue;

	if (idx >= 0) {
		/* set single color */
		cmap[idx].r = r[idx] << 8;
		cmap[idx].g = g[idx] << 8;
		cmap[idx].b = b[idx] << 8;
		ggiSetPalette(Visual, idx, 1, ggi_cmap);
		return 0;
	}

	/* set all colors in palette */
	colors = colTab->colors;
	while (--colors >= 0) {
		cmap->r = *r++ << 8;
		cmap->g = *g++ << 8;
		cmap->b = *b++ << 8;
		cmap++;
	}

	ggiSetPalette(Visual, 0, colTab->colors, ggi_cmap);
	return 0;
}
#endif


/*
 * the real init function
 */

SCREEN *ggi_init(int mono)
{
	BITMAP bm;

	if (ggi_screen_init(&bm, mono)) {
		fprintf(stderr, "fatal: unknown or unsupported graphics mode requested!\r\n");
		return NULL;
	}
	if (giiInit()) {
		fprintf(stderr, "fatal: input initialization failed!\r\n");
	  	/* init input */
		ggiExit();
		return NULL;
	}
	if (ggiSetEventMask(Visual, WGGI_KEY_MASK | WGGI_MOUSE_MASK)) {
		fprintf(stderr, "fatal: input mask failed!\r\n");
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
		direct8_screen.changePalette = WggiPalette;
		return &direct8_screen;
	}
#endif
	fprintf(stderr, "fatal: SCREEN struct for mode missing!\r\n");
	ggiExit();

	return NULL;
}


void ggi_exit(void)
{
	if (DB) {
		ggiResourceRelease(DB->resource);
	}
	if (Visual) {
		/* black screen */
		ggiSetGCForeground(Visual, 0);
		ggiFillscreen(Visual);
		ggiClose(Visual);
	}
	giiExit();
	ggiExit();
}


/* ******************** event handling ************************ */


long get_eventmask(long timeout, fd_set *retrfd)
{
  fd_set rfd;
  struct timeval tv, *tvp;
  ggi_event_mask mask;
  long events = 0;
  int rfds; 

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

    /* setup ggi event masks */
    mask = WGGI_MOUSE_MASK | WGGI_KEY_MASK;

    /* select the events and descriptors */
    if ((rfds = ggiEventSelect(Visual, &mask,
         FD_SETSIZE, &rfd, NULL, NULL, tvp)) < 0) {

      /* experience has shown that it is not safe
       * to test the fd_sets after an error
       */
      return 0;
    }

    /* what have we got? */
    if (mask & WGGI_MOUSE_MASK) {
      events |= EV_MOUSE;
    }
    if (mask & WGGI_KEY_MASK) {
      events |= EV_KEYS;
    }

    if (FD_ISSET(glob_unixh, &rfd)) {
      tv.tv_sec = tv.tv_usec = 0;
      events |= EV_UCONN;
      rfds--;
    }

#ifndef AF_UNIX_ONLY
    if (glob_ineth >= 0 && FD_ISSET(glob_ineth, &rfd)) {
      events |= EV_ICONN;
      rfds--;
    }
#endif

    if (rfds) {
      /* what remains? */
      events |= EV_CLIENT;
      if (retrfd) {
	*retrfd = rfd;
      }
    }  

  } while (!timeout && !events);

DEBUG(("events: 0x%lx mask: 0x%x (of: 0x%x)\r\n", events, mask, ggiGetEventMask(Visual)));
  return events;
}


/* this could be streamlined a bit after I get confident enough that
 * GGI has finally got it's event API/functionality together.
 */
const WEVENT *event_mouse(void)
{
  static uchar lastbutton = (BUTTON_LEFT | BUTTON_MID | BUTTON_RIGHT);
  static WEVENT someevent;
  int relative;
  short dx, dy;
  uchar button;

  struct timeval tm = {0,0};
  ggi_event ev;

  dx = dy = 0;
  relative = 0;
  button = lastbutton;

  do {
    ggiEventRead(Visual, &ev, WGGI_MOUSE_MASK);

    switch (ev.any.type) {

      /* Note that button status is *not* a mask (according
       * to the only doc I found, the events.h header comment).
       */
      case evPtrButtonPress:
        /* clear button bits */
	if (ev.pbutton.button == GII_PBUTTON_LEFT) {
	  button &= ~BUTTON_LEFT;
	} else if (ev.pbutton.button == GII_PBUTTON_RIGHT) {
	  button &= ~BUTTON_RIGHT;
	} else if (ev.pbutton.button == GII_PBUTTON_MIDDLE) {
	  button &= ~BUTTON_MID;
	}
	break;

      case evPtrButtonRelease:
        /* set button bits */
	if (ev.pbutton.button == GII_PBUTTON_LEFT) {
	  button |= BUTTON_LEFT;
	} else if (ev.pbutton.button == GII_PBUTTON_RIGHT) {
	  button |= BUTTON_RIGHT;
	} else if (ev.pbutton.button == GII_PBUTTON_MIDDLE) {
	  button |= BUTTON_MID;
	}
	break;

      case evPtrRelative:
	dx += ev.pmove.x;
	dy += ev.pmove.y;
	relative = 1;
	break;

      case evPtrAbsolute:
	dx = ev.pmove.x - glob_mouse.real.x0;
	dy = ev.pmove.y - glob_mouse.real.y0;
	break;
    }
  /* compress mouse moves */
  } while (ggiEventPoll(Visual, WGGI_MOUSE_MASK, &tm) & WGGI_MMOVE_MASK);

DEBUG(("button %d->%d, dx: %d, dy: %d!\r\n", lastbutton, button, dx, dy));

  if(dx || dy) {
    if (relative) {
      mouse_accelerate(&dx, &dy);
    }
    someevent.x = dx;
    someevent.y = dy;

    /* mouse position changed */
    someevent.type = EVENT_MMOVE;
    return &someevent;
  }
 
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


/* all input is done through the Visual,
 * therefore no initialization needed
 */
static long kbd_translate(gii_key_event *key)
{
  if (GII_KTYP(key->sym) == GII_KT_LATIN1) {
    return GII_KVAL(key->sym);
  }
  switch (key->label) {
    case GIIK_F1:	return WKEY_F1;
    case GIIK_F2:	return WKEY_F2;
    case GIIK_F3:	return WKEY_F3;
    case GIIK_F4:	return WKEY_F4;
    case GIIK_F5:	return WKEY_F5;
    case GIIK_F6:	return WKEY_F6;
    case GIIK_F7:	return WKEY_F7;
    case GIIK_F8:	return WKEY_F8;
    case GIIK_F9:	return WKEY_F9;
    case GIIK_F10:	return WKEY_F10;
    case GIIK_F11:	return WKEY_F11;
    case GIIK_F12:	return WKEY_F12;

    case GIIK_Up:       return WKEY_UP;
    case GIIK_Down:     return WKEY_DOWN;
    case GIIK_Left:     return WKEY_LEFT;
    case GIIK_Right:    return WKEY_RIGHT;

    case GIIK_PageUp:   return WKEY_PGUP;
    case GIIK_PageDown: return WKEY_PGDOWN;
    case GIIK_Home:     return WKEY_HOME;
    case GIIK_End:      return WKEY_END;

    case GIIK_Insert:   return WKEY_INS;
    case GIIK_Delete:   return WKEY_DEL;

    /* modifiers */
    case GIIK_ShiftL:	return WMOD_SHIFT | WMOD_LEFT;
    case GIIK_ShiftR:	return WMOD_SHIFT | WMOD_RIGHT;
    case GIIK_CtrlL:	return WMOD_CTRL  | WMOD_LEFT;
    case GIIK_CtrlR:	return WMOD_CTRL  | WMOD_RIGHT;
    case GIIK_AltL:	return WMOD_ALT   | WMOD_LEFT;
    case GIIK_AltR:	return WMOD_ALT   | WMOD_RIGHT;
    case GIIK_MetaL:	return WMOD_META  | WMOD_LEFT;
    case GIIK_MetaR:	return WMOD_META  | WMOD_RIGHT;
    case GIIK_SuperL:	return WMOD_SUPER | WMOD_LEFT;
    case GIIK_SuperR:	return WMOD_SUPER | WMOD_RIGHT;
    case GIIK_HyperL:	return WMOD_HYPER | WMOD_LEFT;
    case GIIK_HyperR:	return WMOD_HYPER | WMOD_RIGHT;
    case GIIK_AltGr:		return WMOD_ALTGR;
    case GIIK_CapsLock:		return WMOD_CAPS;
    case GIIK_NumLock:		return WMOD_NUM;
    case GIIK_ScrollLock:	return WMOD_SCROLL;
  }
  return -1;
}


void event_key(void)
{
  static EVENTP paket[MAXKEYS];
  int mods, max = 0;
  long ch;

  struct timeval tm = {0,0};
  ggi_event ev;

  if ((glob_activewindow == glob_backgroundwin) ||
      !(glob_activewindow->flags & (EV_KEYS|EV_MODIFIERS))) {

    while (ggiEventPoll(Visual, WGGI_KEY_MASK, &tm)) {
      /* discard keys */
      ggiEventRead(Visual, &ev, WGGI_KEY_MASK);
    }
    return;
  }

  /* get also key releases & modifiers */
  mods = glob_activewindow->flags & EV_MODIFIERS;

  /* check that there's a *key* event pending */
  while (ggiEventPoll(Visual, WGGI_KEY_MASK, &tm)) {
    ggiEventRead(Visual, &ev, WGGI_KEY_MASK);

    if ((ch = kbd_translate(&(ev.key))) >= 0) {
      if (ev.any.type == evKeyRelease) {
        if (!mods) {
					/* without modifiers discard key releases */
					continue;
				}
				paket[max].event.type = htons(EVENT_KRELEASE);
      } else {
        if (IS_WMOD(ch) && !mods) {
					/* discard modifiers */
					continue;
				}
				paket[max].event.type = htons(EVENT_KEY);
      }
DEBUG(("key: %ld\r\n", ch));

      paket[max].type = htons(PAK_EVENT);
      paket[max].len = htons(sizeof(EVENTP));
      paket[max].event.time = htonl(glob_evtime);
      paket[max].event.key = htonl(ch);
      paket[max].event.win = glob_activewindow->libPtr;

      if (++max >= MAXKEYS) {
	/* processed enough keys, leave rest */
	break;
      }
    }
  }
  if (max) {
    write(glob_activewindow->cptr->sh, paket, max * sizeof(EVENTP));
  }
}
