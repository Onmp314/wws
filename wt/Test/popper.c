/* test file for popups
 *
 * This demonstrates how popups can be used with images, with wt_popup_cb(),
 * from button widget or generic event callbacks and with menu widgets.  It
 * also shows how to compose the needed menu structures.
 *
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

static void item_cb(widget_t *w, wt_menu_item *item)
{
	if (item->type == MenuCheck) {
		if (item->sub) {
			printf("checked   ");
		} else {
			printf("unchecked ");
		}
	}
	printf("menu item: `%s' with info: `%s'\n", item->string, (char*)item->info);
}

/* you can store some misc info under submenu member in menu struct
 * as long as you'll remember to put the menu type right
 */
typedef wt_menu_item *info;

static long bullet_data[8] = {
	0x18000000,
	0x7e000000,
	0x7e000000,
	0xff000000,
	0xff000000,
	0x7e000000,
	0x7e000000,
	0x18000000
};
static BITMAP bullet = {
	/* width, height, type, upl, planes, *data */
	8, 8, BM_PACKEDMONO, 4, 1, 1, bullet_data
};

static wt_menu_item more_menu[] = {
	{ "Disabled",       0, "D", 0, 0,       MenuItem },
	{ "Four",     &bullet, "4", 0, item_cb, MenuItem },
	{ "Five",     &bullet, "5", 0, item_cb, MenuItem },
	{ 0,0,0,0,0, MenuEnd }
};
static wt_menu_item popup_menu[] = {
	/* menu item string, item bitmap (icon), info pointer, submenu
	 * pointer / check flag, select callback and finally item type:
	 */
	{ "One",    0, "1",  0, item_cb,   MenuItem },
	{ "Two",    0, "2",  0, item_cb,   MenuItem },
	{ 0,0,0,0,0, MenuItem },	/* separator */
	{ "Three",  0, "3",  0, item_cb,   MenuItem },
	{ "...",    0,   0,  more_menu, 0, MenuSub },
	{ 0,0,0,0,0, MenuEnd }
};

typedef wt_menu_item *check_t;

static wt_menu_item foo_menu[] = {
	{ "Hubba", 0, 0, (check_t)1, item_cb, MenuCheck },
	{ "Zubba", 0, 0, (check_t)0, item_cb, MenuCheck },
	{ 0,0,0,0, MenuEnd }
};
static wt_menu_item root_menu[] = {
	{ "Fool",   0, 0, foo_menu,   0, MenuSub },
	{ "Number", 0, 0, popup_menu, 0, MenuSub },
	{ 0,0,0,0, MenuEnd }
};

static widget_t *Top;


static void button_cb(widget_t *w, int pressed)
{
	/* here we should actually use &popup_menu[0], but at least GCC
	 * converts array into an array pointer all right.
	 */
	wt_popup_cb(Top, popup_menu, pressed);
}

static void event_cb(widget_t *w, WEVENT *ev)
{
	short event, x, y;

	/* similar semantics as in button widget(s) */
	switch(ev->type) {
		case EVENT_MPRESS:
			event = 1;
			break;
		case EVENT_MRELEASE:
			if (w_querymousepos(wt_widget2win(w), &x, &y)) {
				event = -1;	/* outside */
				break;
			}
			event = 0;		/* inside */
			break;
		default:
			return;
	}
	if (ev->key == BUTTON_LEFT)
		wt_popup_cb(Top, popup_menu, event);
}

int main()
{
	widget_t *shell, *pane, *menu, *label, *draw, *test;
	long a, b;

	Top   = wt_init();
	shell = wt_create(wt_shell_class, Top);
	pane  = wt_create(wt_pane_class, shell);
	menu  = wt_create(wt_menu_class, pane);
	draw  = wt_create(wt_drawable_class, pane);
	test  = wt_create(wt_pushbutton_class, pane);

	wt_setopt(shell, WT_LABEL, " Popup Test ", WT_EOL);

	wt_setopt(menu,
		WT_DRAW_FN, wt_drawbm_fn,
		WT_LIST_ADDRESS, &root_menu[0],
		WT_EOL);

	/* if I want this after menu buttons, it has to be create after
	 * setting the menu structure...
	 */
	label = wt_create(wt_label_class, menu);
	a = LabelModeWithBorder;
	wt_setopt(label,
		WT_LABEL, "<- menu",	/* just to be sure it's noticed <g> */
		WT_MODE, &a,
		WT_EOL);

	a = EV_MOUSE;
	b = 120;
	wt_setopt(draw,
		WT_EVENT_CB, event_cb,
		WT_EVENT_MASK, &a,
		WT_WIDTH, &b,
		WT_EOL);

	wt_setopt(test,
		WT_LABEL, "Test above!",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_realize(Top);
	return wt_run();
}
