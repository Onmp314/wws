/*
 * direct8.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- prototypes for `direct8' graphics driver routines
 */

extern void direct8_mouseShow (MOUSEPOINTER *mptr);
extern void direct8_mouseHide (void);
extern void direct8_plot (BITMAP *bm, long x0, long y0);
extern long direct8_test (BITMAP *bm, long x0, long y0);
extern void direct8_line (BITMAP *bm, long x0, long y0, long xe, long ye);
extern void direct8_hline (BITMAP *bm, long x0, long y0, long xe);
extern void direct8_vline (BITMAP *bm, long x0, long y0, long ye);
extern void direct8_box (BITMAP *bm, long x0, long y0, long width, long height);
extern void direct8_pbox (BITMAP *bm, long x0, long y0, long width, long height);
extern void direct8_dvline (BITMAP *bm, long x0, long y0, long ye);
extern void direct8_dhline (BITMAP *bm, long x0, long y0, long xe);
extern void direct8_dbox (BITMAP *bm, long x0, long y0, long width, long height);
extern void direct8_dpbox (BITMAP *bm, long x0, long y0, long width, long height);
extern void direct8_bitblk (BITMAP *bm, long x0, long y0, long width, long height, BITMAP *bm1, long x1, long y1);
extern void direct8_scroll (BITMAP *bm, long x0, long y0, long width, long height, long y1);
extern void direct8_prints (BITMAP *bm, long x0, long y0, const uchar *s);
extern void direct8_dplot (BITMAP *bm, long x0, long y0);
extern void direct8_dline (BITMAP *bm, long x0, long y0, long xe, long ye);
extern BITMAP *direct8_createbm(BITMAP *bm, short width, short height, short do_alloc);
