/*
 * wterm.c, a part of the W Window System
 *
 * Copyright (C) 1994-1999 by Torsten Scherer, Kay Römer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a W terminal emulating VT52 with several additions
 *
 * CHANGES
 * ++TeSche 01/96:
 * - supports W_ICON & W_CLOSE
 * - supports /etc/utmp logging for SunOS4 and Linux
 * - supports catching of console output for SunOS4
 * ++Phx 02-06/96:
 * - supports NetBSD-Amiga
 * ++eero, 11/97:
 * - unsetenv(DISPLAY), setenv(LINES, COLUMNS). These functions are BSD 4.3,
 *   not POSIX, so you might need to ifdef them out.
 * - Added new text modes (you need to use terminfo...).
 * ++eero, 2/98:
 * - Implemented fg/bgcolor setting.  With monochrome server these change
 *   bgmode variable, which tells in which mode to draw to screen
 *   (M_CLEAR/M_DRAW) and affects F_REVERSE settings.
 * ++eero, 3/98:
 * - Added xterm(1) mouse sequences for Midnight Commander. Try 'mc -x'.
 * ++eero, 5/98:
 * - Completed color setting / interpretation.  Using the reverse option in
 *   color (8 or more) mode, changes the color indeces, not font attributes.
 *   For simplicity's sake uses just the shared server colors.
 * - Doesn't strip 8th bit anymore.
 * ++eero, 6/98:
 * - ESC-G will now enable cursor in addition to resetting text style.
 * ++eero, 4/99:
 * - Translate W special keys to ESC-sequences.
 * ++bsittler, 2000.07.04:
 * - Colors are reset by attribute resets.
 * - Initial reversed state is remembered even on color displays.
 * - Improved handling of ESC '[' ... 'm' strings,
 *   including ESC '[' '0' 'm' (equivalent to ESC 'G'.)
 * - A blinking cursor is now properly disabled, without leaving
 *   garbage on the screen.
 * - TERMCAP is removed from the environment unless EXPORT_TERMCAP
 *   is defined.
 * ++eero, 2/09:
 * - Add support for Unix98 PTY handling
 *   (Linux doesn't anymore support old PTY handling)
 *
 * TODO:
 * - add scroll-region ('cs') command. Fairly many programs
 *   can take advantage of that.
 * - check if current NetBSD really needs ifdefs below with
 *   current W server/library.
 */

#define _GNU_SOURCE 1

#include <stdio.h>
#include <signal.h>
#include <utmp.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <pwd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Wlib.h>
#include "../lib/proto.h"	/* _wserver */
#ifdef EXPORT_TERMCAP
#include "termtype.h"
#endif


/* send xterm mouse sequences if application requests */
#define XTERM_EMU

/* color support */
#define MAX_COLORS	8
static short Colors;

/* mapping default shared server colors to ANSI */
static short color[MAX_COLORS] = { 1, 2, 3, 7, 4, 6, 5, 0 };


#define	SMALLBUFFER 63
#define	LARGEBUFFER 1024

/*
 * some pty helper functions
 */

#ifdef __MINT__
#define NSIG __NSIG
#elif linux
#define NSIG _NSIG
#endif

#ifdef __MINT__

#include <support.h>
#include <mintbind.h>
extern int forkpty(int *master, char *pty, struct termios *t, struct winsize *w);

#else

static void _write_utmp(const char *line, const char *user, const char *host, int time)
{
  int fh, offset, isEmpty, isLine;
  struct utmp ut;

  if ((fh = open("/etc/utmp", O_RDWR)) < 0) {
    return;
  }

  /* first of all try to find an entry with the same line */

  offset = 0;
  isEmpty = -1;
  isLine = -1;

  while ((isLine < 0) && (read(fh, &ut, sizeof(ut)) == sizeof(ut))) {
    if (!ut.ut_line[0]) {
      if (isEmpty < 0) {
	isEmpty = offset;
      }
    } else {
      if (!strncmp(ut.ut_line, line, sizeof(ut.ut_line))) {
	isLine = offset;
      }
    }
    offset += sizeof(ut);
  }

  if (isLine != -1) {
    /* we've found a match */
    lseek(fh, isLine, SEEK_SET);
  } else if (isEmpty != -1) {
    /* no match found, but at least an empty entry */
    lseek(fh, isLine, SEEK_SET);
  } else {
    /* not even an empty entry found, assume we can append to the file */
  }

  if (time) {
    strncpy(ut.ut_line, line, sizeof(ut.ut_line));
    strncpy(ut.ut_name, user, sizeof(ut.ut_name));
    strncpy(ut.ut_host, host, sizeof(ut.ut_host));
    ut.ut_time = time;
#ifdef linux
    ut.ut_type = USER_PROCESS;
#endif
  } else {
    memset(&ut, 0, sizeof(ut));
#ifdef linux
    ut.ut_type = DEAD_PROCESS;
#endif
  }
  write(fh, &ut, sizeof(ut));
  close(fh);
}

