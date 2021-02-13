/* A two player Wyrms game
 *
 * Copyright (C) 1995 by Eero Tamminen
 *
 * The source includes:
 * + Wyrms.c - takes care of the game logic. Includes the level editor.
 * + Wyrms.h contains defaults, defines prototypes etc.
 * + Level.c contains code for loading and initalizing levels.
 * + system.c contains machine / resolution specific information
 *   and functions including low level user input and screen output
 * + Joyisr[z].s is a short assembly code for handling the joystick
 *   interrupts on TOS version.
 * + Makefile is for GCC.
 * + Wyrms.mak is for SozobonX C.
 * + Readme.txt contains info about the game and level files.
 *
 * Porting this to another OS would need a rewrite of the whole
 * system.c file and probably changing a couple of defines in the
 * wyrms.h.
 */

#include <stdio.h>
#include <stdlib.h>		/* atoi(), rand() */
#include <time.h>
#define __WYRMS_C__
#include "wyrms.h"		/* common defines etc. */

#define INIT_PATH	""
#define HELP_FILE	"help.h"

/* collision types */
#define FATAL		1
#define NEXT_LEVEL	2

/* bonus identifiers:
 * - Passive ones act once when you hit a certain thing.
 * - Active ones are activated with the 'fire' button.
 * - Activated ones have a timer (one common timer).
 * You can have only one of each at the time and of the activated ones,
 * the last one decides the timer value.
 */
#define B_NONE		0x0	/* no bonus */
#define B_KEY		0x1	/* passive, opens a door */
#define B_BANG		0x2	/* passive, no chrash on next collision */
#define B_LEAF		0x4	/* active, makes you able to shit */
#define B_TURD		0x8	/* activated, 'make turd(s)' mode ON ;-) */
#define B_GREASE	0x10	/* activated, shit hit the fan, no controls */
#define B_FLASH		0x20	/* active, you got turbo :) */
#define B_TURBO		0x40	/* activated, turbo activated */
#define B_LENGHT	0x80	/* activated, increases lenght */
#define B_FIRST		0x100	/* the first random B_LENGHT bonus */

/* how many wyrms there are in the game */
#define WYRMS		2

/* program modes */
#define MODE_PLAY	1
#define MODE_EDIT	2

/* default values */
static int
  InitSpeed = FRAME_RATE / DEF_SPEED,
  InitLives = DEF_LIVES,
  LenghtInc = LEN_INC,
  LenboSize = LB_SIZE;

/* not array of pointers to arrays */
static unsigned char LevelMap[SCREEN_W][SCREEN_H];
static unsigned char *pLevelMap = (unsigned char*)LevelMap;
static Wyrms WyrmPlr[WYRMS];

/* local prototypes */
static void help(int argc, char *argv[]);
static void  process_args(int *idx, int argc, char *argv[], const char **path, int *mode);
static void init_wyrms(void);
static void direct(Wyrms *w);
static void wait_input(void);
static int play_level(void);
static void activate(Wyrms *w);
static void do_bonus(Wyrms *w);
static void move_wyrm(Wyrms *w);
static int  check_move(Wyrms *w);
static int  collision(Wyrms *w, int stone);
static int  lenght_bonus(int which);
static void new_bonus(int bonus);
static void show_score(Wyrms *w);
static void show_lives(Wyrms *w);
static void death(int who);
static int  edit_level(const char *file_name);
static void get_headings(void);


int main(int argc, char *argv[])
{
  int i = 0, mode = MODE_PLAY;
  const char *path = INIT_PATH;

  if(argc < 2)
  {
    help(argc, argv);
    return 1;
  }

  /* initial wyrm initializations */
  WyrmPlr[1].lives = WyrmPlr[0].lives = InitLives;
  WyrmPlr[1].score = WyrmPlr[0].score = 0;
  WyrmPlr[0].device = DEF_DEV0;
  WyrmPlr[1].device = DEF_DEV1;
  WyrmPlr[0].id = I_BLACK;
  WyrmPlr[1].id = I_WHITE;

  /* initalize system specific things */
  init();

  /* seed random generator */
  srand(time(0));

  /* play levels given as arguments */
  while(++i < argc)
  {
    /* process command line arguments */
    if(argv[i][0] == '-')
      process_args(&i, argc, argv, &path, &mode);
    else
    {
      /* read the next level */
      if(!read_map(path, argv[i]))
	message("Level file not found");

      if(mode == MODE_PLAY)
      {
        if(!play_level())
          i = argc;
      }
      else
        /* filename for saving... */
        if(!edit_level(argv[i]))
          i = argc;
    }
  }
  restore(0);		/* restore system things and exit */
  return 0;
}

