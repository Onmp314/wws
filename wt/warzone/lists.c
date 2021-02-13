/*
 * WarZone game for W Window System and W Toolkit
 *
 * Shield and Weapon list management:
 * - buying / selling of gadgets at the start of the round.
 * - managing lists of stuff each of player has.
 *
 * TODO (about everything):
 * - marketplace dialog for players.
 * - List allocation for players (called form hill.c).
 * - Selecting stuff for player lists from the major list (called form hill.c).
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include "hill.h"		/* global prototypes */


/* Lists of all shields and missiles */

/* shield action functions */
static void (*Shielder[])(long, long, Ammo*) =
{
  NULL,
  shield_bounce,
  shield_absorb,
  shield_raise,
  shield_repulse
};

static const char *ShieldNames[] =
{
  "None",
  "Bounce",
  "Absorb",
  "Raise",
  "Repulse",
  NULL
};


/* bomb action functions */
static Ammo *(*Shooter[])(Ammo *) =
{
  shot_standard,
  shot_bounce,
  shot_roll,
  shot_burrow,
  shot_laser,
  shot_big,
  shot_nuke,
  shot_triple,
  shot_surprise
};

static const char *BombNames[] =
{
  "Standard",
  "Bouncer",
  "Roller",
  "Digger",
  "Laser",
  "Bigger",
  "Nuker",
  "Tripler",
  "Surprise",
  NULL
};


widget_t
  *Shell;


/* --------------
 * marketplace initializations
 */

/* create window */
void market_init(void)
{
}

/* close window */
void market_close(void)
{
  wt_close(Shell);
}

/* open window and process market */
void market_open(void)
{
  if(!Shell)
  {
    market_init();
    wt_realize(Top);
  }

  /* set vars and
   * wt_open(Shell);
   */

  /* actually should be on a market exit callback */
  round_start();
}


/* --------------
 * list initialization / exit functions
 */

/* free players' weapon and shield lists */
void lists_free(void)
{
}


/* --------------
 * list use functions
 */

/* return array of names of the bombs player has */
const char **list_bombs(int player)
{
  return BombNames;
}

/* return array of names of the shields player has */
const char **list_shields(int player)
{
  return ShieldNames;
}


/* decrease the count for item 'idx' and tif here are still items left
 * return it, otherwise the default.
 */
int list_usebomb(int player, int idx)
{
  return idx;
}

int list_useshield(int player, int idx)
{
  return idx;
}


/* return the handler function for the item with index 'idx' */
Ammo*(*list_getbomb(int player, int idx))(Ammo*)
{
  return Shooter[idx];
}

void (*list_getshield(int player, int idx))(long, long, Ammo*)
{
  return Shielder[idx];
}

