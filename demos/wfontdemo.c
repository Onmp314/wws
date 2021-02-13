/*
 * programs/wfontdemo.c, part of W (C) 94-02/96 by Torsten Scherer
 * (TeSche) itschere@techfak.uni-bielefeld.de
 *
 * Shows fonts in the given directory (provided that W server can load them).
 *
 * CHANGES
 * - lots and many, TeSche
 */

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <Wlib.h>


#define WIDTH 600
#define HEIGHT 350

#define TEXT_STYLES	6

static ushort Style[TEXT_STYLES] = {
  F_NORMAL,
  F_REVERSE,
  F_UNDERLINE,
  F_BOLD,
  F_LIGHT,
  F_ITALIC
};


static WWIN *getwindow(short width, short height)
{
  WSERVER *wserver;
  WWIN *win;

  if (!(wserver = w_init())) {
    return NULL;
  }

  if (!(win = w_create(width, height, W_MOVE|W_TITLE|W_CLOSE|EV_KEYS|EV_MOUSE))) {
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
  WWIN *win;
  DIR *dp;
  ushort styles;
  short idx, size;
  struct dirent *dentry;
  const char *family;
  char msg[80];
  WFONT *font;
  WEVENT *ev;

  if (argc != 2 || argv[1][0] == '-') {
    fprintf(stderr, "usage: %s <font directory>\n", *argv);
    return -1;
  }

  if (!(win = getwindow(WIDTH, HEIGHT))) {
    fprintf(stderr, "error: can't open window\n");
    return -1;
  }

  w_setmode(win, M_CLEAR);

  if ((dp = opendir(argv[1]))) {

    /* skip `.' and `..' */
    readdir(dp);
    readdir(dp);

    while ((dentry = readdir(dp))) {
      family = w_fonttype(dentry->d_name, &size, &styles);
      if ((font = w_loadfont(family, size, styles))) {
	sprintf(msg, "%s: Fontdemo ~123@#", dentry->d_name);
	w_setfont(win, font);

	for (idx = 0; idx < TEXT_STYLES; idx++) {
	  w_settextstyle(win, Style[idx]);
	  w_vscroll(win, 0, font->height, WIDTH, HEIGHT - font->height, 0);
	  w_pbox(win, 0, HEIGHT - font->height - 1, WIDTH, font->height);
	  w_printstring(win, 0, HEIGHT - font->height - 1, msg);
	}
	w_unloadfont(font);

	if ((ev = w_queryevent(NULL, NULL, NULL, 2000))) {
	  if ((ev->type == EVENT_GADGET) &&
	      ((ev->key == GADGET_EXIT) || (ev->key == GADGET_CLOSE))) {
	    return -1;
	  }
	}
      } else {
        fprintf (stderr, "unable to load font: '%s'\n", dentry->d_name);
      }
    }

    closedir(dp);
  }

  w_settitle(win, " wfontdemo - click anywhere to dismiss ");

  w_queryevent(NULL, NULL, NULL, -1);
  w_delete(win);

  return 0;
}
