/******************************************************************************
 * Dragon - a version of Mah-Jongg for X Windows
 *
 * W Port: Jens Kilian			February 1996
 *
 * wfuncs.c - Special functions for the W port
 ******************************************************************************/

#include <Wlib.h>
#include <stdlib.h>
#include "main.h"
#include "proto.h"


/* 
 * window settings 
 */

void wsetmode(short mode)
{
  w_setmode(Board, mode);
}

void wsetpattern(ushort pattern)
{
  w_setpattern(Board, pattern);
}

/*
 *		Draw a polyline
 */

void wpolyline(Point *pnts, int npnts)
{
  short x0, y0, x1, y1;
  int i;
    
  x0 = pnts->x;
  y0 = pnts->y;
    
  for (i = 1; i < npnts; ++i) {
    x1 = x0 + (++pnts)->x;
    y1 = y0 + pnts->y;

    if (x0 == x1) {
      w_vline(Board, x0, y0, y1);
    } else if (y0 == y1) {
      w_hline(Board, x0, y0, x1);
    } else {
      w_line(Board, x0, y0, x1, y1);
    }

    x0 = x1;
    y0 = y1;
  }
}

/*
 *		Draw a filled polygon
 */

void wpolygon(Point *pnts, int npnts)
{
  short *coords = (short *)malloc(sizeof(short)*npnts*2);
  
  if (coords) {
    short *cptr = coords;
    short x, y;
    int i;
    
    *cptr++ = x = pnts->x;
    *cptr++ = y = pnts->y;
    
    for (i = 1; i < npnts; ++i) {
      *cptr++ = (x += (++pnts)->x);
      *cptr++ = (y += pnts->y);
    }
    
    w_ppoly(Board, npnts, coords);
    free(coords);
  }
}

/*
 *		Draw a patterned polyline
 */

void wdpolyline(Point *pnts, int npnts)
{
  short x0, y0, x1, y1;
  int i;
    
  x0 = pnts->x;
  y0 = pnts->y;
    
  for (i = 1; i < npnts; ++i) {
    x1 = x0 + (++pnts)->x;
    y1 = y0 + pnts->y;

    if (x0 == x1) {
      w_dvline(Board, x0, y0, y1);
    } else if (y0 == y1) {
      w_dhline(Board, x0, y0, x1);
    } else {
      w_dline(Board, x0, y0, x1, y1);
    }

    x0 = x1;
    y0 = y1;
  }
}

/*
 *		Draw a patterned polygon
 */

void wdpolygon(Point *pnts, int npnts)
{
  short *coords = (short *)malloc(sizeof(short)*npnts*2);
  
  if (coords) {
    short *cptr = coords;
    short x, y;
    int i;
    
    *cptr++ = x = pnts->x;
    *cptr++ = y = pnts->y;
    
    for (i = 1; i < npnts; ++i) {
      *cptr++ = (x += (++pnts)->x);
      *cptr++ = (y += pnts->y);
    }
    
    w_dppoly(Board, npnts, coords);
    free(coords);
  }
}

/*
 *		Blit a BITMAP to the Board at (x,y).
 */

void wputblock(BITMAP *bm, short x, short y)
{
  w_putblock(bm, Board, x, y);
}
