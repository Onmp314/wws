/*
 * common variables and functions between GUI dependent and independent
 * parts of Go related games working on top of games framework.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "game.h"		/* message codes etc. */

/* possible board position states */
typedef enum { EMPTY = 0, BLACK, WHITE } color_t;

/* competition size, other common sizes are: 9x9, 13x13 */
#define BOARD_SIZE	19

extern int BoardWidth, BoardHeight;

/* common, draws the pieces into a window */
extern void draw_mark(int x, int y, color_t color);
extern void remove_mark(int x, int y, color_t color);
extern void draw_empty(int x, int y);
extern void draw_black(int x, int y);
extern void draw_white(int x, int y);

/* pente, go */
extern void draw_captures(color_t color, int captures);
extern void mark_captures(color_t color);

/* handles mouse events, in the game specific part */
extern void my_move(int x, int y);
