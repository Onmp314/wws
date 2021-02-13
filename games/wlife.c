/*
 * wlife.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a Conway's Life 'game'
 *
 * CHANGES
 * ++TeSche, 12/95:
 * - added variable fields
 * ++TeSche, 01/96:
 * - new button concept
 * ++Eero, 10/97:
 * - use w_buttonqueryevent() instead of w_queryevent()
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Wlib.h>


/*
 *
 */

typedef struct {
  int width, height;
  char *data;
} FIELD;


/*
 * some global variables
 */

static WWIN *glob_win, *buttonQuit, *buttonCont, *buttonInt;
static int glob_yoffset, glob_terminate;


/*
 * allocField(): allocate a field and initialize its cells to zero. internally
 * we make the field two cells bigger in each dimension and 'mirror' over the
 * border when a cell at the edge of the real field is set. this way we can
 * save a lot of boundary checks when calculating the neighbours of a pixel.
 */

static FIELD *allocField(int width, int height)
{
  FIELD *ret;
  int size;

  if (!(ret = malloc(sizeof(FIELD))))
    return NULL;

  width += 2;
  height += 2;

  size = width * height;
  if (!(ret->data = malloc(size))) {
    free(ret);
    return NULL;
  }

  memset(ret->data, 0, size);

  ret->width = width;
  ret->height = height;

  return ret;
}


static inline void copyField(FIELD *dest, FIELD *source)
{
  memcpy(dest->data, source->data, source->width * source->height);
}


/*
 * indices to these functions here are within the original range!
 */

#define XLO 0x0001
#define XHI 0x0002
#define YLO 0x0004
#define YHI 0x0008

static inline void set(register FIELD *dest,
		register int x,
		register int y,
		register char val)
{
  register int idx = (y + 1) * dest->width + x + 1;
  register char *data = dest->data;
  register int edges;

  data[idx] = val;

  if ((edges = (!x ? XLO : 0) | ((x == (dest->width - 3)) ? XHI : 0) |
       (!y ? YLO : 0) | ((y == (dest->height - 3)) ? YHI : 0))) {

    switch (edges) {

      case XLO | YLO:
        data[idx + dest->width - 2] = val;
	idx += (dest->height - 2) * dest->width;
	data[idx] = val;
        data[idx + dest->width - 2] = val;
	break;

      case YLO:
	idx += (dest->height - 2) * dest->width;
	data[idx] = val;
	break;

      case XHI | YLO:
	data[dest->width] = val;
	idx += (dest->height - 2) * dest->width;
	data[idx] = val;
	idx -= dest->width - 2;
	data[idx] = val;
	break;

      case XHI:
	idx -= dest->width - 2;
	data[idx] = val;
	break;

      case XHI | YHI:
	data[0] = val;
	data[dest->width - 2] = val;
	idx -= dest->width - 2;
	data[idx] = val;
	break;

      case YHI:
	data[x + 1] = val;
	break;

      case XLO | YHI:
	data[1] = val;
	data[dest->width - 1] = val;
	idx += dest->width - 2;
	data[idx] = val;
	break;

      case XLO:
	idx += dest->width - 2;
	data[idx] = val;
	break;
    }
  }
}


static inline char isSet(FIELD *dest, int x, int y)
{
  return dest->data[(y + 1) * dest->width + x + 1];
}



static inline int numNeighbours(register FIELD *dest,
			 register int x,
			 register int y)
{
  register int bpl = dest->width;
  register char *data = dest->data + y * bpl + x;   /* upper left */
  register int n = 0;

  n = *data++;
  n += *data++;
  n += *data;
  data += bpl;
  n += *data;
  data += bpl;
  n += *data--;
  n += *data--;
  n += *data;
  data -= bpl;
  n += *data;

  return n;
}


/*
 * the main 'life' routine
 */

static void do_life(FIELD *field, int width, int height)
{
  WEVENT *wevent;
  int end, x, y;
  static FIELD *copy = NULL;

  if (!copy) {
    if (!(copy = allocField(width, height))) {
      fprintf(stderr, "error: out of memory\n");
      exit(-1);
    }
  }

  w_setmode(glob_win, M_INVERS);

  end = 0;
  while (!end) {

    /* compute new field */
    for (y=0; y<height; y++) {
      for (x=0; x<width; x++) {
	switch (numNeighbours(field, x, y)) {
	  case 2:
	    set(copy, x, y, isSet(field, x, y));
	    break;
	  case 3:
	    set(copy, x, y, 1);
	    break;
	  default:
	    set(copy, x, y, 0);
	    break;
	  }
      }
    }

    /* update screen */
    for (y=0; y<height; y++) {
      for (x=0; x<width; x++) {
	if (isSet(copy, x, y) != isSet(field, x, y)) {
	  w_pbox(glob_win, 4*x, 4*y + glob_yoffset, 4, 4);
	}
      }
    }

    /* update field */
    copyField(field, copy);

    /* query interrupts */
    if ((wevent = w_querybuttonevent(NULL, NULL, NULL, 0))) {
      if (wevent->type == EVENT_BUTTON) {
	if (wevent->win == buttonInt) {
	  end = 1;
	} else if (wevent->win == buttonQuit) {
	  end = 1;
	  glob_terminate = 1;
	}
      }

      if ((wevent->type == EVENT_GADGET) &&
	  ((wevent->key == GADGET_EXIT) || (wevent->key == GADGET_CLOSE))) {
	end = 1;
	glob_terminate = 1;
      }
    }
  }
}


