/*
 * gproto.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997,2003 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- backend (screen, mouse and keyboard) specific init, exit and
 *    event processing functions
 *
 * backends implement:
 * - get_eventmask()	check incoming events
 * - event_mouse()	process mouse events
 * - event_key()	process key events
 *
 * NOTES
 *
 * get_eventmask():  reports a bitmask of ready events.
 *
 * event_mouse():  read (and wait for) mouse events.
 *
 * TeSche:  other than that, we still lack a way to simultaneously report
 * mouse movement *and* button presses which is why the stuff here tries to
 * treat them seperately.  perhaps I should really implement a higher level
 * buffer of mouse events...
 *
 * event.reserved[0] reports buttons that came down and
 * event.reserved[1] reports buttons that came up
 * 
 * event_key():  send up to MAXKEYS of keyboard input to the active window.
 * normally this would be called only when get_eventmask() call has shown
 * there to be keys available.
 * 
 * When adding key translation functions, remember to add
 * WSERVER_KEY_MAPPING to server flags for PAK_INIT in loop.c!
 *
 * 
 * TODO
 * 
 * - Special key (arrow etc.) mapping to W defined values for all the
 *   backends.
 * - OS native charset to ISO-latin1 mapping as all W fonts (converted
 *   from X11 ones) use that.
 *
 *
 * CHANGES
 * 
 * ++eero 2/99:
 * - mouse acceleration for all backends.
 * ++eero 3/99:
 * - simplified event handling a bit.  get_eventmask checks now always all
 *   the things and stuff wanting just mouse or key events call those
 *   functions straight.  This needed adding mevent_read() call to mouse
 *   function.
 * ++eero 5/99:
 * - added key release events.
 * ++eero 4/03:
 * - moved event handling to backends and backend protos to backend.h
 */

#ifndef __GPROTO_H
#define __GPROTO_H

#include "../rect.h"


/* in init.c, call suitable backed init and exit functions */
extern SCREEN *screen_init(int forcemono);
extern void screen_exit(void);

/* global graphics context */
extern ushort DefaultPattern[16];	/* 50% gray */
extern ushort BackGround[16];		/* screen background */
extern SCREEN *theScreen;		/* should be same as glob_screen */
extern int FirstPoint;			/* line: draw first point */
extern REC *glob_clip0, *glob_clip1;
extern REC glob_screen_clip;
extern GCONTEXT *gc0;
/*
 * These could be used for all kinds of operations where a temporary bitmap
 * is needed.
 */
extern BITMAP bitmapTmp;
extern REC clipTmp;


/* in ../mouse.c */
extern MOUSE glob_mouse;


/* in all the backends or unix_input.c */
extern long get_eventmask(long timeout, fd_set *retrfd);
extern const WEVENT *event_mouse(void);
extern void event_key(void);


/****************************************************
 * define function which does the screen update
 */
#ifdef REFRESH

# ifdef SDL
#include <SDL/SDL_video.h>
extern SDL_Surface *sdl_screen; /* in sdl.c, for screen refresh */
static inline void screen_update(REC *u)
{
  REC c;
  /* clip update rect to screen */
  if (rect_intersect(u, &(glob_screen_clip), &c)) {
    SDL_UpdateRect(sdl_screen, c.x0, c.y0, c.w, c.h);
  }
}
#define SCREEN_REFRESH 1
#else

#ifdef GGI
#include <ggi/ggi.h>
extern ggi_visual_t Visual; /* in ggi.c, for screen refresh */
static inline void screen_update(REC *u)
{
  REC c;
  /* clip update rect to screen */
  if (rect_intersect(u, &(glob_screen_clip), &c)) {
    ggiFlushRegion(Visual, c.x0, c.y0, c.w, c.h);
  }
}
#define SCREEN_REFRESH 1
#else

/* no screen refresh required */
#undef SCREEN_REFRESH
#endif /* GGI */
#endif /* SDL */


#endif /* REFRESH */
/*****************************************************/

#endif /* __GPROTO_H */
