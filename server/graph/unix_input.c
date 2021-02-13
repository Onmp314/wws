/*
 * unix_input.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998,2003 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- "generic" unix input (kbd, mouse/gpm) and event handling functions
 * 
 * 
 * TODO
 * 
 * - This is such an awful mess of ifdefs that it might be better to move
 *   MiNT and NetBSD mouse and keyboard handling stuff to their own files
 *   and leave just GPM and normal mouse handling here...
 * 
 * 
 * CHANGES
 * 
 * Phx, 06/96:
 * - mouse support for NetBSD (Sun Firm_event)
 * - NetBSD /dev/kbd support! ;)
 * - global keymap structure for NetBSD
 * ++eero 12/98:
 * - GPM support for x86 linux without GGI or SVGAlib
 * ++oddie 4/99:
 * - arrow key translation for /dev/mackbd on MacMiNT
 * ++eero, 4/03:
 * - moved all initializations and event code to their respective backends
 */

#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef __NetBSD__
#include <sys/device.h>
#include <sys/uio.h>
#include <amiga/dev/itevar.h>
#include <amiga/dev/kbdmap.h>
#include <amiga/dev/vuid_event.h>
extern int tgetent(char *, char *);
extern char *tgetstr(char *, char **);
extern void tputs(register char *, int, int (*)());
#else
#include <termcap.h>
#endif
#ifdef GPM
# include <gpm.h>
#endif
#include "../config.h"
#include "../types.h"
#include "../pakets.h"
#include "../proto.h"
#include "../window.h"
#include "backend.h"



/****************** event handling *************************/


int glob_kbd;


/* how many mouse events to buffer (and compress movements) */
#define BUF_MEVS	85

/* bytes / single mouse event */
#ifdef __NetBSD__
# define MEV_SIZE	sizeof(Firm_event)
  static char mbuf[BUF_MEVS * MEV_SIZE], *mptr = mbuf;
  /* keymap for rawkey translation */
  static struct kbdmap netbsd_kmap;
#else
# ifdef __MINT__
#  define MEV_SIZE	3	/* mouse packet size */
   static char mbuf[BUF_MEVS * MEV_SIZE], *mptr = mbuf;
# else
#  ifndef GPM
#   error "Unsupported event system for W Window System"
#  endif
# endif
#endif

static int mevents = 0;

/* - should be called *only* when there's mouse input.
 * - returns EV_MOUSE if there's at least one event, otherwise 0
 */
static long mevent_move(void)
{
#ifdef GPM
  /* almost NOP ;-) */
  return EV_MOUSE;
#else
  static int count, offset = 0;

  /* both processed packets AND unprocessed buffer space? */
  if ((count = mptr - mbuf)) {
    if ((offset -= count)) {
#ifdef DEBUG
      if (offset < 0) {
        DEBUG(("mptr went over mbuf+count!"));
      } else
#endif
      memcpy(mbuf, (char*)mptr, offset);
    }
    /* either *all* packets processed or moved to buffer start */
    mptr = mbuf;
  }

  if ((count = read(glob_mouse.fh, &mbuf[offset], BUF_MEVS*MEV_SIZE - offset)) >= 0) {
    offset += count;
    mevents = offset / MEV_SIZE;
  } else {
    DEBUG(("error in reading mouse!"));
  }
  if (mevents) {
    return EV_MOUSE;
  }
  return 0;
#endif
}

static void mevent_read(void)
{
  fd_set rfd;
  struct timeval tv;

  /* STonX froze after I pressed AltGR until mouse was moved, so I use here
   * a 1/2 sec timeout.  I suspect that this might be STonX bug as keyboard
   * events shouldn't generate mouse events (and this worked on
   * linux-fb/GPM)...  ;-)
   */
  tv.tv_sec = 0;
  tv.tv_usec = 500000;

  FD_ZERO(&rfd);
  FD_SET(glob_mouse.fh, &rfd);
  if (select(FD_SETSIZE, &rfd, NULL, NULL, &tv) > 0) {
    if (FD_ISSET(glob_mouse.fh, &rfd)) {
      mevent_move();
    }
  }
}