static void help(int argc, char *argv[])
{
  int i;
  /* show the erronous commandline */
  for(i = 0; i < argc; i++)
  {
    fputs(argv[i], stderr);
    fputs(" ", stderr);
  }

  fputs("\nUsage: wyrms [options] level-file\n", stderr);
  fputs("\nOptions:\n", stderr);
  fputs(" -w # How many wyrms you got at the start\n", stderr);
  fputs(" -s # Set wyrms' speed (moves / second)\n", stderr);
  fputs(" -b # How many blocks bonuses are in size\n", stderr);
  fputs(" -l # Worm lenght is increment in blocks\n", stderr);
  fputs(" -1 # Specifies which controls (0-3) player one uses\n", stderr);
  fputs(" -2 # Specifies which controls (0-3) player two uses\n", stderr);
  fputs(" -h   Shows all the bonuses and what they do\n", stderr);
  fputs(" -e   Edit level file(s)\n", stderr);
}


/* process command line arguments:
 * Initialize things changable at compile time and
 * at program startup ---
 * set how many lives, what speed, which control devices etc.
 * return the current arg index
 */
static void process_args(int *idx, int argc, char *argv[], const char **path, int *mode)
{
  int dev;

  switch(argv[*idx][1])
  {
    /* how many lives the wyrms got */
    case 'w':
      InitLives = atoi(argv[++(*idx)]);
      if(InitLives < 1)
	InitLives = 1;
      break;

    /* where the level files are */
    case 'p':
      *path = argv[++(*idx)];
      break;

    /* wyrm speed -> how many frames before next move */
    case 's':
      InitSpeed = atoi(argv[++(*idx)]);
      if(InitSpeed < 1)
	InitSpeed = FRAME_RATE;
      else if(InitSpeed >= FRAME_RATE)
	InitSpeed = 1;
      else
	InitSpeed = FRAME_RATE / InitSpeed;
      break;

    /* of how many blocks bonuses are composed of */
    case 'b':
      LenboSize = atoi(argv[++(*idx)]);
      if(LenboSize < 1 || LenboSize > 3)
	LenboSize = LB_SIZE;
      break;

    /* how much wyrm lenght is increased */
    case 'l':
      LenghtInc = atoi(argv[++(*idx)]);
      if(LenghtInc < 1 || LenghtInc > (MAX_LENGHT / 9))
	LenghtInc = LEN_INC;
      break;

    /* set the control devices */
    case '1':
      dev = atoi(argv[++(*idx)]);
      if(dev >= 0 && dev < DEVICES)
        WyrmPlr[0].device = dev;
      break;
    case '2':
      dev = atoi(argv[++(*idx)]);
      if(dev >= 0 && dev < DEVICES)
        WyrmPlr[1].device = dev;
      break;

    /* into level editing mode */
    case 'e':
      *mode = MODE_EDIT;
      break;

    /* show bonuses */
    case 'h':
      if(read_map(INIT_PATH, HELP_FILE))
      {
	init_level(pLevelMap, WyrmPlr);
	draw_map(pLevelMap);
      }
      else
	message(HELP_FILE " not found");
      get_key();
      restore(0);
      exit(1);
    default:
      help(argc, argv);
      restore(0);
  }
}

