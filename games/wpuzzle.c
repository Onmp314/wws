/*
 * wpuzzle.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Jens Kilian, Torsten Scherer, Simon Kagedal
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- an interactive puzzle game.  Inspired by an X client program.
 *
 *    Contributors:
 *	Jens Kilian	(original author)
 *	Torsten Scherer	(image reading functions)
 *	Simon Kagedal	(multiple moves)
 *
 *    $Log: wpuzzle.c,v $
 *    Revision 1.2  1999/05/16 13:09:55  eero
 *    add static keywords
 *
 *    Revision 1.1.1.1  1998/11/01 17:03:27  eero
 *    Entered W window system to CVS
 *
 *    Revision 1.5  1996/02/05  16:10:00  tesche
 *    w_readimg() moved into Wlib
 *
 *    Revision 1.4  1996/01/25  20:59:22  root
 *    Speed up multiple moves.
 *
 *    Revision 1.3  1996/01/24  17:47:12  root
 *    Fix that RCS ID ...
 *
 *    Revision 1.2  1996/01/24  17:38:16  root
 *    Added multiple-move enhancements by Simon Kagedal.
 *
 *    Revision 1.1  1996/01/24  17:12:52  root
 *    Initial revision
 */

static const char rcsid[] = "$Id: wpuzzle.c,v 1.2 1999/05/16 13:09:55 eero Exp $";

#include <Wlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/*  Makes a position in the puzzle field from coordinates. (/simon) */

#define	POS(x,y) (N*(y)+(x))


/*
 *  Allocate & initialize the puzzle.
 */

static int *init_puzzle(int N)
{
  int size = N*N;
  int i, *puzzle, x, y, m, n;
  
  /* Allocate memory. */
  
  if ((puzzle = malloc(sizeof(int)*size)) == 0) {
    return 0;
  }
  
  /* Initialize to solved state. */
  
  for (i = 0; i < size-1; ++i) {
    puzzle[i] = i+1;
  }
  puzzle[n = (size-1)] = 0;
  x = y = N-1;
  m = 0;
  
  /* Perform random moves. */
  
  srand(time(0));
  for (i = 0; i < N*N*100;) {
    switch (rand() & 3) {
      
    case 0:
      if (y > 0) m = N*--y + x; else continue;
      break;
      
    case 1:
      if (x > 0) m = N*y + --x; else continue;
      break;
      
    case 2:
      if (x<N-1) m = N*y + ++x; else continue;
      break;
      
    case 3:
      if (y<N-1) m = N*++y + x; else continue;
      break;
    }
    
    puzzle[n] = puzzle[m];
    puzzle[n = m] = 0;
    ++i;
  }
  
  return puzzle;
}

/*
 *  Draw the bitmap version of the puzzle.
 */

static void bitmap_puzzle(WWIN *window, WWIN *bm, int bm_width, int bm_height,
			int N, int x_size, int y_size, int *puzzle)
{
  int i, size = N*N;
  int xoff = (N*x_size - bm_width)/2;
  int yoff = (N*y_size - bm_height)/2;

  /* Clear the window. */

  w_setmode(window, M_CLEAR);
  w_pbox(window, 0, 0, N*x_size, N*y_size);
  w_setmode(window, M_DRAW);

  /* Draw the cells. */

  for (i = 0; i < size; ++i) {
    int x0 = (i % N)*x_size;
    int y0 = (i / N)*y_size;
    
    if (puzzle[i]) {
      int x1 = ((puzzle[i]-1) % N)*x_size - xoff;
      int y1 = ((puzzle[i]-1) / N)*y_size - yoff;
      int width = x_size, height = y_size;

      if (x1 < 0) {
	width += x1;
	x0 -= x1;
	x1 = 0;
      } else if (x1+width > bm_width) {
	width = bm_width - x1;
      }
      if (y1 < 0) {
	height += y1;
	y0 -= y1;
	y1 = 0;
      } else if (y1+height > bm_height) {
	height = bm_height - y1;
      }
      w_bitblk2(bm, x1, y1, width, height, window, x0, y0);

    } else {
      w_pbox(window, x0, y0, x_size, y_size);
    }
  }
}

