/*
 * Networked, two player, turn based (board) game framework.
 *
 * A programming interface between game frontend and engine(s),
 * W window system specific function prototypes.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <Wlib.h>
#include <Wt.h>
#include "game.h"		/* standard includes & interface */


/* add wigets to the left pane, return True for success */
extern int add_options(widget_t *left_pane);

/* returned widget (if any) will be linked to right of the board */
extern widget_t *add_rpane(void);

/* load images, draw board grid, etc. return True for success */
extern int initialize_board(widget_t *brd, int x, int y, int wd, int ht);

/* process user input (mouse events). */
extern void process_mouse(WEVENT *ev);
