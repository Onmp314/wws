/* 
 * fileselector widget test program
 * 
 * (w) 1996 by Eero Tamminen
 */

#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

/* exit when done... */
static void done_cb(widget_t *w, char *name)
{
  if(name)
    printf("file: '%s'\n", name);
  else
    printf("canceled\n");

  wt_break(1);
}

int main()
{
  widget_t *top, *fsel;

  if(!(top = wt_init()))
    return -1;
  if(!(fsel = wt_create(wt_filesel_class, top)))
    return -1;

  /* used WT_FILESEL PATH & MASK are defaults, so they could be discarded */
  if(wt_setopt(fsel,
  	WT_LABEL, "Load an Image...",
  	WT_FILESEL_PATH, "/ram/Wt/",
	WT_FILESEL_MASK, "*",
	WT_ACTION_CB, done_cb,
	WT_EOL) < 0)
    return -1;

  if (wt_realize(top) < 0)
    return -1;
  wt_run ();
  return 0;
}