/* initalize wyrm structures */
static void init_wyrms(void)
{
  int i;

  draw_map(pLevelMap);
  for(i = 0; i < WYRMS; i++)
  {
    show_score(&WyrmPlr[i]);
    show_lives(&WyrmPlr[i]);
    WyrmPlr[i].speed  = InitSpeed;
    WyrmPlr[i].timer  = 0;
    WyrmPlr[i].lenght = 1;
    WyrmPlr[i].btime  = 2;		/* at least 1! */
    WyrmPlr[i].bonus  = B_LENGHT;	/* add lenght at the start by btime */
    WyrmPlr[i].pdir   = 0;		/* previous direction */
    WyrmPlr[i].incx   = 0;		/* current movement increments */
    WyrmPlr[i].incy   = 0;
    direct(&WyrmPlr[i]);		/* set initial x/y increments */
    WyrmPlr[i].head = WyrmPlr[i].pos;	/* head segment place in the array */
    WyrmPlr[i].head->next = WyrmPlr[i].head;	/* segment refers to itself */
  }
}

/* set x/y increments according to direction */
static void direct(Wyrms *w)
{
  static int against[5] = { 0, DOWN, UP, RIGHT, LEFT };
  int dir;

  dir = input(w->device);

  /* ensure increments */
  if(w->incx == 0 && w->incy == 0)
    dir = w->dir;

  /* don't break the wyrm's neck ;-) */
  if(dir == against[w->pdir])
    return;

  switch(dir)
  {
    case UP:
      w->incx = 0;
      w->incy = -1;
      w->pdir = 1;
      break;
    case DOWN:
      w->incx = 0;
      w->incy = 1;
      w->pdir = 2;
      break;
    case LEFT:
      w->incx = -1;
      w->incy = 0;
      w->pdir = 3;
      break;
    case RIGHT:
      w->incx = 1;
      w->incy = 0;
      w->pdir = 4;
      break;
    case BUTTON:
      activate(w);		/* bonus */
      break;
    default:
      return;
  }
  w->dir = dir;
}

/* clear input, then wait until input */
static void wait_input(void)
{
  while((input(WyrmPlr[0].device) || input(WyrmPlr[1].device)));
  while(!(input(WyrmPlr[0].device) || input(WyrmPlr[1].device)));
}

/* play the level */
static int play_level(void)
{
  int i, dead = 0;

  /* until all either players lives spent */
  while(WyrmPlr[0].lives && WyrmPlr[1].lives)
  {
    /* initialize */
    /* level map, wyrm positions & directions */
    init_level(pLevelMap, WyrmPlr);
    /* wyrm speed, bonuses etc. */
    init_wyrms();
    lenght_bonus(B_FIRST);
    wait_input();

    /* exit with chrash */
    dead = FALSE;
    while(!dead)
    {
      /* do system things & timing */
      wait_frame();

      /* notice that every step has to be done for all
       * players before moving to the next step. Otherwise the
       * actions and results would not sync.
       */

      /* new directions */
      for(i = 0; i < WYRMS; i++)
	/* time to move? */
	if(WyrmPlr[i].timer <= 0)
          /* set wyrm direction variables if got controls */
          if(!(WyrmPlr[i].bonus & B_GREASE))
            direct(&WyrmPlr[i]);

      /* moving */
      for(i = 0; i < WYRMS; i++)
      {
	/* time to move? */
	if(WyrmPlr[i].timer <= 0)
	{
	  /* will also optionally increase the wyrm lenght */
	  move_wyrm(&WyrmPlr[i]);

          /* act on time-out bonuses */
	  if(WyrmPlr[i].btime)
	    do_bonus(&WyrmPlr[i]);
	}
      }
      /* collision checking */
      for(i = 0; i < WYRMS; i++)
	if(WyrmPlr[i].timer-- <= 0)
	{
	  /* this will also output the head! */
	  switch(check_move(&WyrmPlr[i]))
          {
	    /* a fatal collision with something */
            case FATAL:
              do_sound(SND_EXPLOSION);
	      dead |= (1 << i);
	      WyrmPlr[i].lives--;
	      show_lives(&WyrmPlr[i]);
              break;
            case NEXT_LEVEL:
	      do_sound(SND_JINGLE);
	      return TRUE;
	      break;
          }
	  WyrmPlr[i].timer = WyrmPlr[i].speed;
	}
      /* head-on chrash? */
      if(WyrmPlr[0].head->x == WyrmPlr[1].head->x &&
         WyrmPlr[0].head->y == WyrmPlr[1].head->y)
	 dead = 3;
    }
    death(dead);
    get_key();
  }
  /* they didn't make it... */
  return FALSE;
}

