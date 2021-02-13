/*
 * wrobots.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * - robots / daleks game for W
 *
 * TODO
 * - draw some nice bitmaps for player, robots and robot 'heap'
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Wlib.h>

#define GAME_NAME	"W-Robots"
#define DEF_ROBOTS	8		/* robots at start */
#define MAX_ROBOTS	128		/* maximum amount of robots */
#define MIN_WIDTH	10		/* minimum play area size */
#define MIN_HEIGHT	10
#ifdef WTINYSCREEN
  #define DEF_WIDTH	19		/* play area size */
  #define DEF_HEIGHT	17
#else
  #define DEF_WIDTH	20		/* play area size */
  #define DEF_HEIGHT	20
#endif
#define SCORE		10		/* points / robots */

/* moves */
#define INVALID		0
#define MOVE		1
#define STAY_HERE	2
#define TELEPORT	3
#define LAST_STAND	4

typedef struct
{
  int x, y;
  int alive;
} ROBOTS;

static int Width = DEF_WIDTH, Height = DEF_HEIGHT;

/* game prototypes */
int  parse_args(int argc, char *argv[]);
void random_pos(int robots, ROBOTS *pos, int *x, int *y);
void init_level(int robots, ROBOTS *pos, int *manx, int *many);
int  move(int robots, ROBOTS *pos, int *manx, int *many);
int  hunt(int robots, ROBOTS *pos, int manx, int many);

/* window / event / graphics handling prototypes */
const char *win_init(void);
void clear_win(void);
void wait_input(long msecs);
int  get_dir(int *x, int *y);
int  process_input(WEVENT *ev, int *x, int *y);
void draw_rip(int x, int y, int score);
void show_info(int level, int score);
void draw_dalek(int x, int y);
void draw_heap(int x, int y);
void draw_man(int x, int y);
void clear(int x, int y);


int main(int argc, char *argv[])
{
  int level = 0, score = 0, enemies, manx, many, dir, alive = 1,
       robots, dead, dbase = DEF_ROBOTS;
  static ROBOTS pos[MAX_ROBOTS];

  if(!parse_args(argc, argv))
  {
    fprintf(stderr, "\n" GAME_NAME " w/ 1996 Eero Tamminen\n\n");
    fprintf(stderr, "Usage: %s [-w <width (blocks)>] [-h <height (blocks)>]\n", *argv);
    fprintf(stderr, "\nTo move, use either keypad or mouse.\n");
    return -1;
  }
  /* base number of robots on the play area */
  dbase = Width * Height / 48;
  srand(time(0));
  win_init();
  do
  {
    enemies = robots = dbase + level * dbase / 4;
    init_level(robots, pos, &manx, &many);
    level++;
    dir = INVALID;
    show_info(level, score);
    do
    {
      if(dir != LAST_STAND)
        dir = move(robots, pos, &manx, &many);

      dead = hunt(robots, pos, manx, many);
      score += dead * SCORE * level;
      if(dead)
        show_info(level, score);
      if(dead < 0)
	alive = 0;
      else
        enemies -= dead;
      wait_input(128);

    } while(alive && enemies);		/* player and robots alive? */

  } while(alive);

  draw_rip(manx, many, score);
  wait_input(-1);
  return 0;
}

/* get play area size */
int parse_args(int argc, char *argv[])
{
  int idx = 0;
  while(++idx < argc)
  {
    /* an one letter option and argument for it? */
    if(argv[idx][0] == '-' && argv[idx][2] == '\0' && idx+1 < argc)
      switch(argv[idx++][1])
      {
	case 'w':				/* window width */
	  if((Width = atoi(argv[idx])) < MIN_WIDTH)
	    Width = MIN_WIDTH;
	  break;
	case 'h':				/* window height */
	  if((Height = atoi(argv[idx])) < MIN_WIDTH)
	    Height = MIN_HEIGHT;
	  break;
	default:
	  return 0;
      }
    else
      return 1;
  }
  return 1;	/* success */
}

void random_pos(int robots, ROBOTS *pos, int *x, int *y)
{
  int idx;

  for(;;)
  {
    *x = rand() % Width;
    *y = rand() % Height;

    /* not onto a robots? */
    for(idx = 0; idx < robots; idx++)
      if(*x == pos[idx].x && *y == pos[idx].y)
	continue;

    return;
  }
}

