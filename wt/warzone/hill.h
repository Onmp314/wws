/*
 * WarZone game for W Window System and W Toolkit
 *
 * Global prototypes, defines and structures.
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#ifndef __HILL_H__
#define __HILL_H__
#define HILL_VERSION	"v0.7"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <Wlib.h>
#include <Wt.h>

/* type of co-ordinates values used for moving the shots */
typedef double coord_t;

#ifndef RND
#define RND(x)	(rand() % (x))
#endif
#ifndef SGN
#define SGN(x)	((x) < 0 ? -1 : 1)
#endif

#ifndef SWAP
#define	SWAP(a,b)	((a)=(a)^((b)=(b)^((a)=(a)^(b))))
#endif

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif


#define MAX_PLAYERS	8	/* max. number of players */
#define BUNKER_SIZE	12	/* bunker radius */
#define BOMB_SIZE	1	/* flying shot size = 2*BOMB_SIZE+1 */
#define MAX_VALUE	100	/* max. shooting power */
#define MAX_VALUE_STR	"100"
#define ENERGY_VALUE	200	/* staring values for shields and energy */
#define DAMAGE_VALUE	200	/* default explosion 'power' */
#define DESTROY_BONUS	100	/* bonus score for destroying a bunker */

#define WIND_UNIT	0.0001
#define WIND_MAX	(20.0 * WIND_UNIT)
#define WIND_CHANGE	3	/* max units at time */

/* NOTICE: handler routine can change the handler,
 * while the shot is flying!
 */
typedef struct AMMO
{
  coord_t sx;			/* bomb position */
  coord_t sy;
  coord_t x_inc;		/* movement vector */
  coord_t y_inc;
  coord_t weight;		/* => gravity effect */
  coord_t wind;			/* => wind effect */

  int prev_x;			/* to minimize flicker */
  int prev_y;

  int special;			/* bomb type specific info */
  int pitsize;			/* explosion size */
  int power;			/* explosion 'power' */
  int sound;			/* explosion sound id */

  int childs;
  struct AMMO *prev;
  struct AMMO *next;
  struct AMMO *child;
  struct AMMO *parent;
  struct AMMO *(*handler)(struct AMMO *);
} Ammo;


/* shield types */
typedef enum { SHIELD_NONE, SHIELD_CONTACT, SHIELD_REMOTE } shield_t;
#define REMOTE_DISTANCE	(BUNKER_SIZE * 4L)

/* first hits take away shields, after that they start to wear on
 * energy, which corresponds straight to how much power there is
 * for the shots...
 */
typedef struct
{
  /* used only by player.c */

  int alive;
  int bx;			/* bunker position */
  int by;

  int shield;			/* tank shielding left */
  void (*shielder)(long, long, Ammo*);
  shield_t type;		/* shield type */

  /* edited also by input.c */

  int score;			/* player score/money */
  int energy;			/* max value for power */
  int power;			/* shooting power */
  double angle;			/* shooting angle */
  coord_t wind;			/* current wind factor */

  char *name;			/* user name */
  const char **shields;		/* available shields */
  const char **bombs;		/* available bombs */

  int index;			/* player index */
  int s_idx;			/* shield index (selected) */
  int b_idx;			/* bomb index (selected) */
} BUNKER;


/* hill.c */
extern widget_t *Top;
extern void round_start(void);
extern void round_over(void);


/* range.c,
 * the general order of calling these functions
 */
extern int  range_alloc(void);
extern void range_free(void);
extern void range_init(void);
extern void range_open(void);
extern void range_close(void);

extern int  range_y(int x);
extern int  range_width(void);
extern void range_area(int x, int r);
extern void range_bunker(int x, int y, int border);
extern void range_shot(int *prev_x, int *prev_y, int new_x, int new_y);
extern void range_shotoff(int px, int py);
extern void range_lower(int minx, int maxx);
extern void range_pit(int x, int y, int r, int snd);


/* player.c */
extern int  players_alloc(int players);
extern void players_free(void);
extern void players_score(int score);
extern void player_name(int idx, char *name);
extern void players_init(void);

extern int  player_select(int idx);
extern void player_shoot(Ammo *shot);
extern void player_hit(int x, int y, int r, int max, int snd);
extern int  player_check(int *sx, int *sy, long sr, Ammo *shot);


/* input.c */
extern void input_init(void);
extern void input_close(void);
extern void input_open(BUNKER *input);


/* shot.c */
extern Ammo *shot_alloc(BUNKER *param);
extern Ammo *shot_child(Ammo *parent);
extern Ammo *shot_free(Ammo *shot);

extern Ammo *shot_standard(Ammo *shot);
extern Ammo *shot_laser(Ammo *shot);
extern Ammo *shot_big(Ammo *shot);
extern Ammo *shot_nuke(Ammo *shot);
extern Ammo *shot_triple(Ammo *shot);
extern Ammo *shot_surprise(Ammo *shot);
extern Ammo *shot_superjam(Ammo *shot);
extern Ammo *shot_burrow(Ammo *shot);
extern Ammo *shot_bounce(Ammo *shot);
extern Ammo *shot_roll(Ammo *shot);


/* shield.c */
extern shield_t shield_type(void (*handler)(long, long, Ammo*));
extern void shield_raise(long dx, long dy, Ammo *shot);
extern void shield_absorb(long dx, long dy, Ammo *shot);
extern void shield_bounce(long dx, long dy, Ammo *shot);
extern void shield_repulse(long dx, long dy, Ammo *shot);


/* lists.c */
extern void lists_free(void);
extern const char **list_bombs(int player);
extern const char **list_shields(int player);
extern int list_usebomb(int player, int idx);
extern int list_useshield(int player, int idx);
extern Ammo*(*list_getbomb(int player, int idx))(Ammo*);
extern void (*list_getshield(int player, int idx))(long, long, Ammo*);

extern void market_init(void);
extern void market_open(void);
extern void market_close(void);

#endif
