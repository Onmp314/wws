/*
 * program/wbuttons.c, part of W
 * (C) 1994,95,96 by Torsten Scherer (TeSche)
 * itschere@techfak.uni-bielefeld.de
 *
 * demo program for Wlib button concept, W_CONTAINER windows and gadget stuff
 */

#include <stdio.h>
#include <Wlib.h>

static WWIN *win, *button[3], *icon;

int main(void)
{
  WEVENT *ev;
  WFONT *font;
  WWIN *evwin;
  short x0, y0;

#if 1
  w_trace (1);
#endif

  if (!w_init()) {
    return -1;
  }

  if (!(font = w_loadfont("fixed", 7, 0))) {
    fprintf(stderr, "can't load font\n");
    return -1;
  }

  if (!(win = w_create(160, 100,
		       W_MOVE | W_TITLE | W_CLOSE | W_ICON | W_CONTAINER))) {
    return -1;
  }
  w_settitle(win, "WButtons");

  if (w_open(win, UNDEF, UNDEF) < 0) {
    w_delete(win);
    return -1;
  }

  if (!(icon = w_create(32, 10, W_MOVE | EV_MOUSE))) {
    return -1;
  }
  w_centerPrints(icon, font, "Icon");

  if (!(button[0]= w_createChild(win, 40, 10, EV_ACTIVE | W_MOVE))) {
    fprintf(stderr, "can't create button #1\n");
    return -1;
  }
  if (!(button[1]= w_createChild(win, 40, 10, EV_ACTIVE | W_MOVE))) {
    fprintf(stderr, "can't create button #2\n");
    return -1;
  }
  if (!(button[2]= w_createButton(win, 100, 0, 40, 10))) {
    fprintf(stderr, "can't create button #3\n");
    return -1;
  }

  w_setfont(button[0], font);
  w_setfont(button[1], font);

  w_printstring(button[0], 0, 0, "CHILD1");
  w_printstring(button[1], 0, 0, "CHILD2");
  w_centerPrints(button[2], font, "BUTTON");

  if (w_open(button[0], 0, 0)) {
    fprintf(stderr, "can't open button #1\n");
    return -1;
  }
  if (w_open(button[1], 50, 0)) {
    fprintf(stderr, "can't open button #2\n");
    return -1;
  }
  if (w_showButton(button[2])) {
    fprintf(stderr, "can't open button #3\n");
    return -1;
  }

  while(42) {
    if ((ev = w_querybuttonevent(NULL, NULL, NULL, -1))) {

      evwin = ev->win;

      switch (ev->type) {

        case EVENT_GADGET:

	  switch (ev->key) {
	    case GADGET_CLOSE:
	    case GADGET_EXIT:
	      printf("exit1\n");
	      w_delete(win);
	      printf("exit2\n");
	      return -1;
	    case GADGET_ICON:
	      printf("iconify\n");
	      w_querywindowpos(win, 1, &x0, &y0);
	      w_close(win);
	      w_open(icon, x0, y0);
	  }
	  break;

	case EVENT_MPRESS:
	  if (evwin == icon) {
	    printf("deiconify\n");
	    w_querywindowpos(icon, 1, &x0, &y0);
	    w_close(icon);
	    w_open(win, x0, y0);
	  }
	  break;

	case EVENT_ACTIVE:
	  printf("activate(0x%08x)\n", (unsigned int)evwin);
	  w_setmode(evwin, M_INVERS);
	  w_line(evwin, 0, 0, evwin->width-1, evwin->height-1);
	  w_line(evwin, 0, evwin->height-1, evwin->width-1, 0);
	  break;

	case EVENT_INACTIVE:
	  printf("inactivate(0x%08x)\n", (unsigned int)evwin);
	  w_setmode(evwin, M_INVERS);
	  w_line(evwin, 0, 0, evwin->width-1, evwin->height-1);
	  w_line(evwin, 0, evwin->height-1, evwin->width-1, 0);
	  break;

	case EVENT_BUTTON:
	  printf("button(0x%08x)\n", (unsigned int)evwin);
      }
    }
  }
}
