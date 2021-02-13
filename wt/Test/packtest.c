/*
 * $Id: packtest.c,v 1.1.1.1 1998/11/01 19:15:06 eero Exp $
 */
#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

int
main (int argc, char *argv[])
{
	widget_t *top, *shell, *pack, *b1, *b2, *b3;

	top = wt_init ();
	shell = wt_create (wt_shell_class, top);
	pack = wt_create (wt_packer_class, shell);

	b1 = wt_create (wt_pushbutton_class, pack);
	b2 = wt_create (wt_pushbutton_class, pack);
	b3 = wt_create (wt_pushbutton_class, pack);

	wt_setopt (b1, WT_LABEL, "aaa", WT_EOL);
	wt_setopt (b2, WT_LABEL, "aa", WT_EOL);
	wt_setopt (b3, WT_LABEL, "a", WT_EOL);

	wt_setopt (pack,
		WT_PACK_WIDGET, b1,
		WT_PACK_INFO,   "-side bottom -fill both -padx 2 -pady 2 -expand 1",
		WT_PACK_WIDGET, b2,
		WT_PACK_WIDGET, b3,
		WT_PACK_INFO,   "-side left -padx 2 -pady 2 -expand 1 -fill both",
		WT_EOL);

	wt_realize (top);
	wt_run ();
	return 0;
}
