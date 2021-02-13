/*
 * snails.c, a part of the W Window System
 *
 * Copyright (C) 1994-1996 by Torsten Will (itwill@techfak...)
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- WSaver module: 'snails'
 *
 * CHANGES
 * ++eero 3/98:
 * - changed into a module for the new wsaver.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Wlib.h>
#include "wsaver.h"


#define	MAXPOINTS 150
#define STEP 0.05
#define LOOP 20


typedef struct {
  short x, y;
} POINT;


static POINT point[MAXPOINTS];
static short offsx, offsy,	/* Offset to screen middle */
       hwidth, hheight, 	/* half dimensions (aspect ratio!) */
       idx;			/* index to snail head */
static float u,v,w;
static short mirror = 3;	/* 0=no, 1=x, 2=y, 3=xy */

/* Global function that moves the points 
 *   p:     point to calculate
 *   u,v,w: depending angels; will be changed to new u,v,w
 */

static void func (POINT* p, float* u, float* v, float* w)
{
  static float x,y;

  /* calc point (-1.0 .. +1.0) */
  x = qcos (*w) * qsin (*u);
  y = qsin (*w) * qcos (*v);

  /* scale to screen */
  p->x = (short) (x * hwidth)  + offsx;
  p->y = (short) (y * hheight) + offsy;

  /* new angels */
  *w = *w + STEP;
  *v = *v + qsin(*w)/100;
  *u = *u + qcos(*v)/100;
} 

static void movepoints(void)
{
  short newhead;
  short newtail;

  newhead = (idx+1) % MAXPOINTS;
  newtail = (idx+2) % MAXPOINTS;

  /* delete the last line */

  w_setmode(win, M_DRAW);
  w_line(win, point[newhead].x, point[newhead].y, 
	 point[newtail].x, point[newtail].y);
  if (mirror==1)
    w_line(win, swidth-point[newhead].x, point[newhead].y, 
	   swidth-point[newtail].x, point[newtail].y);
  else if (mirror==2)
    w_line(win, point[newhead].x, sheight-point[newhead].y, 
	   point[newtail].x, sheight-point[newtail].y);
  else if (mirror==3)
    w_line(win, swidth-point[newhead].x, sheight-point[newhead].y, 
	   swidth-point[newtail].x, sheight-point[newtail].y);


  /* calculate new pos */

  func (&point[newhead], &u,&v,&w);

  /* then draw new line */

  w_setmode(win, M_CLEAR);
  w_line(win, point[idx].x,     point[idx].y, 
	 point[newhead].x, point[newhead].y);
  if (mirror==1)
    w_line(win, swidth-point[idx].x,     point[idx].y, 
	   swidth-point[newhead].x, point[newhead].y);
  else if (mirror==2)
    w_line(win, point[idx].x,     sheight-point[idx].y, 
	   point[newhead].x, sheight-point[newhead].y);
  else if (mirror==3)
    w_line(win, swidth-point[idx].x,     sheight-point[idx].y, 
	   swidth-point[newhead].x, sheight-point[newhead].y);

  /* store new head */

  idx = newhead;
}

void save_snails(void)
{
  short	l;
  WEVENT *ev;
  float	iu,iv,iw;

  hwidth  = MIN(swidth, sheight) /2;
  hheight = hwidth;                     /* make dawing qudratic */
  offsx   = swidth / 2;
  offsy   = sheight / 2;

  iu = (random() % 31415) / 10000;
  iv = (random() % 31415) / 10000;
  iw = (random() % 31415) / 10000;

  for (idx=0; idx<MAXPOINTS; idx++) {
    u= iu;
    v= iv;
    w= iw;
    func(&point[idx], &u, &v, &w);
  };
  idx--;

  while (42) {

    if ((ev = w_queryevent(NULL, NULL, NULL, timeout))) switch (ev->type) {

    case EVENT_GADGET:
      if (ev->key == GADGET_EXIT) {
	w_exit();
	exit(0);
      }
      break;

    default:
      return;
    }

    for (l=0; l<LOOP; l++) {
      movepoints();
    }
  }
}