static int forkpty(int *master, char *pty, struct termios *t, struct winsize *w)
{
  int mh, sh, pid;

/* Unix98 specification conforming PTY handling */
#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
  const char *fname;
  /* get new master pty handle */
  if ((mh = posix_openpt(O_RDWR)) < 0) {
    return -1;
  }
  /* prepare slave pty */
  if (unlockpt(mh) < 0 || grantpt(mh) < 0 || !(fname = ptsname(mh))) {
    close(mh);
    return -1;
  }
#else

  int c, i;
  char fname[64];
  /* get a free pty */
  for (c='p'; c <= 'z'; c++) {
    for (i = 0; i < 16; i++) {
      sprintf(fname, "/dev/pty%c%x", c, i);
      if ((mh = open(fname, O_RDWR)) >= 0) {
        /* opened master pty, set slave name */
        fname[5] = 't';
        /* the infernal nested loop exit... */
        goto got_pty;
      }
    }
  }
  return -1;
got_pty:
#endif

  /* open slave */
  if ((sh = open(fname, O_RDWR)) < 0) {
    close(mh);
    return -1;
  }

  /* set terminal size */
  if (w) {
    ioctl(sh, TIOCSWINSZ, w);
  }

  /* try to fork */
  switch (pid = fork()) {

    case -1: /* parent: error */
      close(sh);
      close(mh);
      return -1;

    case 0: /* child */
#if 0
      close(mh);
#endif
      setsid();
      ioctl(sh, TIOCSCTTY, 0);

      dup2(sh, 0);
      dup2(sh, 1);
      dup2(sh, 2);
      if (sh > 2) {
	close(sh);
      }
      return 0;

    default: /* parent: ok */
#if 0
      close(sh);
#endif
      *master = mh;
      strcpy(pty, fname);
      return pid;
  }

  return -1;
}

#endif


/*
 * some global variables
 */

static WWIN *win;
static short winw, winh, pid, fonw, fonh, console, cfh;
static int pipeh;
static char pty[SMALLBUFFER];
static short xtermemu = 0, cblink = 0, visualbell = 0, debug = 0;
static struct winsize winsz;


/****************************************************************************/

/*
 *
 */

static WWIN *iconWin;
static int isIconified;

static int iconSetup(const char *family, int fsize)
{
  WFONT *font;

  /* use the same font family as the terminal itself
   * (in case user wants to specify it)
   */
  if (!(font = w_loadfont(family, fsize, F_BOLD))) {
    if (!(font = w_loadfont(NULL, 0, 0)))
      return -1;
  }

  if (!(iconWin = w_create(w_strlen(font, "wterm") + 4, font->height + 2,
			   W_MOVE | EV_MOUSE))) {
    w_unloadfont(font);
    return -1;
  }

  w_setfont(iconWin, font);
  w_printstring(iconWin, 2, 1, "wterm");
  w_unloadfont(font);

  isIconified = 0;

  return 0;
}

static void iconify(void)
{
  short x0, y0;

  if (isIconified) {
    w_querywindowpos(iconWin, 1, &x0, &y0);
    w_close(iconWin);
    w_open(win, x0, y0);
  } else {
    w_querywindowpos(win, 1, &x0, &y0);
    w_close(win);
    w_open(iconWin, x0, y0);
  }

  isIconified = 1 - isIconified;
}


/****************************************************************************/


/*
 * some common tool functions
 */

static void sigpipe(int sig)
{
  /* this one musn't close the window */
  _write_utmp(pty, "", "", 0);
  kill(-pid, SIGHUP);
  _exit(sig);
}


static void sigchld(int sig)
{
  _write_utmp(pty, "", "", 0);
  _exit(sig);
}


static void sigquit(int sig)
{
  signal(sig, SIG_IGN);
  kill(-pid, SIGHUP);
}


/*
 * this is the wterm terminal code, almost like VT52
 */

static short	reversed;
static short	bgmode, escstate, curx, cury, curon, curvis;
static short	savx, savy, wrap, style;
static short	col, row, colmask = 0x7f, rowmask = 0x7f;


static void draw_cursor (void)
{
  if (!curvis) {
    curvis = 1;
    w_setmode(win, M_INVERS);
    w_pbox(win, curx*fonw, cury*fonh, fonw, fonh);
  }
}


static void hide_cursor (void)
{
  if (curvis) {
    curvis = 0;
    w_setmode(win, M_INVERS);
    w_pbox(win, curx*fonw, cury*fonh, fonw, fonh);
  }
}


