/*
 * wcpyrgt.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- W copyright notice
 *
 * CHANGES
 * ++eero 2/98:
 * - Uses my new bitmap functions and can be quit with a mouse click
 *   or keypress (after a while this notice comes a bit too familiar).
 * - calculates string position so you can change bitmap and fonts.
 * ++eero 3/03:
 * - Update the version number and copyright
 */

#include <stdio.h>
#include <string.h>
#include <Wlib.h>

/* so that it will fit even on the smallest screen... */
#define WIDTH 312
#define HEIGHT 200

#define Center(fn,s)	((WIDTH - w_strlen(fn,s)) / 2)

/*
 * a picture...
 */

static const char *pic[] = {
  "72 72 2 1",
  "# c #ffffff",
  "  c #000000",
  "##########   #   #   #####################           #    ##############",
  "#######     # # # # ##    ############    # # # # # # # # # #   ########",
  "#####          #       #    #######            #       #         #######",
  "####    # # # # # # # # # # ###   # # # # # # # # # # # # # # #   ######",
  "###      # # #   # # #   #       #   #   #   #   # # ##  #   #     #####",
  "##    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #   ####",
  "##     #   #   # # #   #   #       #   #   #   # # #   #   #   #     ###",
  "#   # # # # # # # # ##  # # # # # # # # # # # # # # ### # # # # # #  ###",
  "#    # # # # # # # # #   #   # # # # # # # # # # # # # # # # # # #   ###",
  "# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #  ###",
  "       #           #   #   #         # # # # # # #         # # #     ###",
  "  # # #           # # # # #           # ### ### #           ### # #  ###",
  "   # #             # # # # #           # # # # # #           # # #    ##",
  "  # # #           ### # # #            ## ### ### #           # # #   ##",
  "   # #             #   # # #           # # # # #   #         # # #   ###",
  "  # ###           ##### ### #          ########## # #        ## #   ####",
  " # # # #           ### # # #           # # ### # # # #       # #   # ###",
  "  # # ###         ####### ###          ########## # #        #### # ####",
  "   # # # #         # # # # # #         # # # ### # #         # # # #####",
  "  # ### ##          ######### #        ############ # #      ## #   ####",
  " # # # # #         ### # # # #          ## ### # # # #       # # #  ####",
  "  # # ##            ######### ##        ######### # # #      ## #   ####",
  "   # #   #         ### # # # # #         ### ### # #           #    ####",
  "  # ### # #         ###########         #########     #      ## #    ###",
  " # # #   #         # ##### # # #         ###         #       # # #   ###",
  "  # ##  # ##        ########### #               # # # #       ###    ###",
  "   # # #   #         ### ##### #     #         #   #   #     # # #   ###",
  "  # # # # ##        ########### # #     # # # # # # # #      ## #    ###",
  " # #     # #         ######### #         # # # # # # #       # # #   ###",
  "  ### # # ##          ########            # # # # # # #       ###    ###",
  "   #   # # #         ######                # # # # # # #     # # # # ###",
  "# #   # ### #        ####     # #         # ### ### ###     #####   ####",
  "#  # # # # #         ###     # # #         # # # # # # #    ## # #   ###",
  "# #   # # ####        ### # # # #           # ### #####     #####   ####",
  "#      # # # #         #   # # # #         # # # # # # #     # #   # ###",
  "# # # # ######        # # # # # ###         ###########     ##### # # ##",
  "##   # # # # #         # # # # # #           # # # ### #   ### #   #  ##",
  "### # # # #####        ## # # # #           ############   ###### # # ##",
  "##     # # # #         # # # # # #           # # ### #     ### # # #  ##",
  "##  # # ##### #        #### ######           ##########   #####  ## # ##",
  "###  # # #   # #       # # # # # #           # #######     ### # #    ##",
  "### # # #   # #         # #######             ########    ####### # # ##",
  "###    #   #   #        ## # # #              ########   #####   # #  ##",
  "### # #   # # # #        ########   #         #######    ########## # ##",
  "#### # # # # # #         # ###     #          #######    ##### # # #  ##",
  "##### # # # # # #         #####   # #          #####    # ###  #### # ##",
  "#####      # # # #        ## #     #           #####   # ##### # # #  ##",
  "##### # # # ######         ##   # # #           ###   # ##### ##### # ##",
  "#####  # # # # # #               # # #          #    # # ####  ### #  ##",
  "####  # # # # ####              # # # #               ######  ##### # ##",
  "####   # # # # # ##            # # # #               # ####  ### # ## ##",
  "###   # ### #######           # #######             ######  #######  ###",
  "###  # # # # # ###           # # # # #             # # ### # ##### # ###",
  "### # # # ##########        # # # ### #           # # ### ####### ## ###",
  "##     # # # # # ###       # # # # # # #         # # # # # ##### # # ###",
  "##  # # #############   # # ### #######       # # # ### ### #######  ###",
  "##   # # # ### ######    # # # # # ### #     # # # # # # # ##### #   ###",
  "##  # ### ############# # # # ########### # # # # ### # # #######   ####",
  "##     # # # ######### # # # # # ##### #   # # # # #   # ####### #  ####",
  "###  ## ############### ### ############# # # # ### # # #########  #####",
  "###    # # ########### # # # # ####### # # # # #   # # # #######   #####",
  "####   ################ # ############### ### # # # # ##########   #####",
  "####     ############### # # # ######### # #   # # # # # ######  #######",
  "#####     ############# ################### # # # # ##########   #######",
  "######        ########## # ############  #   # #   # # #####     #######",
  "#######           ###############     # # # # ### #           ##########",
  "###### #       #               #       # # # # #   #       # # #########",
  "####### # # # # # # # # # # # # # # # # ### ##### # # # # # # # ########",
  "######## # # # # # # # # # # ### # # # # # # # # # # # # # # # #########",
  "######### ### # # # # # # # # # ### # ### ### ### ### # # ### ##########",
  "########## # # # # # # # # # # # # # # # # # # # # # # # # # ###########",
  "################### ####### ####################### ####################"
};




