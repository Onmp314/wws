/*
 * wload.c, a part of the W Window System
 *
 * Copyright (C) 1994 by Benjamin Lorenz and
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a system 'load' meter
 *
 * CHANGES:
 * benni 9/94:
 * - Version 1.0
 * ++tesche 95-96:
 * - small bugfix: scale lines were drawn
 *   one pixel too long, fixing this involved more changes in
 *   when to bitblk and so on... :(
 * - okok, another small bugfix, as suggested by Benni
 * - changed for W1R0
 * - added support for Linux68k
 * - major rewrite (-95)
 * - YAABF (-96) :)
 * ++Phx 6/96:
 * - added support for NetBSD
 * ++eero 97-98:
 * - hostname can be > 16 chars, fixed
 * - long options and --parent option
 * - simplified a bit (lines are clipped by server anyway now :))
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#ifdef __NetBSD__
#include <sys/types.h>
#include <sys/param.h>
#include <amiga/vmparam.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#include <vm/vm_param.h>
#endif
#include <Wlib.h>

#define LFRACTION 2048


static struct option long_options [] = {
  { "geometry", required_argument, NULL, 'g' },
  { "help", no_argument, NULL, 'h' },
  { "parent", required_argument, NULL, 'p'},
  { "seconds", required_argument, NULL, 'u'},
  { NULL, 0, NULL, 0}
};


#ifdef __MINT__

#include <macros.h>
#include <support.h>
#include <mintbind.h>

#elif linux


#define max(a,b) (((a) > (b)) ? (a) : (b))

static void Suptime(ulong *uptime, ulong *load)
{
  static FILE *fp = NULL;
  float f1, f2, f3;

  /* sorry, can't make this static 'cause Linux can't rewind this file
   */
  if (!(fp = fopen("/proc/loadavg", "r"))) {
    fprintf(stderr, "error: can't open /proc/loadavg\n");
    exit(-1);
  }

  fscanf(fp, "%f %f %f\n", &f1, &f2, &f3);
  fclose (fp);

  *uptime = 0; /* we don't need that */
  load[0] = f1 * LFRACTION;
  load[1] = f2 * LFRACTION;
  load[2] = f3 * LFRACTION;
}


#elif __NetBSD__

void Suptime(ulong *uptime, ulong *load)
{
  struct loadavg lavg;
  int mib[2];
  size_t len = sizeof(struct loadavg);
  ulong fscale;
  
  mib[0] = CTL_VM;
  mib[1] = VM_LOADAVG;
  sysctl(mib, 2, &lavg, &len, NULL, 0);  /* fill loadavg structure */
  fscale = (ulong)lavg.fscale;
  *uptime = 0;  /* as for linux ... we don't need that neither? */
  load[0] = ((ulong)lavg.ldavg[0] * LFRACTION) / fscale;
  load[1] = ((ulong)lavg.ldavg[1] * LFRACTION) / fscale;
  load[2] = ((ulong)lavg.ldavg[2] * LFRACTION) / fscale;
}


#else

void Suptime(ulong *uptime, ulong *load)
{
  *uptime = 1;
  load[0] = 1;
  load[1] = 1;
  load[2] = 1;
}

#endif


/*
 * global variables
 */

#define WIN_W	80
#define WIN_H	128

static short winw;
static short winh;
static short header;

static WWIN *win;
static int *loads;


/*
 *
 */

static void w_clear()
{
  w_setmode(win, M_CLEAR);
  w_pbox(win, 0, header, winw, winh - header);
}