static void change_bgmode(uchar color)
{
  /* ATM only black/white */
  if (color) {
    if (bgmode != M_DRAW) {
      bgmode = M_DRAW;
      style ^= F_REVERSE;
      w_settextstyle(win, style);
    }
  } else {
    if (bgmode != M_CLEAR) {
      bgmode = M_CLEAR;
      style ^= F_REVERSE;
      w_settextstyle(win, style);
    }
  }
}

static void esc5(uchar c)	/* setting background color */
{
  c -= '0';
  if (Colors) {
    if (c < Colors)
      w_setBackgroundColor(win, color[c]);
  } else {
    change_bgmode(!c);
  }
  escstate = 0;
}

static void esc4(uchar c)	/* setting foreground color */
{
  c -= '0';
  if (Colors) {
    if (c < Colors)
      w_setForegroundColor(win, color[c]);
  } else {
    change_bgmode(c);
  }
  escstate = 0;
}


static void esc1(uchar c);

#ifdef XTERM_EMU

#define MAX_PARAMETERS	6

static int params, parameter[MAX_PARAMETERS];

/* \E[?xxx... xterm terminal parameter handling
 *
 * ATM implements only xterm mouse mode setting
 */
static void esc20(uchar c)
{
  int value, i;

  /* read next parameter? */
  if (c == ';' && params < MAX_PARAMETERS-1) {
    params++;
    return;
  }

  /* parameter? */
  if (c >= '0' && c <= '9')  {
    parameter[params] *= 10;
    parameter[params] += c - '0';
    return;
  }

  /* h = set mode
   * l = clear (reset) mode
   * r = restore mode (from restored)
   * s = save mode
   */
  if (c == 'h' || c == 'l') {
    value = 0;
    for (i = params; i >= 0; i--) {

      switch(parameter[i]) {

        /* set mouse emulation? */
	case 9:
	  value |= 1;	/* x10 mode */
	  break;
	case 1000:	/* report */
	case 1001:	/* track */
	  value |= 2;
	  break;
      }
    }
    if (c == 'l') {
      xtermemu &= ~value;
    } else {
      xtermemu |= value;
    }
  } else {

    if (c == 'm') {

      /* set colors and other modes */
      for (i = 0; i <= params; i++) {

	value = parameter[i];
	if (value >= 30 && value <= 37) {
	  esc4(value - 30 + '0');
	  continue;
	}
	if (value >= 40 && value <= 47) {
	  esc5(value - 40 + '0');
	  continue;
	}
	if (!value) {
	  esc1('G');
	  continue;
	}
      }
    }
  }

  while (params >= 0) {
    parameter[params--] = 0;
  }
  params = 0;
  escstate = 0;
}

/* hopefully this minimal vtxxx '\E[' code handler helps if VT52 isn't
 * enough for you program (at least it will be easier to add xterm
 * compatible functionality).
 */
static void esc10(uchar c)
{
  if (c >= '0' && c <= '9') {
    parameter[params] = c - '0';
    escstate = 20;
    return;
  }

  switch(c) {

    case '?': /* parameters */
      escstate = 20;
      return;

    /* these are already implemented by VT52 code */

    /* case 'A':  cursor up */
    /* case 'B':  cursor down */
    /* case 'C':  cursor right */
    /* case 'D':  cursor left */

    /* case '[':  function keys */

  }
  esc20(c);
}

#endif /* XTERM_EMU */


static void esc3(uchar c)	/* cursor position x axis */
{
  curx = (c - 32) & colmask;
  if (curx >= col)
    curx = col - 1;
  else if (curx < 0)
    curx = 0;
  escstate = 0;
}


static void esc2(uchar c)	/* cursor position y axis */
{
  cury = (c - 32) & rowmask;
  if (cury >= row)
    cury = row - 1;
  else if (cury < 0)
    cury = 0;
  escstate = 3;
}


