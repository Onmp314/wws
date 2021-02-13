/* test for the thick lines */

#include <stdio.h>
#include <Wlib.h>

int main(void)
{
	WWIN *win;
	WEVENT *ev;
	short x = 100, y = 100, th = 9, ox, oy, oth;

	w_init();

	win = w_create(200, 200, W_TITLE|W_MOVE|W_CLOSE|EV_MOUSE|EV_KEYS);
	w_open(win, UNDEF, UNDEF);

	w_setmode(win, M_INVERS);
	w_setlinewidth(win, th);
	w_box(win, 100, 100, x-100, y-100);
	oth = th;
	ox = x;
	oy = y;

	for(;;) {
		ev = w_queryevent(NULL, NULL, NULL, 0L);
		if (ev) {
			switch (ev->type) {
				case EVENT_MRELEASE:
					if (ev->key == BUTTON_LEFT) {
						th++;
					} else {
						th--;
					}
					break;
				case EVENT_GADGET:
					w_exit();
					return 0;
			}
		}
		w_querymousepos(win, &x, &y);
		if (x != ox || y != oy) {
			if (oth != th) {
				w_setlinewidth(win, oth);
			}
			w_box(win, 100, 100, ox-100, oy-100);
			if (oth != th) {
				w_setlinewidth(win, th);
			}
			w_box(win, 100, 100, x-100, y-100);
			oth = th;
			ox = x;
			oy = y;
		}
	}
	return 0;
}
