/*
 * sdl.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998,2002 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- intialization for SDL, Simple DirectMedia Layer
 *
 * NOTES
 * - actually we should only set colors that are used in the palette so that
 *   other windows aren't affected so much.  If the average palette use is
 *   significantly smaller than the palette size, setting only used colors
 *   could be faster too.
 *
 * CHANGES
 * ++eero, 9/2002:
 * - Started on a SDL driver for W window system. Supports only 8-bit
 *   graphics for now.
 * ++eero, 4/2003:
 * - had finally time to finish the SDL port
 * - moved initializations and event code to their respective backends
 * ++eero, 5/2003:
 * - keyboard repeat and other things finally work
 * ++eero, 7/2008:
 * - use fullscreen and largest available video mode
 */

#if !defined(DIRECT8)
#error "this works only with the DIRECT8 graphics driver!"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <SDL/SDL.h>
#include <SDL/SDL_events.h>
#include <SDL/SDL_syswm.h>
#include <X11/Xlib.h>
#include "../window.h"
#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "gstructs.h"
#include "backend.h"

/* global so that we can both init and set colors */
SDL_Surface *sdl_screen;


static int sdl_screen_init(BITMAP *bm, int mono)
{
	int stride;
	int wd, ht, bits = 8;
	unsigned int flags = SDL_FULLSCREEN;
	SDL_Rect **modes;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr,	"SDL error: %s\n", SDL_GetError());
		return -1;
	}
	
	modes = SDL_ListModes(NULL, flags);
	if (!modes || modes == (SDL_Rect**)-1) {
		/* no modes or all are acceptable */
		wd = 800;
		ht = 600;
	} else {
		/* first one is largest */
		wd = modes[0]->w;
		ht = modes[0]->h;
	}
	
	sdl_screen = SDL_SetVideoMode(wd, ht, bits, flags);
	if (sdl_screen == NULL) {
		fprintf(stderr,	"SDL error: %s\r\n", SDL_GetError());
		return -1;
	}
	SDL_WM_SetCaption("W Window System", "W Window System");

	bm->data   = sdl_screen->pixels;
	bm->width  = sdl_screen->w;
	bm->height = sdl_screen->h;
	bm->planes = bits;
	stride     = (bm->width * bits) >> 3;

	switch (bm->planes) {
#ifdef BMONO
	case 1:
		/* check for driver requirements */
		if (bm->width & 31 || stride & 3) {
			SDL_Quit();
			sdl_screen = NULL;
			fprintf(stderr, "wserver: screen width not long aligned (32 multiple)!\r\n");
			return -1;
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
			SDL_Quit();
			fprintf(stderr, "wserver: screen width not long aligned (4 multiple)!\r\n");
			return -1;
		}
		bm->type	= BM_DIRECT8;
		bm->upl		= stride;
		bm->unitsize	= 1;
		break;
#endif
	default:
		SDL_Quit();
		fprintf(stderr, "wserver: SDL mode depth error!\r\n");
		return -1;
	}

	printf ("wserver: %ix%i SDL screen with %i colors.\r\n",
		bm->width, bm->height, 1 << bm->planes);

	return 0;
}


#ifdef DIRECT8
/*
 * set color palette
 */
static short WSDLPalette(COLORTABLE *colTab, short idx)
{
	static SDL_Color *cmap;
	uchar *used, *r, *g, *b;
	int colors;
	
	if (!cmap) {
		colors = 1 << theScreen->bm.planes;
		cmap = calloc(colors, sizeof(SDL_Color));
		if (!cmap) {
			fprintf(stderr, "wserver: color map allocations failed!\r\n");
			return -1;
		}
	}

	r = colTab->red;
	g = colTab->green;
	b = colTab->blue;

	/* just one color */
	if (idx >= 0) {
		cmap[idx].r = r[idx];
		cmap[idx].g = g[idx];
		cmap[idx].b = b[idx];
		SDL_SetColors(sdl_screen, cmap, idx, 1);
		return 0;
	}

	/* all the colors */
	used = colTab->used;
	colors = colTab->colors;

	for (idx = 0; idx < colors; idx++) {
		cmap[idx].r = r[idx];
		cmap[idx].g = g[idx];
		cmap[idx].b = b[idx];
	}
	SDL_SetColors(sdl_screen, cmap, 0, colors);
	return 0;
}
#endif


/*
 * the real init function
 */

SCREEN *sdl_init(int mono)
{
	BITMAP bm;
	
	if (sdl_screen_init(&bm, mono)) {
		fprintf(stderr, "fatal: unknown or unsupported graphics mode requested!\r\n");
		return NULL;
	}
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);

	SDL_ShowCursor(0);
#if 0
	/* grab/hide mouse */
	SDL_WM_GrabInput(SDL_GRAB_ON);
