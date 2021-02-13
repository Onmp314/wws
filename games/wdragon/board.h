#include	<stdlib.h>
#include	<time.h>

#define BLACKSIDES	1

#define NFACES	42		/* number of distinct tile faces */
#define NTILES	((NFACES-8) * 4 + 8) /* number of distinct tiles in game */
#define NROWS	8		/* number of rows in game */
#define NCOLS	16		/* number of columns in game */
#define NO_TILE	0		/* tile codes are 1..NFACES */

/*--Size of the Board. */
extern int Board_Width;
extern int Board_Height;
extern int Score;
/*--X/Y coordinate of a tile at coord 0,0 of the playing surface. */

extern int		Board_Tile0_X;
extern int		Board_Tile0_Y;

/*--Actual size of a tile, not counting the border around it. */

extern unsigned int	Tile_Width;
extern unsigned int	Tile_Height;

/*--X depth and Y width of a shadowed area above/to-the-right-of a tile. */

extern int		Shadow_X;
extern int		Shadow_Y;

/*--X depth and Y width of tile edge below/to-the-left-of a tile. */

extern int		Side_X;
extern int		Side_Y;

/*--Each playing tile position is represented by one of these. */

typedef struct _Board_Position {
    char		tiles[4];	/* at most 4 tiles at one position */
    Boolean		draw;		/* TRUE if needs drawing */
    int			level;		/* how many tiles do we have */
    int			x;		/* x coord of tile */
    int			y;		/* x coord of tile */
} Board_Position_Rec, *Board_Position;

#define Board_Position_NULL	((Board_Position)0)

extern Board_Position_Rec	Board_Tiles[NROWS][NCOLS];

#define SPEC1		0][14		/* farthest right odd tile */
#define SPEC1row	0		/* farthest right odd tile */
#define SPEC1col	14		/* farthest right odd tile */
#define SPEC2		2][14		/* next farthest right odd tile */
#define SPEC2row	2		/* next farthest right odd tile */
#define SPEC2col	14		/* next farthest right odd tile */
#define SPEC3		4][14		/* farthest left odd tile */
#define SPEC3row	4		/* farthest left odd tile */
#define SPEC3col	14		/* farthest left odd tile */
#define SPEC4		6][14		/* center piece tile */
#define SPEC4row	6		/* center piece tile */
#define SPEC4col	14		/* center piece tile */


/*--Record tile clicks and watch for double clicks. */
extern Board_Position		Click1;
extern int			Click1_Row;
extern int			Click1_Col;
extern Board_Position		Click2;
extern int			Click2_Row;
extern int			Click2_Col;
