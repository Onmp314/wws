/*
 * client_noret.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- graphics functions called by clients that don't need to return
 * anything
 *
 * CHANGES
 * ++eero, 2/98:
 * - moved ellipse and bezier functions from client_others.c here
 *   and moved all the settings there.
 * - added arc and pie drawing functions.
 * - removed the mouse intersection/hide code when drawing to
 *   backup bitmap.
 * - corrected the dirty rectange size to take into account line width
 *   (boxes, circles, ellipses and arcs) and italics correction for
 *   slanted font style.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"
#include "rect.h"


/*
 * and here are the items...
 *
 * the first one, client_plot() will serve as a demo of how this works and
 * therefore contains extensive comments, the others just follow this scheme.
 */

void client_plot(CLIENT *cptr,
		 ushort handle,
		 short x0, short y0)
{
  WINDOW *win;
  void (*plot)(BITMAP *, long, long);

  /* first thing is to get the window pointer from the handle
   */
  if (!(win = windowLookup(handle))) {
    return;
  }

  /* you cannot draw to W_CONTAINER windows
   */
  if (win->flags & W_CONTAINER) {
    return;
  }

  plot = (glob_pakettype == PAK_PLOT) ? glob_screen->plot : glob_screen->dplot;

  /* then we must get the effective coordinates to draw to as relative to
   * the window. for now this is merely just adding the size of the border.
   * this does not yet specify if and/or where on the screen the plot will
   * occur.
   */
  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

  /* set the graphic context for this window. `gc0' is a global variable
   * used by all graphics calls.
   */
  gc0 = &win->gc;

  /* if the window is (at least partially) hidden or not open at all we'll draw
   * into the window bitmap, mark the window as dirty and let a later routine
   * do the job of updating the screen contents. since this is always done via
   * bitblk operations we can save quite a lot of time by doing so only when
   * it's really necessary, say, when all requests from this client currently
   * in the receive buffer are processed and we don't know if anything more
   * will come in time.
   *
   * if the server was compiled with -DREFRESH all drawing stuff will always go
   * to the window bitmap. if the bitmap is always guaranteed to be up to date
   * we'll be able to do an easy refresh of the whole screen.
   */
#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {

    /* window is open and completely visible, so there's no point in using
     * the bitmap. we better draw directly on the screen because that saves
     * double work. therefore we actually MUST disable the mouse here if
     * the pointer interferes with the region in question.
     *
     * If the mouse isn't in the *visible* area of the window, this wouldn't
     * be necessary. However, checking that would take more time...
     */
    if (mouse_rcintersect(win->pos.x0 + x0,
			  win->pos.y0 + y0, 1, 1)) {

      /* this will only hide it if it's not already hidden
       */
      mouse_hide();
    }

    /* set clipping to the work area of the window on screen
     */
    glob_clip0 = &win->work;

    /* plot into screen
     */
    (*plot)(&glob_screen->bm, win->pos.x0 + x0,
			      win->pos.y0 + y0);

    /* mark that the screen is more up-to-date than the bitmap
     */
    win->is_dirty = 1;
  } else
#endif
  {
    /* set clipping to the work area of the bitmap
     */
    glob_clip0 = &win->area[AREA_WORK];

    /* plot into the window bitmap
     */
    (*plot)(&win->bitmap, x0, y0);

    /* if the window is in fact open but hidden we must mark it as dirty, else
     * we don't care about that: since the window isn't on the screen there's
     * no point in updating it's contents.
     */
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0, y0, 1, 1);
    }
  }
}


void client_line(CLIENT *cptr,
		 ushort handle,
		 short x0, short y0,
		 short xe, short ye)
{
  WINDOW *win;
  void (*line)(BITMAP *, long, long, long, long);
  short ex0, ey0, ewidth, eheight;

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }

  line = (glob_pakettype == PAK_LINE) ? glob_screen->line : glob_screen->dline;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;
  xe += win->area[AREA_WORK].x0;
  ye += win->area[AREA_WORK].y0;

  if ((ex0 = x0) > xe)
    ex0 = xe;
  ewidth = abs(xe - x0) + 1;
  if ((ey0 = y0) > ye)
    ey0 = ye;
  eheight = abs(ye - y0) + 1;

  gc0 = &win->gc;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0+ex0,
			  win->pos.y0+ey0, ewidth, eheight)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*line)(&glob_screen->bm,
	    win->pos.x0+x0, win->pos.y0+y0, win->pos.x0+xe, win->pos.y0+ye);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*line)(&win->bitmap, x0, y0, xe, ye);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, ex0, ey0, ewidth, eheight);
    }
  }
}


