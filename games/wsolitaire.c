/*
 * wsolitaire.c, a part of the W Window System
 *
 * Copyright (C) 1995 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Solitaire (a well known 'remove pieces from board') game
 */

#include <stdio.h>		/* fprintf() */
#include <Wlib.h>


/* W specific stuff */

#define WIN_PROPERTIES	(W_MOVE | W_TITLE | W_CLOSE | EV_MOUSE | EV_KEYS)
#define WIN_NAME	" SOLITAIRE "

/* board and pieces are square (latter actually circle) sized */
#define BLOCK	13			/* block size (should be odd) */
#define LARGE	5			/* radiuses for the circles inside */
#define SMALL	3
#define SIZE	9			/* board size */
#define PIECES	44			/* pieces on the board at start */

/* what can be at a board position */
#define SPACE	0
#define EMPTY	1
#define PIECE	2

/* user actions defines */
#define EXIT	0
#define PROCEED	1
#define UNDO	2

#define DIST(a,b)	((b) > (a) ? (b) - (a) : (a) - (b))


/* global variables */
static WWIN *Window;

static unsigned char Board[SIZE][SIZE];
static short FromX, FromY;


/* prototypes */
static int  initialize(char *argv[]);
static void restore(int signal);
static void init_board(void);
static void draw_piece(short x, short y);
static void select_piece(short x, short y);
static int  move_from(short x, short y);
static int  move_to(short x, short y);
static void push(short x, short y);
static int  undo(void);
static int  wait_input(void);


int main(int argc, char *argv[])
{
  int
    action,
    done_already = 1, enabled = 0,	/* 1 / unlimited undo */
    todo = PIECES, valid = 0;		/* moves to do / valid move */
  short
    mx, my;

  /* check for unlimited undo argument */
  if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'u' && argv[1][2] == '\0')
    enabled = 1;

  /* initialize W */
  if(!initialize(argv))
  {
    restore(1);
    return 1;
  }

  w_setmode(Window, M_INVERS);
  init_board();

  do
  {
    if((action = wait_input()) == EXIT)
      break;

    if(action == UNDO)
    {
      if(valid)
        select_piece(FromX, FromY);		/* deselect */
      valid = 0;

      if(enabled || !done_already)
      {
        done_already = 1;
        if(undo())
          todo++;
      }
      continue;
    }

    w_querymousepos(Window, &mx, &my);
    mx /= BLOCK;
    my /= BLOCK;

    if(valid)
    {
      if(move_to(mx, my))		/* with checks of course */
      {
        done_already = 0;
        push(mx, my);			/* store for undo-pop */
        todo--;
      }
      valid = 0;			/* next move */
    }
    else
      valid = move_from(mx, my);	/* valid moe beginning? */
  } while(todo > 1);

  if(todo == 1)
  {
    wait_input();
    /* congratulate! */
  }

  restore(0);
  return 0;
}

/* --------------------
 * Initialize W stuff (connect server, open window, set font/title)
 * and allocate Board. Return 0 on error.
 */
static int initialize(char *argv[])
{
  WSERVER *wserver;

  if(!(wserver = w_init()))
  {
    fprintf(stderr, "%s: can't connect to wserver\n", *argv);
    return 0;
  }

  if(!(Window = w_create(BLOCK * SIZE, BLOCK * SIZE, WIN_PROPERTIES)))
  {
    fprintf(stderr, "%s: can't create window\n", *argv);
    return 0;
  }
  if(w_open(Window, UNDEF, UNDEF) < 0)
  {
    fprintf(stderr, "%s: can't open window\n", *argv);
    return 0;
  }
  w_settitle(Window, WIN_NAME);
  return 1;
}

/* signal / exit handler */
static void restore(int signal)
{
  if(Window)
  {
    w_exit();
  }
}


