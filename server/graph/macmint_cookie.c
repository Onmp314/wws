/* 
 * cookie.c, a part of the W Window System
 *
 * Copyright (C) 1999 by Jonathan Oddie
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- routines for finding and using the MacMiNT-specific 'SVAR' cookie
 * 
 * cookieinit(); -- get the sysvar cookie in global variable sysvar
 * cookiesetup(); -- set up the environment for making Mac system calls
 * cookiecleanup(); -- restore a normal MiNT environment
 *
 */

#if (!defined(__MINT__) || !defined(MAC))
#error This file is only needed when building wserver under *MacMiNT*
#endif

#include <stdio.h>
#include <osbind.h>
 /* in ../xconout2/mint */
#include <locore.h>

#define SIG_SVAR 0x53564152 /* 'SVAR' */

struct sysvars *sysvar;
void *saveda5 asm("saveda5");
void *savedsp asm("savedsp");

long ssp;

/* prototypes */
void cookieinit(void);
void cookiesetup(void);
void cookiecleanup(void);

void cookieinit(void)
{
	long *cookie;

	ssp = (long)Super(0L);
	cookie = *(long **)0x5a0L;
	while (cookie[0]) {
		if (cookie[0] == SIG_SVAR)
			sysvar = (struct sysvars *)cookie[1];
		cookie += 2;
	}
	if (sysvar == NULL) {
		fprintf(stderr, "can't find SVAR cookie\n");
		exit(1);
	}
	if (sysvar->version != SVAR_VERS) {
		fprintf(stderr, "sysvar structure does not match\n");
		exit(1);
	}
	saveda5 = sysvar->saveda5;
	savedsp = sysvar->savedsp;
	(void)Super((void *)ssp);
}

void cookiesetup(void)
{
	ssp = (long)Super(0L);
	MAC_ENV;
}

void cookiecleanup(void)
{
	MINT_ENV;
	(void)Super((void *)ssp);
}
