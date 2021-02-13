/*
 * programs/wresize.c, part of W
 * (C) 1994,95,96 by Torsten Scherer (TeSche)
 * itschere@techfak.uni-bielefeld.de
 *
 * 11/96 ++TeSche: small demo program for resizeable windows
 */

#include <stdio.h>
#include <Wlib.h>


/*
 * guess what...
 */

int main (int argc, char *argv[])
{
  WSERVER *server;
  WWIN *win;
  short width = 160, height = 100, x, y;
  WEVENT *ev;

  if (!(server = w_init ())) {
    fprintf (stderr, "error: wlines can't open wserver pipe\n");
    return -1;
  }

  if (!(win = w_create (width, height, W_MOVE | W_RESIZE))) {
    fprintf (stderr, "error: wlines can't create window\n");
    return -1;
  }

  if ((w_open (win, UNDEF, UNDEF)) < 0) {
    fprintf (stderr, "error: wlines can't open window\n");
    return -1;
  }

  while(42) {

    w_setmode (win, M_CLEAR);
    w_pbox (win, 0, 0, width, height);
    w_setmode (win, M_INVERS);

    for (x=0; x<width; x++)
      w_line (win, x, 0, width-x-1, height-1);
    for (y=0; y<height; y++)
      w_line (win, 0, y, width-1, height-y-1);

    if ((ev = w_queryevent (NULL, NULL, NULL, -1))) {

      switch (ev->type) {

        case EVENT_GADGET:
	  w_delete (win);
	  return 0;

	case EVENT_RESIZE:
	  w_move (win, ev->x, ev->y);
	  w_resize (win, ev->w, ev->h);
	  width = ev->w;
	  height = ev->h;
	  break;
      }
    }
  }
}
