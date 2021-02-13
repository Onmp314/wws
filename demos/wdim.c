/* this demonstrates the new F_LIGHT text effect, which uses the
 * current window pattern...
 *
 * (w) 1998 by Eero Tamminen
 */

#include <stdio.h>
#include <Wlib.h>

#define PROPERTIES	(W_MOVE|W_TITLE|W_CLOSE)

int main(void)
{
	const char *name = "W-server";
	short x, y, cmp, add, id = 1;
	WFONT *font;
	WWIN *win;

	w_init();
	if (!(font = w_loadfont("lucidat", 27, 0))) {
		return -1;
	}
	x = font->maxwidth;
	y = font->height;

	if (!(win = w_create(2*x + w_strlen(font, name), 3*y, PROPERTIES))) {
		return -1;
	}

	w_setfont(win, font);
	w_settextstyle(win, F_LIGHT|F_ITALIC|F_BOLD|F_REVERSE);
	w_pbox(win, 0, 0, win->width, win->height);
	w_open(win, UNDEF, UNDEF);

	for (;;) {
		if (id) {
			cmp = 0;
			add = -1;
			id = MAX_GRAYSCALES;
		} else {
			cmp = MAX_GRAYSCALES;
			add = 1;
			id = 0;
		}
		while ((id += add) != cmp) {
			w_setpattern(win, id);
			w_printstring(win, x, y, name);
			if (w_queryevent(NULL, NULL, NULL, 40)) {
				w_exit();
				return 0;
			}
		}
	}
	return 0;
}
