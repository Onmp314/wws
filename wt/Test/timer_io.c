/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * test program for the timeouts and additional inputs.
 *
 * $Id: timer_io.c,v 1.2 1999/05/16 15:05:38 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <Wlib.h>
#include <Wt.h>

long timeout_handle;
long input_handle;

static void timer_callback (long arg)
{
	w_beep ();
	timeout_handle = wt_addtimeout (500, timer_callback, 0);
}

static void input_callback (long arg, fd_set *r, fd_set *w, fd_set *e)
{
	while (fgetc (stdin) != '\n')
		;
	printf ("input ready\n");
}

int
main ()
{
	widget_t *top;
	fd_set rfdset;

	top = wt_init ();
	if (!top)
		return 1;

	timeout_handle = wt_addtimeout (500, timer_callback, 0);
	if (timeout_handle < 0)
		return 2;

	FD_ZERO (&rfdset);
	FD_SET (0, &rfdset);
	input_handle = wt_addinput (&rfdset, NULL, NULL, input_callback, 0);
	if (input_handle < 0)
		return 3;

	wt_realize (top);
	wt_run ();
	return 0;
}