/*
 * usage()
 */

static void usage(void)
{
  fprintf(stderr, "usage: wlife [-g <geometry>]\n");
  exit(-1);
}


/*
 * guess what...
 */

int main(int argc, char *argv[])
{
  short width = 64, height = 40, winw, winh, cx, cy;
  WEVENT *wevent;
  WFONT *wfont;
  FIELD *field;

  signal(SIGTTOU, SIG_IGN);

  /*
   *
   */

  switch (argc) {
    case 3:
      width = 64;
      height = 40;
      scan_geometry(argv[2], &width, &height, NULL, NULL);
      if (width < 64) {
	width = 64;
      }
      if (height < 40) {
	height = 40;
      }
      /* fall thru */
    case 1:
      break;

    default:
      usage();
  }

  /*
   *
   */

  if (!w_init()) {
    fprintf(stderr, "fatal: wlife can't connect to wserver\n");
    exit(-1);
  }

  if (!(wfont = w_loadfont("fixed", 8, 0))) {
    fprintf(stderr, "error: wlife can't load font\n");
    exit(-1);
  }

  glob_yoffset = wfont->height + 5;
  winw = width * 4;
  winh = height * 4 + glob_yoffset;

  if (!(field = allocField(width, height))) {
    fprintf(stderr, "error: out of memory\n");
    exit(-1);
  }

  if (!(glob_win = w_create(winw, winh, W_MOVE | W_TITLE | W_CLOSE | EV_MOUSE))) {
    fprintf(stderr, "error: wlife can't create window\n");
    exit(-1);
  }

  w_settitle(glob_win, " wlife, (C) 12/95 by TeSche ");

  if (w_open(glob_win, UNDEF, UNDEF) < 0) {
    w_delete(glob_win);
    fprintf(stderr, "error: wlife can't open window\n");
    exit(-1);
  }

  buttonQuit = w_createButton(glob_win,   0, 0, 50, 9);
  buttonCont = w_createButton(glob_win,  58, 0, 90, 9);
  buttonInt =  w_createButton(glob_win, 156, 0, 90, 9);
  if (!buttonQuit || !buttonCont || !buttonInt) {
    fprintf(stderr, "error: wlife can't create buttons\n");
    exit(-1);
  }

  w_centerPrints(buttonQuit, wfont, "quit");
  w_centerPrints(buttonCont, wfont, "continue");
  w_centerPrints(buttonInt, wfont, "interrupt");

  w_showButton(buttonQuit);
  w_showButton(buttonCont);
  w_showButton(buttonInt);

  glob_terminate = 0;
  while (!glob_terminate) {

    if ((wevent = w_querybuttonevent(NULL, NULL, NULL, -1))) switch (wevent->type) {

      case EVENT_BUTTON:
        if (wevent->win == buttonCont) {
	  do_life(field, width, height);
	} else if (wevent->win == buttonQuit) {
	  glob_terminate = 1;
	}

      case EVENT_MPRESS:
	if (wevent->y > glob_yoffset) {
	  cx = wevent->x / 4;
	  cy = (wevent->y - glob_yoffset) / 4;
	  switch (wevent->key) {

	    case BUTTON_LEFT:
	      set(field, cx, cy, 1);
	      w_setmode(glob_win, M_DRAW);
	      w_pbox(glob_win, 4*cx, 4*cy + glob_yoffset, 4, 4);
	      break;

	    case BUTTON_RIGHT:
	      set(field, cx, cy, 0);
	      w_setmode(glob_win, M_CLEAR);
	      w_pbox(glob_win, 4*cx, 4*cy + glob_yoffset, 4, 4);
	      break;
	  }
	}
	break;

      case EVENT_GADGET:
	if ((wevent->key == GADGET_EXIT) ||
	    (wevent->key == GADGET_CLOSE)) {
	  glob_terminate = 1;
	}
	break;
      }
  }

  return 0;
}
