/*
 * Draw a circle through three points (from comp.graphics.algorithms FAQ)
 *
 * (w) 1998 by Eero Tamminen
 */

#include <stdio.h>
#include <Wlib.h>

#define X_SIZE	5

typedef struct {
	short x, y;
} Point;


static void draw_x(WWIN *win, short x, short y)
{
	w_line(win, x-X_SIZE, y-X_SIZE, x+X_SIZE, y+X_SIZE);
	w_line(win, x-X_SIZE, y+X_SIZE, x+X_SIZE, y-X_SIZE);
}

static void draw_circle(WWIN *win, Point *point)
{
	int A, B, C, D, E, F, G, x, y, r, i;

	for(i = 0; i < 3; i++) {
		draw_x(win, point[i].x, point[i].y);
	}

	A = point[1].x - point[0].x;
	B = point[1].y - point[0].y;
	C = point[2].x - point[0].x;
	D = point[2].y - point[0].y;

	E = A*(point[0].x + point[1].x) + B*(point[0].y + point[1].y);
	F = C*(point[0].x + point[2].x) + D*(point[0].y + point[2].y);

	G = 2.0 * (A*(point[2].y - point[1].y) - B*(point[2].x - point[1].x));
	if (!G) {
		fprintf(stderr, "Points collinear -> infinite circle!\n");
		return;
	} 

	x = (D*E - B*F) / G;
	y = (A*F - C*E) / G;

	A = point[0].x - x;
	B = point[0].y - y;
	r = isqrt(A*A + B*B);

	w_circle(win, x, y, r);
}

int main()
{
	Point point[3];
	int points;
	WEVENT *ev;
	WWIN *win;

	w_init();
	if (!(win = w_create(200, 200, W_TITLE|W_MOVE|W_CLOSE|EV_MOUSE))) {
		w_exit();
		return -1;
	}

	w_setmode(win, M_INVERS);
	w_open(win, UNDEF, UNDEF);

	points = 0;
	for (;;) {
		ev = w_queryevent(NULL, NULL, NULL, -1L);
		switch(ev->type) {
			case EVENT_GADGET:
				w_exit();
				return 0;

			case EVENT_MPRESS:
				draw_x(win, ev->x, ev->y);
				point[points].x = ev->x;
				point[points].y = ev->y;
				break;

			case EVENT_MRELEASE:
				if (++points == 3) {
					draw_circle(win, point);
					points = 0;
				}
				break;
		}
	}
	return 0;
}