/* activate a collected, active bonus */
static void activate(Wyrms *w)
{
  if(w->bonus & B_LEAF)
  {					/* make a dropping */
    w->bonus &= ~B_LEAF;		/* OFF */
    w->bonus |= B_TURD;			/* ON */
    w->btime += 1;			/* one piece */
    w->score += SCORE_INC;
    show_score(w);
  }
  if(w->bonus & B_FLASH)
  {
    w->bonus &= ~B_FLASH;
    w->bonus |= B_TURBO;		/* activate! */
    w->btime += FLASH_TIME;	        /* double speed for some moves */
    w->speed = InitSpeed / 2;
  }
  /* more to come? */
}

/* act on bonuses that are activated. */
static void do_bonus(Wyrms *w)
{
  w->btime--;

  if(w->btime <= 0)
  {
    if(w->bonus & B_TURBO)
      w->speed = InitSpeed;

    /* switch all the activated bonus(es) OFF */
    w->bonus &= ~(B_TURBO | B_LENGHT | B_TURD | B_GREASE);

    /* more to come? */
    w->btime = 0;			/* no bonus active */
  }
}

/* move wyrm by one position, optionally add a segment to the wyrm
 * too. Show it on screen (except the head)
 */
static void move_wyrm(Wyrms *w)
{
  /* either add a one segment */
  if(w->bonus & B_LENGHT && w->lenght + 1 < MAX_LENGHT)
  {
    Ring *tmp;
    tmp = w->head->next;
    w->head->next = &w->pos[w->lenght];
    w->pos[w->lenght].next = tmp;
    w->lenght++;
  } else
  {
    /* or remove the trace of the last segment */
    int x, y, piece;
    x = w->head->next->x;
    y = w->head->next->y;

    if(w->bonus & B_TURD)
      piece = I_GREASE;
    else
      piece = I_BG;
    bitmap(x, y, piece);
    LevelMap[x][y] = piece;
  }
  /* change the head segment to a body segment */
  bitmap(w->head->x, w->head->y, w->id);
  LevelMap[w->head->x][w->head->y] = w->id;

  /* a new place for the wyrm head */
  w->head->next->x = w->head->x + w->incx;
  w->head->next->y = w->head->y + w->incy;
  /* new array pos for head */
  w->head = w->head->next;
}

/* move and check whether the heads chrash (-> return true) */
static int check_move(Wyrms *w)
{
  int x, y, collide;

  x = w->head->x;
  y = w->head->y;

  /* check for a collision (screen border equals a wall) */
  if(x <= 0 || y <= 0 || x >= SCREEN_W - 1 || y >= SCREEN_H - 1)
    return FATAL;			/* collision */
  else
    collide = LevelMap[x][y];

  /* output the head */
  bitmap(x, y, I_HEAD);
  LevelMap[x][y] = I_HEAD;

  return collision(w, collide);
}

/* actions following a chrash with something, return true if play ends */
static int collision(Wyrms *w, int stone)
{
  if(stone == I_BG)
    return FALSE;
  else
    if(w->bonus & B_BANG)
    {
      w->bonus &= ~B_BANG;	/* use the bonus */
      do_sound(SND_EXPLOSION);
      return FALSE;		/* by going through */
    }

  switch(stone)
  {
    case I_HOME:
      w->lives += 1;		/* player gets one more wyrm from home */
      break;
    case I_DOOR:
      if(w->bonus & B_KEY)
      {
        w->score += SCORE_INC;
        show_score(w);
      }
      else
        return FATAL;		/* no key -> chrash with the door */
      break;
    case I_UP:
      if(w->dir != UP)		/* can't go this direction */
        return FATAL;
      break;
    case I_DOWN:
      if(w->dir != DOWN)
        return FATAL;
      break;
    case I_LEFT:
      if(w->dir != LEFT)
        return FATAL;
      break;
    case I_RIGHT:
      if(w->dir != RIGHT)
        return FATAL;
      break;
    case I_KEY:
      w->bonus |= B_KEY;	/* bonuses: key to door */
      break;
    case I_BANG:
      w->bonus |= B_BANG;	/* explosives for going THROUGH */
      break;
    case I_LEAF:
      w->bonus |= B_LEAF;	/* food so that you're able to drop */
      new_bonus(I_LEAF);
      break;
    case I_GREASE:
      w->bonus |= B_GREASE;
      w->btime = 6;		/* no controls for some moves */
      break;
    case I_FLASH:
      w->bonus |= B_FLASH;
      new_bonus(I_FLASH);
      break;
    case I_1:
    case I_2:
    case I_3:
    case I_4:
    case I_5:
    case I_6:
    case I_7:
    case I_8:
    case I_9:
      w->score += SCORE_INC;
      show_score(w);
      if(lenght_bonus(B_LENGHT))	/* new one onto screen */
      {
        w->bonus |= B_LENGHT;
        w->btime = LenghtInc;		/* increase lenght a bit */
      }
      else				/* all collected */
      {
	w->lives++;
	return NEXT_LEVEL;
      }
      break;
    /* more to come? */
    default:			/* others */
      return FATAL;			/* -> chrash */
  }
  do_sound(SND_DING);
  return FALSE;
}

