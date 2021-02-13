/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * tests the various sorts of buttons.
 *
 * $Id: buttons.c,v 1.2 2004/10/11 18:17:09 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include <Wt.h>

widget_t *shell, *top, *vpane, *vbox, *radio_buttons[4], *switch_button;
widget_t *normal_button;

int
main ()
{
	static char label[10] = "Radio0";
	int i, mode;

	top = wt_init ();
	shell = wt_create (wt_shell_class, top);
	vbox = wt_create (wt_box_class, shell);
	vpane = wt_create (wt_pane_class, vbox);

	mode = OrientVert;
	wt_setopt (vbox,
		WT_ORIENTATION, &mode,
		WT_EOL);

	for (i = 0; i < 4; ++i) {
		label[strlen(label) - 1] = '1' + i;
		radio_buttons[i] = wt_create (wt_radiobutton_class, vpane);
		wt_setopt (radio_buttons[i],
			WT_LABEL, label,
			WT_EOL);
	}

	mode = ButtonStatePressed;
	wt_setopt (radio_buttons[0],
		WT_STATE, &mode,
		WT_EOL);

	switch_button = wt_create (wt_checkbutton_class, vbox);

	wt_setopt (switch_button,
		WT_LABEL, "Switch",
		WT_EOL);

	normal_button = wt_create (wt_pushbutton_class, vbox);

	wt_setopt (normal_button,
		WT_LABEL, "Normal",
		WT_EOL);

	wt_realize (top);
	wt_run ();
	return 0;
}
