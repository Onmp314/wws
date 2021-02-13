/* Startings for a real simple W icon editor...
 * (actually this is a test file for my icon editor widget)
 *
 * (w) 1996 by Eero Tamminen
 *
 * $Id: iconeditor.c,v 1.3 1999/05/16 15:05:38 eero Exp $
 */

#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

#define SAVE_FILE	"icon.pbm"
#define ICON_SIZE	32	/* annoying Wlib limit for w_getblock()... */

static widget_t *Icon;


static void save_callback (widget_t * button, int down)
{
  FILE *fp;
  BITMAP *bitmap;
  long w, h, x, y; /* must be long. kay. */

  if(down)
    return;

  if((fp = fopen(SAVE_FILE, "wb")))
  {
    wt_getopt(Icon,
	      WT_ICON_XPOS, &x,
	      WT_ICON_YPOS, &y,
	      WT_ICON_WIDTH, &w,
	      WT_ICON_HEIGHT, &h,
	      WT_EOL);

    bitmap = w_getblock(wt_widget2win(Icon), x, y, w, h);
    if(bitmap)
    {
      fprintf(fp, "P4\n%ld %ld\n", w, h);
      fwrite(bitmap->data,
        bitmap->upl * bitmap->unitsize * bitmap->height, 1, fp);
    }
    else
      wt_setopt(button,
	        WT_LABEL, "Fail",
	        WT_EOL);
    fclose(fp);
  }
  else
    wt_setopt(button,
	      WT_LABEL, "Fail",
	      WT_EOL);
}

static void quit_callback (widget_t * wp, int down)
{
  if (!down)
    wt_break (1);
}

int main(int argc, char *argv[])
{
  widget_t *top, *shell, *box, *quit, *save;
  long size;

  if (!(top   = wt_init())) return 1;				/* toolkit */
  if (!(shell = wt_create(wt_shell_class, top)))   return 2;	/* window */
  if (!(box   = wt_create(wt_box_class, shell)))   return 3;	/* container */
  if (!(quit  = wt_create(wt_button_class, box)))  return 4;	/* button */
  if (!(save  = wt_create(wt_button_class, box)))  return 5;	/* button */
  if (!(Icon  = wt_create(wt_iconedit_class, box)))return 6;	/* editor */

  wt_setopt (shell, WT_LABEL, " IconEdit ", WT_EOL);

  wt_setopt (quit,
	     WT_LABEL, "Quit",
	     WT_ACTION_CB, quit_callback,
	     WT_EOL);

  wt_setopt (save,
	     WT_LABEL, "Save",
	     WT_ACTION_CB, save_callback,
	     WT_EOL);

  size = ICON_SIZE;
  wt_setopt (Icon,
	     WT_ICON_WIDTH, &size,
	     WT_ICON_HEIGHT, &size,
	     WT_EOL);

  if (wt_realize(top) < 0) return 7;
  wt_run ();
  return 0;
}
