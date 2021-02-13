/*
 * Wlib.h, a part of the W Window System
 *
 * Copyright (C) 1994-1998 by Torsten Scherer, Kay Roemer
 * and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- generic string output routines using driver specific character routines
 *
 * NOTES:
 * - This needs a FUNCTION macro on the file including this that which will
 *   expand the canonical FUNCTION(normalc), FUNCTION(styles),
 *   FUNCTION(clipc) and FUNCTION(prints) to their driver specific names.
 *
 * CHANGES
 * ++eero, 11/96:
 * - Split TeSche's and Kay's text.c code into generic and driver specific
 *   parts and moved clipping to prints.  I have no-styles character output
 *   as a separate routine now.
 * - Implemented underline with hline().
 * ++eero, 2/98:
 * - This is now a generic include file so that the driver specific
 *   character output routines can be inlined for it.
 * ++eero, 4/98:
 * - Made proper clipped char drawing routines to overcome troubles
 *   with bitblitting slanted characters.
 * - With very thin slanted font one might need to clip several characters
 *   from both ends of the string.  Take this into account.
 */


void FUNCTION(prints) (bm, x0, y0, s)
     BITMAP *bm;
     long x0;
     long y0;
     const uchar *s;
{
  FONT  *font = gc0->font;
  ushort effects = gc0->textstyle;
  uchar *widths = font->widths;
  int height = font->hdr.height;
  int slant, ul_x, width = 0, count = 0;
  const uchar *end;

  /* for underline */
  ul_x = x0;

  /* string lenght & width */
  end = s;
  while (*end) {
    width += widths[*end++];
    count++;
  }

  /* only necessary and allowed (readable) styles */
/*  effects &= font->hdr.effect_mask; */
  font->effects = effects;

  /* italic correction */
  if (effects & F_ITALIC) {
    /* check clipping against leftmost position */
    x0 -= font->slant_offset;
    slant = font->slant_size;
    x0 += slant;
  } else {
    slant = 0;
  }

  /* 
   * Clip necessary chars.  If there's a single character which needs to be
   * clipped both from left and right size, this clips only the right side.
   */
  if (glob_clip0) {

    int wd, yoff;

    /* check vertical clipping */
    if (y0 < glob_clip0->y0 || y0 + height - 1 > glob_clip0->y1) {
      if (y0 > glob_clip0->y1 || y0 + height <= glob_clip0->y0) {
        return;
      }
      yoff = glob_clip0->y0 - y0;
      if (yoff < 0 || yoff >= height) {
	yoff = 0;
      }
      wd = glob_clip0->y1 - y0;
      if (wd >= 0 && wd < height) {
        height = wd;
      }
      height -= yoff;
      y0 += yoff;
    } else {
      yoff = 0;
    }

    /* find the rightmost visible char and clip it if necessary */
    wd = glob_clip0->x1 - (x0 + width);

    if (wd < 0) {
      while (count) {
        count--;
	wd += widths[*--end];
	if (wd + slant >= 0) {
	  int ss = wd - widths[*end];
	  FUNCTION(clipc) (bm, glob_clip0->x1-wd, y0, 0, yoff, ss, wd, height, *end);
	  if (wd >= 0) {
	    break;
	  }
        }
      }
    }

    /* find the leftmost visible char and clip it if necessary */
    wd = x0 - glob_clip0->x0;

    if (wd - slant < 0) {
      int cw, xoff, ss, xx;

      while(count) {

        count--;
	cw = widths[*s++];
	wd += cw;

        if (wd >= 0) {

	  xx = glob_clip0->x0;
	  if (wd > cw) {
	    ss = wd - cw;
	    xx += ss;
	    xoff = 0;
	  } else {
	    xoff = cw - wd;
	    cw = wd;
	    ss = 1;
	  }

	  FUNCTION(clipc) (bm, xx, y0, xoff, yoff, ss, cw, height, *(s-1));
	  if (wd - slant >= 0) {
	    x0 = glob_clip0->x0 + wd;
	    break;
	  }
        }
      }
    }

    if (height != font->hdr.height) {	/* all characters clipped */

      while (--count >= 0) {
	FUNCTION(clipc) (bm, x0, y0, 0, yoff, 0, widths[*s], height, *s);
	x0 += widths[*s++];
      }
    }
  }


  /*
   * rest of the string doesn't need clipping...
   */

  while (--count >= 0) {
    FUNCTION(stylec) (bm, x0, y0, *s);
    x0 += widths[*s++];
  }


  if (effects & F_UNDERLINE) {
    ushort old_mode = gc0->drawmode;

    if ((effects | font->hdr.styles) & F_REVERSE) {
      gc0->drawmode = M_CLEAR;
    } else {
      gc0->drawmode = M_DRAW;
    }

    /* this baseline check is needed for e.g. console */
    if (font->hdr.baseline + 2 < height) {
      y0 += font->hdr.baseline + 2;
    } else {
      y0 += height - 1;
    }
    (*theScreen->hline)(bm, ul_x, y0, ul_x + width - 1);
    gc0->drawmode = old_mode;
  }
}
