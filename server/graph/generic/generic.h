/*
 * generic.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- prototypes for generic graphic routines
 */


/* in box.c */

extern void generic_box (BITMAP *, long x0, long y0, long w, long h);
extern void generic_pbox (BITMAP *, long x0, long y0, long w, long h);
extern void generic_dbox (BITMAP *, long x0, long y0, long w, long h);
extern void generic_dpbox (BITMAP *, long x0, long y0, long w, long h);

/* in circle.c */

extern void generic_circ (BITMAP *, long x0, long y0, long r);
extern void generic_pcirc (BITMAP *, long x0, long y0, long r);
extern void generic_dcirc (BITMAP *, long x0, long y0, long r);
extern void generic_dpcirc (BITMAP *, long x0, long y0, long r);
extern void generic_ellipse (BITMAP *, long x0, long y0, long rx, long ry);
extern void generic_pellipse (BITMAP *, long x0, long y0, long rx, long ry);
extern void generic_dellipse (BITMAP *, long x0, long y0, long rx, long ry);
extern void generic_dpellipse (BITMAP *, long x0, long y0, long rx, long ry);
extern void generic_arc (BITMAP *, PIZZASLICE *slice);
extern void generic_pie (BITMAP *, PIZZASLICE *slice);
extern void generic_darc (BITMAP *, PIZZASLICE *slice);
extern void generic_dpie (BITMAP *, PIZZASLICE *slice);

/* in polygon.c */

extern void generic_poly (BITMAP *, long n, long *vertices);
extern void generic_ppoly (BITMAP *, long n, long *vertices);
extern void generic_dpoly (BITMAP *, long n, long *vertices);
extern void generic_dppoly (BITMAP *, long n, long *vertices);

/* in bezier.c */
extern void generic_bezier (BITMAP *, long *controls);
extern void generic_dbezier (BITMAP *, long *controls);