void client_hline(CLIENT *cptr, ushort handle, short x0, short y0, short xe)
{
  WINDOW *win;
  void (*hline)(BITMAP *, long, long, long);

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }

  hline = (glob_pakettype == PAK_HLINE) ? glob_screen->hline : glob_screen->dhline;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;
  xe += win->area[AREA_WORK].x0;

  gc0 = &win->gc;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0 + x0, win->pos.y0 + y0,
			  xe - x0 + 1, 1)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*hline)(&glob_screen->bm,
	     win->pos.x0 + x0, win->pos.y0 + y0,
	     win->pos.x0 + xe);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*hline)(&win->bitmap, x0, y0, xe);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0, y0, xe - x0 + 1, 1);
    }
  }
}


void client_vline(CLIENT *cptr, ushort handle, short x0, short y0, short ye)
{
  WINDOW *win;
  void (*vline)(BITMAP *, long, long, long);

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }

  vline = (glob_pakettype == PAK_VLINE) ? glob_screen->vline : glob_screen->dvline;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;
  ye += win->area[AREA_WORK].y0;

  gc0 = &win->gc;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0+x0, win->pos.y0+y0,
			  1, abs(ye-y0)+1)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*vline)(&glob_screen->bm,
	     win->pos.x0 + x0, win->pos.y0 + y0, win->pos.y0 + ye);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*vline)(&win->bitmap, x0, y0, ye);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0, y0, 1, ye - y0 + 1);
    }
  }
}


void client_box(CLIENT *cptr, ushort handle, short x0, short y0, short width,
		short height)
{
  WINDOW *win;
  void (*box)(BITMAP *, long, long, long, long);
  int lwd;

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }
  gc0 = &win->gc;
  lwd = gc0->linewidth >> 1;

  switch (glob_pakettype) {
    case PAK_BOX:
      box = glob_screen->box;
      break;
    case PAK_PBOX:
      box = glob_screen->pbox;
      lwd = 0;
      break;
    case PAK_DBOX:
      box = glob_screen->dbox;
      break;
    default:   /* PAK_DPBOX */
      box = glob_screen->dpbox;
      lwd = 0;
  }

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

  if (width < 0) {
    x0 += width;
    width = -width;
  }
  if (height < 0) {
    y0 += height;
    height = -height;
  }

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0+x0 - lwd, win->pos.y0+y0 - lwd,
			  width + lwd*2, height + lwd*2)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*box)(&glob_screen->bm,
	   win->pos.x0+x0, win->pos.y0+y0, width, height);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*box)(&win->bitmap, x0, y0, width, height);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0 - lwd, y0 - lwd,
		      width + lwd*2, height + lwd*2);
    }
  }
}


void client_bitblk (CLIENT *cptr, ushort handle, short x0, short y0,
		    short width, short height, short x1, short y1)
{
  WINDOW *win;

  if (!(win = windowLookup(handle)))
    return;

  if (win->flags & W_CONTAINER)
    return;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;
  x1 += win->area[AREA_WORK].x0;
  y1 += win->area[AREA_WORK].y0;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if ((mouse_rcintersect(win->pos.x0+x0,
			   win->pos.y0+y0, width, height)) ||
	(mouse_rcintersect(win->pos.x0+x1,
			   win->pos.y0+y1, width, height))) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    glob_clip1 = &win->work;
    (*glob_screen->bitblk)(&glob_screen->bm, win->pos.x0+x0,
			   win->pos.y0+y0, width, height,
			   &glob_screen->bm, win->pos.x0+x1,
			   win->pos.y0+y1);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    glob_clip1 = &win->area[AREA_WORK];
    (*glob_screen->bitblk)(&win->bitmap, x0, y0, width, height,
			   &win->bitmap, x1, y1);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x1, y1, width, height);
    }
  }
}


