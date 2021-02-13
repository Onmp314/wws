/*
 * WarZone game for W Window System and W Toolkit
 *
 * Atari TOS XBios() sound chip effects for all kinds of things in game...
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include "sound.h"

#ifndef SOUND

void do_sound(int index)  {}

#else /* SOUND */

#ifndef atarist
#error "This sound system works only on Atari computers"
#error "Select the right file in Makefile or do not use the -DSOUND define"
#endif

#include <osbind.h>
#include "st_snd.h"

/* correlates to identifiers in "sound.h" */
static UBYTE *Sounds[SOUNDS] =
{
  snd_jingle,
  snd_laser,
  snd_gong,
  snd_pieuw,
  snd_steam,
  snd_explosion1,
  snd_bell,
  snd_bounce5,
  snd_tingeling2
};

void do_sound(int index)
{
  Dosound(Sounds[index]);
}

#endif
