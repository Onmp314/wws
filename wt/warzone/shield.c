/*
 * WarZone game for W Window System and W Toolkit
 *
 * Implements different bunker shield types:
 * - calculates shield specific effect onto given shot.
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include "hill.h"
#include "sound.h"


shield_t shield_type(void (*handler)(long, long, Ammo*))
{
  if(handler == shield_absorb || handler == shield_bounce)
    return SHIELD_CONTACT;
  
  if(handler == shield_repulse || handler == shield_raise)
    return SHIELD_REMOTE;

  return SHIELD_NONE;
}


/* raise shot amount inversely equal to distance square */
void shield_raise(long dx, long dy, Ammo *shot)
{
  shot->y_inc -= 28.0 / (coord_t)(dx*dx + dy*dy);
}


/* remove the shot */
void shield_absorb(long dx, long dy, Ammo *shot)
{
  shot->sx = -1000.0;		/* out of window -> will remove shot */
  do_sound(SND_ABSORB);
}


/* Project the shot movement vector S to the center(shot-obstacle) vector D,
 * divide by D lenght add to S.  New direction / speed:  S = S + S->D / |D|.
 * Totally bouncy :-)
 */
void shield_bounce(long dx, long dy, Ammo *shot)
{
  coord_t sd, d;

  d = dx*dx + dy*dy;					/* |D|^2 */

  sd = shot->x_inc * dx + shot->y_inc * dy;		/* S->D */
  if(sd < 0.0)
    sd = -sd;						/* away! */

  sd /= d;						/* normalize */
  shot->x_inc += sd * dx;
  shot->y_inc += sd * dy;
  shot->sx += shot->x_inc;
  shot->sy += shot->y_inc;

  do_sound(SND_PING);
}


/* same as bounce except that here the force is inversely equal
 * to the square of the distance
 */
void shield_repulse(long dx, long dy, Ammo *shot)
{
  coord_t sd, d;

  d = dx*dx + dy*dy;					/* |D|^2 */

  sd = shot->x_inc * dx + shot->y_inc * dy;		/* S->D */
  if(sd < 0.0)
    sd = -sd;						/* away! */

  sd /= d;
  sd /= d * 0.03;

  shot->x_inc += sd * dx;
  shot->y_inc += sd * dy;
}

