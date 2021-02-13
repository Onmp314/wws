/*
 * WarZone game for W Window System and W Toolkit
 *
 * implements different shot types:
 * - shot 'part' list management (better not to spawn spawning ones :)).
 * - shot movements and checks for going off the WarZone or hitting ground.
 *
 * Notes:
 * - Laser is a special case.
 * - It's easy add new sizes of bombs, see shot_nuke().
 * - Range WIDTH, shot GRAVITY (100% power = flies over range width), shot
 *   POWER_FACTOR (not too small or big large position differencies in shot
 *   movements) and shield REMOTE_DISTANCE / effect values better correlate
 *   well enough...
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include <string.h>		/* memcpy */
#include <math.h>		/* trig functions */
#include "hill.h"
#include "sound.h"


#define DEG2RAD(deg)	((double)(deg) * (M_PI / 180.0))

#define POWER_FACTOR	0.02	/* multiply user given power with this */
#define GRAVITY		0.006	/* downward force->speed */
#define BOMB_PIT	12	/* def. bomb explosion radius */


/* local prototypes */
static Ammo *shot_parent(Ammo *shot);		/* shot part managing */


Ammo *shot_alloc(BUNKER *player)
{
  int power = player->power;
  Ammo *shot;

  /* allocate parent object */

  if(!(shot = malloc(sizeof(Ammo))))
  {
    fprintf(stderr, "shot_alloc: not enough memory\n");
    exit(-1);
  }
  shot->next = shot->prev = shot;


  /* setup for child inheritance */

  shot->handler = list_getbomb(player->index, player->b_idx);
  if(shot->handler == shot_laser)
  {
    shot->wind = shot->weight = 0.0;
    shot->sound = SND_LASER;
    shot->special = power;	    /* (note: x_inc/y_inc affect range too) */
    power = MAX_VALUE;
  }
  else
  {
    shot->wind = player->wind;
    shot->weight = GRAVITY;
    shot->sound = SND_STANDARD;
    shot->special = 0;
  }

  shot->x_inc = cos(player->angle);
  shot->y_inc = -sin(player->angle);		/* upwards on window */
  shot->sx = player->bx + (coord_t)shot->x_inc * (BUNKER_SIZE + BOMB_SIZE*2+2);
  shot->sy = player->by + (coord_t)shot->y_inc * (BUNKER_SIZE + BOMB_SIZE*2+2);
  shot->x_inc *= player->power * POWER_FACTOR;
  shot->y_inc *= player->power * POWER_FACTOR;

  shot->prev_x = shot->sx;
  shot->prev_y = shot->sy;
  shot->pitsize = BOMB_PIT;
  shot->power = DAMAGE_VALUE;

  shot->childs = 0;
  shot->parent = NULL;
  shot->child = NULL;

  return shot;
}

Ammo *shot_child(Ammo *parent)
{
  Ammo *shot;

  if(!parent)
    return NULL;

  if(!(shot = malloc(sizeof(Ammo))))
  {
    fprintf(stderr, "shot_child: not enough memory\n");
    exit(-1);
  }

  /* inherit (copy) properties from the parent */
  memcpy(shot, parent, sizeof(Ammo));

  if(parent->child)
  {
    shot->prev = parent->child;
    shot->next = parent->child->next;
    parent->child->next->prev = shot;
    parent->child->next = shot;
  }
  else
    shot->prev = shot->next = shot;

  shot->childs = 0;
  shot->child = NULL;
  shot->parent = parent;

  parent->handler = shot_parent;
  parent->child = shot;
  parent->childs++;
  return shot;
}


Ammo *shot_free(Ammo *shot)
{
  Ammo *tmp;

  /* recursively free children */
  if(shot->child)
    while(shot->childs)
      shot_free(shot->child);		/* modifies shot attributes too */

  /* relink */
  if(shot == shot->next)
    tmp = NULL;
  else
  {
    shot->prev->next = shot->next;
    shot->next->prev = shot->prev;
    tmp = shot->next;
  }

  /* modify parent */
  if(shot->parent)
  {
    shot->parent->childs--;
    if(shot->parent->child == shot)
      shot->parent->child = tmp;
  }

  free(shot);
  return tmp;
}

static Ammo *shot_parent(Ammo *shot)
{
  Ammo *child;
  int idx;

  if(!(child = shot->child))
    return shot_free(shot);

  /* handle all the child parts */
  idx = shot->childs;
  while(idx--)
   child = child->handler(child);

  return shot->next;
}

