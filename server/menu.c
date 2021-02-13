/*
 * menu.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- background menu handling
 *
 * ++eero, 3/03:
 * - SDL/GGI redraw hacks
 */

#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef __MINT__
# include <mintbind.h>
#endif
#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"

#ifdef __MINT__
#define NSIG __NSIG
#elif linux
#define NSIG _NSIG
#endif

#define MAXPARTS 8


/*
 * the thing itself
 */

MENU menu;


/*
 * some private stuff...
 */

static REC menurec;
static short menuwidth, menuheight, itemheight, mx, my;
static GCONTEXT menugc = { M_DRAW, F_NORMAL, 1, &glob_font[MENUFONT],
			   NULL, FGCOL_INDEX, BGCOL_INDEX };


static void draw_menu(void)
{
  int i, ox, itemlen[MAXITEMS];
  static int menu_set = 0;

  if (!menu_set) {
    colorGetMask (menugc.fgCol, menugc.fgColMask);
    menu_set = 1;
  }

  menuwidth = 0;
  i = menu.items;
  while (--i >= 0) {
    if ((itemlen[i] = fontStrLen(&glob_font[MENUFONT], menu.item[i].title)) > menuwidth) {
      menuwidth = itemlen[i];
    }
  }
  menuwidth += 12;
  itemheight = glob_font[MENUFONT].hdr.height;
  menuheight = 8 + itemheight * menu.items;

  mx = glob_mouse.real.x0 - menuwidth/2;
  if (mx < 0)
    mx = 0;
  if (mx + menuwidth >= glob_screen->bm.width)
    mx = glob_screen->bm.width - menuwidth;
  my = glob_mouse.real.y0;
  if (my + menuheight >= glob_screen->bm.height)
    my = glob_screen->bm.height - menuheight;

  menurec.x0 = mx;
  menurec.y0 = my;
  menurec.x1 = (menurec.w = menuwidth) - 1;
  menurec.y1 = (menurec.h = menuheight) - 1;
  menurec.next = NULL;
  wtree_traverse_pre(glob_rootwindow, &menurec, window_save_contents_rect);

  menugc.drawmode = M_CLEAR;
  (*glob_screen->pbox)(&glob_screen->bm, mx, my, menuwidth, menuheight);
  menugc.drawmode = M_DRAW;
  (*glob_screen->box)(&glob_screen->bm, mx+1, my+1, menuwidth-2, menuheight-2);
  (*glob_screen->box)(&glob_screen->bm, mx+2, my+2, menuwidth-4, menuheight-4);

  i = menu.items;
  while (--i >= 0) {
    ox = mx + ((menuwidth - itemlen[i]) >> 1);
    (*glob_screen->prints)(&glob_screen->bm, ox, my + 4 + itemheight * i,
			   menu.item[i].title);
  }
}


static void run_command(const char *cmd)
{
  char buf[strlen(cmd)+1], *bufptr;
  char *part[MAXPARTS];
  short parts, pid;
  int i;

  parts = 0;
  bufptr = buf;
  strcpy(buf, cmd);
  while (bufptr) {
    part[parts++] = bufptr;
    if ((bufptr = strchr(bufptr, ' ')))
      *bufptr++ = 0;
  }
  part[parts++] = 0;

  if (!(pid = fork())) {
#ifdef __MINT__
    /* back to normal nice level */
    (void)Prenice(Pgetpid(), 20);
#endif
    i = open ("/dev/null", O_RDWR);
    dup2 (i, 0);
    dup2 (i, 1);
    dup2 (i, 2);
    /* if getdtablesize() missing, use NOFILE define in <sys/param.h> */
    for (i = getdtablesize(); --i >= 2; )
      close (i);
    for (i = NSIG; --i >= 0; )
      mysignal (i, SIG_DFL);
#ifdef __MINT__
    setsid();
#endif
    execvp(part[0], &part[0]);
    _exit(-99);
  }
}


/*
 * what we're going to export...
 */

