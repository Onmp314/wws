/*
 * Server driver (w) 1996 by Eero Tamminen.
 * Based on the Pente program 'brain'
 * Copyright (C) 1994 William Shubert.
 *
 * See "copyright.h" for more copyright information.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
/* framework */
#include "server.h"
#include "messages.h"
#include "ports.h"
/* pente brain */
#include "play.h"
#include "comp.h"

#define VERSION		"0.6"
#define DEF_LEVEL	3
#define MIN_LEVEL	1
#define MAX_LEVEL	5

#ifdef __MINT__
long _stksize = 32000;
#endif

Rnd *pe_rnd;					/* for brain */
static pl_t Play;
static int Level, StartLevel = DEF_LEVEL;	/* computer level */
static int Moving, CompX, CompY;
static GAME *Game;

/* engine function prototypes. */
static void help(void);
static int set_level(int level);
static void initialize_game(void);
static void compute_move(void);			/* calculate computer move */
static int send_move(void);			/* send it */
static int make_move(int x, int y);		/* perform moves on board */
static int messages(short type, uchar x, uchar y);
static int parse_args(char *name, int argc, char *argv[]);


/* information for the frontend. */
void get_configuration(GAME *game)
{
  /* startup variables */
  game->game_id = PENTE_ID;

  /* function */
  game->level   = set_level;
  game->args    = parse_args;
  game->message = messages;
  game->start   = initialize_game;
  game->comp    = compute_move;
  game->move    = send_move;
  /* no 'pass' */

  Game = game;
  pe_rnd = rnd_create(time(NULL) ^ getpid());
}

static int set_level(int level)
{
  if(level < MIN_LEVEL || level > MAX_LEVEL)
    level = DEF_LEVEL;

  StartLevel = level;
  /* level ok */
  return True;
}

static void help(void)
{
  fprintf(stderr, "\nPente brain Copyright (C) 1994 William Schubert\n");
  fprintf(stderr, "Pente server " VERSION " (w) 1996 by Eero Tamminen\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -l  computer level (%d-%d)\n", MIN_LEVEL, MAX_LEVEL);
  fprintf(stderr, "  -h  help\n\n");
  fprintf(stderr, "This program is free software; you can redistribute it and/or modify\n");
  fprintf(stderr, "it under the terms of the GNU General Public License as published by\n");
  fprintf(stderr, "the Free Software Foundation, see copyright.h and GPL for more info.\n");
  sleep(2);
}

static int parse_args(char *name, int argc, char *argv[])
{
  int idx = -1;

  while(++idx < argc)
  {
    /* one letter option */
    if(argv[idx][0] == '-' && argv[idx][1] && !argv[idx][2])
    {
      switch(argv[idx][1])
      {
        /* level */
        case 'l':
	  if(++idx >= argc || !set_level(atoi(argv[idx])))
	  {
	    help();
	    return False;
	  }
#ifdef DEBUG
	fprintf(stderr, "Level = %d\n", StartLevel);
#endif
	break;

	case 'h':
	  help();
	  break;

	default:
	  help();
	  return False;
      }
    }
    else
    {
      help();
      return False;
    }
  }
  /* ok */
  return True;
}

static void initialize_game(void)
{
  Level = StartLevel;
  pl_init(&Play);
}

/* calculate the computer move */
static void compute_move(void)
{
  bd_loc_t move;
  move = comp_selectmove(&Play, Level);
  CompX = bd_loc_x(move);
  CompY = bd_loc_y(move);
  Moving = True;
}

/* first send the computers move to opponent, and then inform
 * frameworks about the game turn change
 */
static int send_move(void)
{
  if(Moving)
  {
    send_msg(PIECE_MINE, CompX, CompY);
    Moving = False;
    return True;
  }
  /* moving done */
  send_msg(MOVE_NEXT, 0, 0);
  return False;
}

static int messages(short type, uchar x, uchar y)
{
  if(!Game->playing)
  {
    /* no board editing */
    send_msg(RETURN_FAIL, 0, 0);
    return True;
  }

  switch(type)
  {
    case PIECE_MINE:
      send_msg(RETURN_MINE, x, y);
      return make_move(x, y);

    case RETURN_MINE:
      return make_move(x, y);
  }

  /* unknown game message -> error */  
  return False;
}

/* make computer and opponent moves. return True if move ok */
static int make_move(int x, int y)
{
  bd_loc_t move = bd_xy_loc(x, y);

  /* check move validity */
  if(!pl_movep(&Play, move))
    return False;

  /* make the move */
  if(pl_move(&Play, move))
  {
#ifdef DEBUG
    if(Game->my_move)
      DEBUG_PRINT("I won!");
    else
      DEBUG_PRINT("I lost...");
#endif
    game_over();
  }
#ifdef DEBUG
  {
    int x, y;

    for(y = 0; y < 19; y++)
    {
      for(x = 0; x < 19; x++)
      {
	switch(Play.board.grid[bd_xy_loc(x,y)])
	{
	  case BD_PLAYER(0):
	    fprintf(stderr, ("#");
	    break;
	  case BD_PLAYER(1):
	    fprintf(stderr, ("O");
	    break;
	  case BD_WINNER(0):
	  case BD_WINNER(1):
	    fprintf(stderr, ("*");
	    break;
	  default:
	    fprintf(stderr, (".");
	}
      }
      fprintf(stderr, ("\n");
    }
  }
#endif
  return True;
}