/*
 * what we've going to export...
 */

int main (int argc, char *argv[])
{
  short x0, y0, wd, ht;
  short ewidth, eheight, x, y, i;
  WFONT *tiny, *small, *medium, *big;
  WSERVER *wserver;
  WWIN *win;
  WEVENT *event;
  BITMAP *grafik;
  const char *msg;
  char buf[40];

  if (!(grafik = w_xpm2bm(pic)))
    return -1;

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: cpyrgt can't connect to wserver\n");
    return -1;
  }

  tiny   = w_loadfont("cour", 10, 0);
  small  = w_loadfont("lucidat", 11, 0);
  medium = w_loadfont("lucidat", 15, 0);
  big    = w_loadfont("lucidat", 27, 0);

  if (!(tiny && small && medium && big)) {
    fprintf(stderr, "error: cpyrgt can't load (all) fonts\n");
    return -1;
  }

  if (!(win = w_create(WIDTH, HEIGHT, W_TOP|EV_MOUSE|EV_KEYS))) {
    fprintf(stderr, "error: cpyrgt can't create window\n");
    return -1;
  }

  for (x = 0; x < WIDTH; x++)
    w_line(win, x, 0, WIDTH-1-x, HEIGHT-1);
  for (y = 0; y < HEIGHT; y++)
    w_line(win, 0, y, WIDTH-1, HEIGHT-1-y);

  w_setmode(win, M_CLEAR);
  w_pbox(win, 20, 16, WIDTH-40, HEIGHT-32);
  y = 16;

  w_setfont(win, small);
  msg = "welcome to";
  w_printstring(win, Center(small, msg), y, msg);
  y += small->height;

  w_setmode(win, M_DRAW);
  w_putblock(grafik, win, (WIDTH - grafik->width) / 2, y);
  y += grafik->height;
  
  w_setfont(win, big);
  sprintf(buf, "W %d Release %d.%d", _WMAJ, _WMIN, _WPL);
  w_printstring(win,  Center(big, buf), y, buf);
  y0 = y;
  x0 = 20;
  y += big->height;
  wd = WIDTH - 2 * x0;
  ht = y - y0;

  w_setfont(win, tiny);
  msg = "Copyright (C)";
  w_printstring(win, Center(tiny, msg), y, msg);
  y += tiny->height;

  w_setfont(win, small);
  msg = "1994-1998 by Torsten Scherer";
  w_printstring(win, Center(small, msg), y, msg);
  y += small->height;

  msg = "1994-1998 by Kay Römer";
  w_printstring(win, Center(small, msg), y, msg);
  y += small->height;

  msg = "1995-2000,2003 by Eero Tamminen";
  w_printstring(win, Center(small, msg), y, msg);
  y += small->height;

  w_setfont(win, tiny);
  msg = "Licensed under GPL/LPGL";
  w_printstring(win, Center(tiny, msg), y, msg);
  y += tiny->height;

  w_querywinsize(win, 1, &ewidth, &eheight);
  w_open(win, (wserver->width-ewidth)/2, (wserver->height-eheight)/2);
  w_flush();

  w_setmode(win, M_INVERS);
  for (i = 0; i < 24; i++) {
    if ((event = w_queryevent(NULL, NULL, NULL, 500))) {
      return 0;
    }
    w_pbox(win, x0, y0, wd, ht);
  }

  return 0;
}
