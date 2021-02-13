/*
 * WarZone game for W Window System and W Toolkit
 *
 * Player bunker handling:
 * - initialize whole game and round variables.
 * - bunker drawing / player input / shot move / shield check delegating.
 * - checking of bunker damages / scores.
 * - wind.
 *
 * TODO:
 * - Bunker dropping when land beneath them goes away.
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include <string.h>			/* string functions */
#include <math.h>			/* M_PI */
#include <time.h>			/* usleep() */
#include <stdio.h>
#include <unistd.h>
#include "hill.h"			/* global prototypes */
#include "sound.h"


#define BUNKER_PIT	(2*BUNKER_SIZE)	/* explosion size */

/* players can be manipulated both by pointer and index */
static BUNKER
  *Bunkers,		/* player array */
  *Bunker;		/* current player ptr */

static int
  Current,		/* current player */
  Players;		/* number of players */


/* local functions */
static int hit_damage(int x, int y, int r, int damage);


/* ------------
 * Initializations
 *
 * These manipulate player values through Bunkers[] array.
 */

/* Linked list of players might be nicer? */
int players_alloc(int players)
{
  if(Bunkers)
    players_free();

  if((Bunkers = malloc(sizeof(BUNKER) * players)))
  {
    memset(Bunkers, 0, sizeof(BUNKER) * players);
    Players = players;
    return 1;
  }
  fprintf(stderr, "players_alloc: not enough memory!\n");
  return 0;
}


/* set the player name */
void player_name(int idx, char *name)
{
  if(idx < Players)
  {
    if(Bunkers[idx].name)
    {
      if(strcmp(name, Bunkers[idx].name))
        free(Bunkers[idx].name);
      else
        return;
    }
    if((Bunkers[idx].name = malloc(strlen(name)+1)))
      strcpy(Bunkers[idx].name, name);
  }
}


void players_free(void)
{
  int idx;

  if(Bunkers)
  {
    for(idx = 0; idx < Players; idx++)
      if(Bunkers[idx].name)
        free(Bunkers[idx].name);

    free(Bunkers);
  }
  Players = 0;
}


void players_score(int amount)
{
  int idx;

  for(idx = 0; idx < Players; idx++)
    Bunkers[idx].score = amount;
}


/* initilize all the players and position them on the range */
void players_init(void)
{
  int range, idx, min, max, offset;

  /* player initializations */
  range = range_width() - (BUNKER_SIZE * 2);
  max = range / (Players-1);
  min = range / Players;
  max -= min;

  Bunkers[0].bx = 0;
  for(idx = 1; idx < Players; idx++)
  {
    /* position the bunkers on range with a little random */
    Bunkers[idx].bx = Bunkers[idx-1].bx + min + RND(max);
  }

  /* for centering the bunker positions onto the range */
  offset = (range - Bunkers[idx-1].bx) / 2 + BUNKER_SIZE;


  /* suffle the player positions */
  for(idx = 0; idx < Players; idx++)
  {
    max = RND(Players);
    min = Bunkers[idx].bx;
    Bunkers[idx].bx = Bunkers[max].bx;
    Bunkers[max].bx = min;
  }


  for(idx = 0; idx < Players; idx++)
  {
    Bunkers[idx].index = idx;
    Bunkers[idx].bx += offset;
    Bunkers[idx].by = range_y(Bunkers[idx].bx);

    /* clear an area on the range for the bunker */
    range_area(Bunkers[idx].bx, BUNKER_SIZE + 1);

    /* set other values */
    if(Bunkers[idx].bx * 2 < range_width())
      Bunkers[idx].angle = M_PI / 4.0;		/* shoot right */
    else
      Bunkers[idx].angle = M_PI * 3.0 / 4.0;	/* and left */
    Bunkers[idx].energy = MAX_VALUE;
    Bunkers[idx].power = MAX_VALUE / 2;

    Bunkers[idx].shield = MAX_VALUE;
    Bunkers[idx].shielder = NULL;
    Bunkers[idx].type = SHIELD_NONE;
    Bunkers[idx].s_idx = 0;
    Bunkers[idx].b_idx = 0;

    Bunkers[idx].wind = 0.0;
    Bunkers[idx].alive = TRUE;
  }

  Bunker = &Bunkers[0];
}


/* ----------
 * Playing
 *
 * Player_select() uses first Bunkers[] array and then Bunker pointer,
 * but rest use only the pointer.
 */

