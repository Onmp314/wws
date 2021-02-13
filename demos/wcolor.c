/*
 * programs/wcolor.c, part of W
 * (C) 94-07/96 by Torsten Scherer (TeSche)
 * itschere@techfak.uni-bielefeld.de
 *
 * dumb color demo program
 */

#include <stdio.h>
#include <Wlib.h>


int main (void)
{
  WSERVER *wserver;
  WWIN *window;
  int i, x, y, col[256];

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: can't connect to wserver\n");
    return -1;
  }

  if (wserver->planes < 8) {
    fprintf(stderr, "error: not enough colors on screen (%i)\n",
	    1 << wserver->planes);
    return -1;
  }

  if (!(window = w_create(320, 200, W_MOVE|W_TITLE|W_CLOSE|EV_MOUSE|EV_KEYS))) {
    fprintf(stderr, "error: can't create window\n");
    return -1;
  }

  w_open(window, UNDEF, UNDEF);
  w_setmode(window, M_DRAW);

  w_freeColor(window, 0);
  w_freeColor(window, 1);

  for (i=0; i<256; i++) {
    col[i] = w_allocColor(window, i<<4, i<<4, i<<4);
  }

  for (y=0; y<16; y++) {
    for (x=0; x<16; x++) {
      w_setForegroundColor(window, col[(x + y) & 255]);
      w_plot(window, x, y);
    }
  }

  for (y=0; y<200; y+=16) {
    for (x=0; x<320; x+=16) {
      if (x || y) {
	w_bitblk(window, 0, 0, 16, 16, x, y);
      }
    }
  }

  w_queryevent(NULL, NULL, NULL, -1);

  return 0;
}