static void print_load(int l)
{
  static int x = 0, lceil_old = 1;
  int i, lceil;

  /* insert new load value */
  if (x == winw) {
    for (i=0; i<winw-1; i++) {
      loads[i] = loads[i+1];
    }
    loads[winw - 1] = l;
  } else {
    loads[x] = l;
  }

  /* calc new ceil() value of loads */
  lceil = 0;
  for (i=0; i<winw; i++) {
    if (loads[i] > lceil) {
      lceil = loads[i];
    }
  }
  if ((lceil = (lceil + LFRACTION - 1) / LFRACTION) == 0) {
    lceil = 1;
  }

  if (lceil != lceil_old) {

    /* completely redraw window */
    w_clear();
    w_setmode(win, M_DRAW);
    for (i=0; i<winw; i++) {
      w_vline(win, i, winh - 1, winh - (loads[i] * (winh - header)) / (lceil * LFRACTION));
    }
    w_setmode(win, M_INVERS);
    for (i=1; i<lceil; i++) {
      w_hline(win, 0, winh - (i * (winh - header)) / lceil, winw-1);
    }

    if (x == winw) {
      x--;
    }

  } else {

    if (x >= winw) {
      w_bitblk(win, 1, header, winw-1, winh-header, 0, header);
      x--;
    }

    w_setmode(win, M_CLEAR);
    w_vline(win, x, header, winh-1);
    w_setmode(win, M_DRAW);
    w_vline(win, x, winh - 1, winh  - (loads[x] * (winh - header)) / (lceil * LFRACTION));
    w_setmode(win, M_INVERS);
    for (i=1; i<lceil; i++) {
      w_plot(win, x, winh - (i * (winh - header)) / lceil);
    }
  }

  lceil_old = lceil;
  x++;
}


static void usage (char *name) {
  fprintf (stderr, "\
Usage: %s [OPTION]...\n\
Shows the machine load.\n\
\n\
  -g, --geometry             set size and position\n\
  -u, --seconds              seconds between updates\n\
  -h, --help                 display this help and exit\n\
\n\
", name);
}


int main(int argc, char **argv)
{
  long u = 10000;
  int i, l, c, show_usage = 0;
  ulong parentid = 0;
  ulong uptime;
  ulong load[3];
  ulong avg[3];
  char hostname[32];
  WFONT *font;
  WEVENT *ev;
  short xp = UNDEF;
  short yp = UNDEF;
  WSERVER *wserver;

  while ((c = getopt_long (argc, argv, "g:u:p:h", long_options, NULL)) != -1) {
    switch (c) {

      case 'g':
        scan_geometry (optarg, &winw, &winh, &xp, &yp);
        break;

      case 'u':
	if ((u = atoi(optarg)) < 1) {
	  u = 1;
	}
	if (u > 60) {
	  u = 60;
	}
	u *= 1000;
	break;

      case 'p':
        sscanf (optarg, "%lx", &parentid);
        break;

      default:
        show_usage = 1;
        break;
    }
  }

  if (show_usage) {
    usage (argv[0]);
    exit (0);
  }

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: can't connect to wserver\n");
    exit(1);
  }
  limit2screen(win, &xp, &yp);

  if (parentid) {
    WWIN *parent;
    parent = w_winFromID(parentid);
    winw = parent->width;
    winh = parent->height;
    win = w_createChild (parent, winw, winh, W_NOBORDER);
    xp = yp = 0;
  } else {
    if (winw <= 0) {
      winw = WIN_W;
    }
    if (winh <= 0) {
      winh = WIN_H;
    }
    win = w_create(winw, winh, W_MOVE);
  }
  if (!win) {
    fprintf(stderr, "error: can't create window\n");
    exit(1);
  }

  if (w_open(win, xp, yp)) {
    fprintf(stderr, "error: can't open window\n");
    w_exit();
    exit(1);
  }

  gethostname(hostname, sizeof(hostname)-1);

  if (!(loads = malloc(sizeof(*loads) * winw))) {
    fprintf(stderr, "error: can't open window\n");
    w_exit();
    exit(1);
  }
  for (i = 0; i < winw; i++)
    loads[i] = 0;
  for (i = 0; i < 3; i++)
    avg[i]=0;

  w_setmode(win, M_DRAW);
  if (!(font = w_loadfont("cour", 0, F_BOLD))) {
    font = w_loadfont(NULL, 0, 0);
  }
  w_setfont(win, font);
  w_printstring(win, 1, 1, hostname);
  w_hline(win, 0, winh - 1, winw - 1);
  header = font->height;

  while (1) {
    Suptime(&uptime, load);
    avg[0] = avg[1];
    avg[1] = avg[2];
    avg[2] = load[0];

    /* calc load in 1/LFRACTION fractions  */
    l = (avg[0] + avg[1] + avg[2]) / 3;
    print_load(l);

    /* watch out for `exit' event or 'q' key */
    if ((ev = w_queryevent(NULL, NULL, NULL, u))) {
      if ((ev->type == EVENT_KEY && ev->key == 'q') ||
          (ev->type == EVENT_GADGET && ev->key == GADGET_EXIT)) {
	w_exit();
	exit(0);
      }
    }
  }
}
