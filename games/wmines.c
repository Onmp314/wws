/*
 * wmines.c, a part of the W Window System
 *
 * Copyright (C) 1995 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- minesweeper game clone for W
 */

#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* rand(), srand() */
#include <time.h>		/* time() */
#include <Wlib.h>

/* W specific stuff */

/* space lost for window borders horizontally and vertically */
#define HOR_BORDERS	8
#define VER_BORDERS	18
#define WIN_PROPERTIES	(W_MOVE | W_TITLE | W_CLOSE | EV_MOUSE | EV_KEYS)

#ifdef WTINYSCREEN
  #define BLOCK_W	10			/* block size */
  #define BLOCK_H	10
  #define WIDTH 	15			/* default field size */
  #define HEIGHT	14
  #define MIN_W		6			/* minimum field size */
  #define MIN_H		6
#else
  #define BLOCK_W	12			/* block size */
  #define BLOCK_H	12
  #define WIDTH 	20			/* default field size */
  #define HEIGHT	20
  #define MIN_W		10			/* minimum field size */
  #define MIN_H		10
#endif
#define MINES	(WIDTH * HEIGHT / 10)
#define ROUNDS	1

/* font settings */

#define FNAME	"fixed"
#ifdef WTINYSCREEN
  #define FSIZE (BLOCK_W - 2)
#else
  #define FSIZE	(BLOCK_W - 4)		/* smaller than blocksize */
#endif

/* global variables */
static WWIN *Window;
static WFONT *Font;

typedef struct
{
  unsigned char **field;		/* (height + 2) * (width + 2) */
  int block_w;
  int block_h;
  int width;
  int height;
  int mines;
  int rounds;
} MINEFIELD;

MINEFIELD MineField =
{
  0, BLOCK_W, BLOCK_H, WIDTH, HEIGHT, MINES, ROUNDS
};

/* bit that indicates that block hasn't been yet 'turned' */
#define HIDDEN	64


/* prototypes */
static void process_args(char *argv[]);
static int  initialize(char *argv[]);
static void exit_game(int signal);
static void random_field(void);
static void first_clear(short *todo);
static void seed_clear(short *todo, short x, short y);
static void number_block(short num, short x, short y);
static void draw_game(void);
static void show_mines(void);
static void wait_input(void);

int main(int argc, char *argv[])
{
  short mx, my, todo, hit;

  /* check command line arguments */
  process_args(argv);

  /* initialize W and Minefield struct */
  if(!initialize(argv))
    exit_game(1);

  srand(time(0));

  do
  {
    w_settitle(Window, " MINES ");
    draw_game();
    random_field();
    todo = MineField.width * MineField.height - MineField.mines;
    w_setmode(Window, M_CLEAR);
    first_clear(&todo);

    hit = 0;
    do
    {
      wait_input();
      w_querymousepos(Window, &mx, &my);
      mx = mx / MineField.block_w + 1;
      my = my / MineField.block_h + 1;
      if(MineField.field[mx][my] >= HIDDEN) {
	if(MineField.field[mx][my] > (HIDDEN + 8))
	  hit = 1;
	else
	  seed_clear(&todo, mx, my);
      }
    }
    while(!hit && todo);

    show_mines();
    if(todo)
      w_settitle(Window, " KABOOM! ");
    else
      w_settitle(Window, " SURVIVED... ");
    wait_input();
    MineField.mines += MineField.mines / 4;

  } while(--MineField.rounds && !todo);

  exit_game(0);
  return 0;
}

/* process command line arguments */
static void process_args(char *argv[])
{
  short err = 0, opt, arg;

  while(*++argv)
  {
    /* option, letter (only one), with an argument? */
    if(**argv == '-' && (opt = *(*argv+1)) && !*(*argv+2) && *(argv+1))
    {
      arg = atoi(*++argv);

      switch(opt)
      {
	case 'w':
	  if(arg >= MIN_W)
	    MineField.width = arg;
	  break;

        case 'h':
	  if(arg >= MIN_H)
	    MineField.height = arg;
	  break;

        case 'r':
	  if(arg > ROUNDS)
	    MineField.rounds = arg;
	  break;

        case 'm':
          MineField.mines = arg;
	  break;

	default:
	  err = 1;
      }
    }
    else
      err = 1;
  }

  /* not too easy nor hard... */
  if(MineField.mines < (arg = MineField.width * MineField.height / 10) ||
     MineField.mines >  arg * 4)
    MineField.mines = arg;

  if (err)
  {
    fprintf(stderr, "usage: wmines [-w <width>] [-h <height>] [-m <mines>] [-r <rounds>]\n");
    exit(1);
  }
}

