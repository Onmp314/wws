/*
 * WarZone game for W Window System and W Toolkit
 *
 * Sound effect identifiers.
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

extern void do_sound(int index);

enum
{
  SND_RANGE,		/* WarZone opened */
  SND_SHOOT,		/* bomb shot */
  SND_PING,		/* bunker shield 'bounce' */
  SND_ABSORB,		/* bunker shield 'absorb' */
  SND_BUNKER,		/* bunker explosion */
  SND_STANDARD,		/* bomb explosion (default) */
  SND_GROUNDHOG,	/* 'goundhog' bomb */
  SND_BOUNCE,		/* bomb bounce */
  SND_LASER,		/* 'laser' bomb */
  SOUNDS		/* how many sounds */
};