/* initialize and draw the Board. */
static void init_board(void)
{
  short x, y, xx, yy;

  for(y = 0; y < SIZE; y++)
    for(x = 0; x < SIZE; x++)
      Board[x][y] = SPACE;

  for(y = 0; y < SIZE; y++)
    for(x = 3; x < SIZE-3; x++)
      Board[x][y] = PIECE;

  for(y = 3; y < SIZE-3; y++)
    for(x = 0; x < SIZE; x++)
      Board[x][y] = PIECE;

  Board[SIZE / 2][SIZE / 2] = EMPTY;

  yy = BLOCK / 2;
  for(y = 0;   y < SIZE; y++)
  {
    xx = BLOCK / 2;
    for(x = 0; x < SIZE; x++)
    {
      switch(Board[x][y])
      {
	case PIECE:
	  w_pcircle(Window, xx, yy, SMALL);
	case EMPTY:
	  w_circle(Window, xx, yy, LARGE);
      }
      xx += BLOCK;
    }
    yy += BLOCK;
  }
}

/* ----------------------- */
static void draw_piece(short x, short y)
{
  short
    ox = x * BLOCK + (BLOCK / 2),
    oy = y * BLOCK + (BLOCK / 2);

  switch(Board[x][y])
  {
    case SPACE:
      return;
    case PIECE:
      w_pcircle(Window, ox, oy, SMALL);
      break;
    case EMPTY:
      w_circle(Window, ox, oy, LARGE);
      break;
  }
}

/* de/select a piece */
static void select_piece(short x, short y)
{
  x *= BLOCK;
  y *= BLOCK;
  w_box(Window, x, y, BLOCK, BLOCK);
}

/* moving a piece? -> select */
static int move_from(short x, short y)
{
  if(Board[x][y] == PIECE)
  {
    FromX = x; FromY = y;
    select_piece(x, y);
    return 1;
  }
  return 0;
}

/* checks if the move is valid and executes it */
static int move_to(short to_x, short to_y)
{
  short x, y, d1, d2;

  x  = (FromX + to_x) >> 1;		/* (from -> to) midpoint */
  y  = (FromY + to_y) >> 1;
  d1 = DIST(FromX, to_x);
  d2 = DIST(FromY, to_y);

  select_piece(FromX, FromY);		/* deselect */

  /* To empty, two block away, over a piece? */
  if(Board[to_x][to_y] == EMPTY &&
     ((d1 == 2 && d2 == 2) || (d1 == 2 && d2 == 0) || (d1 == 0 && d2 == 2)) &&
     Board[x][y] == PIECE)
  {
    draw_piece(FromX, FromY);		/* empty */
    Board[FromX][FromY] = EMPTY;

    draw_piece(x, y);
    Board[x][y] = EMPTY;

    Board[to_x][to_y] = PIECE;
    draw_piece(to_x, to_y);		/* fill with piece */

    return 1;
  }
  return 0;
}

/* ----------------------- */
static char Stack[PIECES*4];
static short Idx;

/* stack (FIFO) for undo */
static void push(short x, short y)
{
  Stack[Idx++] = FromY;
  Stack[Idx++] = FromX;
  Stack[Idx++] = y;
  Stack[Idx++] = x;
}

static int undo(void)
{
  short x1, y1, x2, y2;

  if(Idx < 4)
    return 0;

  /* remove moved-to */
  x1 = Stack[--Idx];
  y1 = Stack[--Idx];
  draw_piece(x1, y1);
  Board[x1][y1] = EMPTY;	/* remember that mode is invert... */

  /* insert moved-from */
  x2 = Stack[--Idx];
  y2 = Stack[--Idx];
  Board[x2][y2] = PIECE;
  draw_piece(x2, y2);

  /* insert removed-from-between */
  x2 = (x2 + x1) >> 1;
  y2 = (y2 + y1) >> 1;
  Board[x2][y2] = PIECE;
  draw_piece(x2, y2);

  return 1;
}

/* ----------------------- */
static int wait_input(void)
{
  WEVENT *ev;

  /* until no events, mouse button press */
  while((ev = w_queryevent(NULL, NULL, NULL, 0)) && ev->type == EVENT_MPRESS);

  /* until mouse button pressed or exited by a W event or with the ESC key */
  while(ev = w_queryevent(NULL, NULL, NULL, -1), ev->type != EVENT_MPRESS)
    if(((ev->type == EVENT_GADGET && ((ev->key == GADGET_EXIT) || (ev->key == GADGET_CLOSE)))) ||
       (ev->type == EVENT_KEY && (ev->key & 0xFF) == 'q'))
      return EXIT;

  if(ev->key == BUTTON_RIGHT)
    return UNDO;

  return PROCEED;
}
