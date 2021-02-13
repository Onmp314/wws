/*
 * wchars.c, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- show characters in the given W font
 */

#include <stdio.h>
#include <stdlib.h>
#include <Wlib.h>

static short xsize, ysize;		/* (maximum) char size */

#define WIN_FLAGS (W_MOVE | W_TITLE | W_CLOSE | EV_KEYS | EV_MOUSE)


static void show_key(WWIN *win, WFONT *font, int key)
{
  char msg[16];
  int x, y;

  if(key < 0 || key > 255)
    return;

  x = key % 32;
  y = key / 32;
  x = xsize * x + x + (xsize - font->widths[y * 32 + x]) / 2,
  y = ysize * y + y;

  w_pbox(win, x, y, font->widths[key], ysize);
  sprintf(msg, " char: %03d ", key);
  w_settitle(win, msg);
}

int main(int argc, char *argv[])
{
  int x, y, size, old = -1;
  ushort styles;
  char *family;
  WFONT *font;
  WEVENT *ev;
  WWIN *win;
  uchar msg;
 
  if((argc != 3 && argc != 4) || argv[1][0] == '-')
  {
    fprintf(stderr, "Show font character table\n");
    fprintf(stderr, "usage: %s <family> <size> [<styles: bilru>]\n", *argv);
    fprintf(stderr, "for example: %s fixed 14 b\n", *argv);
    return -1;
  }

  family = argv[1];
  size = atoi(argv[2]);

  styles = 0;
  if (argc == 4)
  {
    int idx = 0;
    for (;;)
    {
      switch(argv[3][idx++])
      {
	case 'b':
	  styles |= F_BOLD;
	  break;
	case 'i':
	  styles |= F_ITALIC;
	  break;
	case 'l':
	  styles |= F_LIGHT;
	  break;
	case 'r':
	  styles |= F_REVERSE;
	  break;
	case 'u':
	  styles |= F_UNDERLINE;
	  break;
	default:
	  /* sure, I'd like a multilevel break, but this is OK too... */
	  goto styles_done;
      }
    }
  }
styles_done:


  if(!w_init())
    return -1;

  if(!(font = w_loadfont(family, size, styles)))
  {
    fprintf(stderr, "%s: unable to load font %s\n", argv[0], argv[1]);
    return -1;
  }
  xsize = font->maxwidth;
  ysize = font->height;

  if (!(win = w_create(xsize * 32 + 31, ysize * 8 + 7, WIN_FLAGS)))
  {
    fprintf(stderr, "%s: unable to create an output window\n", *argv);
    return -1;
  }
  w_setmode(win, M_DRAW);
  w_setfont(win, font);

  /* character grid */
  for(y = 1; y < 8; y++)
    w_hline(win, 0, ysize * y + y - 1, win->width);
  for(x = 1; x < 32; x++)
    w_vline(win, xsize * x + x - 1, 0, win->height);

  /* characters */
  for(y = 0; y < 8; y++)
    for(x = 0; x < 32; x++)
      w_printchar(win,
        xsize * x + x + (xsize - font->widths[y * 32 + x]) / 2,
	ysize * y + y,
	32 * y + x);

  w_settitle(win, " ASCII-table ");
  if (w_open(win, UNDEF, UNDEF) < 0)
  {
    fprintf(stderr, "%s: unable to open an output window\n", *argv);
    w_delete(win);
    return -1;
  }

  w_setmode(win, M_INVERS);
  for(;;)
  {
    if((ev = w_queryevent(NULL, NULL, NULL, -1)))
    {
      switch(ev->type)
      {
        case EVENT_KEY:
	  show_key(win, font, old);		/* remove old mark */
	  show_key(win, font, ev->key);
	  old = ev->key;
	  break;

	case EVENT_MPRESS:
	  show_key(win, font, old);		/* remove old mark */
	  break;

	case EVENT_MRELEASE:
	  old = (ev->y / (ysize + 1)) * 32 + ev->x / (xsize + 1);
	  show_key(win, font, old); 
	  if(ev->key == BUTTON_RIGHT)
	  {
	    msg = old;
	    w_putselection(W_SEL_TEXT, &msg, 1);
	  }
	  break;

	case EVENT_GADGET:
	if(ev->key == GADGET_EXIT || ev->key == GADGET_CLOSE)
	{
	  w_unloadfont(font);
	  w_delete(win);
	  return 0;
	}
      }
    }
  }
  return 0;
}
