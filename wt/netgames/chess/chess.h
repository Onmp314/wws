/* defines and function prototypes for Chess */

/* game event message IDs */
enum {
  PIECE_SELECT = ENGINE_MSG,		/* select piece to move */
  PIECE_MOVE,				/* move piece to a new location */
  PIECE_PROMOTE,			/* promote soldier */
  EN_PASSANT,				/* capture soldier 'in passing' */
  HIDE_KING,				/* 'fortificate' */

  RETURN_SELECT = ENGINE_RET,
  RETURN_MOVE,
  RETURN_PROMOTE,
  RETURN_PASSANT,
  RETURN_KING
};

/* piece flags */
#define EMPTY	0	/* none, empty board postion */
#define ROOK	1
#define KNIGHT	2
#define BISHOP	4
#define QUEEN	8
#define KING	16
#define PAWN	32
#define PIECES	63	/* bitmask */

/* piece color flags */
typedef enum { BLACK = 0, WHITE = 64 } color_t;
#define MOVED	128

#define COLOR(x)	(x & WHITE)		/* piece color */
#define PIECE(x)	(x & PIECES)		/* piece rank */
#define OPPONENT(x)	(x ^ WHITE)		/* opponent's color */
#define COLORPIECE(x)	(x & (PIECES | WHITE))	/* color & rank */


#define BOARD_SIZE 8
extern int BoardWidth, BoardHeight;

/* draws the pieces into a window */
extern void draw_mark(int x, int y, int on);
extern void draw_piece(int x, int y, char id);
extern int  promote_to(color_t color);

/* runs/shows the timer */
extern void start_timer(void);
extern void stop_timer(void);

/* handles mouse events, in the game specific part */
extern void my_move(int x, int y);
