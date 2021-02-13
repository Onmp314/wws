/*
 * jewel.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wjewel main loop, argument parsing and help
 */

#include <stdio.h>
#include <stdlib.h>		/* atoi(), srand() */
#include <memory.h>		/* malloc() */
#include <time.h>		/* clock() */
#include "jewel.h"		/* defines and prototypes */

/* argument limits */
#define MAX_STARTLEVEL	(BASE_SPEED / SPEED_INCREASE / 2)


/* local prototypes */
static const char *parse_args(int argc, char *argv[],
	PlayField *field, Blocks *pieces);
static void show_help(const char *name);

int main(int argc, char *argv[])
{
  int i;
  const char *error;
  Blocks pieces = LargeBlocks;
  PlayField field = { GAME_WIDTH, GAME_HEIGHT, OPT_NEXT, 0L, 0, 0 };

  /* parse command line arguments if any */
  if((error = parse_args(argc, argv, &field, &pieces)))
  {
    fprintf(stderr, "%s: %s\n\n", *argv, error);
    show_help(*argv);
    return -1;
  }

  /* allocate the colums field space */
  if((field.area = (char **)malloc(field.width * sizeof(*field.area))))
  {
    for(i = 0; i < field.width; i++)
      if(!(field.area[i] = (char*)malloc(field.height)))
        jewel_exit("unable to allocate memory");
  }
  else
    jewel_exit("unable to allocate memory");

  /* initialize GUI */
  if((error = init_win(field.width, field.height, field.options, &pieces)))
    jewel_exit(error);

  srand(time(0));

  while(do_game(&field))
  {
    field.level++;
    if(!(field.level % BLK_JEWEL))
      field.random++;
    while(!check_input(-1));			/* until something */
  }
  while(!check_input(-1));

  fprintf(stderr, "Jewel score: %ld (level %d)\n", field.score, field.level);
  return 0;
}

/* exit. We'll trust that W server removes the windows and that
 * C-lib frees the allocated memory as they should...
 */
void jewel_exit(const char *error)
{
  if(error)
  {
    fprintf(stderr, "%s\n", error);
    exit(-1);
  }
  exit(0);
}

/* show game options */
static void show_help(const char *name)
{
  fprintf(stderr, WJEWEL " w/ 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi\n\n");
  fprintf(stderr, "Usage: %s [<options>]\n", name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -w <width (blocks)>\n");
  fprintf(stderr, "  -h <height (blocks)>\n");
  fprintf(stderr, "  -f <first level (0 - %d)>\n", MAX_STARTLEVEL);
  fprintf(stderr, "  -r <rows of random blocks at start>\n");
  fprintf(stderr, "  -n Toggle showing of the next column\n");
  fprintf(stderr, "  -b Toggle border around game area\n");
  fprintf(stderr, "  -l Use large bitmaps (default)\n");
  fprintf(stderr, "  -s Use small bitmaps\n");
  fprintf(stderr, "  -d switch debugging on\n");

  fprintf(stderr, "\nKeys: '%c' - left, '%c' - right, '%c' - roll, "
       "'%c' - drop and '%c' - pause.\n",
  	KEY_LEFT, KEY_RIGHT, KEY_TURN, KEY_DROP, KEY_PAUSE);
}

/* set global variables according to arguments */
static const char *parse_args(int argc, char *argv[],
  PlayField *field, Blocks *pieces)
{
  int idx = 0;
  while(++idx < argc)
  {
    /* an one letter option? */
    if(argv[idx][0] == '-' && argv[idx][2] == '\0')
      switch(argv[idx][1])
      {
	case 'w':				/* game width (blocks) */
	  if(++idx >= argc || (field->width = atoi(argv[idx])) > 7 ||
	     field->width < SCORE_WIDTH)
	    return "width argument error";
	  break;
	case 'h':				/* game height (blocks) */
	  if(++idx >= argc || (field->height = atoi(argv[idx])) < COLUMN * 2)
	    return "height argument error";
	  break;
	case 'f':				/* game starting level */
	  if(++idx >= argc || (field->level = atoi(argv[idx])) < 1 ||
	     field->level > MAX_STARTLEVEL)
	    return "level argument error";
	  field->random = field->level / BLK_JEWEL;
	  break;
	case 'r':
	  if(++idx >= argc || (field->random = atoi(argv[idx])) < 1)
	    return "random argument error";
	  break;
	case 'b':				/* toggle border */
          field->options ^= OPT_BORDER;
	  break;
	case 'n':				/* toggle next column win */
          field->options ^= OPT_NEXT;
	  break;
	case 's':				/* small pieces */
          *pieces = SmallBlocks;
	  break;
	case 'l':				/* big pieces */
          *pieces = LargeBlocks;
	  break;
	case 'd':
	  debugging_on();
	  break;
	default:
	  return "unrecognized argument";
      }
    else
      return "not an argument";
  }
  if(field->height - COLUMN < field->random)
    return "Insensible argument values";

  return 0;	/* success */
}

