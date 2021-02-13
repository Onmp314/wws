/*
 * rect.h, a part of the W Window System
 *
 * Copyright (C) 1996 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 */

#ifndef _RECT_H
#define _RECT_H

#define XL(R)	((R)->x0)
#define XR(R)	((R)->x0 + (R)->w)
#define YO(R)	((R)->y0)
#define YU(R)	((R)->y0 + (R)->h)

#define RECT_ERROR	((REC *)1)

extern REC *rect_create (int, int, int, int);
extern void rect_destroy (REC *);
extern void rect_list_destroy (REC *);
extern REC *rect_subtract (REC *, REC *, REC *, int *cut);
extern REC *rect_list_subtract (REC *, REC *, int dofree, int *cut);
extern REC *rect_clip (REC *, REC *, REC *, int *cut);
extern REC *rect_list_clip (REC *, REC *, int dofree, int *cut);
extern int rect_intersect (REC *, REC *, REC *);
extern void rect_union (REC *, REC *, REC *);
extern void rectUpdateDirty (REC *, int x, int y, int w, int h);

/*
 * return nonzero if the point is inside rectangle R.
 */

#define rect_cont_point(R,x,y) \
(XL(R) <= (x) && (x) < XR(R) && YO (R) <= (y) && (y) < YU (R))

/*
 * return nonzero if rectangle `inner' is inside of `outer'.
 */

#define rect_cont_rect(outer,inner) \
(XL (outer) <= XL (inner) && XR (inner) <= XR (outer) && \
 YO (outer) <= YO (inner) && YU (inner) <= YU (outer))

#endif
