/*
 * Test for list box widget
 *	shows how you can compose an autolocator
 *
 * (w) 1996 by Eero Tamminen
 */

#include <string.h>
#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

static widget_t *text, *list;

#define ITEMS 10
static const char *list_array[ITEMS+1] =
{
  "first",
  "second",
  "third",
  "fourth",
  "fifth",
  "sixth",
  "seventh",
  "eight",
  "ninth",
  "tenth",
  NULL
};


/* search first item from 'list' beginning with 'string' */
static int locate_first (const char *string, const char **list, int items)
{
  int index, len = strlen (string);

  if (!(len && list && items))
    return -1;

  for (index = 0; index < items; index++)
  {
    if (!strncmp (string, *list++, len))
      return index;
  }
  return -1;
}

static void enter_cb (widget_t *w, const char *txt, int cursor)
{
  long idx;

  idx = locate_first (txt, list_array, ITEMS);
  wt_setopt (list, WT_CURSOR, &idx, WT_EOL);

  /* if item located, but it isn't complete match? -> complete */
  if (idx >= 0 && strcmp (list_array[idx], txt))
    wt_setopt (text, WT_STRING_ADDRESS, list_array[idx], WT_EOL);

  printf("ENTER pressed, string: %s, cursor at: %d\n", txt, cursor);
}

/* search the string from list and set list cursor
 * and getstring widget accordingly
 */
static void text_change_cb (widget_t *w, const char *txt, int cursor)
{
  long idx;

  idx = locate_first (txt, list_array, ITEMS);
  wt_setopt (list, WT_CURSOR, &idx, WT_EOL);
}

/* show the list item on getstring widget */
static void select_cb (widget_t *w, char *txt, int index)
{
  wt_setopt(text, WT_STRING_ADDRESS, txt, WT_EOL);
}

/* feed keys that listbox doesn't recognice to the getstring
 * widget which will feed the new string (if it changed)
 * to the text change function...
 */
static void route_list_key (widget_t *w, WEVENT *ev)
{
  (*wt_getstring_class->event) (text, ev);
}

/* ---------------------------- */

int main(int argc, char *argv[])
{
  long tmp;
  widget_t *top, *shell, *pane;

  if (!(top   = wt_init())) return 1;
  if (!(shell = wt_create(wt_shell_class, top)))      return 2;
  if (!(pane  = wt_create(wt_pane_class, shell)))     return 3;
  if (!(text  = wt_create(wt_getstring_class, pane))) return 4;
  if (!(list  = wt_create(wt_listbox_class, pane)))   return 5;

  wt_setopt (shell, WT_ICON_STRING, "List Boxer", WT_EOL);
  tmp = AlignFill;
  wt_setopt (pane, WT_ALIGNMENT, &tmp, WT_EOL);

  tmp = 8;
  wt_setopt (text,
	WT_STRING_LENGTH, &tmp,
	WT_STRING_WIDTH, &tmp,
	WT_CHANGE_CB, text_change_cb,
	WT_ACTION_CB, enter_cb,
	WT_EOL);

  tmp = 5;
  wt_setopt (list,
	WT_LIST_HEIGHT, &tmp,
	WT_LIST_ADDRESS, list_array,
	WT_INKEY_CB, route_list_key,
	WT_CHANGE_CB, select_cb,
	WT_ACTION_CB, select_cb,
	WT_EOL);

  if (wt_realize(top) < 0) return 6;
  wt_run ();
  return 0;
}
