/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Eero Tamminen
 *
 * tests the (label) button widget image capability.
 * shows how the widgets could be moved around with mouse.
 */
#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>
#include <Wt.h>

static unsigned char tower_bits[128] = 
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x7f, 0xff, 0xff, 0xfe, 0x55, 0x55, 0x55, 0x54,
  0x7f, 0x5b, 0xff, 0xfe, 0x2a, 0xfc, 0xaa, 0xaa,
  0x7b, 0x87, 0x7f, 0xfe, 0x56, 0x01, 0xd5, 0x54,
  0x74, 0x00, 0xff, 0xfe, 0x2c, 0x00, 0x6a, 0xaa,
  0x68, 0x00, 0x7f, 0xfe, 0x58, 0x00, 0x09, 0x54,
  0x68, 0x39, 0xde, 0xfe, 0x2c, 0x29, 0x5a, 0xaa,
  0x74, 0x2f, 0x7e, 0xfe, 0x56, 0x2a, 0xab, 0x54,
  0x7b, 0xb5, 0x56, 0xfe, 0x2a, 0xaa, 0xae, 0xaa,
  0x7f, 0xb5, 0x7e, 0xfe, 0x55, 0x3d, 0xff, 0x54,
  0x7f, 0xdd, 0x7d, 0xfe, 0x2a, 0xaa, 0xba, 0xaa,
  0x7f, 0xed, 0xfb, 0xfe, 0x55, 0x5a, 0xbd, 0x54,
  0x7f, 0xed, 0xfb, 0xfe, 0x2a, 0xaa, 0xfa, 0xaa,
  0x7f, 0xed, 0xfb, 0xfe, 0x55, 0x5b, 0xfd, 0x54,
  0x7f, 0xef, 0xfb, 0xfe, 0x2a, 0xaf, 0xfa, 0xaa,
  0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00
};

static BITMAP def_bm = { 32, 32, 1, 4, 1, 1, tower_bits };

static widget_t *label;
static short px, py;
long timer;


static void draw_bm_cb (widget_t *w, short x, short y, BITMAP *bm)
{
  w_putblock (bm, wt_widget2win(w), x, y);
}

/* put the widget into a new position */
static void timer_cb (long _w)
{
	widget_t *w = (widget_t *)_w;
	short x, y;

	w_querymousepos (wt_widget2win(w), &x, &y);

        if (x != px || y != py) {
		long ox, oy;
		wt_getopt (w, WT_XPOS, &ox, WT_YPOS, &oy, WT_EOL);
		wt_reshape (w, ox + x - px, oy + y - py, WT_UNSPEC, WT_UNSPEC);
	}
	timer = wt_addtimeout (200, timer_cb, (long)w);
}

/* on mouse press invert widget, and on release de-invert the widget.
 * meanwhile the timer takes care that the widget moves with the mouse
 * (if the mouse press was done with the left button).
 */
static void event_cb (widget_t *w, WEVENT *ev)
{
	long a;

	switch (ev->type) {
	case EVENT_MPRESS:
		a = ButtonStatePressed;
		wt_setopt (w, WT_STATE, &a, WT_EOL);
		if (ev->key == BUTTON_LEFT) {
			w_querymousepos (wt_widget2win(w), &px, &py);
			timer = wt_addtimeout (200, timer_cb, (long)w);
		}
		break;

	case EVENT_MRELEASE:
		if (ev->key == BUTTON_LEFT) {
			wt_deltimeout (timer);
		}
		a = ButtonStateReleased;
		wt_setopt (w, WT_STATE, &a, WT_EOL);
		break;
	}
}

/* read the image specified on the command line or if that fails,
 * use the default icon.
 */
int main (int argc, char *argv[])
{
	widget_t *top, *shell, *pane;
	long x, y, w, h;
	BITMAP *bm;
	short d;

	if (argc != 2 || !(bm = w_readimg(argv[1], &d, &d)))
	{
	  fprintf(stderr, "reading image '%s' failed\n", argv[1]);
	  bm = &def_bm;
	}

	top = wt_init ();
	shell = wt_create (wt_shell_class, top);
	pane  = wt_create (wt_pane_class, shell);
	label = wt_create (wt_label_class, pane);

	x = EV_MOUSE;
	wt_setopt (label,
		WT_LABEL, "Test Image",
		WT_EVENT_MASK, &x,
		WT_EVENT_CB, event_cb,
		WT_DRAW_FN, draw_bm_cb,
		WT_ICON, bm,
		WT_EOL);

	wt_geometry (label, &x, &y, &w, &h);
	w *= 4; h *= 4;
	wt_setopt (pane, WT_WIDTH, &w, WT_HEIGHT, &h, WT_EOL);

	wt_setopt (shell, WT_LABEL, "Move the Image!", WT_EOL);

	wt_realize (top);
	wt_run ();
	return 0;
}
