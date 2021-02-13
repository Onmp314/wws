/*
 * backend.h, a part of the W Window System
 *
 * Copyright (C) 2003 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- prototypes for backend init and exit functions
 */

#ifndef __BACKEND_H
#define __BACKEND_H

#include "../proto.h"


/* how many key (events) to process at the time at maximum */
#define MAXKEYS 8

#define DEBUG(text)
/* #define DEBUG(text)	{ printf text ;  fflush(stdout); } */


/*
 * Use accelerate() function only for relative mouse movements events.
 * For absolute mouse positions report:
 *	<newpos> - glob_mouse.real.[xy]0
 */
static inline void mouse_accelerate(short *dx, short *dy)
{
  switch (glob_mouse_accel) {
    case ACCEL_DOUBLE:
      if (*dx)  *dx = (*dx << 1) + (*dx >= 0 ? 1 : -1);
      if (*dy)  *dy = (*dy << 1) + (*dy >= 0 ? 1 : -1);
      break;
    case ACCEL_POWER:
      *dx = (*dx >= 0 ? *dx * *dx : -*dx * *dx);
      *dy = (*dy >= 0 ? *dy * *dy : -*dy * *dy);
      break;
  }
}


#ifdef SDL
/* in sdl.c */
extern SCREEN *sdl_init(int forcemono);
extern void sdl_exit(void);
#endif

#ifdef __MINT__
# ifdef MAC
/* in macmint.c */
extern SCREEN *macmint_init(int forcemono);
extern void macmint_exit(void);
# else
/* in mint.c */
extern SCREEN *mint_init(int forcemono);
extern void mint_exit(void);
# endif
#endif

#ifdef linux
/* in linux.c */
extern SCREEN *linux_init(int forcemono);
extern void linux_exit(void);
#endif

#ifdef SVGALIB
/* in svgalib.c */
extern SCREEN *svgalib_init(int forcemono);
extern void svgalib_exit(void);
#endif

#ifdef GGI
/* in ggi.c */
extern SCREEN *ggi_init(int forcemono);
extern void ggi_exit(void);
#endif

#ifdef __NETBSD__
/* in netbsd_amiga.c */
extern SCREEN *netbsd_amiga_init(int forcemono);
extern void netbsd_amiga_exit(void);
#endif

#ifdef sun
/* in sun.c */
extern SCREEN *sun_init(int forcemono);
extern void sun_exit(void);
#endif

/* unix_input.c */
/* - name of the KBD device (or NULL for stdin)
 * - name of the mouse device
 */
extern int unix_input_init(char *kbd, char *mouse);
extern void unix_input_exit(void);


#endif /* __BACKEND_H */