/* put a new bonus of a desired type onto the screen
 * return FALSE if all bonuses have been picked up.
*/
static int lenght_bonus(int which)
{
  static int index, x, y;
  int ok, xx, yy, bonus;

  if(which == B_FIRST)
    index = 1;
  else
  {
    if(index > 9)			/* last bonus? */
      return FALSE;

    /* remove the last bonus block */
    bonus = I_0 + index - 1;
    for(xx = x; xx < x + LenboSize; xx++)
      for(yy = y; yy < y + LenboSize; yy++)
	if(LevelMap[xx][yy] == bonus)
	{
	  LevelMap[xx][yy] = I_BG;
	  bitmap(xx, yy, I_BG);
	}
  }
  /* search a new place */
  do
  {
    /* a place on the map (not on the border) */
    x = rand() % (SCREEN_W - 1 - LenboSize) + 1;
    y = rand() % (SCREEN_H - 2 - LenboSize) + 1;
    ok = TRUE;
    /* is there enough space around? */
    for(xx = x; xx < x + LenboSize; xx++)
      for(yy = y; yy < y + LenboSize; yy++)
	if(LevelMap[xx][yy] != I_BG)
	  ok = FALSE;
  }
  while(!ok);

  /* output a new bonus */
  bonus = I_0 + index;
  for(xx = x; xx < x + LenboSize; xx++)
    for(yy = y; yy < y + LenboSize; yy++)
    {
      LevelMap[xx][yy] = bonus;
      bitmap(xx, yy, bonus);
    }

  /* next */
  index++;
  return TRUE;
}

/* search a new place for a bonus and output it */
static void new_bonus(int bonus)
{
  int x, y;

  while(1)
  {
    /* a place on the map (not on the border) */
    x = rand() % (SCREEN_W - 2) + 1;
    y = rand() % (SCREEN_H - 3) + 1;
    if(LevelMap[x][y] == I_BG)
    {
      LevelMap[x][y] = bonus;
      bitmap(x, y, bonus);
      return;
    }
  }
}

/* increase and show player scores */
static void show_score(Wyrms *w)
{
  int x = 0;

  switch(w->id)
  {
    case I_BLACK:
      x = PLAYER0_SCORE;
      break;
    case I_WHITE:
      x = PLAYER1_SCORE;
      break;
  }
  /* show the score on the screen */
  bitmap(x++, INFOLINE, w->score / 100 + '0');
  bitmap(x++, INFOLINE, w->score /  10 + '0');
  bitmap(x,   INFOLINE, w->score %  10 + '0');
}

/* decrease and show player lives */
static void show_lives(Wyrms *w)
{
  int x = 0;

  switch(w->id)
  {
    case I_BLACK:
      x = PLAYER0_LIVE;
      break;
    case I_WHITE:
      x = PLAYER1_LIVE;
      break;
  }
  bitmap(x, INFOLINE, w->lives % 10 + '0');
}

/* mark the other (or both) wyrm dead (who's 1st bit is for player one
 * and second bit for the player two)
 */