/* Initialize W stuff (connect server, open window, set font/title)
 * and allocate Minefield. Return 0 on error.
 */
static int initialize(char *argv[])
{
  WSERVER *wserver;
  short w_width, w_height, i;

  if (!(wserver = w_init()))
  {
    fprintf(stderr, "%s: can't connect to wserver\n", *argv);
    return 0;
  }
  w_width = wserver->width;
  w_height = wserver->height;

  /* limit window size to screen size */
  w_width  -= HOR_BORDERS;
  w_height -= VER_BORDERS;
  if(MineField.block_w * MineField.width > w_width)
    MineField.width  = w_width / MineField.block_w;
  if(MineField.block_h * MineField.height > w_height)
    MineField.height = w_height / MineField.block_h;

  /* calculate window size */
  w_width  = MineField.block_w * MineField.width;
  w_height = MineField.block_h * MineField.height;

  if(!(Window = w_create(w_width, w_height, WIN_PROPERTIES)))
  {
    fprintf(stderr, "%s: unable to create a window\n", *argv);
    return 0;
  }
  if (!(Font = w_loadfont(FNAME, FSIZE, 0)))
  {
    fprintf(stderr, "%s: unable to load '" FNAME "%d' font\n", *argv, FSIZE);
    return 0;
  }
  w_setfont(Window, Font);
  if(w_open(Window, UNDEF, UNDEF) < 0)
  {
    fprintf(stderr, "%s: unable to open a window\n", *argv);
    return 0;
  }

  /* allocate space for the field pointers and rows */
  MineField.field = (unsigned char **)malloc(sizeof(void *) * (MineField.width + 2));
  for(i = 0; i < MineField.width + 2; i ++)
    if(!(MineField.field[i] = (unsigned char *)malloc((MineField.height + 2))))
    {
      fprintf(stderr, "%s: unable to allocate minefield\n", *argv);
      return 0;
    }
  return 1;
}

/* signal / exit handler */
static void exit_game(int signal)
{
  int i;

  if(MineField.field)
  {
    for(i = 0; i < MineField.width + 2; i++)
      free(MineField.field[i]);
    free(MineField.field);
  }
  if(Window)
  {
    w_close(Window);
    w_delete(Window);
  }
  exit(signal);
}


/* randomize the minefield, check how many mines any square adjoins
 * > 8 means that the square contains a mine itself
 */
static void random_field(void)
{
  int i, x, y;

  for(y = 0; y < MineField.height + 2; y++)
    for(x = 0; x < MineField.width + 2; x++)
      MineField.field[x][y] = HIDDEN;

  for(i = 0; i < MineField.mines; i++)
  {
    /* search an empty place */
    do
    {
      x = rand() % MineField.width  + 1;
      y = rand() % MineField.height + 1;
    } while(MineField.field[x][y] > (HIDDEN + 8));

    /* position mine */
    MineField.field[x][y] += 9;

    /* increase mine count in the neighbourhood */
    MineField.field[x-1][y] += 1;
    MineField.field[x+1][y] += 1;
    y -= 1;
    MineField.field[x][y]   += 1;
    MineField.field[x-1][y] += 1;
    MineField.field[x+1][y] += 1;
    y += 2;
    MineField.field[x][y]   += 1;
    MineField.field[x-1][y] += 1;
    MineField.field[x+1][y] += 1;
  }
}

/* makes one clearing on start of the game */
static void first_clear(short *todo)
{
  short x, y, i = MineField.width * MineField.height;

  /* get an empty place */
  do
  {
    x = rand() % MineField.width  + 1;
    y = rand() % MineField.height + 1;
  } while(i-- && MineField.field[x][y] > HIDDEN);

  /* and clear it */
  seed_clear(todo, x, y);
}


