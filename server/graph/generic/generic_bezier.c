/*
 * generic_bezier.c, a part of the W Window System
 *
 * Copyright (C) 1997 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines for drawing cubic bezier curves.
 *
 * NOTES
 * - I just added wrappers for free do_bezier() code... ++eero
 */

#include <stdio.h>
#include "../../config.h"
#include "../../types.h"
#include "../gproto.h"
#include "../clip.h"
#include "generic.h"

#define BITS	5			/* how many fixed point 'digits' */
#define SCALE	(1 << BITS)
#define SCALE_2	(1 << (BITS - 1))

#define MAXAREA	(1 << (2*BITS-1))	/* recursion limit (convex area) */

static void do_bezier(
  long x0, long y0, long x1, long y1,
  long x2, long y2, long x3, long y3
);

/* so that these needs not to be recursed */
static void (*LineFn)(BITMAP *, long, long, long, long);
static void (*PlotFn)(BITMAP *, long, long);
static BITMAP *BitMap;


/* convert arguments to fixed point and proceed... */
static void bezier(BITMAP *bm, register long *coord)
{
  register long minx = 64000, miny = 64000, maxx = -1, maxy = -1;
  REC *oclip = glob_clip0;
  int i;

  for(i = 0; i < 8;)
  {
    if(coord[i] > maxx)
      maxx = coord[i];
    if(coord[i] < minx)
      minx = coord[i];
    i++;
    if(coord[i] > maxy)
      maxy = coord[i];
    if(coord[i] < miny)
      miny = coord[i];
    i++;
  }
  if(maxx < 0 || maxy < 0 || minx >= bm->width || miny >= bm->height)
    return;

  maxx -= minx;
  maxy -= miny;
  if(!BOX_NEEDS_CLIPPING(minx, miny, maxx, maxy, glob_clip0))
    glob_clip0 = NULL;

  (*PlotFn)(bm, coord[0], coord[1]);
  FirstPoint = 0;
  BitMap = bm;

  /* convert to fixed point and add 0.5 */
  do_bezier(
    (long)coord[0] * SCALE + SCALE_2,
    (long)coord[1] * SCALE + SCALE_2,
    (long)coord[2] * SCALE + SCALE_2,
    (long)coord[3] * SCALE + SCALE_2,
    (long)coord[4] * SCALE + SCALE_2,
    (long)coord[5] * SCALE + SCALE_2,
    (long)coord[6] * SCALE + SCALE_2,
    (long)coord[7] * SCALE + SCALE_2
  );
  FirstPoint = 1;
  glob_clip0 = oclip;
}

void generic_bezier(BITMAP *bm, long *controls)
{
  LineFn = theScreen->line;
  PlotFn = theScreen->plot;
  bezier(bm, controls);
}

void generic_dbezier(BITMAP *bm, long *controls)
{
  LineFn = theScreen->dline;
  PlotFn = theScreen->dplot;
  bezier(bm, controls);
}


/*	do_bezier()
**
**	Draw the cubic Bezier curve specified by P0, P1, P2 and P3
**	by subdividing it recursively into sub-curves until it
**	can be approximated smoothly by straight line segments.
**	We use fixed point arithmetic to achieve increased precision
**	in coordinate computations. The control points have been
**	scaled with SCALE and 0.5 has been added to the coordinates
**	so that we can feed rounded coordinate values to line draw.
*/

static void do_bezier(long x0, long y0, long x1, long y1,
	long x2, long y2, long x3, long y3)
{
  long v1x, v1y, v2x, v2y, v3x, v3y;
  long vcp1, vcp2, vcp3, area;
  long mid_x, mid_y;
  int code;

  /*
  ** Stop the recursion when the size of the area covered
  ** by the convex hull of the Bezier curve is less than or
  ** equal to MAXAREA.
  */

  /* First, compute direction vectors. */
  v3x = x3 - x0;    v3y = y3 - y0;
  v2x = x2 - x0;    v2y = y2 - y0;
  v1x = x1 - x0;    v1y = y1 - y0;

  /* Then, compute vector cross products. */
  code = 0;
  if ((vcp1 = v3x * v2y - v2x * v3y) < 0) code += 4;
  if ((vcp2 = v3x * v1y - v1x * v3y) < 0) code += 2;
  if ((vcp3 = v2x * v1y - v1x * v2y) < 0) code += 1;

  /* Finally, compute size of area covered by convex hull.	  */
  /* We actually compute 2*area, but that doesn't matter much.  */
  switch (code)
  {
    case 0:
    case 2:
    case 5:
    case 7:  area = vcp1 + vcp3;  break;
    case 1:
    case 6:  area = vcp2 - vcp3;  break;
    case 3:
    case 4:  area = vcp1 - vcp2;  break;
    default: return;
  }
  if (code & 4)
    area = -area;

  if (area < MAXAREA)
  {
    /* Stop recursion and draw a line from P0 to P3. */
    (*LineFn)(BitMap, (x0 >> BITS), (y0 >> BITS), (x3 >> BITS), (y3 >> BITS));
  }
  else
  {
    /* Area is still too big, so subdivide the curve into	*/
    /* two sub-curves and draw these recursively.		*/

    mid_x = (x0 + x1+x1+x1 + x2+x2+x2 + x3) >> 3;
    mid_y = (y0 + y1+y1+y1 + y2+y2+y2 + y3) >> 3;
    do_bezier(
      x0, y0,
      (x0 + x1) >> 1, (y0 + y1) >> 1,
      (x0 + x1+x1 + x2) >> 2, (y0 + y1+y1 + y2) >> 2,
      mid_x, mid_y);
    do_bezier(
      mid_x, mid_y,
      (x1 + x2+x2 + x3) >> 2, (y1 + y2+y2 + y3) >> 2,
      (x2 + x3) >> 1, (y2 + y3) >> 1,
      x3, y3);
  }
}