void client_bitblk2(CLIENT *cptr, ushort shandle, short x0, short y0,
		    short width, short height, ushort dhandle, short x1,
		    short y1)
{
  WINDOW *swin, *dwin;

  if (!(swin = windowLookup(shandle)) || !(dwin = windowLookup(dhandle))) {
    return;
  }

  if ((swin->flags & W_CONTAINER) || (dwin->flags & W_CONTAINER)) {
    return;
  }

  x0 += swin->area[AREA_WORK].x0;
  y0 += swin->area[AREA_WORK].y0;
  x1 += dwin->area[AREA_WORK].x0;
  y1 += dwin->area[AREA_WORK].y0;

#ifndef REFRESH
  if (dwin->is_open && !dwin->is_hidden) {
    /* destination: screen */
    if (!swin->is_open || swin->is_hidden) {
      /* source: bitmap */
      if (mouse_rcintersect(dwin->pos.x0+x1,
			    dwin->pos.y0+y1, width, height)) {
	mouse_hide();
      }
      glob_clip0 = &swin->area[AREA_WORK];
      glob_clip1 = &dwin->work;
      (*glob_screen->bitblk)(&swin->bitmap, x0, y0, width,
			     height, &glob_screen->bm,
			     dwin->pos.x0+x1,
			     dwin->pos.y0+y1);
    } else {
      /* source: screen */
      if ((mouse_rcintersect(swin->pos.x0+x0,
			     swin->pos.y0+y0, width, height)) ||
	  (mouse_rcintersect(dwin->pos.x0+x1,
			     dwin->pos.y0+y1, width, height))) {
	mouse_hide();
      }
      glob_clip0 = &swin->work;
      glob_clip1 = &dwin->work;
      (*glob_screen->bitblk)(&glob_screen->bm, swin->pos.x0+x0,
			     swin->pos.y0+y0, width, height,
			     &glob_screen->bm, dwin->pos.x0+x1,
			     dwin->pos.y0+y1);
    }
    dwin->is_dirty = 1;
  } else
#endif
  {
    /* destination: bitmap */
#ifndef REFRESH
    if (swin->is_open && !swin->is_hidden) {
      /* source: screen */
      if (mouse_rcintersect(swin->pos.x0 + x0,
			    swin->pos.y0 + y0, width, height)) {
	mouse_hide();
      }
      glob_clip0 = &swin->work;
      glob_clip1 = &dwin->area[AREA_WORK];
      (*glob_screen->bitblk)(&glob_screen->bm, swin->pos.x0+x0,
			     swin->pos.y0+y0, width, height,
			     &dwin->bitmap, x1, y1);
    } else
#endif
    {
      /* source: bitmap */
      glob_clip0 = &swin->area[AREA_WORK];
      glob_clip1 = &dwin->area[AREA_WORK];
      (*glob_screen->bitblk)(&swin->bitmap, x0, y0, width,
			     height, &dwin->bitmap, x1, y1);
    }
    if (dwin->is_open) {
      rectUpdateDirty(&dwin->dirty, x1, y1, width, height);
    }
  }
}


void client_vscroll(CLIENT *cptr, ushort handle, short x0, short y0,
		    short width, short height, short y1)
{
  WINDOW *win;

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->work.x0+x0, win->work.y0+y0,
			  width, height) ||
	mouse_rcintersect(win->work.x0+x0, win->work.y0+y1,
			  width, height)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*glob_screen->scroll)(&glob_screen->bm, win->work.x0+x0,
			   win->work.y0+y0, width, height,
			   win->work.y0+y1);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*glob_screen->scroll)(&win->bitmap, win->area[AREA_WORK].x0 + x0,
			   win->area[AREA_WORK].y0 + y0, width,
			   height, win->area[AREA_WORK].y0 + y1);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0, y1, width, height);
    }
  }
}


void client_prints(CLIENT *cptr, ushort handle, short x0, short y0, const uchar *s)
{
  WINDOW *win;
  FONT *f;
  int len;

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }

  if (!(f = win->gc.font)) {
    return;
  }

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

  len = fontStrLen(f, s);
  gc0 = &win->gc;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0+x0 - f->slant_offset, win->pos.y0+y0,
			  len + f->slant_size, f->hdr.height)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*glob_screen->prints)(&glob_screen->bm, win->pos.x0+x0,
			   win->pos.y0+y0, s);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*glob_screen->prints)(&win->bitmap, x0, y0, s);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0 - f->slant_offset, y0,
                      len + f->slant_size, f->hdr.height);
    }
  }
}


void client_circle(CLIENT *cptr, ushort handle, short x0, short y0, short r)
{
  WINDOW *win;
  void (*circ)(BITMAP *, long, long, long);
  long lwd, size;

  if (r < 0) {
    return;
  }

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }
  gc0 = &win->gc;
  lwd = gc0->linewidth >> 1;

  switch (glob_pakettype) {
    case PAK_CIRCLE:
      circ = glob_screen->circ;
      break;
    case PAK_PCIRCLE:
      circ = glob_screen->pcirc;
      lwd = 0;
      break;
    case PAK_DCIRCLE:
      circ = glob_screen->dcirc;
      break;
    default:   /* PAK_DPCIRCLE */
      circ = glob_screen->dpcirc;
      lwd = 0;
  }
  size = ((r+lwd) << 1) + 1;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0+x0-r-lwd,
			  win->pos.y0+y0-r-lwd, size, size)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*circ)(&glob_screen->bm, win->pos.x0 + x0,
	    win->pos.y0 + y0, r);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*circ)(&win->bitmap, x0, y0, r);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0-r-lwd, y0-r-lwd, size, size);
    }
  }
}


