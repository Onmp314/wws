/*
 * Networked two player, turn based (board) game framework.
 *
 * A programming interface between game frontend and engine(s),
 * Curses specific function prototypes.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <curses.h>
#include "game.h"		/* standard includes & interface */


/* draw board grid, etc. return True for success */
extern int initialize_board(void);

/* place for some short information lines */
extern void add_text(int x, int y);

/* process user input (key events). */
extern void process_key(int key);
