/*
 * wt-util.c, a part of the W Window System
 *
 * Copyright (C) 2000 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- functions needed for overloading var-arg Wt functions such as setopt
 * -- this is inlined into wt.c produced by 'tolua'
 */

#include <Wt.h>

/* set/get numeric option */
static inline long wt_setopt_number (widget_t *w, long option, long value)
{
	return w->class->setopt(w, option, &value);
}
static inline long wt_getopt_number (widget_t *w, long option, long* value)
{
	return w->class->getopt(w, option, value);
}

/* set/get string option */
static inline long wt_setopt_string (widget_t *w, long option, char* value)
{
	return w->class->setopt(w, option, value);
}
static inline long wt_getopt_string (widget_t *w, long option, char** value)
{
	return w->class->getopt(w, option, value);
}

/* set userdata option */
static inline long wt_setopt_userdata (widget_t *w, long option, void* value)
{
	return w->class->setopt(w, option, value);
}