/*
 *  Draw the text version of the puzzle.
 */

static void text_puzzle(WWIN *window, WFONT *wfont, int N,
			int x_size, int y_size, int *puzzle)
{
  int i, size = N*N, width;
  char line[64];
  
  /* Clear the window. */
  
  w_setmode(window, M_CLEAR);
  w_pbox(window, 0, 0, N*x_size, N*y_size);
  w_setmode(window, M_DRAW);

  /* Draw the cells. */

  w_setfont(window, wfont);
  
  for (i = 0; i < size; ++i) {
    int x0 = (i % N)*x_size;
    int y0 = (i / N)*y_size;
    
    if (puzzle[i]) {
      w_box(window, x0, y0, x_size, y_size);
      sprintf(line, "%d", puzzle[i]);
      width = w_strlen(wfont, line);
      w_printstring(window,
		    x0 + (x_size-width)/2,
		    y0 + (y_size-wfont->height)/2,
		    line);
    } else {
      w_pbox(window, x0, y0, x_size, y_size);
    }
  }
}

/*
 *  Main event loop.
 */

static void do_puzzle(WWIN *window, int N, int x_size, int y_size, int *puzzle)
{
  WEVENT *ev;
  int stop = 0;
  int x, y, dx, dy, l, i, from_x, from_y, to_x, to_y, width, height;
  
  while (!stop) {
    ev = w_queryevent(NULL, NULL, NULL, -1);

    switch (ev->type) {

    case EVENT_MPRESS:
      x = ev->x / x_size;
      y = ev->y / y_size;

      /* Determine if move is legal and which direction it takes. */

      dx = dy = l = 0;

      for (i = 0; i < N; ++i) {
	if ((!puzzle[POS(x,i)]) && (i != y)) { /* Vertical move? */
	  l  = (y > i) ? y - i : i - y;
	  dy = (y > i) ? -1 : 1;
	  break;
	}
	if ((!puzzle[POS(i,y)]) && (i != x)) { /* Horizontal move? */
	  l  = (x > i) ? x - i: i - x;
	  dx = (x > i) ? -1 : 1;
	  break;
	}
      }

      if (l) {
	/* Perform the move. */
	
	for (i = l; i > 0; --i) {
	  from_x = (to_x = x + (dx*i)) - dx;
	  from_y = (to_y = y + (dy*i)) - dy;

	  puzzle[POS(to_x,to_y)] = puzzle[POS(from_x,from_y)];
	  puzzle[POS(from_x,from_y)] = 0;
	}

	/* Animate. */

	x = x_size * (x - (dx<0)*(l-1));
	y = y_size * (y - (dy<0)*(l-1));
	width  = x_size * (dx ? l : 1);
	height = y_size * (dy ? l : 1);

	for (i = (dx ? x_size : y_size); i > 0; --i) {
	  w_bitblk(window, x, y, width, height, x+dx, y+dy);
	  if (dx) {
	    w_vline(window, (dx<0) ? x+width-1 : x, y, y+height-1);
	  } else {
	    w_hline(window, x, (dy<0) ? y+height-1 : y, x+width-1);
	  }
	  x += dx;
	  y += dy;
	}

	/* Check if the puzzle is solved. */
	
	for (i = 0; i < N*N-1; ++i) {
	  if (puzzle[i] != i+1) {
	    break;
	  }
	}
	if (i == N*N-1) {
	  w_settitle(window, "SOLVED!");
	}
      } else {
	w_beep();
      }
      break;

    case EVENT_KEY:
      if ((ev->key & 0xff) == 'q') {
	stop = 1;
      }
      break;

    case EVENT_GADGET:
      if ((ev->key == GADGET_EXIT) ||
	  (ev->key == GADGET_CLOSE)) {
	stop = 1;
      }
      break;
    }
  }
}

/*
 *  Main program - window system initialization & termination.
 */

