/*
 * gstructs.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- graphics driver structs for different screen types
 *
 * NOTES
 * - correct defines have to be present...
 * - drivers built completely upon graphics libraries (like libGGI),
 *   can and should have their own structs instead
 */

#include "generic/generic.h"
#ifdef BMONO
#include "monochrome/bmono.h"
#endif
#ifdef DIRECT8
#include "direct8/direct8.h"
#endif
#if defined(PMONO) || defined(PCOLORMONO) || defined(PCOLOR)
#include "packed/packed.h"	/* planar atari modes */
#endif

#if PACKEDMONO
#error is mono
#endif
#ifdef PACKEDCOLOR
#error is color
#endif

/*
 * a dummy default routine:
 * - monochrome (or yet unimplemented color) drivers may use this
 * - but in general case this should be overrided by the code including this
 */
static short dummyPutCmap(COLORTABLE *colTab, short index)
{
  /* dummy, dummy, dummy... */
  return 0;
}

#ifdef PCOLOR
static SCREEN packed_color_screen = {
  {}, dummyPutCmap, NULL, packed_color_mouseShow, packed_color_mouseHide,
  packed_color_createbm, packed_color_bitblk, packed_color_scroll,
  packed_color_prints, packed_color_test, packed_color_plot,
  packed_color_line, packed_color_hline, packed_color_vline, generic_box,
  generic_pbox, generic_circ, generic_pcirc, generic_ellipse,
  generic_pellipse, generic_arc, generic_pie, generic_poly,
  generic_ppoly, generic_bezier, packed_color_dplot, packed_color_dline,
  packed_color_dvline, packed_color_dhline, generic_dbox, generic_dpbox,
  generic_dcirc, generic_dpcirc, generic_dellipse, generic_dpellipse,
  generic_darc, generic_dpie, generic_dpoly, generic_dppoly,
  generic_dbezier };
#endif

#ifdef PCOLORMONO
static SCREEN packed_colormono_screen = {
  {}, dummyPutCmap, NULL, packed_colormono_mouseShow,
  packed_colormono_mouseHide, packed_colormono_createbm,
  packed_colormono_bitblk, packed_colormono_scroll,
  packed_colormono_prints, packed_colormono_test, packed_colormono_plot,
  packed_colormono_line, packed_colormono_hline, packed_colormono_vline,
  generic_box, generic_pbox, generic_circ, generic_pcirc,
  generic_ellipse, generic_pellipse, generic_arc, generic_pie,
  generic_poly, generic_ppoly, generic_bezier, packed_colormono_dplot,
  packed_colormono_dline, packed_colormono_dvline,
  packed_colormono_dhline, generic_dbox, generic_dpbox, generic_dcirc,
  generic_dpcirc, generic_dellipse, generic_dpellipse, generic_darc,
  generic_dpie, generic_dpoly, generic_dppoly, generic_dbezier };
#endif

#ifdef PMONO
static SCREEN packed_mono_screen = {
  {}, dummyPutCmap, NULL, packed_mono_mouseShow, packed_mono_mouseHide,
  packed_mono_createbm, packed_mono_bitblk, packed_mono_scroll,
  packed_mono_prints, packed_mono_test, packed_mono_plot,
  packed_mono_line, packed_mono_hline, packed_mono_vline, generic_box,
  generic_pbox, generic_circ, generic_pcirc, generic_ellipse,
  generic_pellipse, generic_arc, generic_pie, generic_poly,
  generic_ppoly, generic_bezier, packed_mono_dplot, packed_mono_dline,
  packed_mono_dvline, packed_mono_dhline, generic_dbox, generic_dpbox,
  generic_dcirc, generic_dpcirc, generic_dellipse, generic_dpellipse,
  generic_darc, generic_dpie, generic_dpoly, generic_dppoly,
  generic_dbezier };
#endif

#ifdef BMONO
static SCREEN bmono_screen = {
	{}, dummyPutCmap, NULL, bmono_mouseShow, bmono_mouseHide,
	bmono_createbm, bmono_bitblk, bmono_scroll,
	bmono_prints, bmono_test,
	bmono_plot, bmono_line, bmono_hline, bmono_vline,
	generic_box, generic_pbox, generic_circ, generic_pcirc,
	generic_ellipse, generic_pellipse, generic_arc, generic_pie,
	generic_poly, generic_ppoly, generic_bezier, bmono_dplot,
	bmono_dline, bmono_dvline, bmono_dhline, generic_dbox,
	generic_dpbox, generic_dcirc, generic_dpcirc, generic_dellipse,
	generic_dpellipse, generic_darc, generic_dpie, generic_dpoly,
	generic_dppoly, generic_bezier
};
#endif

#ifdef DIRECT8
static SCREEN direct8_screen = {
	{}, dummyPutCmap, NULL, direct8_mouseShow, direct8_mouseHide,
	direct8_createbm, direct8_bitblk, direct8_scroll,
	direct8_prints, direct8_test,
	direct8_plot, direct8_line, direct8_hline, direct8_vline,
	generic_box, generic_pbox, generic_circ, generic_pcirc,
	generic_ellipse, generic_pellipse, generic_arc, generic_pie,
	generic_poly, generic_ppoly, generic_bezier, direct8_dplot,
	direct8_dline, direct8_dvline, direct8_dhline, generic_dbox,
	generic_dpbox, generic_dcirc, generic_dpcirc, generic_dellipse,
	generic_dpellipse, generic_darc, generic_dpie, generic_dpoly,
	generic_dppoly, generic_bezier
};
#endif

