/*
 * wsaver.c, a part of the W Window System
 *
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a simple W screen saver
 *
 * CHANGES
 * 2/98, ++eero:
 * - The saver window is created here, drawn black and then exported.
 * - Runs in the root window if delay is less than 10 seconds.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <Wlib.h>
#include "wsaver.h"


struct saver_func {
  void (*func)(void);
  const char *name;
};

#define SAVERS	7

static struct saver_func saver[SAVERS] = {
  { save_ants,   "ants"   },
  { save_bounce, "bounce" },
  { save_cracks, "cracks" },
  { save_lines,  "lines"  },
  { save_pyro,   "pyro"   },
  { save_snails, "snails" },
  { save_stars,  "stars"  }
};


#define TIMEOUT 70	/* milliseconds between updates */

/* exported variables */
long timeout = TIMEOUT;
short swidth, sheight, scolors;
WWIN *win;


static void usage(void)
{
  int i;
  fprintf(stderr, "usage: wsaver <seconds>\n");
  fprintf(stderr, "       where seconds > %d\n\n", MAX(10,SAVERS));
  fprintf(stderr, "If you'll give a smaller number, a saver corresponding\n");
  fprintf(stderr, "to that number will be run in the root window.\n");
  for (i = 0; i < SAVERS; i++) {
    fprintf(stderr, "\t%d: %s\n", i, saver[i].name);
  }
}

int main(int argc, char *argv[])
{
  short seconds;
  WSERVER *wserver;
  WEVENT *ev;

  if (argc != 2) {
    usage();
    return -1;
  }

  seconds = atoi(argv[1]);

  if (!(wserver = w_init())) {
    fprintf(stderr, "%s: can't connect to wserver\n", *argv);
    return -1;
  }

  srandom(time(NULL));
  swidth = wserver->width;
  sheight = wserver->height;
  scolors = wserver->sharedcolors;

  if (seconds > MAX(10,SAVERS)) {
    if (w_setsaver(seconds) < 0) {
      fprintf(stderr, "%s: can't install screensaver\n", *argv);
      fprintf(stderr, "maybe you have already installed one?\n");
      w_exit();
      return -1;
    }
  } else {
    win = WROOT;
    w_setlinewidth(win, 1);		/* just in case... */
    w_setmode(win, M_DRAW);
    w_pbox(win, 0, 0, swidth, sheight);
    /* run wsaver in the root window now! */
    saver[seconds%SAVERS].func();
    w_exit();
    return 0;
  }

  while (42) {

    if ((ev = w_queryevent(NULL, NULL, NULL, -1))) {

      switch (ev->type) {

        case EVENT_GADGET:
	  if (ev->key == GADGET_EXIT) {
	    w_exit();
	    return 0;
	  }
	  break;

	case EVENT_SAVEON:
          win = w_create(swidth, sheight, W_TOP|W_NOBORDER|W_NOMOUSE);
	  if (!win) {
	    fprintf(stderr, "%s: unable to create a W_TOP window\n", *argv);
	    w_exit();
	    return -1;
          }
	  /* savers should run with most of the screen black... */
	  w_pbox(win, 0, 0, swidth, sheight);
	  w_open(win, 0, 0);

	  saver[random()%SAVERS].func();
	  w_delete(win);
	  break;
      }
    }
  }
}
