/*
 * bmono.h, a part of the W Window System
 *
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- prototypes for byte order neutral mono graphic routines
 */

/* this defines what the function names are perpended with */
#define FUNCTION(name) bmono_ ## name

/*
 * internal
 */

extern uchar bfmask8[8][8];


/*
 * exported
 */


extern void bmono_mouseShow (MOUSEPOINTER *mptr);
extern void bmono_mouseHide (void);
extern void bmono_plot (BITMAP *bm, long x0, long y0);
extern long bmono_test (BITMAP *bm, long x0, long y0);
extern void bmono_line (BITMAP *bm, long x0, long y0, long xe, long ye);
extern void bmono_hline (BITMAP *bm, long x0, long y0, long xe);
extern void bmono_vline (BITMAP *bm, long x0, long y0, long ye);
extern void bmono_box (BITMAP *bm, long x0, long y0, long width,
			     long height);
extern void bmono_pbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void bmono_dvline (BITMAP *bm, long x0, long y0, long ye);
extern void bmono_dhline (BITMAP *bm, long x0, long y0, long xe);
extern void bmono_dbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void bmono_dpbox (BITMAP *bm, long x0, long y0, long width,
			       long height);
extern void bmono_bitblk (BITMAP *bm, long x0, long y0, long width,
				long height, BITMAP *bm1, long x1, long y1);
extern void bmono_scroll (BITMAP *bm, long x0, long y0, long width,
				long height, long y1);
extern void bmono_prints (BITMAP *bm, long x0, long y0, const uchar *s);
extern void bmono_dplot (BITMAP *bm, long x0, long y0);
extern void bmono_dline (BITMAP *bm, long x0, long y0, long xe, long ye);
extern BITMAP *bmono_createbm (BITMAP *bm, short width, short height, short do_alloc);
