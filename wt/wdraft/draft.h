/* Draw for W
 *
 * draw.h contains the callback structure typedef and
 * callback function prototypes.
 * 
 * w/ 1996 by Eero Tamminen
 */

#ifndef __DRAW_H__
#define __DRAW_H__

#include <Wlib.h>
#include <Wt.h>

#define DRAW_PROPERTIES	(EV_MOUSE | EV_KEYS | EV_ACTIVE)
#define TOOL_PROPERTIES	(EV_MOUSE | EV_KEYS)
#define PROG_NAME	"DRAFT"

/* the toolkit widget grandma */
extern widget_t *Top;
extern WWIN *Current;

/* flags for the different tool types
 *
 * FLAG_CALLNOW and FLAG_NODRAW tools are called immediately when their
 * icons are clicked.
 *
 * FLAG_CALLNOW, FLAG_NODRAW and FLAG_MULTI got to have a valid init
 * function.
 *
 * Tools that have FLAG_CALLNOW but not FLAG_NODRAW init function will be
 * called with a != 0 'next' argument when another tool is selected so
 * that the tool can remove it's dialogs etc. if necessary.
 */
#define FLAG_NORMAL	0	/* normal drawing function */
#define FLAG_MULTI	1	/* multiple points */
#define FLAG_RESUME	2	/* after use, get back to old */
#define FLAG_CALLNOW	4	/* eg. zoom or block load */
#define FLAG_NODRAW	8	/* eg. image load */


/* ToolBar entries contain the following things:
 *
 * 'shortcut'	keyboard shortcut for the tool.
 * 'icon'	called to draw the tool 'icon'.
 * 'init'	called at the first mouse click or on all left mouse button
 *		clicks upto first right click if FLAG_MULTI is set.  On first
 *		point the 'next' argument is zero.  If returns != 0; shape,
 *		move and final functions will not be called.
 * 'shape'	called at frequent intervals until there's another click.
 * 'move'	called at frequent intervals if previous click was accepted.
 * 'final'	called at position acception.
 * 'attrib'	called to set the tool attributes if available.
 * 'flag'	tells what kind of thing the tool is.
 *
 * If attrib function is NULL, it will be skipped.
 * If FLAG_MULTI isn't set, shape can be NULL (->skipped).
 * if FLAG_NODRAW function is set, init, shape, move and final
 * functions aren't called (and can therefore be NULL).
 *
 * function from init() to move() will be called in M_INVERS mode.  It
 * doesn't matter if functions place the graphics to different place with
 * same arguments (like circle_size() and circle_move() do) as the previous
 * drawing is always inversed away before drawing a new.
 */
typedef struct
{
  int shortcut;
  void (*icon)(WWIN *win, int x, int y, int size);
  int  (*init)(WWIN *win, int x, int y, int next);
  void (*shape)(WWIN *win, int x, int y);
  void (*move)(WWIN *win, int x, int y);
  void (*final)(WWIN *win, int x, int y);
  void (*attrib)(void);
  int flags;
} TOOLBAR;


/* toolbar.c (main) */

extern WEVENT * handle_drawing(widget_t *w, WEVENT *ev);

/* create.c */

extern void load_file(widget_t *w, const char *file);
extern void icon_new(WWIN *win, int x, int y, int size);
extern int  init_new(WWIN *win, int x, int y, int next);


/* graphics.c */

extern int  set_point(WWIN *win, int x, int y, int next);
extern int  set_points(WWIN *win, int x, int y, int next);
extern void line_attrib(void);

extern void icon_line(WWIN *win, int x, int y, int size);
extern void size_line(WWIN *win, int x, int y);
extern void move_line(WWIN *win, int x, int y);
extern void final_line(WWIN *win, int x, int y);

extern void icon_box(WWIN *win, int x, int y, int size);
extern void size_box(WWIN *win, int x, int y);
extern void move_box(WWIN *win, int x, int y);
extern void final_box(WWIN *win, int x, int y);

extern void icon_circle(WWIN *win, int x, int y, int size);
extern void size_circle(WWIN *win, int x, int y);
extern void move_circle(WWIN *win, int x, int y);
extern void final_circle(WWIN *win, int x, int y);

extern void icon_poly(WWIN *win, int x, int y, int size);
extern void size_poly(WWIN *win, int x, int y);
extern void move_poly(WWIN *win, int x, int y);
extern void final_poly(WWIN *win, int x, int y);


/* zoom.c */

extern void icon_zoom(WWIN *win, int x, int y, int size);
extern int  init_zoom(WWIN *win, int x, int y, int next);
extern void zoom_block(WWIN *win, int x, int y);
extern void zoom_attrib(void);

#endif

