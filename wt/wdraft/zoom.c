/* Draw for W
 * 
 * zoom.c implements a pixel editor
 * 
 * w/ 1996 by Eero Tamminen
 */

#include <stdlib.h>
#include "draft.h"

#define MASK_PROPERTIES	(EV_MOUSE|EV_ACTIVE)

static WWIN
  *ZoomWin,
  *DrawWin;

static int
  UnitSize = 8,		/* magnifying factor */
  ZoomW = 32,		/* zoom window size in edited pixels */
  ZoomH = 32,
  Xoffset,
  Yoffset;

static long Timer = -1;

static widget_t *Zoom;


extern void icon_zoom(WWIN *win, int x, int y, int size)
{
  /* a magnifying class */
  w_circle(win, x+size/2, y+size/4, size/4);  
  w_pbox(win, x+size/2-1, y+size/2, 3, size/2);
}

static void timer_callback(long w)
{
  short mx, my;

  if(w_querymousepos(ZoomWin, &mx, &my))
    return;

  mx = (mx - 1) / UnitSize;
  my = (my - 1) / UnitSize;
  if(mx >= 0 && mx < ZoomW && my >= 0 && my < ZoomH)
    {
      w_plot(DrawWin, mx + Xoffset, my + Yoffset);
      w_pbox(ZoomWin, mx * UnitSize, my * UnitSize,
             UnitSize - 1, UnitSize - 1);
    }
  Timer = wt_addtimeout(100, timer_callback, w);
}

/* need to take care of returning M_INVERS mode for the drawing
 * window as the program is non-modal...
 */
static void handle_zoom(widget_t *w, WEVENT *ev)
{
  static short mode = 0;

  switch (ev->type)
    {
    case EVENT_MPRESS:
      if (ev->key & BUTTON_LEFT)
	mode = M_DRAW;
      else
	mode = M_CLEAR;

      w_setmode (ZoomWin, mode);
      w_setmode (DrawWin, mode);
      timer_callback((long)w);
      break;

    case EVENT_ACTIVE:
      if(Timer >= 0)
      {
	timer_callback((long)w);	/* if still editing add callback */
        w_setmode (DrawWin, mode);
      }
      break;

    case EVENT_INACTIVE:
      if(Timer >= 0)			/* disable draw callback */
      {
	wt_deltimeout(Timer);
        w_setmode (DrawWin, M_INVERS);
      }
      break;

    case EVENT_MRELEASE:
      if(Timer >= 0)
      {
        wt_deltimeout(Timer);		/* editing ended */
        w_setmode (DrawWin, M_INVERS);
        Timer = -1;
      }
      break;
    }
}

static void close_zoom(widget_t *w)
{
  if(DrawWin)
    move_box(DrawWin, Xoffset-1, Yoffset-1);

  wt_delete(Zoom);
  ZoomWin = DrawWin = 0;
  Zoom = 0;
}

int init_zoom(WWIN *win, int x, int y, int next)
{
  long mask, width, height;
  widget_t *draw;

  if(next)
  {
    if(Zoom)
      close_zoom(Zoom);
    return 1;
  }
  if(Zoom)
    return 0;

  Zoom = wt_create(wt_shell_class, Top);
  draw = wt_create(wt_drawable_class, Zoom);

  wt_setopt(Zoom,
        WT_LABEL, "Magnify",
	WT_ACTION_CB, close_zoom,
	WT_EOL);

  mask = MASK_PROPERTIES;
  width = UnitSize * ZoomW - 1;
  height = UnitSize * ZoomH - 1;
  wt_setopt (draw,
  	     WT_WIDTH, &width,
	     WT_HEIGHT, &height,
	     WT_EVENT_MASK, &mask,
	     WT_EVENT_CB, handle_zoom,
	     WT_EOL);

  /* set zooming 'box' size one pixel larger than magnified area */
  set_point(win, 0, 0, 0);
  size_box(win, ZoomW, ZoomH);

  wt_realize(Top);
  ZoomWin = wt_widget2win(draw);

  return 0;
}

extern void zoom_block(WWIN *win, int x, int y)
{
  unsigned char *data;
  int w, h, byte, bytes, bits;
  BITMAP *block;

  /* remove old borders */
  if(DrawWin)
    move_box(DrawWin, Xoffset-1, Yoffset-1);

  /* draw new borders */
  move_box(win, x-1, y-1);

  /* faster to check the pixels here */
  if(!(block = w_getblock(win, x, y, ZoomW, ZoomH)))
  {
    close_zoom(Zoom);
    return;
  }

  w_setmode(ZoomWin, M_CLEAR);
  w_pbox(ZoomWin, 0, 0, UnitSize * ZoomW - 1, UnitSize * ZoomH - 1);
  w_setmode(ZoomWin, M_DRAW);

  data = block->data;
  if(block->type == BM_DIRECT8)
  {
    for(h = 0; h < ZoomH; h++)
    {
      for(w = 0; w < ZoomW; w++)
      {
	if(*data++)
	  w_pbox(ZoomWin, w * UnitSize, h * UnitSize,
	    UnitSize - 1, UnitSize - 1);
      }
    }
  }
  else
  {
    /* should be monochrome */
    bytes = (ZoomW + 7) >> 3;
    for(h = 0; h < ZoomH; h++)
    {
      for(byte = 0; byte < bytes; byte++)
      {
	if((bits = *(data++)))
	{
	  for(w = 7; w >= 0; w--)
	  {
	    if(bits & 1)
	      w_pbox(ZoomWin,
		(byte * 8 + w) * UnitSize, h * UnitSize,
		UnitSize - 1, UnitSize - 1);
	    bits >>= 1;
	  }
        }
      }
    }
  }
  DrawWin = win;
  Xoffset = x;
  Yoffset = y;
  free(block->data);
  free(block);
}