static void death(int who)
{
  int i, color;

  if(who == 3)
    color = I_GRAY;
  else
    color = WyrmPlr[-who + 2].id;	/* the other wyrm's color */

  /* wyrm 0 */
  if(who & 1)
    for(i = 0; i < WyrmPlr[0].lenght; i++)
      bitmap(WyrmPlr[0].pos[i].x, WyrmPlr[0].pos[i].y, color);

  /* wyrm 1 */
  if(who & 2)
    for(i = 0; i < WyrmPlr[1].lenght; i++)
      bitmap(WyrmPlr[1].pos[i].x, WyrmPlr[1].pos[i].y, color);
}


/* last possible item */
#define MAX_ITEM I_WHITE

/* edit the level */
static int edit_level(const char *file_name)
{
  int x = SCREEN_W / 2, y = SCREEN_H / 2, item = I_BG;
  int key, edit = TRUE;

  init_level(pLevelMap, WyrmPlr);
  LevelMap[SCREEN_W / 2 - 1][INFOLINE] = I_BG;
  LevelMap[SCREEN_W / 2 + 1][INFOLINE] = I_BG;
  LevelMap[SCREEN_W / 2][INFOLINE] = I_BG;
  draw_map(pLevelMap);

  while(edit)
  {
    key = get_position(&x, &y);
    switch(key)
    {
      /* ESC quits */
      case '\33':
        return FALSE;

      /* stop editing and / or save the result */
      case 'q':
      case 's':
        edit = FALSE;
        break;

      /* wyrm direction changing */
      case 'h':
        get_headings();
        break;

      /* change which item to put onto map */
      case '+':
        item++;
	if(item > MAX_ITEM)
	  item = 0;
	break;
      case '-':
        item--;
	if(item < 0)
	 item = MAX_ITEM;
	break;

      case '0':
        x = SCREEN_W / 2;
	y = SCREEN_H / 2;
	break;
    }
    /* assure that the cursor is on a valid place */
    if(x < 1)
      x  = 1;
    if(x > SCREEN_W - 2)
      x  = SCREEN_W - 2;
    if(y < 1)
      y  = 1;
    if(y > SCREEN_H - 3)
      y  = SCREEN_H - 3;

    /* show on screen and save into map */
    bitmap(SCREEN_W / 2, INFOLINE, item);
    LevelMap[x][y] = item;
    bitmap(x, y, item);
  }

  /* save the level data */
  if(key == 's')
  {
    if(!write_map(file_name, pLevelMap, WyrmPlr))
    {
      message("Error while writing the level file");
      return FALSE;
    }
    message("Saved!");
  }
  return TRUE;
}

/* get the wyrms' initial direction */
static void get_headings(void)
{
  char headings[SCREEN_W+1] = HEADINGS;
  int x, idx, dir;

  for(idx = 0; idx < WYRMS; idx++)
  {
    if(idx == 0)	x = PLAYER0_DIR;
    else		x = PLAYER1_DIR;
    switch(WyrmPlr[idx].dir)
    {
      case UP:		headings[x] = I_UP;	break;
      case DOWN:	headings[x] = I_DOWN;	break;
      case LEFT:	headings[x] = I_LEFT;	break;
      case RIGHT:	headings[x] = I_RIGHT;	break;
    }
  }
  for(x = 0; x < SCREEN_W; x++)
    bitmap(x, INFOLINE, headings[x]);

  while(1)
  {
    for(idx = 0; idx < WYRMS; idx++)
    {
      if(idx == 0)	x = PLAYER0_DIR;
      else		x = PLAYER1_DIR;
      dir = input(WyrmPlr[idx].device);
      switch(dir)
      {
	case BUTTON:
	  for(x = 0; x < SCREEN_W; x++)
	    bitmap(x, INFOLINE, LevelMap[x][INFOLINE]);
	  return;
	case UP:	bitmap(x, INFOLINE, I_UP);	break;
	case DOWN:	bitmap(x, INFOLINE, I_DOWN);	break;
	case LEFT:	bitmap(x, INFOLINE, I_LEFT);	break;
	case RIGHT:	bitmap(x, INFOLINE, I_RIGHT);	break;
	default:	dir = 0;
      }
      if(dir)	WyrmPlr[idx].dir = dir;
    }
  }
}