/* randomize robots and player positions */
void init_level(int robots, ROBOTS *pos, int *manx, int *many)
{
  int idx;

  clear_win();
  for(idx = 0; idx < robots; idx++)
  {
    pos[idx].alive = 1;
    random_pos(idx-1, pos, &pos[idx].x, &pos[idx].y);
    draw_dalek(pos[idx].x, pos[idx].y);
  }

  random_pos(robots, pos, manx, many);
  draw_man(*manx, *many);
}

/* let the user make a move (show valid ones) */
int move(int robots, ROBOTS *pos, int *manx, int *many)
{
  int x, y, dir, idx;

  for(;;)
  {
    x = *manx;
    y = *many;
    dir = get_dir(&x, &y);
    if(dir != MOVE)
      break;

    if(x >= 0 && y >= 0 && x < Width && y < Height)
    {
      for(idx = 0; idx < robots; idx++)
      {
        if(x == pos[idx].x && y == pos[idx].y)
	  continue;
      }
      break;
    }
  }

  /* keep in place / last stand */
  if(dir == STAY_HERE || dir == LAST_STAND)
    return dir;

  clear(*manx, *many);
  if(dir == TELEPORT)
  {
    random_pos(robots, pos, manx, many);
  }
  else
  {
    *manx = x;
    *many = y;
  }

  draw_man(*manx, *many);
  return dir;
}

/* move the robots into same direction that the player is
 * return the number of collided robots or -1 if they got you
 */
int  hunt(int robots, ROBOTS *pos, int manx, int many)
{
  int test, idx, hits = 0, dead = 0;

  /* move the alive robots ones closer to the player */
  for(idx = 0; idx < robots; idx++)
    if(pos[idx].alive)
    {
      /* clear old robots from the screen */
      clear(pos[idx].x, pos[idx].y);

      /* get closer */
      if(pos[idx].x < manx)
        pos[idx].x++;
      else if(pos[idx].x > manx)
        pos[idx].x--;

      if(pos[idx].y < many)
        pos[idx].y++;
      else if(pos[idx].y > many)
        pos[idx].y--;

      /* check whether they got you... */
      if(pos[idx].x == manx && pos[idx].y == many)
	  hits--;
    }

  /* check for collisions */
  for(idx = 0; idx < robots; idx++)
    if(pos[idx].alive)
    {
      for(test = 0; test < robots; test++)
      {
        if(test != idx && pos[idx].x == pos[test].x && pos[idx].y == pos[test].y)
        {
	  if(pos[idx].alive)
	  {
	    pos[idx].alive = 0;
	    draw_heap(pos[idx].x, pos[idx].y);
	    dead++;
	  }
	  if(pos[test].alive)
	  {
	    pos[test].alive = 0;
	    dead++;
	  }
	  /* check next alive one */
	  continue;
	}
      }
      /* no collision? */
      if(pos[idx].alive)
        draw_dalek(pos[idx].x, pos[idx].y);
    }

  if(hits)
    return hits;
  return dead;
}


/* window / graphics handling */

static WWIN *PlayWin;

#define PROPERTIES	(W_MOVE | W_TITLE | W_CLOSE | EV_KEYS | EV_MOUSE)
#ifdef WTINYSCREEN
#define BLK_SIZE	8	/* one area unit size */
#else
#define BLK_SIZE	16	/* one area unit size */
#endif

/* M_DRAW is the default graphics mode */

const char *win_init(void)
{
  if(!(w_init()))
    return "can't connect W server";

  if(!(PlayWin = w_create(Width * BLK_SIZE, Height * BLK_SIZE, PROPERTIES)))
    return "unable to create game window";

  w_settitle(PlayWin, GAME_NAME);

  if(w_open(PlayWin, UNDEF, UNDEF) < 0)
    return "unable to open input window";

  return 0;
}

void clear_win(void)
{
  w_setmode(PlayWin, M_CLEAR);
  w_pbox(PlayWin, 0, 0, Width * BLK_SIZE, Height * BLK_SIZE);
  w_setmode(PlayWin, M_DRAW);
}

