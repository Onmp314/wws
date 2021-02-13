/*
 * window.h, a part of the W Window System
 *
 * Copyright (C) 1996 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- prototypes for window tree handling functions
 */

#ifndef _WINDOW_H
#define _WINDOW_H

#include "types.h"


extern WINDOW *glob_rootwindow, *glob_activewindow, *glob_activetopwin, *glob_backgroundwin;
extern short globOpenWindows, globTotalWindows;

extern long window_init (void);
extern WINDOW *window_create (WINDOW *parent, int flags);
extern long window_delete (WINDOW *);
extern long window_open (WINDOW *, int x, int y);
extern long window_open_all (WINDOW *, int x, int y);
extern long window_close (WINDOW *);
extern long window_move (WINDOW *, int x, int y);
extern long window_to_top (WINDOW *);
extern long window_to_bottom (WINDOW *);
extern WINDOW *window_find (int x, int y, int usecache);
extern int window_save_contents_rect (WINDOW *, void *rect);
extern int window_redraw_contents_rect (WINDOW *, void *rect);
extern WINDOW *wtree_traverse_pre (WINDOW *wtree, void *arg,
				   int (*f) (WINDOW *, void *));
extern void windowKillClient(CLIENT *cptr);
extern void windowActivate(WINDOW *win);
extern WINDOW *windowLookup (ushort id);
extern void windowDeactivate(WINDOW *win);
extern void windowRedrawAllIfDirty(void);

extern void windowDrawFrame (WINDOW *win, int dashed, int tobitmap, REC *clip);

#endif