/* 
 * helper functions
 */

static void shot_newdir(Ammo *shot, double angle, double power)
{
  shot->x_inc = cos(angle);
  shot->y_inc = -sin(angle);			/* upwards on window */
  shot->x_inc *= power * POWER_FACTOR;
  shot->y_inc *= power * POWER_FACTOR;
}

static int check_move(Ammo *shot, int *x, int *y)
{
  /* new shot position */
  *x = (int)(shot->sx + 0.5);
  *y = (int)(shot->sy + 0.5);

  shot->y_inc += shot->weight;
  shot->x_inc += shot->wind;

  /* hit bunker(s)? */
  if(player_check(x, y, BOMB_SIZE+1, shot))
  {
    range_shotoff(shot->prev_x, shot->prev_y);
    player_hit(*x, *y, shot->pitsize, shot->power, shot->sound);
    return 1;
  }

  /* shot went out of the window or escaped gravity? */
  if(*x < 0 || *x >= range_width())
  {
    range_shotoff(shot->prev_x, shot->prev_y);
    return 1;
  }

  shot->sx += shot->x_inc;
  shot->sy += shot->y_inc;

  return 0;
}

static inline int shot_in_air(Ammo *shot, int x, int y)
{
  /* shot still on air? */
  if(y+BOMB_SIZE < range_y(x))
  {
    /* remove old shot and draw new */
    range_shot(&(shot->prev_x), &(shot->prev_y), x, y);
    return 1;
  }
  return 0;
}


/*
 * different shot type handling functions
 */

/* standard shot */
Ammo *shot_standard(Ammo *shot)
{
  int x, y;

  /* move the shot and make some checks */
  if(check_move(shot, &x, &y))
    return shot_free(shot);

  /* if the shot didn't hit anything, parse/return next shot */
  if(shot_in_air(shot, x, y))
    return shot->next;

  /* hit bunker(s)? */
  range_shotoff(shot->prev_x, shot->prev_y);
  player_hit(x, y, shot->pitsize, shot->power, shot->sound);
  return shot_free(shot);
}

Ammo *shot_big(Ammo *shot)
{
  shot->pitsize = shot->pitsize * 3 / 2;
  shot->handler = shot_standard;
  return shot_standard(shot);
}

/* twice as big, heavy and devastating as default shot */
Ammo *shot_nuke(Ammo *shot)
{
  shot->handler = shot_standard;
  shot->pitsize *= 2;
  shot->weight *= 2;
  shot->power *= 2;

  return shot_standard(shot);
}

/* goes straight and only for certain distance */
Ammo *shot_laser(Ammo *shot)
{
  static int min, max = -1;
  int x, y;

  if(check_move(shot, &x, &y))
  {
    if(max >= 0)
    {
      range_lower(min, max);
      max = -1;
    }
    return shot_free(shot);
  }

  /* lower range part if shot was underground */
  if(y+BOMB_SIZE >= range_y(x))
  {
    if(max < 0)
    {
      min = range_width();
      max = 0;
    }
    if(x > max)
      max = x;
    if(x < min)
      min = x;
  }
  else if(max >= 0)
  {
    range_lower(min, max);
    max = -1;
  }

  /* laser power spent? */
  if(shot->special--)
  {
    /* through anything except bunkers */
    range_shot(&(shot->prev_x), &(shot->prev_y), x, y);
    return shot->next;
  }
  else
  {
    /* remove shot */
    range_shotoff(shot->prev_x, shot->prev_y);
    if(max >= 0)
    {
      range_lower(min, max);
      max = -1;
    }
    return shot_free(shot);
  }
}

/* bomb that spawns into three before starting to descent */
Ammo *shot_triple(Ammo *shot)
{
  Ammo *new;
  int x, y;

  /* move the shot and make some checks */
  if(check_move(shot, &x, &y))
    return shot_free(shot);

  /* starting to go downwards? */
  if(shot->y_inc >= 0.0)
  {
    double inc = 0.8;
    int idx;

    for(idx = 0; idx < 3; idx++)
    {
      /* spawn new one */
      new = shot_child(shot);
      new->handler = shot_standard;
      new->x_inc = shot->x_inc * inc;
      inc += 0.2;
    }
    return shot->child;
  }

  if(shot_in_air(shot, x, y))
    return shot->next;

  /* hit ground and exploded -> hit bunker(s)? */
  range_shotoff(shot->prev_x, shot->prev_y);
  player_hit(x, y, shot->pitsize, shot->power, shot->sound);
  return shot_free(shot);
}

