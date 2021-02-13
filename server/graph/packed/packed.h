/*
 * packed.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- prototypes for `packed' graphics driver routines
 */


#if defined(MONO)
#define FUNCTION(name) packed_mono_ ## name
#elif defined(COLORMONO)
#define FUNCTION(name) packed_colormono_ ## name
#elif defined(COLOR)
#define FUNCTION(name) packed_color_ ## name
#endif


/*
 * internal
 */

extern ushort bfmask16[16][16];
extern ulong bfmask32[32][32];


/*
 * exported
 */


extern void packed_mono_mouseShow (MOUSEPOINTER *mptr);
extern void packed_mono_mouseHide (void);
extern void packed_mono_plot (BITMAP *bm, long x0, long y0);
extern long packed_mono_test (BITMAP *bm, long x0, long y0);
extern void packed_mono_line (BITMAP *bm, long x0, long y0, long xe, long ye);
extern void packed_mono_hline (BITMAP *bm, long x0, long y0, long xe);
extern void packed_mono_vline (BITMAP *bm, long x0, long y0, long ye);
extern void packed_mono_box (BITMAP *bm, long x0, long y0, long width,
			     long height);
extern void packed_mono_pbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void packed_mono_dvline (BITMAP *bm, long x0, long y0, long ye);
extern void packed_mono_dhline (BITMAP *bm, long x0, long y0, long xe);
extern void packed_mono_dbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void packed_mono_dpbox (BITMAP *bm, long x0, long y0, long width,
			       long height);
extern void packed_mono_bitblk (BITMAP *bm, long x0, long y0, long width,
				long height, BITMAP *bm1, long x1, long y1);
extern void packed_mono_scroll (BITMAP *bm, long x0, long y0, long width,
				long height, long y1);
extern void packed_mono_prints (BITMAP *bm, long x0, long y0, const uchar *s);
extern void packed_mono_dplot (BITMAP *bm, long x0, long y0);
extern void packed_mono_dline (BITMAP *bm, long x0, long y0, long xe, long ye);
extern BITMAP *packed_mono_createbm (BITMAP *bm, short width, short height, short do_alloc);

extern void packed_colormono_mouseShow (MOUSEPOINTER *mptr);
extern void packed_colormono_mouseHide (void);
extern void packed_colormono_plot (BITMAP *bm, long x0, long y0);
extern long packed_colormono_test (BITMAP *bm, long x0, long y0);
extern void packed_colormono_line (BITMAP *bm, long x0, long y0, long xe, long ye);
extern void packed_colormono_hline (BITMAP *bm, long x0, long y0, long xe);
extern void packed_colormono_vline (BITMAP *bm, long x0, long y0, long ye);
extern void packed_colormono_box (BITMAP *bm, long x0, long y0, long width,
			     long height);
extern void packed_colormono_pbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void packed_colormono_dvline (BITMAP *bm, long x0, long y0, long ye);
extern void packed_colormono_dhline (BITMAP *bm, long x0, long y0, long xe);
extern void packed_colormono_dbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void packed_colormono_dpbox (BITMAP *bm, long x0, long y0, long width,
			       long height);
extern void packed_colormono_bitblk (BITMAP *bm, long x0, long y0, long width,
				long height, BITMAP *bm1, long x1, long y1);
extern void packed_colormono_scroll (BITMAP *bm, long x0, long y0, long width,
				long height, long y1);
extern void packed_colormono_prints (BITMAP *bm, long x0, long y0, const uchar *s);
extern void packed_colormono_dplot (BITMAP *bm, long x0, long y0);
extern void packed_colormono_dline (BITMAP *bm, long x0, long y0, long xe, long ye);
extern BITMAP *packed_colormono_createbm (BITMAP *bm, short width, short height, short do_alloc);


extern void packed_color_mouseShow (MOUSEPOINTER *mptr);
extern void packed_color_mouseHide (void);
extern void packed_color_plot (BITMAP *bm, long x0, long y0);
extern long packed_color_test (BITMAP *bm, long x0, long y0);
extern void packed_color_line (BITMAP *bm, long x0, long y0, long xe, long ye);
extern void packed_color_hline (BITMAP *bm, long x0, long y0, long xe);
extern void packed_color_vline (BITMAP *bm, long x0, long y0, long ye);
extern void packed_color_box (BITMAP *bm, long x0, long y0, long width,
			     long height);
extern void packed_color_pbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void packed_color_dvline (BITMAP *bm, long x0, long y0, long ye);
extern void packed_color_dhline (BITMAP *bm, long x0, long y0, long xe);
extern void packed_color_dbox (BITMAP *bm, long x0, long y0, long width,
			      long height);
extern void packed_color_dpbox (BITMAP *bm, long x0, long y0, long width,
			       long height);
extern void packed_color_bitblk (BITMAP *bm, long x0, long y0, long width,
				long height, BITMAP *bm1, long x1, long y1);
extern void packed_color_scroll (BITMAP *bm, long x0, long y0, long width,
				long height, long y1);
extern void packed_color_prints (BITMAP *bm, long x0, long y0, const uchar *s);
extern void packed_color_dplot (BITMAP *bm, long x0, long y0);
extern void packed_color_dline (BITMAP *bm, long x0, long y0, long xe, long ye);
extern BITMAP *packed_color_createbm (BITMAP *bm, short width, short height, short do_alloc);
