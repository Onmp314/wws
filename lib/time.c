/*
 * time.c, a part of the W Window System
 *
 * Copyright (C) 1997 by Kay Römer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- get (signed) millisecond ticks which can be compared to each other
 *
 * CHANGES
 * ++eero, 5/98:
 * - I moved this into Wlib so that user has a function where he can
 *   get ticks with same resolution that w_queryevent() uses.
 * ++oddie, 4/99:
 * - w_gettime for MacMiNT, using sysvar cookie - see init.c
 */ 


#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "Wlib.h"

#if defined(__MINT__) && defined(MAC)
#include "../server/xconout2/mint/locore.h"
#endif


long w_gettime (void)
{
#ifdef __MINT__
# ifdef MAC
        /* MacMiNT: MiNTlib clock doesn't work on the mac! */
        return (sysvar->hz200 * 1000L / 50);
# else
	/* MiNTlib PL <48 have 2 sec gettimeofday() resolution!!! */
	return (clock () * 1000L / CLK_TCK);
# endif
#else
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (tv.tv_sec * 1000L + tv.tv_usec / 1000);
#endif
}