#endif

#ifdef BMONO
	if (bm.type == BM_PACKEDMONO) {
		bmono_screen.bm = bm;
		return &bmono_screen;
	}
#endif
#ifdef DIRECT8
	if (bm.type == BM_DIRECT8) {
		direct8_screen.bm = bm;
		direct8_screen.changePalette = WSDLPalette;
		return &direct8_screen;
	}
#endif
	fprintf(stderr, "fatal: SCREEN struct for mode missing!\r\n");
	SDL_Quit();

	return NULL;
}

void sdl_exit(void)
{
	SDL_ShowCursor(1);
	SDL_Quit();
}


/* ******************** event handling ************************ */


long get_eventmask(long timeout, fd_set *retrfd)
{
  fd_set rfd;
  struct timeval tv, *tvp;
  SDL_Event event;
  long events = 0;
  int rfds; 

  do {
    /* because SDL doesn't have select() type interface,
     * we have to alternate polling of W server / client sockets
     * and SDL events :-/
     */

    SDL_PumpEvents();
    /* check queued SDL events */
    while (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_ALLEVENTS) > 0) {
      
      switch (event.type) {
      case SDL_KEYUP:
      case SDL_KEYDOWN:
DEBUG(("event: key\r\n"));
	events |= EV_KEYS;
	break;
	
      case SDL_MOUSEMOTION:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
DEBUG(("event: mouse\r\n"));
	events |= EV_MOUSE;
	break;
	
      case SDL_QUIT:
	terminate(SIGHUP, "SDL_QUIT");
      default:
DEBUG(("event: other\r\n"));
	/* remove unrecognized events from the queue */
	SDL_PollEvent(&event);
      }
      if (events) {
	break;
      }
    }

    /* set up file descriptors */
    rfd = glob_crfd;

    FD_SET(glob_unixh, &rfd);

#ifndef AF_UNIX_ONLY
    if (glob_ineth >= 0) {
      FD_SET(glob_ineth, &rfd);
    }
#endif

#if 0
    /* set up timeout */
    if (timeout < 0) {
      tvp = NULL;
    } else {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout - 1000 * tv.tv_sec;
      tvp = &tv;
    }
#else
    /* set slight delay */
    tv.tv_sec = 0;
    tv.tv_usec = 50;
    tvp = &tv;
#endif

    /* select the descriptors */
    if ((rfds = select(FD_SETSIZE, &rfd, NULL, NULL, tvp)) < 0) {
      /* experience has shown that it is not safe
       * to test the fd_sets after an error
       */
      return 0;
    }

    if (FD_ISSET(glob_unixh, &rfd)) {
DEBUG(("event: unix connect\r\n"));
      events |= EV_UCONN;
      rfds--;
    }

#ifndef AF_UNIX_ONLY
    if (glob_ineth >= 0 && FD_ISSET(glob_ineth, &rfd)) {
DEBUG(("event: inet connect\r\n"));
      events |= EV_ICONN;
      rfds--;
    }
#endif

    if (rfds) {
      /* what remains? */
DEBUG(("event: client\r\n"));
      events |= EV_CLIENT;
      if (retrfd) {
	*retrfd = rfd;
      }
    }
    
  } while (!timeout && !events);

  return events;
}


const WEVENT *event_mouse(void)
{
  static uchar lastbutton = (BUTTON_LEFT | BUTTON_MID | BUTTON_RIGHT);
  static WEVENT someevent;
  int relative;
  short dx, dy;
  uchar button;
  SDL_Event ev;

  dx = dy = 0;
  relative = 0;
  button = lastbutton;

  /* do while mouse events are in the queue */
  SDL_PumpEvents();
  while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEEVENTMASK) > 0) {

    switch (ev.type) {

    case SDL_MOUSEBUTTONDOWN:
      /* clear button bits */
      if (ev.button.button & SDL_BUTTON(1)) {
	button &= ~BUTTON_LEFT;
      }
      if (ev.button.button & SDL_BUTTON(2)) {
	/* KLUDGE: buggy SDL mouse button values */
	button &= ~BUTTON_RIGHT;
      }
      if (ev.button.button & SDL_BUTTON(3)) {
	button &= ~BUTTON_RIGHT;
      }
      break;

    case SDL_MOUSEBUTTONUP:
      /* set button bits */
      if (ev.button.button & SDL_BUTTON(1)) {
	button |= BUTTON_LEFT;
      }
      if (ev.button.button & SDL_BUTTON(2)) {
	/* KLUDGE: buggy SDL mouse button values */
	button |= BUTTON_RIGHT;
      }
      if (ev.button.button & SDL_BUTTON(3)) {
	button |= BUTTON_RIGHT;
      }
      break;

    case SDL_MOUSEMOTION:
      /* compress mouse moves */
      dx += ev.motion.xrel;
      dy += ev.motion.yrel;
      relative = 1;

/* Absolute mouse movements
 *   case SDL_MOUSEMOTION:
 *     dx = ev.motion.x - glob_mouse.real.x0;
 *     dy = ev.motion.y - glob_mouse.real.y0;
 *     break;
 */
    }
  }