long get_eventmask(long timeout, fd_set *retrfd)
{
  struct timeval tv, *tvp;
  long events = 0;
  fd_set rfd;
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

    FD_SET(glob_kbd, &rfd);
    FD_SET(glob_mouse.fh, &rfd);

    /* set up timeout */
    if (timeout < 0) {
      tvp = NULL;
    } else {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout - 1000 * tv.tv_sec;
      tvp = &tv;
    }

    /* select the descriptors */
    if ((rfds = select(FD_SETSIZE, &rfd, NULL, NULL, tvp)) < 0) {
      /* experience has shown that it is not safe
       * to test the fd_sets after an error
       */
      return 0;
    }

    /* what have we got? */
    if (FD_ISSET(glob_mouse.fh, &rfd)) {
      events |= mevent_move();
      rfds--;
    }

    if (FD_ISSET(glob_kbd, &rfd)) {
      events |= EV_KEYS;
      rfds--;
    }

    if (FD_ISSET(glob_unixh, &rfd)) {
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

  return events;
}


#if defined(__MINT__)


const WEVENT *event_mouse(void)
{
  static char lastbutton = (BUTTON_LEFT | BUTTON_MID | BUTTON_RIGHT);
  static WEVENT someevent;
  short dx, dy;
  char button;

  dx = 0;
  dy = 0;

  /* 'compress' successive relative movement events */
  while (mevents && ((*mptr++ & 7) == lastbutton)) {
    dx += *mptr++;
    dy -= *mptr++;
    mevents--;
  }

  if (mevents) {
    mptr--;   /* adjust pointer */
  }

  if (dx || dy) {
    mouse_accelerate(&dx, &dy);
    someevent.x = dx;
    someevent.y = dy;

    /* the mouse moved */
    someevent.type = EVENT_MMOVE;
    return &someevent;
  }

  if (mevents && (*mptr != lastbutton)) {
    /* some button event occured */
    button = *mptr++ & 7;
    mptr++; mptr++;		/* ignore movement here */
    mevents--;

    someevent.reserved[0] = lastbutton & ~button;
    someevent.reserved[1] = ~lastbutton & button;
    lastbutton = button;

    someevent.x = glob_mouse.real.x0;
    someevent.y = glob_mouse.real.y0;

    /* or EVENT_MRELEASE, we don't care here. */
    someevent.type = EVENT_MPRESS;
    return &someevent;
  }

  /* ensure that next time around we get something... */
  if (!mevents) {
    mevent_read();
  }

  return NULL;
}


#elif defined(GPM)	/* GPM, !MINT */


const WEVENT *event_mouse(void)
{
  static WEVENT someevent;
  Gpm_Event ev;
  int b, c;

  if (Gpm_GetEvent(&ev) > 0) {

    /* if I could compress mouse move events, acceleration would
     * be more effective.  However, there's no Gpm_CheckForEvent().
     */
    if (ev.type & (GPM_MOVE | GPM_DRAG)) {
      someevent.x = ev.dx;
      someevent.y = ev.dy;
      mouse_accelerate(&someevent.x, &someevent.y);
      someevent.type = EVENT_MMOVE;
      return &someevent;
    }

    if (ev.type & (GPM_UP | GPM_DOWN)) {
      c = 0;
      b = ev.buttons;
      if (b & GPM_B_LEFT)
        c |= BUTTON_LEFT;
      if (b & GPM_B_RIGHT)
        c |= BUTTON_RIGHT;
      if (b & GPM_B_MIDDLE)
        c |= BUTTON_MID;

      if (ev.type & GPM_UP) {
	someevent.reserved[0] = 0;
	someevent.reserved[1] = c;
      } else {
	someevent.reserved[0] = c;
	someevent.reserved[1] = 0;
      }
      someevent.x = glob_mouse.real.x0;
      someevent.y = glob_mouse.real.y0;

      /* or EVENT_MRELEASE, we don't care here. */
      someevent.type = EVENT_MPRESS;
      return &someevent;
    }
  }

  /* ensure that next time around we get something... */
  mevent_read();

  return NULL;
}

#elif defined(__NetBSD__)

/* Phx 06/96
 * This is the mouse event handler for NetBSD, which uses an emulation
 * of Sun's Firm_events.
 */

const WEVENT *event_mouse(void)
{
  static uchar lastbutton = BUTTON_ALL;
  static WEVENT someevent;
  short val, dx=0, dy=0;
  uchar button;

  button = lastbutton;
  while (mevents) {
    val = (short)((Firm_event *)mptr)->value;
    switch (((Firm_event *)mptr)->id) {
      case LOC_X_DELTA: 
        dx += val;
	break;
      case LOC_Y_DELTA:
	dy += val;
	break;
      case MS_LEFT:
	if (val)
	  button &= ~BUTTON_LEFT;
	else
	  button |= BUTTON_LEFT;
	break;
      case MS_MIDDLE:
	if (val)
	  button &= ~BUTTON_MID;
	else
	  button |= BUTTON_MID;
	break;
      case MS_RIGHT:
	if (val)
	  button &= ~BUTTON_RIGHT;
	else
	  button |= BUTTON_RIGHT;
	break;
    }
    --mevents;
    mptr += sizeof(Firm_event);
  }

  /* unfortunately the only solution in the current environment: */
  /* if a button event was received, the mouse movements are discarded, */
  /* so it is best to allow only a single event in the buffer... */
  if (button != lastbutton) {
    someevent.type = EVENT_MPRESS; /* or EVENT_MRELEASE, we don't care here. */
    someevent.x = glob_mouse.real.x0;
    someevent.y = glob_mouse.real.y0;
    someevent.reserved[0] = lastbutton & ~button;
    someevent.reserved[1] = ~lastbutton & button;
    lastbutton = button;

    return &someevent;
  }

  if (dx || dy) {
    mouse_accelerate(&dx, &dy);
    someevent.x = dx;
    someevent.y = dy;
    someevent.type = EVENT_MMOVE;

    return &someevent;
  }

  /* ensure that next time around we get something... */
  if (!mevents) {
    mevent_read();
  }
  return NULL;
}

#else
#warning "other than GPM, NetBSD, MINT mouse required"
#endif


#if defined(linux) || defined(__MINT__)


#if defined(__MINT__) && defined(MAC)
/* ATM this translates only mackbd arrow keys... */
static long kbd_translate(long c)
{
  switch(c) {
    case 28: return WKEY_LEFT;
    case 29: return WKEY_RIGHT;
    case 30: return WKEY_UP;
    case 31: return WKEY_DOWN;
    default: return c;
  }
}

#elif defined(__NetBSD__)


/*
 * Phx 06/96 - support for NetBSD/Amiga. /dev/kbd supplies us with
 * Firm_events containing (Amiga specific) raw key codes.
 */
void event_key(void)
{
  static unsigned char key_mod=0;
  EVENTP paket[MAXKEYS];
  uchar	c[MAXKEYS];
  Firm_event fe;
  int nchr=0, i;
  struct key key;
  unsigned char mask, code;
  char *str;

  if (read(glob_kbd,&fe,sizeof(Firm_event)) > 0) {
    code = (unsigned char)fe.id;

    /* check for qualifier */
    if (code >= KBD_LEFT_SHIFT) {
      mask = 1 << (code - KBD_LEFT_SHIFT);
      if (!fe.value)
	key_mod &= ~mask;  /* qualifier released */
      else
	key_mod |= mask;  /* qualifier pressed */
    }

    else if (fe.value) {
      /* a new key was pressed - translate it */
      if (key_mod & KBD_MOD_SHIFT) {
	if (key_mod & KBD_MOD_ALT)
	  key = netbsd_kmap.alt_shift_keys[code];
	else 
	  key = netbsd_kmap.shift_keys[code];
      } else if (key_mod & KBD_MOD_ALT)
	key = netbsd_kmap.alt_keys[code];
      else {
	key = netbsd_kmap.keys[code];
	if ((key_mod & KBD_MOD_CAPS) && (key.mode & KBD_MODE_CAPS))
	  key = netbsd_kmap.shift_keys[code];
      }
      code = key.code;

      /* 
       * For simplicity KBD_MODE_KPAD is ignored, so the keypad-keys are
       * treated as normals keys, instead of generating strings.
       */

      if (key.mode & KBD_MODE_STRING) {
	/* key generates a string */
	str = netbsd_kmap.strings + code;
	i = (int)*str++;
	while (--i >= 0)
	  c[nchr++] = *str++;
      }
      /* Dead keys and accents are (currently) ignored */
    
      else {
	if (key_mod & KBD_MOD_CTRL)
	  code &= 0x1f;
	if (key_mod & KBD_MOD_META)
	  code |= 0x80;
	
	/* return single ASCII character */
	c[nchr++] = code;
      }
    }
  }

  if (nchr) {
    if ((glob_activewindow == glob_rootwindow) ||
	!(glob_activewindow->flags & EV_KEYS)) {
      /* want no keys */
      return;
    }

    i = nchr;
    while (--i >= 0) {
      paket[i].len = htons(sizeof(EVENTP));
      paket[i].type = htons(PAK_EVENT);
      paket[i].event.type = htons(EVENT_KEY);
      paket[i].event.time = htonl(glob_evtime);
      paket[i].event.key = htonl(c[i]);
      paket[i].event.win = glob_activewindow->libPtr;   /* no change */
    }
    
    write(glob_activewindow->cptr->sh, paket, nchr * sizeof(EVENTP));
  }
}

#else
/* others trust that terminal programs can interpret the key sequences */
# define kbd_translate(k) (k)
#endif


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

#else
#warning "other than Linux, MiNT key event handler required"
#endif


/****************** terminal handling ****************************/


static struct termios orig_termios;
static char *termname = NULL;
static char buffer[512];
static char cap[512];


static int init (void)
{
	if (termname)
		return 0;
	if (!(termname = getenv ("TERM"))) {
		fprintf (stderr, "wserver: TERM not set\n");
		return 1;
	}
	if (tgetent (buffer, termname) != 1) {
		termname = NULL;
		fprintf (stderr, "wserver: unknown terminal type\n");
		return 1;
	}
	return 0;
}

static inline int tputchar (int c)
{
	return putc (c, stdout);
}

static void cursor_on (void)
{
	char *cp = cap;

	if (init())
		return;
	cp = tgetstr ("ve", &cp);
	tputs (cp, 1, tputchar);
}

static void cursor_off (void)
{
	char *cp = cap;

	if (init())
		return;
	cp = tgetstr ("vi", &cp);
	tputs (cp, 1, tputchar);
}

static void clear_scr (void)
{
	char *cp = cap;

	if (init())
		return;
	cp = tgetstr ("cl", &cp);
	tputs (cp, 1, tputchar);
}

static void makeraw (struct termios *tt)
{
	tt->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|
			ICRNL|IXON);
	tt->c_oflag &= ~OPOST;
	/*
	 * NOTE: we dont clear IEXTEN, so we sill still get
	 * cursor key mapping. (when disabling IEXTEN I get
	 * 0x00 for all cursor key presses and cursor keys
	 * don't work under Mint).
	 *
	 * ++kay.
	 */
	tt->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG);
	tt->c_cflag &= ~(CSIZE|PARENB);
	tt->c_cflag |= CS8;
}