void client_ellipse(CLIENT *cptr, ushort handle, short x0, short y0, short rx, short ry)
{
  WINDOW *win;
  void (*ell)(BITMAP *, long, long, long, long);
  long sizex, sizey, lwd;

  if (rx < 0 || ry < 0) {
    return;
  }

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }
  gc0 = &win->gc;
  lwd = gc0->linewidth >> 1;

  switch (glob_pakettype) {
    case PAK_ELLIPSE:
      ell = glob_screen->ell;
      break;
    case PAK_PELLIPSE:
      ell = glob_screen->pell;
      lwd = 0;
      break;
    case PAK_DELLIPSE:
      ell = glob_screen->dell;
      break;
    default:   /* PAK_DPellipse */
      ell = glob_screen->dpell;
      lwd = 0;
  }
  sizex = ((rx+lwd) << 1) + 1;
  sizey = ((ry+lwd) << 1) + 1;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0+x0-rx-lwd,
			  win->pos.y0+y0-ry-lwd, sizex, sizey)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*ell)(&glob_screen->bm, win->pos.x0 + x0,
	    win->pos.y0 + y0, rx, ry);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*ell)(&win->bitmap, x0, y0, rx, ry);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0-rx-lwd, y0-ry-lwd, sizex, sizey);
    }
  }
}


void client_pie(CLIENT *cptr, ushort handle, short x0, short y0,
		short rx, short ry, short ax, short ay, short bx, short by,
		short adir, short bdir)
{
  WINDOW *win;
  PIZZASLICE slice;
  void (*pie)(BITMAP *, PIZZASLICE *);
  long sizex, sizey, lwd;

  if (rx < 0 || ry < 0) {
    return;
  }

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }
  gc0 = &win->gc;
  lwd = gc0->linewidth >> 1;

  switch (glob_pakettype) {
    case PAK_ARC:
      pie = glob_screen->arc;
      break;
    case PAK_DARC:
      pie = glob_screen->darc;
      lwd = 0;
      break;
    case PAK_PIE:
      pie = glob_screen->pie;
      break;
    default:   /* PAK_DPIE */
      pie = glob_screen->dpie;
      lwd = 0;
  }
  sizex = ((rx+lwd) << 1) + 1;
  sizey = ((ry+lwd) << 1) + 1;

  x0 += win->area[AREA_WORK].x0;
  y0 += win->area[AREA_WORK].y0;

  slice.x0 = x0;  slice.y0 = y0;
  slice.rx = rx;  slice.ry = ry;
  slice.ax = ax;  slice.ay = ay;
  slice.bx = bx;  slice.by = by;
  slice.adir = adir;
  slice.bdir = bdir;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    if (mouse_rcintersect(win->pos.x0+x0-rx-lwd,
			  win->pos.y0+y0-ry-lwd, sizex, sizey)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    slice.x0 += win->pos.x0;
    slice.y0 += win->pos.y0;
    (*pie)(&glob_screen->bm, &slice);
    win->is_dirty = 1;
  } else
#endif
  {
    glob_clip0 = &win->area[AREA_WORK];
    (*pie)(&glob_screen->bm, &slice);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, x0-rx-lwd, y0-ry-lwd, sizex, sizey);
    }
  }
}


