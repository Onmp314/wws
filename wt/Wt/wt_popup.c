/* 
 * this implements showing of popups which can be called either mouse button
 * down or up, and which will vanish when the button is released.
 * 
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

void
wt_drawbm_fn(widget_t *w, short x, short y, BITMAP *bm)
{
	WWIN *win = wt_widget2win(w);
	w_putblock(bm, win, x, y);
}

static void
kill_popup_cb(widget_t *pop)
{
	wt_menu_item *item;
	(*pop->class->getopt) (pop, WT_LIST_ADDRESS, &item);
	while (item->type != MenuEnd) {
		item++;
	}
	if ((widget_t *)item->info != pop) {
		/* better report these... */
		fprintf(stderr, "wt_popup: last menu item->info doesn't match with popup pointer\n");
	}
	item->info = NULL;
	wt_delete(pop);
}

/* creates and opens a `popup' into given parent relative position and
 * stores the popup pointer to the given menu structure.
 */
static widget_t *
create_popup(widget_t *parent, wt_menu_item *menu)
{
	widget_t *pop;
	long wd, ht;
	short x, y;

	if (!(pop = wt_create(wt_popup_class, parent))) {
		return NULL;
	}
	wd = PopupTransient;
	ht = AlignLeft;
	wt_setopt(pop,
		WT_STATE, &wd,
		WT_LIST_ADDRESS, menu,
		WT_ACTION_CB, kill_popup_cb,
		WT_DRAW_FN, wt_drawbm_fn,
		WT_ALIGNMENT, &ht,
		WT_EOL);

	w_querymousepos(WROOT, &x, &y);
	wt_minsize(pop, &wd, &ht);
	x -= wd / 2;
	y += 2;		/* slightly under mouse */
	wt_reshape(pop, x, y, WT_UNSPEC, WT_UNSPEC);
	(*pop->class->realize) (pop, WROOT);
	return pop;
}


void wt_popup_cb(widget_t *parent, wt_menu_item *menu, int pressed)
{
	WEVENT ev;
	widget_t *pop;
	wt_menu_item *item;
	long state;

	item = menu;
	while (item->type != MenuEnd) {
		item++;
	}

	/* button just pressed? */
	if (pressed > 0) {
		if (item->info)
			wt_delete((widget_t *)item->info);
		item->info = create_popup (parent, menu);
		return;
	}
	if (!(pop = item->info))
		return;

	/* released on the button? */
	if (!pressed) {
		state = PopupPersistant;
		/* let popup wait EVENT_MPRESS and handle things itself */
		(*pop->class->setopt) (pop, WT_STATE, &state);
	} else {
		ev.key = BUTTON_LEFT;
		ev.type = EVENT_MRELEASE;
		ev.win = wt_widget2win(pop);
		w_querymousepos(ev.win, &ev.x, &ev.y);
		(*pop->class->event) (pop, &ev);
	}
}
