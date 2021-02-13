/*
 * file for testing the server arc and pie functions.
 */

#include <stdio.h>
#include <Wlib.h>

int main()
{
#define AA	60
#define BB	30
	WWIN *win;
	WEVENT *ev;
	short x, y, ox, oy;
	double a, b, oa, ob;

	w_init();
	win = w_create(200, 200, W_MOVE | EV_MOUSE | EV_KEYS);
	w_setmode(win, M_INVERS);
	w_setlinewidth(win, 50);
	w_open(win, UNDEF, UNDEF);

	x = 100;
	y = 100;
	a = AA;
	b = BB;
	w_arc(win, 100, 100, x, y, a, b);
	ox = x;
	oy = y;
	oa = a;
	ob = b;

	for(;;) {
		ev = w_queryevent(NULL, NULL, NULL, 0L);
		if (ev && ev->type != EVENT_MRELEASE) {
			break;
		}
		w_querymousepos(win, &x, &y);
		if (x < 100) {
			x = 100 - x;
			a = AA + 90;
			b = BB + 90;
			if (y > 100) {
				a += 90;
				b += 90;
			}
		} else {
			x -= 100;
			a = AA;
			b = BB;
			if (y > 100) {
				a += 270;
				b += 270;
			}
		}
		if (y < 100) {
			y = 100 - y;
		} else {
			y -= 100;
		}
		if (x > 500 || y > 500) {
			printf("error x/y: %d/%d\n", x, y);
			continue;
		}
		if (x != ox || y != oy || a != oa || b != ob) {
			w_arc(win, 100, 100, ox, oy, oa, ob);
			w_arc(win, 100, 100, x, y, a, b);
			ox = x;
			oy = y;
			oa = a;
			ob = b;
		}
	}

	w_exit();
	return 0;
}
