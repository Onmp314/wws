/* Test file for the text input stuff
 *
 * For fun try copy&paste between widgets :-)
 *
 * (w) 1996 by Eero Tamminen
 */

#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

#define STRING_LEN	64

static void enter_callback (widget_t *w, char *txt, int cursor)
{
  printf("User pressed ENTER.\nString: %s, cursor at: %d\n", txt, cursor);
}

int main(int argc, char *argv[])
{
  widget_t *top, *shell, *pane, *text1, *text2;
  long a, b;

  if (!(top   = wt_init())) return 1;
  if (!(shell = wt_create(wt_shell_class, top)))      return 2;
  if (!(pane  = wt_create(wt_pane_class, shell)))     return 3;
  if (!(text1 = wt_create(wt_getstring_class, pane))) return 4;
  if (!(text2 = wt_create(wt_getstring_class, pane))) return 5;

  a = AlignFill;
  wt_setopt (pane, WT_ALIGNMENT, &a, WT_EOL);
  wt_setopt (shell, WT_LABEL, " Text Input ", WT_EOL);

  b = 24;
  a = STRING_LEN;
  wt_setopt (text1,
	     WT_STRING_ADDRESS, "Hello World!!!",
	     WT_STRING_LENGTH, &a,
	     WT_STRING_WIDTH, &b,
	     WT_ACTION_CB, enter_callback,
	     WT_EOL);

  wt_setopt (text2,
	     WT_STRING_ADDRESS, "Who am I?",
	     WT_STRING_MASK, "--+a-zA-Z0-9",
	     WT_ACTION_CB, enter_callback,
	     WT_EOL);

  if (wt_realize(top) < 0) return 6;
  wt_run ();
  return 0;
}
