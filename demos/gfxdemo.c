/*
 * Program for testing out ellipse and bezier functions...
 *
 * Note that beziers won't work too well in inverse mode as the used
 * line bits overlap and therefore there will be some gaps :-(.
 *
 * (w) by Eero Tamminen 10/96
 */

#include <stdio.h>
#include <Wlib.h>

/* ++TeSche: there is no such macro outside MiNT */
#define swap(a,b) \
{ \
    int help = a; \
    a = b; \
    b = help; \
}

#define WIDTH 400
#define HEIGHT 200


static WWIN *getwindow(short width, short height)
{
  WSERVER *wserver;
  WWIN *win;

  if (!(wserver = w_init()))
    return NULL;

  if (!(win = w_create(width, height, W_MOVE | W_TITLE | W_CLOSE | EV_MOUSE)))
    return NULL;

  w_settitle(win, " gfxdemo ");
  if (w_open(win, UNDEF, UNDEF) < 0)
  {
    w_delete(win);
    return NULL;
  }
  return win;
}


int main(void)
{
  WWIN *win;
  WEVENT *ev;
  short x = WIDTH/2, y=HEIGHT/2, rx, ry;
  short coord[8];

  if (!(win = getwindow(WIDTH, HEIGHT)))
  {
    fprintf(stderr, "error: can't open window\n");
    return -1;
  }
  w_setmode(win, M_DRAW);

  for(;;)
  {
    ev = w_queryevent(NULL, NULL, NULL, -1);
    switch(ev->type)
    {
      case EVENT_MPRESS:
        x = ev->x;
	y = ev->y;
	break;

      case EVENT_MRELEASE:
        rx = ev->x;
	ry = ev->y;
	if(rx < x)
	  swap(x,rx);
	if(ry < y)
	  swap(y,ry);
	rx -= x;
	ry -= y;

	/* something like the yin/yang symbol */
	w_ellipse(win, x, y, rx, ry);
	w_pellipse(win, x + rx/6, y - ry/2, rx/6, ry/6);
	w_ellipse(win, x - rx/6, y + ry/2, rx/6, ry/6);

	/* two half ellipses */
	coord[0] = x;
	coord[1] = y - ry;
	coord[2] = x + rx;
	coord[3] = y - ry;
	coord[4] = x + rx;
	coord[5] = y;
	coord[6] = x;
	coord[7] = y;
	w_bezier(win, coord);
	coord[0] = x;
	coord[1] = y;
	coord[2] = x - rx;
	coord[3] = y;
	coord[4] = x - rx;
	coord[5] = y + ry;
	coord[6] = x;
	coord[7] = y + ry;
	w_bezier(win, coord);
	break;

      case EVENT_GADGET:
	if(ev->key == GADGET_EXIT || ev->key == GADGET_CLOSE)
	  return 0;
    }
  }
}
