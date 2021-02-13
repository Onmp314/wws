/* Draw for W
 * 
 * create.c implements new drawing window dialog / creation
 * 
 * w/ 1996 by Eero Tamminen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "draft.h"

/* string lenghts */
#define MAX_SIZE	4

static widget_t
  *Open,		/* dialog */
  *Width,		/* new window size */
  *Height;


void icon_new(WWIN *win, int x, int y, int size)
{
  w_box(win, x, y, size, size);
  w_line(win, x, y, x+size-1, y+size-1);
  w_line(win, x+size-1, y, x, y+size-1);
}

static void close_window(widget_t *w)
{
  wt_delete(w);
}

/* create / open a new drawing window */
static WWIN *open_window(long width, long height, const char *title)
{
  widget_t *win, *draw;
  long mask;

  win  = wt_create(wt_shell_class, Top);
  draw = wt_create(wt_drawable_class, win);

  wt_setopt (win,
	WT_LABEL, title,
	WT_ACTION_CB, close_window,
	WT_EOL);

  mask = DRAW_PROPERTIES;
  wt_setopt (draw,
	WT_WIDTH, &width,
	WT_HEIGHT, &height,
	WT_EVENT_MASK, &mask,
	WT_EVENT_CB, handle_drawing,
	WT_EOL);

  /* show on screen */
  wt_realize(Top);

  return wt_widget2win(draw);
}

static void open_new(widget_t *w, int pressed)
{
  char *width, *height;

  if(pressed)
    return;
  
  wt_getopt(Width, WT_STRING_ADDRESS, &width, WT_EOL);
  wt_getopt(Height, WT_STRING_ADDRESS, &height, WT_EOL);
  open_window((long)atol(width), (long)atol(height), "DRAW");
}

static void save_file(widget_t *w, const char *filename)
{
  /* canceled */
  if(!filename)
  {
    wt_delete(w);
    return;
  }
  if(Current)
  {
    BITMAP *save = w_getblock(Current, 0, 0, Current->width, Current->height);
    if(!save)
    {
      wt_dialog(w, "w_getblock() failed!", WT_DIAL_ERROR, "ERROR:", "OK", NULL);
      return;
    }
    if (w_writepbm(filename, save) < 0)
      wt_dialog(w, "Image saving failed!", WT_DIAL_ERROR, "ERROR:", "OK", NULL);
    w_freebm(save);
  }
  else
    wt_dialog(w, "No image!", WT_DIAL_INFO, "ERROR:", "OK", NULL);
}

static void save_new(widget_t *w, int pressed)
{

  widget_t *filesel;

  if(pressed || !(filesel = wt_create (wt_filesel_class, Top)))
    return;

  wt_setopt(filesel, WT_LABEL, "Save PBM", WT_ACTION_CB, save_file, WT_EOL);
  wt_realize(Top);
}

/* note: filename is most probably a temporary variable. */
void load_file(widget_t *w, const char *filename)
{
  const char *title;
  char size[MAX_SIZE+1];
  BITMAP *image;
  WWIN *win;

  /* canceled */
  if(!filename)
  {
    wt_delete(w);
    return;
  }

  if(!(image = w_readpbm(filename)))
    return;

  title = filename + strlen(filename);
  /* exclude path from the filename */
  while(*title != '/' && *title != '\\' && title >= filename)
    title--;
  title++;

  if((win = open_window((long)image->width, (long)image->height, title)))
  {
    w_putblock(image, win, 0, 0);
    if(Width)
    {
      sprintf(size, "%d", image->width);
      wt_setopt(Width,
	  WT_STRING_ADDRESS, size,
	  WT_EOL);

      sprintf(size, "%d", image->height);
      wt_setopt(Height,
	  WT_STRING_ADDRESS, size,
	  WT_EOL);
    }
    Current = win;
  }
  free(image->data);
  free(image);
}

static void load_new(widget_t *w, int pressed)
{

  widget_t *filesel;

  if(pressed || !(filesel = wt_create (wt_filesel_class, Top)))
    return;

  wt_setopt(filesel, WT_LABEL, "Load PBM", WT_ACTION_CB, load_file, WT_EOL);
  wt_realize(Top);
}

/* handle the dialog for new drawing windows */

static void close_open(widget_t *w)
{
  wt_close(w);
}

int init_new(WWIN *win, int x, int y, int next)
{
  long size, mode;
  widget_t *vpane, *hpane1, *hpane2, *new, *load, *save, *times;

  /* Already create */
  if(Width)
  {
    wt_open (Open);
    return 1;
  }

  Open   = wt_create(wt_shell_class, Top);
  vpane  = wt_create(wt_pane_class, Open);
  hpane1 = wt_create(wt_pane_class, vpane);
  hpane2 = wt_create(wt_pane_class, vpane);
  new    = wt_create(wt_button_class, hpane1);
  load   = wt_create(wt_button_class, hpane1);
  save   = wt_create(wt_button_class, hpane1);
  Width  = wt_create(wt_getstring_class, hpane2);
  times  = wt_create(wt_label_class, hpane2);
  Height = wt_create(wt_getstring_class, hpane2);

  wt_setopt(Open,
        WT_LABEL, "Open Window",
	WT_ACTION_CB, close_open,
	WT_EOL);

  mode = OrientHorz;
  wt_setopt(hpane1, WT_ORIENTATION, &mode, WT_EOL);
  wt_setopt(hpane2, WT_ORIENTATION, &mode, WT_EOL);

  size = MAX_SIZE;
  wt_setopt(Width,
  	WT_STRING_ADDRESS, "320",
	WT_STRING_MASK, "0-9",
        WT_STRING_LENGTH, &size,
        WT_STRING_WIDTH, &size,
	WT_EOL);

  wt_setopt(times,
  	WT_LABEL, "x",
	WT_EOL);

  wt_setopt(Height,
  	WT_STRING_ADDRESS, "200",
	WT_STRING_MASK, "0-9",
        WT_STRING_LENGTH, &size,
        WT_STRING_WIDTH, &size,
	WT_EOL);

  wt_setopt(new,
  	WT_LABEL, "New",
	WT_ACTION_CB, open_new,
	WT_EOL);

  wt_setopt(load,
  	WT_LABEL, "Load",
	WT_ACTION_CB, load_new,
	WT_EOL);

  wt_setopt(save,
  	WT_LABEL, "Save",
	WT_ACTION_CB, save_new,
	WT_EOL);

  wt_realize(Top);
  return 1;
}