static void usage(char *progname)
{
  fprintf(stderr, "Usage: %s [-n size] [-f font family] [-s font size] [IMG file]\n", progname);
}

int main(int argc, char **argv)
{
  char *fname = NULL;	/* use defaults */
  short fsize = 0;
  int N = 4;
  int *puzzle;
  char line[64];
  int x_size, y_size;
  int i;
  char opt, *arg;
  BITMAP *bm;
  short bm_width, bm_height;
  WSERVER *wserver;
  WFONT *wfont;
  WWIN *window, *image = NULL;
  
  /* Parse command line arguments. */
  
  i = 1;
  while ((i < argc) && (argv[i][0] == '-')) {
    opt = argv[i][1];
    if (argv[i][2] != '\0') {
      arg = &argv[i][2];
    } else if (++i < argc) {
      arg = argv[i];
    } else {
      usage(argv[0]);
      return 1;
    }

    switch (opt) {

    case 'n':                /* set size of puzzle */
      N = strtol(arg, &arg, 10);
      if ((N < 2) || (*arg != '\0')) {
	usage(argv[0]);
	return 1;
      }
      break;

    case 'f':                /* set font family */
      fname = arg;
      break;

    case 's':                /* set font size */
      fsize = atoi(arg);
      break;

    default:
      usage(argv[0]);
      return 1;
    }

    ++i;
  }

  if (i < argc) {
    /* Try to load bitmap file. */
    if ((bm = w_readimg(argv[i], &bm_width, &bm_height)) == NULL) {
      return 1;
    } else if ((bm_width < N) || (bm_height < N)) {
      fprintf(stderr, "%s: image too small, must be at least %dx%d\n",
	      argv[0], N, N);
      w_freebm(bm);
      return 1;
    }
  } else {
    bm = 0;
  }

  /* Set up connection to server. */
  
  if ((wserver = w_init()) == 0) {
    fprintf(stderr, "%s: can't connect to wserver\n", argv[0]);
    return 1;
  }
  
  /* Calculate cell size. */
  
  if (bm) {
    wfont = 0;
    x_size = (bm_width+N-1)/N;
    y_size = (bm_height+N-1)/N;

  } else {
    if ((wfont = w_loadfont(fname, fsize, 0)) == 0) {
      fprintf(stderr, "%s: can't load font '%s%d'\n", argv[0], fname, fsize);
      return 1;
    }
    
    sprintf(line, "%d", N*N-1);
    x_size = y_size = w_strlen(wfont, line) + 8;
  }

  /* Try to create the window(s), then initialize & open it. */
  
  window = w_create(N*x_size, N*y_size, (W_MOVE|W_TITLE|W_CLOSE|EV_MOUSE|EV_KEYS));
  if (window == 0) {
    fprintf(stderr, "%s: can't create window\n", argv[0]);
    return 1;
  }
  w_settitle(window, "WPuzzle");

  if(bm) {
    if (!(image = w_create(bm_width, bm_height, 0))) {
      fprintf(stderr, "%s: can't create bitmap\n", argv[0]);
      return 1;
    }
    w_putblock(bm, image, 0, 0);
    w_freebm(bm);
  }
  
  if(w_open(window, UNDEF, UNDEF) < 0) {
    fprintf(stderr, "%s: can't open window\n", argv[0]);
    return 1;
  }
  
  /* Initialize the puzzle. */
  
  if ((puzzle = init_puzzle(N)) == 0) {
    fprintf(stderr, "%s: can't allocate memory\n", argv[0]);
    return 1;
  }

  /* Draw the puzzle. */

  if (image) {
    bitmap_puzzle(window, image, bm_width, bm_height, N, x_size, y_size, puzzle);
    w_delete(image);
  } else {
    text_puzzle(window, wfont, N, x_size, y_size, puzzle);
  }
		  
  /* Play the game. */
  
  do_puzzle(window, N, x_size, y_size, puzzle);
  
  /* Exit. */
  
  w_close(window);
  w_delete(window);
  w_exit();
  return 0;
}
