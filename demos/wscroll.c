/*
 * programs/wscroll.c, part of W
 * (C) 94-02/96 by Torsten Scherer (TeSche)
 * itschere@techfak.uni-bielefeld.de
 *
 * stupid demo client... :)
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>	/* MiNT has srandom() here... */
#include <stdlib.h>
#include <Wlib.h>


int main()
{
  WWIN *win;
  WEVENT *ev;

  srandom(time(0));

  if (!w_init()) {
    fprintf(stderr, "error: wscroll can't connect to wserver\n");
    return -1;
  }

  if (!(win = w_create(160, 100, W_MOVE))) {
    fprintf(stderr, "error: wscroll can't create window\n");
    return -1;
  }

  if (w_open(win, UNDEF, UNDEF) < 0) {
    fprintf(stderr, "error: wscroll can't create window\n");
    w_delete(win);
    return -1;
  }

  while(42) {

    if ((ev = w_queryevent(NULL, NULL, NULL, 50))) {
      if ((ev->type == EVENT_GADGET) && (ev->key == GADGET_EXIT)) {
	w_delete(win);
	return 0;
      }
    }

    w_vscroll(win, 0, 1, 160, 99, 0);
    w_setmode(win, 0);
    w_hline(win, 0, 99, 159);
    w_setmode(win, 1);
    w_plot(win, random() % 160, 99);
  }
  return 0;
}