static void esc1(uchar c)	/* various control codes */
{
  escstate = 0;

  switch(c) {

#ifdef XTERM_EMU

/* xterm key codes */

    case '[':
      escstate = 10;
      break;

#endif /* XTERM_EMU */

    case 'A':/* cursor up */
      if ((cury -= 1) < 0)
	cury = 0;
      break;

    case 'B':/* cursor down */
      if ((cury += 1) >= row)
	cury = row - 1;
      break;

    case 'C':/* cursor right */
      if ((curx += 1) >= col)
	curx = col - 1;
      break;

    case 'D':/* cursor left */
      if ((curx -= 1) < 0)
	curx = 0;
      break;

    case 'E':/* clear screen & home */
      w_setmode(win, bgmode);
      w_pbox(win, 0, 0, winw, winh);
      curx = 0;
      cury = 0;
      break;

    case 'H':/* cursor home */
      curx = 0;
      cury = 0;
      break;

    case 'I':/* reverse index */
      if ((cury -= 1) < 0) {
	cury = 0;
	w_vscroll(win, 0, 0, winw, (row-1)*fonh, fonh);
	w_setmode(win, bgmode);
	w_pbox(win, 0, 0, winw, fonh);
      }
      break;

    case 'J':/* erase to end of page */
      w_setmode(win, bgmode);
      if (cury < row-1)
	w_pbox(win, 0, (cury+1)*fonh, winw, (row-1-cury)*fonh);
      w_pbox(win, curx*fonw, cury*fonh, (col-curx)*fonw, fonh);
      break;

    case 'K':/* erase to end of line */
      w_setmode(win, bgmode);
      w_pbox(win, curx*fonw, cury*fonh, (col-curx)*fonw, fonh);
      break;

    case 'L':/* insert line */
      if (cury < row-1) {
	w_vscroll(win, 0, cury*fonh, winw, (row-1-cury)*fonh, (cury+1)*fonh);
      }
      w_setmode(win, bgmode);
      w_pbox(win, 0, cury*fonh, winw, fonh);
      curx = 0;
      break;

    case 'M':/* delete line */
      if (cury < row-1) {
	w_vscroll(win, 0, (cury+1)*fonh, winw, (row-1-cury)*fonh, cury*fonh);
      }
      w_setmode(win, bgmode);
      w_pbox(win, 0, (row-1)*fonh, winw, fonh);
      curx = 0;
      break;

    case 'Y':/* position cursor */
      escstate = 2;
      break;

    case 'b':/* set foreground color */
      escstate = 4;
      break;

    case 'c':/* set background color */
      escstate = 5;
      break;

    case 'd':/* erase beginning of display */
      w_setmode(win, bgmode);
      if (cury > 0)
	w_pbox(win, 0, 0, winw, cury*fonh);
      if (curx > 0)
	w_pbox(win, 0, cury*fonh, curx*fonw, fonh);
      break;

    case 'e':/* enable cursor */
      curon = 1;
      break;

    case 'f':/* disable cursor */
      curon = 0;
      break;

    case 'j':/* save cursor position */
      savx = curx;
      savy = cury;
      break;

    case 'k':/* restore cursor position */
      curx = savx;
      cury = savy;
      break;

    case 'l':/* erase entire line */
      w_setmode(win, bgmode);
      w_pbox(win, 0, cury*fonh, winw, fonh);
      curx = 0;
      break;

    case 'o':/* erase beginning of line */
      w_setmode(win, bgmode);
      if (curx > 0)
	w_pbox(win, 0, cury*fonh, curx*fonw, fonh);
      break;

    case 'p':/* enter reverse video mode */
      if (bgmode == M_CLEAR) {
        style |= F_REVERSE;
      } else {
        style &= ~F_REVERSE;
      }
      w_settextstyle(win, style);
      break;

    case 'q':/* exit reverse video mode */
      if (bgmode == M_CLEAR) {
        style &= ~F_REVERSE;
      } else {
        style |= F_REVERSE;
      }
      w_settextstyle(win, style);
      break;

    case 'v':/* enable wrap at end of line */
      wrap = 1;
      break;

    case 'w':/* disable wrap at end of line */
      wrap = 0;
      break;

/* and these are the extentions not in VT52 */

    case 'G': /* clear all attributes */
      if (Colors) {
	if (reversed) {
          w_setBackgroundColor(win, color[0]);
	  w_setForegroundColor(win, color[Colors-1]);
	} else {
          w_setBackgroundColor(win, color[Colors-1]);
	  w_setForegroundColor(win, color[0]);
	}
	style = F_NORMAL;
	bgmode = M_CLEAR;
      } else {
        if (reversed) {
	  bgmode = M_DRAW;
	  style = F_REVERSE;
	} else {
          style = F_NORMAL;
	  bgmode = M_CLEAR;
	}
      }
      w_settextstyle(win, style);
      curon = 1; /* also show cursor */
      break;

    case 'g': /* enter bold mode */
      style |= F_BOLD;
      w_settextstyle(win, style);
      break;

    case 'h': /* exit bold mode */
      style &= ~F_BOLD;
      w_settextstyle(win, style);
      break;

    case 'i': /* enter underline mode */
      style |= F_UNDERLINE;
      w_settextstyle(win, style);
      break;

      /* j, k and l are already used */

    case 'm': /* exit underline mode */
      style &= ~F_UNDERLINE;
      w_settextstyle(win, style);
      break;

/* these are in terminfo, but not termcap (it hasn't got codes for these) */

    case 'n': /* enter italic mode */
      style |= F_ITALIC;
      w_settextstyle(win, style);
      break;

      /* o, p and q are already used */

    case 'r': /* exit italic mode */
      style &= ~F_ITALIC;
      w_settextstyle(win, style);
      break;

    case 's': /* enter light mode */
      style |= F_LIGHT;
      w_settextstyle(win, style);
      break;

    case 't': /* exit ligth mode */
      style &= ~F_LIGHT;
      w_settextstyle(win, style);
      break;

    default: /* unknown escape sequence */
      break;
  }
}