void menu_domenu(void)
{
  short i, end = 0;
  short oldx, oldy;
  short curitem = -1, olditem = -1;
  const WEVENT *ev;

  mouse_hide();

  glob_clip0 = NULL;
  glob_clip1 = NULL;
  gc0 = &menugc;
  draw_menu();

  menugc.drawmode = M_INVERS;
  oldx = glob_mouse.real.x0;
  oldy = glob_mouse.real.y0;
  if ((oldx >= mx+4) && (oldx < mx+menuwidth-4) &&
      (oldy >= my+4) && (oldy < my+menuheight-4)) {
    curitem = (oldy-my-4) / itemheight;
    (*glob_screen->pbox)(&glob_screen->bm, mx+4, my+4+curitem*itemheight,
			 menuwidth-8, itemheight);
  }
#ifdef SCREEN_REFRESH
  screen_update(&menurec);
#endif

  mouse_show();

  end = 0;
  while (!end) {
    if ((ev = event_mouse())) {
      oldx = glob_mouse.real.x0;
      oldy = glob_mouse.real.y0;

      if (ev->type == EVENT_MMOVE) {
	glob_mouse.real.x0 += ev->x;
	glob_mouse.real.y0 += ev->y;
	if (glob_mouse.real.x0 < 0)
	  glob_mouse.real.x0 = 0;
	if (glob_mouse.real.x0 >= glob_screen->bm.width)
	  glob_mouse.real.x0 = glob_screen->bm.width - 1;
	if (glob_mouse.real.y0 < 0)
	  glob_mouse.real.y0 = 0;
	if (glob_mouse.real.y0 >= glob_screen->bm.height)
	  glob_mouse.real.y0 = glob_screen->bm.height - 1;
      }

      if ((glob_mouse.real.x0 != oldx) || (glob_mouse.real.y0 != oldy)) {
	mouse_hide();

	if ((glob_mouse.real.x0 >= mx+4) && (glob_mouse.real.x0 < mx+menuwidth-4) &&
	    (glob_mouse.real.y0 >= my+4) && (glob_mouse.real.y0 < my+menuheight-4)) {
	  /* in menu */
	  olditem = curitem;
	  curitem = (glob_mouse.real.y0-my-4) / itemheight;
	  if (curitem != olditem) {
	    if (olditem != -1)
	      (*glob_screen->pbox)(&glob_screen->bm, mx+4,
				   my+4+olditem*itemheight, menuwidth-8,
				   itemheight);
	    (*glob_screen->pbox)(&glob_screen->bm, mx+4,
				 my+4+curitem*itemheight, menuwidth-8,
				 itemheight);
	  }
	} else {
	  /* out of menu */
	  if (curitem != -1) {
	    (*glob_screen->pbox)(&glob_screen->bm, mx+4,
				 my+4+curitem*itemheight, menuwidth-8,
				 itemheight);
	    curitem = -1;
	  }
	}

	mouse_show();
      }

      if ((ev->type == EVENT_MPRESS) && (ev->reserved[1] & BUTTON_LEFT)) {
	end = 1;
      }
#ifdef SCREEN_REFRESH
      screen_update(&menurec);
#endif
    }
  }

  mouse_hide();

  if (curitem != -1) {
    struct timeval	tv;
    i = 6;
    while (--i >= 0) {
      (*glob_screen->pbox)(&glob_screen->bm, mx+4, my+4+curitem*itemheight,
			   menuwidth-8, itemheight);
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
      select(0, NULL, NULL, NULL, &tv);
    }
  }

  wtree_traverse_pre(glob_rootwindow, &menurec, window_redraw_contents_rect);

  mouse_show();

  /* now we've got the selected item */

  if (curitem != -1) {
    /* uk: check if the command is a reserved word, beginning with '@';
     *     in this case it is an internal command, so do the thing directly
     * TeSche: '@refresh' will only work when compiled with -DREFRESH
     */
    if (menu.item[curitem].command[0] != '@') {
      /* ok, this is an external command, so run it */
      run_command(menu.item[curitem].command);
    } else {
      if (!strcmp(menu.item[curitem].command, "@exit"))
	terminate(0, "");   /* how to end W? */
#ifdef REFRESH
      else if (!strcmp(menu.item[curitem].command, "@refresh")) {
	wtree_traverse_pre(glob_rootwindow, &glob_rootwindow->pos, window_redraw_contents_rect);
      }
#endif
      /* else .... install here some other things you might want */
    }
  }
}