DEBUG(("button %d->%d, dx: %d, dy: %d\r\n", lastbutton, button, dx, dy));

  if(dx || dy) {
    if (relative) {
      mouse_accelerate(&dx, &dy);
    }
DEBUG(("mouse pos dx: %d, dy: %d\r\n", dx, dy));
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


static long kbd_translate(SDL_KeyboardEvent *key)
{
  SDLKey ch = key->keysym.sym;
  switch (ch) {
    case SDLK_F1:	return WKEY_F1;
    case SDLK_F2:	return WKEY_F2;
    case SDLK_F3:	return WKEY_F3;
    case SDLK_F4:	return WKEY_F4;
    case SDLK_F5:	return WKEY_F5;
    case SDLK_F6:	return WKEY_F6;
    case SDLK_F7:	return WKEY_F7;
    case SDLK_F8:	return WKEY_F8;
    case SDLK_F9:	return WKEY_F9;
    case SDLK_F10:	return WKEY_F10;
    case SDLK_F11:	return WKEY_F11;
    case SDLK_F12:	return WKEY_F12;

    case SDLK_UP:       return WKEY_UP;
    case SDLK_DOWN:     return WKEY_DOWN;
    case SDLK_LEFT:     return WKEY_LEFT;
    case SDLK_RIGHT:    return WKEY_RIGHT;

    case SDLK_PAGEUP:   return WKEY_PGUP;
    case SDLK_PAGEDOWN: return WKEY_PGDOWN;
    case SDLK_HOME:     return WKEY_HOME;
    case SDLK_END:      return WKEY_END;

    case SDLK_INSERT:   return WKEY_INS;
    case SDLK_DELETE:   return WKEY_DEL;
    case SDLK_BACKSPACE: return SDLK_BACKSPACE;

    /* modifiers */
    case SDLK_LSHIFT:	return WMOD_SHIFT | WMOD_LEFT;
    case SDLK_RSHIFT:	return WMOD_SHIFT | WMOD_RIGHT;
    case SDLK_LCTRL:	return WMOD_CTRL  | WMOD_LEFT;
    case SDLK_RCTRL:	return WMOD_CTRL  | WMOD_RIGHT;
    case SDLK_LALT:	return WMOD_ALT   | WMOD_LEFT;
    case SDLK_RALT:	return WMOD_ALT   | WMOD_RIGHT;
    case SDLK_LMETA:	return WMOD_META  | WMOD_LEFT;
    case SDLK_RMETA:	return WMOD_META  | WMOD_RIGHT;
    case SDLK_LSUPER:	return WMOD_SUPER | WMOD_LEFT;
    case SDLK_RSUPER:	return WMOD_SUPER | WMOD_RIGHT;
    case SDLK_MODE:		return WMOD_ALTGR;
    case SDLK_CAPSLOCK:		return WMOD_CAPS;
    case SDLK_NUMLOCK:		return WMOD_NUM;
    case SDLK_SCROLLOCK:	return WMOD_SCROLL;
    default:
      return key->keysym.unicode & 0xFF;
      /* because SDL key mapping doesn't work for normal shifted
       * keys, I'll have to do it myself, works for ASCII, but not
       * for stuff e.g. above number keys.
       */
      ch &= 0xFF;
      if (key->keysym.mod & KMOD_SHIFT) {
        ch = toupper(ch);
      }
      if (key->keysym.mod & KMOD_CTRL) {
	ch &= 0x1f;
      }
      if (key->keysym.mod & KMOD_META) {
	ch |= 0x80;
      }
      return ch;
  }
  return -1;
}


#define SDL_KEYMASK (SDL_KEYDOWNMASK|SDL_KEYUPMASK)

void event_key(void)
{
  static EVENTP paket[MAXKEYS];
  int mods, max = 0;
  SDL_Event ev;
  long ch;

  SDL_PumpEvents();
  if ((glob_activewindow == glob_backgroundwin) ||
      !(glob_activewindow->flags & (EV_KEYS|EV_MODIFIERS))) {
    
    while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_KEYMASK) > 0)
	    ;
    return;
  }

  /* get also key releases & modifiers? */
  mods = glob_activewindow->flags & EV_MODIFIERS;

  /* do while key events are in the queue */
  while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_KEYMASK) > 0) {

    if ((ch = kbd_translate(&(ev.key))) >= 0) {
DEBUG(("key-event\r\n"));
      if (ev.type == SDL_KEYUP) {
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