void client_bezier(CLIENT *cptr, ushort handle, short *controls)
{
  static long lpoints[8];
  long *lptr = lpoints;
  short x, y, xmin, ymin, xmax, ymax, wx0, wy0, count = 4;
  WINDOW *win;
  void (*bez)(BITMAP *, long *);

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }

  if (glob_pakettype == PAK_BEZIER) {
    bez = glob_screen->bez;
  } else {
    bez = glob_screen->dbez;
  }

  wx0 = win->area[AREA_WORK].x0;
  wy0 = win->area[AREA_WORK].y0;
  xmin = 32767;
  ymin = 32767;
  xmax = -32768;
  ymax = -32768;

  gc0 = &win->gc;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    wx0 += win->pos.x0;
    wy0 += win->pos.y0;
    /* max. extent for bezier */
    while (count--) {
      /*
       * ntohs evaluates it's argument more than once
       */
      x = *controls++;
      y = *controls++;
      if ((x = ntohs (x)) < xmin) xmin = x;
      if (x > xmax) xmax = x;
      if ((y = ntohs (y)) < ymin) ymin = y;
      if (y > ymax) ymax = y;
      *lptr++ = x + wx0;
      *lptr++ = y + wy0;
    }
    xmax = xmax - xmin + 1;   /* this really is 'width' */
    ymax = ymax - ymin + 1;
    if (mouse_rcintersect(wx0 + xmin, wy0 + ymin, xmax, ymax)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*bez)(&glob_screen->bm, lpoints);
    win->is_dirty = 1;
  } else
#endif
  {
    /* max. extent for bezier */
    while (count--) {
      x = *controls++;
      y = *controls++;
      if ((x = ntohs(x)) < xmin) xmin = x;
      if (x > xmax) xmax = x;
      if ((y = ntohs(y)) < ymin) ymin = y;
      if (y > ymax) ymax = y;
      *lptr++ = x + wx0;
      *lptr++ = y + wy0;
    }
    xmax = xmax - xmin + 1;   /* this really is 'width' */
    ymax = ymax - ymin + 1;
    glob_clip0 = &win->area[AREA_WORK];
    (*bez)(&win->bitmap, lpoints);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, wx0 + xmin, wy0 + ymin, xmax, ymax);
    }
  }
}


void client_poly(CLIENT *cptr, ushort handle, short numpoints, short *points)
{
  static long lpoints[MAXPOLYPOINTS<<1];
  long *lptr = lpoints;
  short x, y, xmin, ymin, xmax, ymax, wx0, wy0, count = numpoints;
  WINDOW *win;
  void (*poly)(BITMAP *, long, long *);

  if ((numpoints < 3) || (numpoints > MAXPOLYPOINTS)) {
    return;
  }

  if (!(win = windowLookup(handle))) {
    return;
  }

  if (win->flags & W_CONTAINER) {
    return;
  }

  switch (glob_pakettype) {
    case PAK_POLY:
      poly = glob_screen->poly;
      break;
    case PAK_PPOLY:
      poly = glob_screen->ppoly;
      break;
    case PAK_DPOLY:
      poly = glob_screen->dpoly;
      break;
    default:   /* PAK_DPPOLY */
      poly = glob_screen->dppoly;
  }

  wx0 = win->area[AREA_WORK].x0;
  wy0 = win->area[AREA_WORK].y0;
  xmin = 32767;
  ymin = 32767;
  xmax = -32768;
  ymax = -32768;

  gc0 = &win->gc;

#ifndef REFRESH
  if (win->is_open && !win->is_hidden) {
    wx0 += win->pos.x0;
    wy0 += win->pos.y0;
    while (count--) {
      /*
       * ntohs evaluates it's argument more than once
       */
      x = *points++;
      y = *points++;
      if ((x = ntohs (x)) < xmin) xmin = x;
      if (x > xmax) xmax = x;
      if ((y = ntohs (y)) < ymin) ymin = y;
      if (y > ymax) ymax = y;
      *lptr++ = x + wx0;
      *lptr++ = y + wy0;
    }
    xmax = xmax - xmin + 1;   /* this really is 'width' */
    ymax = ymax - ymin + 1;
    if (mouse_rcintersect(wx0 + xmin, wy0 + ymin, xmax, ymax)) {
      mouse_hide();
    }
    glob_clip0 = &win->work;
    (*poly)(&glob_screen->bm, numpoints, lpoints);
    win->is_dirty = 1;
  } else
#endif
  {
    while (count--) {
      x = *points++;
      y = *points++;
      if ((x = ntohs(x)) < xmin) xmin = x;
      if (x > xmax) xmax = x;
      if ((y = ntohs(y)) < ymin) ymin = y;
      if (y > ymax) ymax = y;
      *lptr++ = x + wx0;
      *lptr++ = y + wy0;
    }
    xmax = xmax - xmin + 1;   /* this really is 'width' */
    ymax = ymax - ymin + 1;
    glob_clip0 = &win->area[AREA_WORK];
    (*poly)(&win->bitmap, numpoints, lpoints);
    if (win->is_open) {
      rectUpdateDirty(&win->dirty, wx0 + xmin, wy0 + ymin, xmax, ymax);
    }
  }
}

