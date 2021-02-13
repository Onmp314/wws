/*
 * programs/wfontdemo.c, part of W
 * (C) 94-02/96 by Torsten Scherer (TeSche)
 * itschere@techfak.uni-bielefeld.de
 *
 * CHANGES
 * ++eero, 11/96:
 * - Copied wfontdemo code and changed it to show only selected fonts
 * - Changed this to show fonts in several (new) styles and
 *   to open one window / font and only as large as is needed
 *   (to test font clipping).
 */

#include <stdio.h>
#include <string.h>
#include <Wlib.h>

#define TEXT_STYLES	9

static ushort Style[TEXT_STYLES] = {
  F_NORMAL,
  F_REVERSE,
  F_UNDERLINE,
  F_BOLD,
  F_LIGHT,
  F_ITALIC,
  F_ITALIC | F_BOLD,
  F_ITALIC | F_UNDERLINE,	/* check where underline starts/ends! */
  F_BOLD | F_LIGHT
};

#define WIN_FLAGS (W_MOVE | W_TITLE | W_CLOSE | EV_KEYS | EV_MOUSE)


static WWIN *getwindow(short width, short height)
{
  WWIN *win;

  if (!(win = w_create(width, height, WIN_FLAGS))) {
    return NULL;
  }

  w_settitle(win, " wfontdemo ");
  if (w_open(win, UNDEF, UNDEF) < 0) {
    w_delete(win);
    return NULL;
  }

  return win;
}


int main(int argc, char *argv[])
{
  char msg[80];
  const char *family;
  short idx, size;
  ushort styles;
  WFONT *font = NULL;
  WWIN *win = NULL;
  WSERVER *wserver;
  WEVENT *ev;

  if (argc < 2 || argv[1][0] == '-') {
    fprintf(stderr, "usage: %s <font-1 filename> [font-2]...\n", *argv);
    return -1;
  }

  if (!(wserver = w_init())) {
    return -1;
  }

  while (--argc) {

    family = w_fonttype(*++argv, &size, &styles);

    if ((font = w_loadfont(family, size, styles))) {

      idx = strlen(*argv);
      while(--idx >= 0 && (*argv)[idx] != '/');
      sprintf(msg, "%s: Fontdemo ~123@#", *argv + idx + 1);

      if (!(win = getwindow(w_strlen(font, msg),
          font->height * TEXT_STYLES + TEXT_STYLES + 2))) {
	fprintf(stderr, "error: can't open window\n");
	return -1;
      }
      w_setfont(win, font);
      for (idx = 0; idx < TEXT_STYLES; idx++) {
	w_settextstyle(win, Style[idx]);
	w_printstring(win, 0, font->height * idx + idx + 1, msg);
      }

      w_settitle(win, " click for next ");
      if ((ev = w_queryevent(NULL, NULL, NULL, -1))) {
	if ((ev->type == EVENT_GADGET) &&
	    ((ev->key == GADGET_EXIT) || (ev->key == GADGET_CLOSE))) {
	  return -1;
	}
      }
      w_unloadfont(font);
      w_delete(win);
    } else {
      fprintf(stderr, "unable to load font '%s'\n", *argv);
    }
  }
  return 0;
}