/*************** key / mouse init *****************************/


/* - name of the KBD device (or NULL for stdin)
 * - name of the mouse device
 */
int unix_input_init(char *kbd, char *mouse)
{
  /* get old terminal setting & set terminal to raw mode */
  struct termios mytermios;

  /* save terminal settings & set them to raw */
  tcgetattr(0, &orig_termios);
  mytermios = orig_termios;
  makeraw (&mytermios);
  tcsetattr(0, TCSANOW, &mytermios);
  
  if (kbd) {
    if ((glob_kbd = open(kbd, O_RDONLY)) < 0) {
      fprintf(stderr, "error: can't open keyboard\r\n");
      return -1;
    }
  } else {
    /* read from standard input */
    glob_kbd = 0;
  }

#ifdef __NetBSD__
  /* determine active keymap for rawkey translations */
  if (ioctl (0, ITEIOCGKMAP, &netbsd_kmap)) {
    fprintf (stderr, "error: can't determine system keymap\r\n");
    return -1;
  }
#endif

#if defined(__NetBSD__) || defined(sun)
  if (!glob_debug) {
    /* this will redirect all keyboard input to /dev/kbd rather than to the
     * systems console. since later, in wterm, we can only catch both input and
     * output for the console we must assure here that wserver still gets its
     * input.
     */
    int arg = 1;
    ioctl(glob_kbd, KIOCSDIRECT, &arg);
  }
#endif
  
#if defined(GPM)
  {
    Gpm_Connect conn;
    conn.eventMask = GPM_UP | GPM_DOWN | GPM_MOVE | GPM_DRAG;
    conn.defaultMask = 0;		/* gpm itself shouldn't act on events */
    conn.minMod = 0;		/* Don't need modifiers, but */
    conn.maxMod = ~0;		/*   send mouse events with all modifiers. */
    gpm_zerobased = 1;		/* coords start at zero */
    
    if (Gpm_Open(&conn, 0) < 0) {		/* at current vc */
      fprintf(stderr, "fatal: GPM mouse open failed!\n");
      return -1;
    }
    glob_mouse.fh = gpm_fd;
  }
#else
  if ((glob_mouse.fh = open (mousedev, O_RDONLY | O_NDELAY)) < 0) {
    fprintf(stderr, "fatal: mouse '%s' open failed!\n", mousedev);
    return -1;
  }
#endif
  
  return 0;
}


void unix_input_exit(void)
{
#ifdef GPM
  Gpm_Close();
#else
  close(glob_mouse.fh);
#endif

  if (glob_kbd) {
    close(glob_kbd);
  }
  
#if 0	/* I want to see the error messages */
  clear_scr();
#endif
  cursor_on();

  /* restore terminal settings */
  tcsetattr(0, TCSANOW, &orig_termios);
}
