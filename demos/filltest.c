/* test for the w_fill() function in Wlib (*really* slow) */

#include <stdio.h>
#include <Wlib.h>

int main()
{
	WWIN *win;
	WEVENT *ev;

	w_init();
	win = w_create(100, 100, W_MOVE | EV_MOUSE | EV_KEYS);

	w_setmode(win, M_DRAW);
	w_circle(win, 50, 50, 45);
	w_pcircle(win, 50, 50, 41);

	w_setmode(win, M_CLEAR);
	w_pcircle(win, 50, 50, 30);
	w_pellipse(win, 50, 50, 44, 20);
	w_pellipse(win, 50, 50, 20, 44);

	w_setmode(win, M_DRAW);
	w_pcircle(win, 50, 50, 28);

	w_setmode(win, M_CLEAR);
	w_pbox(win, 30, 30, 41, 41);
	w_pbox(win, 0, 50, 100, 4);

	/* check that only necessary pixels are modified */
	w_setmode(win, M_INVERS);
	w_pbox(win, 0, 20, 100, 4);
	w_pbox(win, 0, 80, 100, 4);

	w_open(win, UNDEF, UNDEF);
	for(;;) {
		ev = w_queryevent(NULL, NULL, NULL, -1L);
		if (ev->type == EVENT_GADGET) {
			break;
		}
		if (ev->type == EVENT_MRELEASE) {
			w_fill(win, ev->x, ev->y);
		}
	}
	w_exit();
	return 0;
}