int player_select(int new)
{
  int idx, alive = 0;
  coord_t wind;

  for(idx = 0; idx < Players; idx++)
    if(Bunkers[idx].alive)
    {
      /* (re)draw / reset the player images */
      if(Bunkers[idx].type == SHIELD_NONE)
        range_bunker(Bunkers[idx].bx, Bunkers[idx].by, 1);
      else
	range_bunker(Bunkers[idx].bx, Bunkers[idx].by, -1);

      alive++;
    }

  if(alive < 2)
  {
    round_over();
    return 0;
  }



  /* select new (next alive) player */
  wind = Bunker->wind;
  while(!Bunkers[new].alive)
    new = (new+1) % Players;

  Bunker = &Bunkers[new];

  /* new wind value */
  wind += (coord_t)(RND(WIND_CHANGE*20) - WIND_CHANGE*10) / 10.0 * WIND_UNIT;
  if(wind > WIND_MAX)
    wind = WIND_MAX;
  if(wind < -WIND_MAX) 
    wind = -WIND_MAX;
  Bunker->wind = wind;


  /* select & get values */
  if(Bunker->type == SHIELD_NONE)
    range_bunker(Bunker->bx, Bunker->by, 3);
  else
    range_bunker(Bunker->bx, Bunker->by, -3);

  input_open(Bunker);
  return 1;
}


/* using timeouts for this is a bit slow, but I hope not too slow :)) */
void player_shoot(Ammo *shot)
{
  int moves = 3;

  while(moves-- > 0)
  {
    /* handle the missile parts */
    if(!(shot = shot->handler(shot)))
    {
      /* mark the bomb used and get a new one */
      Bunker->b_idx = list_usebomb(Bunker->index, Bunker->b_idx);

      /* shooting done -> next player */
      player_select((Bunker->index + 1) % Players);
      return;
    }
  }
  wt_addtimeout(1L, (void(*)(long))player_shoot, (long)shot);
}


/* ------------
 * Player checks
 *
 * These use again the Bunkers[] array.
 */

/* check whether something hit into a bunker */
int player_check(int *x, int *y, long r, Ammo *shot)
{
  int i;
  long dx, dy;

  r += BUNKER_SIZE;
  for(i = 0; i < Players; i++)
  {
    if(Bunkers[i].alive)
    {
      /* if we change something, it's the shot, therefore d{x,y} is relative
       * to shot position.
       */
      dx = *x - Bunkers[i].bx;
      dy = *y - Bunkers[i].by;

      /* contact? */
      if(dx * dx + dy * dy <= r * r)
      {
	if(Bunkers[i].type == SHIELD_CONTACT)
	  Bunkers[i].shielder(dx, dy, shot);
	else
	  return 1;		/* default => report a bunker hit */
      }

      /* remote shields don't affect own shots */
      if(&Bunkers[i] != Bunker && Bunkers[i].type == SHIELD_REMOTE)
      {
        /* close enough? */
        if(dx * dx + dy * dy <= (REMOTE_DISTANCE * REMOTE_DISTANCE))
          Bunkers[i].shielder(dx, dy, shot);
      }
    }
  }
  return 0;
}


/* dig an explosion pit and check for casulties */
void player_hit(int x, int y, int r, int max, int snd)
{
  int i, effect, bonus;

  range_pit(x, y, r, snd);

  r += BUNKER_SIZE;
  for(i = 0; i < Players; i++)
  {
    if(Bunkers[i].alive)
    {
      effect = hit_damage(ABS(Bunkers[i].bx - x), ABS(Bunkers[i].by - y), r, max);

      if(effect)
      {
        bonus = 0;
	if(Bunkers[i].shield > effect)
	{
	  Bunkers[i].shield -= effect;
	  bonus = effect;
	}
	else
	{
	  /* shield ruined */
	  bonus = Bunkers[i].shield;
	  effect -= Bunkers[i].shield;
	  Bunkers[i].type = SHIELD_NONE;
	  Bunkers[i].shield = 0;

	  if(Bunkers[i].energy > effect)
	  {
	    Bunkers[i].energy -= effect;
	    bonus += effect;
	  }
	  else
	  {
	    /* no energy left -> die */
	    bonus += Bunkers[i].energy + DESTROY_BONUS;
	    Bunkers[i].alive = FALSE;

	    /* make and check exlosion explosion pit */
            range_bunker(Bunkers[i].bx, Bunkers[i].by, 0);
	    player_hit(Bunkers[i].bx, Bunkers[i].by, BUNKER_PIT, DAMAGE_VALUE, SND_BUNKER);
	  }
	}
	/* managed to get own bunker scratched */
	if(&Bunkers[i] == Bunker)
	  Bunkers[Current].score -= bonus/10;
	else
	  Bunkers[Current].score += bonus;
      }
    }
  }
}


/* object distances, object connection limit, max return value */
static int hit_damage(int x, int y, int r, int damage)
{
  long d;

  /* close enough for better check? */
  if(x <= r && y <= r)
  {
    d = (long)x * (long)x + (long)y * (long)y;

    /* contact? */
    if(d <= r * r)
      return (damage * (r - isqrt(d)) / r);
  }
  return 0;
}