void wait_input(long msecs)
{
  WEVENT *ev;
  if(!(ev = w_queryevent(NULL, NULL, NULL, msecs)))
    return;

  if(ev->type == EVENT_GADGET &&
     (ev->key == GADGET_EXIT || ev->key == GADGET_CLOSE))
    exit(0);
}

/* return direction (0-8) or teleport (9) index */
int get_dir(int *x, int *y)
{
  int dir;
   while(!(dir = process_input(w_queryevent(NULL, NULL, NULL, -1), x, y)));
  return dir;
}

int process_input(WEVENT *ev, int *x, int *y)
{
  if (!ev) {
    return INVALID;
  } 
  switch(ev->type)
  {
    /* return direction or teleport index */
    case EVENT_KEY:
      switch(ev->key & 0xFF)
      {
	/* directions */
	case '8':
	  (*y)--;
	  break;
	case '2':
	  (*y)++;
	  break;
	case '4':
	  (*x)--;
	  break;
	case '6':
	  (*x)++;
	  break;
	case '7':
	  (*x)--;
	  (*y)--;
	  break;
	case '9':
	  (*x)++;
	  (*y)--;
	  break;
	case '1':
	  (*x)--;
	  (*y)++;
	  break;
	case '3':
	  (*x)++;
	  (*y)++;
	  break;

	/* other 'moves' */
	case '5':
	  return STAY_HERE;
	case 't':
	case 'T':
	  return TELEPORT;
	case 'l':
	case 'L':
	  return LAST_STAND;
	default:
	  return INVALID;
      }
      break;

    case EVENT_MPRESS:
      ev->x /= BLK_SIZE;
      ev->y /= BLK_SIZE;
      if(ev->x == *x && ev->y == *y)
        return STAY_HERE;

      if(ev->x != *x)
        ev->x < *x ? (*x)-- : (*x)++;
      if(ev->y != *y)
        ev->y < *y ? (*y)-- : (*y)++;
      break;

    case EVENT_GADGET:
     if(ev->key == GADGET_EXIT || ev->key == GADGET_CLOSE)
       exit(0);

    default:
     return INVALID;
  }
  return MOVE;
}

void show_info(int level, int score)
{
  char msg[64];
  sprintf(msg, "Level: %d Score: %d", level, score);
  w_settitle(PlayWin, msg);
}


/* these could of course use bitmaps... I decided to try drawing
 * something abstract for a change <g>	- Eero
 */

void draw_dalek(int x, int y)
{
  w_pbox(PlayWin, x*BLK_SIZE + 2, y*BLK_SIZE + 2, BLK_SIZE - 3, BLK_SIZE - 3);
}

void draw_heap(int x, int y)
{
  w_dpbox(PlayWin, x*BLK_SIZE + 1, y*BLK_SIZE + 1, BLK_SIZE - 2, BLK_SIZE - 2);
}

void draw_man(int x, int y)
{
  w_setmode(PlayWin, M_INVERS);
  w_pbox(PlayWin, x*BLK_SIZE + 3, y*BLK_SIZE + 3, BLK_SIZE - 5, BLK_SIZE - 5);
  w_pbox(PlayWin, x*BLK_SIZE + 5, y*BLK_SIZE + 5, BLK_SIZE - 9, BLK_SIZE - 9);
  w_setmode(PlayWin, M_DRAW);
}

void draw_rip(int x, int y, int score)
{
  char msg[64];

  w_setmode(PlayWin, M_CLEAR);
  w_pbox(PlayWin, x*BLK_SIZE, y*BLK_SIZE, BLK_SIZE, BLK_SIZE);
  w_setmode(PlayWin, M_INVERS);
  w_dpbox(PlayWin, x*BLK_SIZE + 3, y*BLK_SIZE + 3, BLK_SIZE - 5, BLK_SIZE - 5);
  w_dpbox(PlayWin, x*BLK_SIZE + 5, y*BLK_SIZE + 5, BLK_SIZE - 9, BLK_SIZE - 9);
  sprintf(msg, "You die... Score: %d", score);
  w_settitle(PlayWin, msg);
}

void clear(int x, int y)
{
  w_setmode(PlayWin, M_CLEAR);
  w_pbox(PlayWin, x*BLK_SIZE, y*BLK_SIZE, BLK_SIZE, BLK_SIZE);
  w_setmode(PlayWin, M_DRAW);
}
