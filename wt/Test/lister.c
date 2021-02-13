/* 
 * A test for select widget
 *
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <sys/types.h>
#include <Wlib.h>
#include <Wt.h>

static void ok_cb(widget_t *w, int pressed)
{
	if (pressed)
		return;

	wt_break(1);
}

static void select_cb(widget_t *w, char *str, int idx)
{
	printf("user selected item `%s' with index: %d\n", str, idx);
}

int main()
{
	widget_t *top, *shell, *vpane, *sel, *ok;
	const char *list[] = {
		"one",
		"two",
		"three",
		"four",
		"five",
		"six",
		"seven",
		"eight",
		"nine",
		"ten",
		NULL
	};
	long a, b;

	top   = wt_init();
	shell = wt_create(wt_shell_class, top);
	vpane = wt_create(wt_pane_class, shell);
	sel   = wt_create(wt_select_class, vpane);
	ok    = wt_create(wt_pushbutton_class, vpane);

	wt_setopt(ok, WT_LABEL, "Done", WT_ACTION_CB, ok_cb, WT_EOL);

	a = 100;
	b = 100;
	wt_setopt(vpane,
		WT_WIDTH, &a,
		WT_HEIGHT, &b,
		WT_EOL);

	a = 1;
	b = 5;
	wt_setopt(sel,
		WT_LIST_ONLY, &a,
		WT_LIST_HEIGHT, &b,
		WT_LIST_ADDRESS, list,
		WT_ACTION_CB, select_cb,
		WT_EOL);

	wt_realize(top);
	return wt_run();
}