/*
 * something to buffer plain text output
 */

static short	sbufcnt = 0;
static short	sbufx, sbufy;
static char	sbuf[SMALLBUFFER+1];

static void sflush (void)
{
  if (sbufcnt) {
    sbuf[sbufcnt] = 0;
    w_printstring(win, sbufx*fonw, sbufy*fonh, sbuf);
    sbufcnt = 0;
  }
}

static void sadd (char c)
{
  if (sbufcnt == SMALLBUFFER)
    sflush ();

  if (!sbufcnt) {
    sbufx = curx;
    sbufy = cury;
  }

  sbuf[sbufcnt++] = c;
}


/*
 * un-escaped character print routine
 */

static void esc0 (uchar c)
{
  switch (c) {
    case 0:
      /*
       * printing \000 on a terminal means "do nothing".
       * But since we use \000 as string terminator none
       * of the characters that follow were printed.
       *
       * perl -e 'printf("a%ca", 0);'
       *
       * said 'a' in a wterm, but should say 'aa'. This
       * bug screwed up most ncurses programs.
       *
       * kay.
       */
    break;
 
  case 7: /* bell */
      if (visualbell) {
	w_setmode(win, M_INVERS);
	w_pbox(win, 0, 0, winw, winh);
	w_test(win, 0, 0);
	w_pbox(win, 0, 0, winw, winh);
      } else {
	w_beep();
      }
      break;

    case 8: /* backspace */
      sflush();
      if (--curx < 0) {
	curx = 0;
      }
      break;

    case 9: /* tab */
      sflush();
      if ((curx = ((curx >> 3) + 1) << 3) >= col) {
	curx = col - 1;
      }
      break;

    case 10: /* line feed */
      sflush();
      if (++cury >= row) {
	w_vscroll(win, 0, fonh, winw, (row-1)*fonh, 0);
	w_setmode(win, bgmode);
	w_pbox(win, 0, (row-1)*fonh, winw, fonh);
	cury = row-1;
      }
      break;

    case 13: /* carriage return */
      sflush();
      curx = 0;
      break;

    case 27: /* escape */
      sflush();
      escstate = 1;
      break;

    case 127: /* delete */
      break;

    default: /* any printable char */
      sadd(c);
      if (++curx >= col) {
	sflush();
	if (!wrap) {
	  curx = col-1;
	} else {
	  curx = 0;
	  if (++cury >= row) {
	    w_vscroll(win, 0, fonh, winw, (row-1)*fonh, 0);
	    w_setmode(win, bgmode);
	    w_pbox(win, 0, (row-1)*fonh, winw, fonh);
	    cury = row-1;
	  }
	}
      }
  }
}


static void printc(uchar c)
{
  switch(escstate) {
    case 0:
      esc0(c);
      break;

    case 1:
      sflush();
      esc1(c);
      break;

    case 2:
      sflush();
      esc2(c);
      break;

    case 3:
      sflush();
      esc3(c);
      break;

    case 4:
      sflush();
      esc4(c);
      break;

    case 5:
      sflush();
      esc5(c);
      break;

#ifdef XTERM_EMU
    case 10:
      sflush();
      esc10(c);
      break;

    case 20:
      esc20(c);
      break;
#endif /* XTERM_EMU */

    default: escstate = 0;
  }
}


static void init(void)
{
  if (_wserver.sharedcolors >= MAX_COLORS) {
    Colors = _wserver.sharedcolors;
  }
  esc1('G');
  if (reversed || Colors) {
    w_setmode(win, bgmode);
    w_pbox(win, 0, 0, winw, winh);
  }

  curx = savx = 0;
  cury = savy = 0;
  wrap = 1;
  curvis = 0;
  escstate = 0;
  draw_cursor();
}


/*
 * general code...
 */

static void prints(const char *s)
{
  while (*s)
    printc((short)*s++);
}

static void translate_key(long key)
{
  uchar buf[4];
  int len = 2;

  switch(key) {
    case WKEY_F1:
    case WKEY_F2:
    case WKEY_F3:
    case WKEY_F4:
    case WKEY_F5:
    case WKEY_F6:
    case WKEY_F7:
    case WKEY_F8:
    case WKEY_F9:
    case WKEY_F10:
	    buf[len++] = '[';
	    buf[len++] = '@' + WKEY_FN(key);
	    break;
    case WKEY_UP:
	    buf[len++] = 'A';
	    break;
    case WKEY_DOWN:
	    buf[len++] = 'B';
	    break;
    case WKEY_LEFT:
	    buf[len++] = 'D';
	    break;
    case WKEY_RIGHT:
	    buf[len++] = 'C';
	    break;
    case WKEY_PGUP:
	    buf[len++] = '5';
	    buf[len++] = '~';
	    break;
    case WKEY_PGDOWN:
	    buf[len++] = '6';
	    buf[len++] = '~';
	    break;
    case WKEY_HOME:
	    buf[len++] = '1';
	    buf[len++] = '~';
	    break;
    case WKEY_END:
	    buf[len++] = '4';
	    buf[len++] = '~';
	    break;
    case WKEY_INS:
	    buf[len++] = '2';
	    buf[len++] = '~';
	    break;
    case WKEY_DEL:
	    buf[len++] = '3';
	    buf[len++] = '~';
	    break;
  }
  if (len > 2) {
    buf[0] = '\e';
    buf[1] = '[';
    write (pipeh, buf, len);
  }	
}