/* bomb that spawns further bombs when it hits the ground */
Ammo *shot_surprise(Ammo *shot)
{
  int x, y, idx;
  Ammo *new;

  /* move the shot and make some checks */
  if(check_move(shot, &x, &y))
    return shot_free(shot);

  if(shot_in_air(shot, x, y))
    return shot->next;

  /* current position above ground */
  shot->sy = range_y(x) - (BOMB_SIZE+1);

  /* hit the ground, spawn new ones */
  for(idx = 0; idx < 5; idx++)
  {
    new = shot_child(shot);
    shot_newdir(new, DEG2RAD(RND(60)+60), RND(10)+20);
    new->handler = shot_standard;
  }
  do_sound(SND_PING);
  return shot->child;
}

/* shot that burrows into ground and then surfaces */
Ammo *shot_burrow(Ammo *shot)
{
  static int min, max = -1;	/* for burrow lenght */
  int x, y;

  /* move the shot and make some checks */
  if(check_move(shot, &x, &y))
  {
    if(max >= 0)
    {
      range_lower(min, max);
      max = -1;
    }
    return shot_free(shot);
  }

  /* going on air? */
  if(shot_in_air(shot, x, y))
  {
    if(shot->weight > 0.0)
      return shot->next;

    /* lower the mountain by shot height */
    range_lower(min, max);
    max = -1;

    /* surfaced and exploded -> hit bunker(s)? */
    range_shotoff(shot->prev_x, shot->prev_y);
    player_hit(x, y, shot->pitsize, shot->power, shot->sound);
    return shot_free(shot);
  }

  /* hit the ground -> start burrowing */
  if(shot->weight > 0.0)
  {
    shot->sound = SND_GROUNDHOG;
    shot->weight = -shot->weight;
    shot->x_inc = shot->x_inc * 0.8;
    shot->y_inc = shot->y_inc * 0.8;
    min = range_width();
    max = 0;
  }
  /* these are needed for range_lower() as groundhog might
   * change it's direction.
   */
  if(x > max)
    max = x;
  if(x < min)
    min = x;

  range_shot(&(shot->prev_x), &(shot->prev_y), x, y);
  return shot->next;
}

/* a bouncing (from ground) shot */
Ammo *shot_bounce(Ammo *shot)
{
  int x, y;

  if(check_move(shot, &x, &y))
    return shot_free(shot);

  if(shot_in_air(shot, x, y))
    return shot->next;

  y = range_y(x) - BOMB_SIZE;

  /* three bounces */
  if(shot->special < 2)
  {
    do_sound(SND_BOUNCE);
    shot->y_inc = -fabs(shot->y_inc) * 0.8;
    shot->x_inc = shot->x_inc * 0.8;
    shot->sy = y-1;	/* less than in a check in shot_in_air() */
    shot->special++;

    /* draw the new shot */
    range_shot(&(shot->prev_x), &(shot->prev_y), x, y);
    return shot->next;
  }
  else
  {
    range_shotoff(shot->prev_x, shot->prev_y);
    player_hit(x, y, shot->pitsize, shot->power, shot->sound);
    return shot_free(shot);
  }
}

/* a rolling (on the ground) shot */
Ammo *shot_roll(Ammo *shot)
{
  int x, y;

  if(check_move(shot, &x, &y))
    return shot_free(shot);

  if(shot_in_air(shot, x, y))
    return shot->next;

  /* hit the ground -> proceed */
  y = range_y(x) - BOMB_SIZE;

  /* current place lower than the next? -> boom */
  if(shot->special)
  {
    if(range_y(x) > range_y(x + shot->special))
    {
      range_shotoff(shot->prev_x, shot->prev_y);
      player_hit(x, y, shot->pitsize, shot->power, shot->sound);
      return shot_free(shot);
    }
  }
  else
  {
    int dir;

    /* rolling direction */
    dir = SGN(shot->x_inc);

    /* note: smaller y value is higher on the window */
    if(range_y(x) <= range_y(x + dir))
      shot->special = dir;
    else
      shot->special = -dir;
    shot->x_inc = shot->special;
  }
  shot->sy = y;

  /* show the shot */
  range_shot(&(shot->prev_x), &(shot->prev_y), x, y);
  return shot->next;
}
