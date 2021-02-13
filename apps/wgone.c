/*
 * wgone.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- lock screen behind password while you're away
 *
 * Phx 06/96:
 * - wgone has to be SUID root for NetBSD
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#define __USE_XOPEN	/* glibc crypt() proto needs this */
#include <unistd.h>
#include <pwd.h>
#include <Wlib.h>

#ifdef __MINT__
extern char *crypt(char *, char *);
#endif

/* should really be zero */
#define	BORDER	0


/*
 * guess what...
 */

static WWIN *win;
static short x, y, wd, ready = 0, ptr = 0;
static WEVENT *ev = NULL;
static struct passwd *pw;
static char buf1[64], buf2[64], buf3[64], passwd[9];
static long t;
static struct tm *tm;


int main(int argc, char **argv)
{
  WSERVER *wserver;
  WFONT *font;

  if (!(pw = getpwuid(getuid()))) {
    fprintf(stderr, "fatal: %s can't get your password entry\n", *argv);
    return -1;
  }

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: %s can't connect to wserver\n", *argv);
    return -1;
  }

  if (!(win = w_create(wserver->width-2*BORDER, wserver->height-2*BORDER,
		       W_TOP | EV_KEYS | W_NOBORDER | W_NOMOUSE))) {
    fprintf(stderr, "error: %s can't create window\n", *argv);
    return -1;
  }

  w_setmode(win, M_DRAW);
  font = w_loadfont(NULL, 0, 0);
  w_setfont(win, font);
  w_settextstyle(win, F_REVERSE);
  w_open(win, BORDER, BORDER);
  srandom(time(0));

  sprintf(buf1, "wgone started by: %s", pw->pw_name);
  t = time(0);
  tm = localtime(&t);
  sprintf(buf2, "started : %s", asctime(tm));
  buf2[strlen(buf2)-1] = 0;
  wd = w_strlen(font, buf2);

  do {
    if (!ev) {
      w_pbox(win, 0, 0, wserver->width-2*BORDER, wserver->height-2*BORDER);
      x = random() % (wserver->width - 2*BORDER - wd);
      y = random() % (wserver->height - 2*BORDER - font->height * 3);
      w_printstring(win, x, y, buf1);
      w_printstring(win, x, y+font->height, buf2);
      t = time(0);
      tm = localtime(&t);
      sprintf(buf3, "time now: %s", asctime(tm));
      buf3[strlen(buf3)-1] = 0;
      w_printstring(win, x, y+2*font->height, buf3);
    }

    if ((ev = w_queryevent(NULL, NULL, NULL, 10000))) switch (ev->type) {

      case EVENT_GADGET:
        if (ev->key == GADGET_EXIT) {
	  ready = 1;
	}
	break;

      case EVENT_KEY:
	ev->key &= 0x7f;
	if ((ev->key != 13) && (ptr < 8)) {
	  passwd[ptr++] = ev->key;
	} else {
	  
	  passwd[ptr] = 0;
	  if (!strcmp(pw->pw_passwd, crypt(passwd, pw->pw_passwd))) {
	    ready = 1;
	  } else {
	    w_beep();
	  }
	  ptr = 0;
	}
	break;
      }

  } while (!ready);

  w_delete(win);
  return 0;
}