static void term (void)
{
  uchar buf[LARGEBUFFER];
  long in, l;
  WEVENT *ev;
  fd_set rfd;
  int ready;
  short newCol, newRow;

  while (42) {
    FD_ZERO(&rfd);
    FD_SET(pipeh, &rfd);
    if (console)
      FD_SET(cfh, &rfd);

    /* how about an event? */
    ready = 0;

#ifdef __NetBSD__
    /* NetBSD 1.1 locks up here :(
     */
    while ((ev = w_queryevent(&rfd, NULL, NULL, 500))) {
#else
    if ((ev = w_queryevent(&rfd, NULL, NULL, 500))) {
#endif
      ready++;
      switch (ev->type) {

        case EVENT_GADGET:
	  switch (ev->key) {
	    case GADGET_CLOSE:
	    case GADGET_EXIT:
	      sigquit (-1);
	      break;
	    case GADGET_ICON:
	      iconify ();
	      break;
	  }
	  break;

	case EVENT_KEY:
	  /* needs translation? */
	  if (IS_WKEY(ev->key)) {
	    translate_key(ev->key);
	  } else {
	    *buf = ev->key & 0xff;
	    write (pipeh, buf, 1);
	  }
	  break;

#ifdef XTERM_EMU
	case EVENT_MPRESS:
	  /* xtermemu = 0, no emulation
	   * xtermemu = 1, x10 mode (report only presses)
	   * xtermemu = 2, x11 mode (report also releases)
	   */
	  if (xtermemu && ev->win == win) {
	    int button = ' ';

	    switch (ev->key) {
	      case BUTTON_RIGHT:
		button++;
	      case BUTTON_MID:
		button++;
	      case BUTTON_LEFT:
		sprintf(buf, "\e[M%c%c%c", button,
			'!' + ev->x / fonw, '!' + ev->y / fonh);
		break;
	    }
	    write (pipeh, buf, 6);
	  }
	  break;
#endif /* XTERM_EMU */

	case EVENT_MRELEASE:
	  if (isIconified) {
	    iconify ();
	  } else {
#ifdef XTERM_EMU
	    if (xtermemu > 1) {
	      sprintf(buf, "\e[M#%c%c", '!' + ev->x / fonw, '!' + ev->y / fonh);
	      write (pipeh, buf, 6);
	    }
#endif /* XTERM_EMU */
	  }
	  break;

	case EVENT_RESIZE:
 	  newCol = ev->w / fonw;
 	  newRow = ev->h / fonh;
	  if ((newCol != col) || (newRow != row)) {

	    int pgrp;

	    winw = (winsz.ws_col = col = newCol) * fonw;
	    winh = (winsz.ws_row = row = newRow) * fonh;

	    w_move (win, ev->x, ev->y);
	    w_resize (win, winw, winh);

	    if (ioctl (pipeh, TIOCGPGRP, &pgrp)) {
	      prints ("wterm: can't get process group (TIOCGPGRP)\r\n");
	      break;
	    }

	    if (ioctl (pipeh, TIOCSWINSZ, &winsz)) {
	      prints ("wterm: can't set terminal size (TIOCSWINSZ)\r\n");
	      break;
	    }

	    kill (-pgrp, SIGWINCH);
	  }
	  break;
      }

#ifdef __NetBSD__
      FD_ZERO(&rfd);
      FD_SET(pipeh, &rfd);
      if (console)
	FD_SET(cfh, &rfd);
#endif
    }

    /* is there tty data to be printed? */
    if (FD_ISSET(pipeh, &rfd)) {

      ready++;

#ifdef __NetBSD__
      if ((in = 64)) {
#else
      if (!ioctl(pipeh, FIONREAD, &in)) if (in > 0) {
#endif

	if (in > LARGEBUFFER)
	  in = LARGEBUFFER;

	if ((in = read(pipeh, buf, in)) > 0) {

	  if (curon) {
	    hide_cursor();
	  }

	  for (l=0; l<in; l++) {
	    printc(buf[l]);
	    if (buf[l] == '\n')
	      printc('\r');
	  }
	  sflush();

	  if (curon) {
	    draw_cursor();
	  }

	  if (debug) {
	    char *ptr = buf;
#if 1
	    while (in-- > 0) {
	      printf("wterm: received 0x%02x\n", *ptr++);
	    }
#else
	    long cnt = in;
	    while (cnt--)
	      *ptr &= 0xff;
	    write(dh, buf, in);
#endif
	  }
	}
      }
    }

#ifdef __MINT__
    /* in any case: watch out for console output */
    if (console && FD_ISSET(cfh, &rfd)) {

      ready++;
      if ((in = read(cfh, buf, LARGEBUFFER)) > 0) {

	if (curon) {
	  hide_cursor();
	}

	for (l=0; l<in; l++) {
	  if (buf[l] == '\n') {
	    printc('\r');
	  }
	  printc(buf[l]);
	}

	sflush();

	if (curon) {
	  draw_cursor();
	}
      }
    }
#endif

    /* cursor blink timeout */
    if (cblink && curon && !ready) {
      if (curvis) {
	hide_cursor();
      } else {
	draw_cursor();
      }
    }
  }
}


static void usage(char *progname)
{
  fprintf(stderr, "usage: %s [-f <font family>] [-s <font size>] [-g <geometry>]\n", progname);
  fprintf(stderr, "       [-b] [-c] [-d] [-r] [-v] [-- program {args}]\n");
}


static void *mysignal(int signum, void *handler)
{
  struct sigaction sa, so;

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
#if defined(linux)
  /* emulate some BSD signal behaviour (catched signal not blocked while in
   * the handler and make some system calls restartable across signals).
   * Why?  ++eero
   */
  sa.sa_flags = SA_NOMASK | SA_RESTART;
#else
  sa.sa_flags = 0;
#endif

  sigaction(signum, &sa, &so);
  return so.sa_handler;
}


/*
 * this will never return
 */

int main(int argc, char **argv)
{
  WFONT *font;
  short xp, yp, fsize;
  const char *family, *shell = NULL, *cptr, *geometry = NULL;
  struct passwd *pw;
  char buf[80], *ptr;
  short uid, flags;
  WSERVER *wserver;
  char thesh[128];
  int c;

#ifdef __MINT__
  int fd;
#endif

#ifdef SIGTTOU
  /* just in case we're started in the background */
  signal(SIGTTOU, SIG_IGN);
#endif

  /* who am I? */
  if (!(pw = getpwuid((uid = getuid())))) {
    fprintf(stderr, "error: wterm can't determine determine your login name\n");
    exit(-1);
  }

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: wterm can't connect to wserver\n");
    exit(-1);
  }

  /*
   * which font shall I use?
   */

  if (wserver->width > 640) {
    family = "lucidat";
    fsize = 11;
  } else {
    family = "fixed";
    fsize = 7;
  }

  /*
   * scan arguments...
   */

  console = 0;
  while ((c = getopt (argc, argv, "bcdrvf:g:s:")) != EOF) switch(c) {
    case 'b':
      cblink = 1;
      break;

    case 'c':
      console = 1;
      break;

    case 'd':
      debug = 1;
      break;

    case 'r':
      reversed = 1;
      break;

    case 'v':
      visualbell = 1;
      break;

    case 'f':
      family = optarg;
      break;

    case 's':
      fsize = atoi(optarg);
      break;

    case 'g':
      geometry = optarg;
      break;

    default:
      usage(argv[0]);
      return 1;
  }
  argv += optind;

  /*
   * now *argv either points to a program to start or is zero
   */

  if (*argv) {
    shell = *argv;
  }
  if (!shell) {
    shell = getenv("SHELL=");
  }
  if (!shell) {
    shell = pw->pw_shell;
  }
  if (!shell) {
    shell = "/bin/sh";
  }

  if (!*argv) {
    /*
     * the '-' makes the shell think it is a login shell,
     * we leave argv[0] alone if it isn`t a shell (ie.
     * the user specified the program to run as an argument
     * to wterm.
     */
    cptr = strrchr(shell, '/');
    sprintf (thesh, "-%s", cptr ? cptr + 1 : shell);
    *--argv = thesh;
  }

  if (debug) {
    w_trace(1);
  }

  iconSetup(family, fsize+2);

  /* create and open window */

  if (!(font = w_loadfont(family, fsize, 0))) {
    fprintf(stderr, "wterm: can't load font %s with size %d\n", family, fsize);
    exit(-1);
  }

  fonw = font->maxwidth;
  fonh = font->height;

  col = 80;
  row = 25;
  xp = UNDEF;
  yp = UNDEF;
  if (geometry) {
    scan_geometry(geometry, &col, &row, &xp, &yp);
    /* why these were set to one in case of UNDEF? ++eero */
    if (col < 1) {
      col = 80;
    }
    if (row < 1) {
      row = 25;
    }
    if (col > 0x7f)
      colmask = 0xffff;
    if (row > 0x7f)
      rowmask = 0xffff;
  }

  winh = row * fonh;
  winw = col * fonw;

  flags = W_MOVE | W_CLOSE | W_TITLE | W_RESIZE | EV_KEYS | EV_MOUSE;
  if (iconWin) {
    flags |= W_ICON;
  }
  if (!(win = w_create (winw, winh, flags))) {
    fprintf(stderr, "error: wterm can't create window\r\n");
    exit(-1);
  }

  sprintf(buf, "wterm: %s", shell);
  w_settitle(win, buf);

  limit2screen(win, &xp, &yp);

  w_setfont(win, font);
  init();

#ifdef EXPORT_TERMCAP

  /*
   * this one should enable us to get rid of an /etc/termcap entry for
   * both curses and ncurses, hopefully...
   */

  if (termcap_string) {
    sprintf (termcap_string + strlen (termcap_string), "li#%d:co#%d:",
	     row, col);
    if (Colors) {
      /* 
       * add ANSI style color changing definitions for ncurses programs
       * (they'll convert these into terminfo database entry under your
       * $HOME/.terminfo/ directory)
       */
      /* colors, pairs, set fg, set bg, original pair */
      sprintf(termcap_string + strlen (termcap_string),
              "Co#%d:pa#%d:AF#\\Eb%%:AB#\\Ec%%:op#\\Ec0\\Eb%d:",
      	      Colors, Colors * Colors, Colors - 1);
    }
    putenv (termcap_string);
  }

#else

  /*
   * we eliminate any pre-existing (incorrect) TERMCAP string
   */
# ifdef __MINT__
  putenv ("TERMCAP");
# else
  unsetenv ("TERMCAP");
# endif

#endif

  /*
   * what kind of terminal do we want to emulate?
   */
#ifdef __MINT__
  putenv ("TERM=wterm");
#else
  unsetenv ("DISPLAY");   /* be sure to disable X11 use */
  setenv ("TERM", "wterm", 1);
#endif

  /* in case program absolutely needs terminfo entry, these 'should'
   * transmit the screen size correctly (at least xterm sets these
   * and everything seems to work correctly...). Unlike putenv(),
   * setenv() allocates also the given string not just a pointer.
   */
  sprintf (buf, "%d", col);
  setenv ("COLUMNS", buf, 1);
  sprintf (buf, "%d", row);
  setenv ("LINES", buf, 1);

  if (w_open(win, xp, yp) < 0) {
    w_delete(win);
    w_exit();
    exit(-1);
  }

  /*
   * create a pty
   */

  winsz.ws_col = col;
  winsz.ws_row = row;
  if ((pid = forkpty(&pipeh, pty, NULL, &winsz)) < 0) {
    prints("wterm: can't create pty\r\n");
    sleep(2);
    w_unloadfont(font);
    w_close(win);
    w_delete(win);
    exit(-1);
  }

  if ((ptr = rindex(pty, '/'))) {
    strcpy(pty, ptr + 1);
  }
  
  if (!pid) {
    int i;
    for (i = getdtablesize(); --i >= 3; )
      close (i);
    /*
     * SIG_IGN are not reset on exec()
     */
    for (i = NSIG; --i >= 0; )
      signal (i, SIG_DFL);
 
    /* caution: start shell with correct user id! */
    seteuid(getuid());
    setegid(getgid());

    /* this shall not return */
    execvp(shell, argv);

    /* oops? */
    prints("wterm: can't start shell\r\n");
    sleep(3);
    w_unloadfont(font);
    w_close(win);
    w_delete(win);
    _exit(-1);
  }

#ifdef __MINT__
  /*
   * the slave line opened in openpty() may still be our ctty.
   * so loose it.
   */
  fd = open ("/dev/null", O_RDWR);
  dup2 (fd, -1);
  close (fd);
#endif

  _write_utmp(pty, pw->pw_name, "", time(0));

  /* catch some signals */
  mysignal(SIGTERM, sigquit);
  mysignal(SIGHUP, sigquit);
  mysignal(SIGINT, sigquit);
  mysignal(SIGQUIT, sigquit);
  mysignal(SIGPIPE, sigpipe);
  mysignal(SIGCHLD, sigchld);

  /* prepare to catch console output */
  if (console) {

    /* for any OS chr$(7) might cause endless loops if catched from console */
    visualbell = 1;

#ifdef __MINT__
    if ((cfh = open("/dev/xconout2", O_RDONLY)) < 0) {
      prints("wterm: can't catch console output, device busy or not present\r\n");
      console = 0;
    }
#elif defined(sun) || defined(linux)
    console = 0;                     /* data will come to normal pipe handle */
    ioctl(pipeh, TIOCCONS, 0);
#else
    prints("wterm: can't catch console output on this operating system\r\n");
    console = 0;
#endif
  }

  term();

  /* make compiler happy */
  return 0;
}