/* clear all the empty blocks around (x,y) using recursive 'seed-fill' */
static void seed_clear(short *todo, short x, short y)
{
#define STACK	40
#define PUSH(a,b) {if(idx < STACK-1) { stack[idx++] = a; stack[idx++] = b; }}
  short stack[STACK], idx = 0, start, above, below;
  short num, width = MineField.width;
  unsigned char **field = MineField.field;

  for(;;)
  {
    /* don't start if not an empty block */
    if((num = field[x][y] & ~HIDDEN))
    {
      number_block(num, x, y);
      (*todo)--;
      return;
    }

    /* search left edge (hidden or visible) */
    while(x > 1 && !(field[x][y] & ~HIDDEN))
      x--;

    start = above = below = x;
    do
    {
      /* check above */
      if(--y > 0 && (num = field[x][y]) & HIDDEN)
      {
	num &= ~HIDDEN;
	if(num)
	{
	  above = x;
	  number_block(num, x, y);
	  (*todo)--;
	}
	else
	  if(above <= x)
	  {
	    PUSH(x, y);
	    above = width;
	  }
      }
      y++;

      /* check below */
      if(++y <= MineField.height && (num = field[x][y]) & HIDDEN)
      {
	num &= ~HIDDEN;
	if(num)
	{
	  below = x;
	  number_block(num, x, y);
	  (*todo)--;
	}
	else
	  if(below <= x)
	  {
	    PUSH(x, y);
	    below = width;
	  }
      }
      y--;

      /* check current */
      num = field[x][y];
      if(num & HIDDEN)
      {
        num &= ~HIDDEN;
	number_block(num, x, y);
	(*todo)--;
      }
    /* until right edge */
    } while((x == start || !num) && x++ < width);

    /* while something in stack, POP(x,y) */
    if(idx > 1)
    {
      /* last-in, first-out! */
      y = stack[--idx];
      x = stack[--idx];
    }
    else
      return;
  }
}

/* clear an area and put a number on it */
static void number_block(short number, short x, short y)
{
  short block_w = MineField.block_w;
  short block_h = MineField.block_h;

  MineField.field[x--][y--] = number;	/* clear hidden bit */

  x *= block_w;
  y *= block_h;

  /* clear (set in main) area */
  w_pbox(Window, x, y, block_w - 1, block_h - 1);

  if(!number)
    return;

  /* output number centered */
  x += (block_w - Font->widths[number + '0']) / 2;
  y += (block_h - Font->height) / 2;
  w_printchar(Window, x, y, number + '0');
}


/* draws the initial game window contents */
static void draw_game(void)
{
  short block_w = MineField.block_w;
  short block_h = MineField.block_h;
  short win_w = MineField.width  * block_w;
  short win_h = MineField.height * block_h;
  short x, y;

  w_dpbox(Window, 0, 0, win_w, win_h);

  w_setmode(Window, M_CLEAR);
  for(y = 0; y < win_h; y += block_h)
    for(x = 0; x < win_w; x += block_w)
    {
      w_hline(Window, x, y, x + block_w - 2);
      w_vline(Window, x, y + 1, y + block_h - 2);
    }

  w_setmode(Window, M_DRAW);
  for(y = 0; y < win_h; y += block_h)
    for(x = 0; x < win_w; x += block_w)
    {
      w_hline(Window, x + 1, y + block_h - 2, x + block_w - 2);
      w_vline(Window, x + block_w - 2, y + 1, y + block_h - 3);
    }
  for(x = -1; (x += block_w) < win_w;)
    w_vline(Window, x, 0, win_h - 1);
  for(y = -1; (y += block_h) < win_h;)
    w_hline(Window, 0, y, win_w - 1);
}

/* show all the mines */
static void show_mines(void)
{
  short block_w = MineField.block_w;
  short block_h = MineField.block_h;
  short x, y;

  w_setmode(Window, M_DRAW);
  for(y = 1; y <= MineField.height; y++)
    for(x = 1; x <= MineField.width; x++)
      if(MineField.field[x][y] > (HIDDEN + 8))
        w_pbox(Window,
	  (x - 1) * block_w + 2, (y - 1) * block_h + 2,
	  block_w-5, block_h-5);
}

static void wait_input(void)
{
  WEVENT *ev;

  /* until mouse button released or exit */
  for(;;)
  {
    while(!(ev = w_queryevent(NULL, NULL, NULL, -1)));
    switch(ev->type)
    {
      case EVENT_MRELEASE:
        return;

      case EVENT_KEY:
        if((ev->key & 0xff) == 'q')
	  exit_game(0);
	break;

      case EVENT_GADGET:
        if(ev->key == GADGET_EXIT || ev->key == GADGET_CLOSE)
	  exit_game(0);
    }
  }
}
