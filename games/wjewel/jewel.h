/*
 * jewel.h, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- wjewel defines and prototypes
 */

#define WJEWEL		" W-Jewel v0.96 "

/* configuration defaults */

#define BASE_SPEED	600		/* msecs between column moves */
#define SPEED_INCREASE	50

/* playing area size in blocks */
#define GAME_WIDTH	6
#define GAME_HEIGHT	12

/* keys */
#define KEY_LEFT	'4'
#define KEY_RIGHT	'6'
#define KEY_TURN	'5'
#define KEY_DROP	'0'
#define KEY_PAUSE	'p'		/* also iconifies */
#define KEY_EXIT	'q'		/* q quits */

/* general game setups */
/* height of the columns */
#define COLUMN		3
/* average interval between jewel columns */
#define JEWEL_INTERVAL	64
/* how many blocks to kill on first level */
#define BLOCKS_FIRST	(GAME_HEIGHT*2)
/* how many blocks more / level */
#define BLOCKS_INC	(GAME_HEIGHT/3)

/* base score for a SCORE_WIDTH blocks found in a row, every additional
 * row piece + BLK_JEWELth level doubles the score...
 *
 * Kill score is added for every killed block so you get slightly
 * more points when you manage to delete separate rows.
 */
#define SCORE_WIDTH		COLUMN
#define VERTICAL_SCORE		20
#define HORIZONTAL_SCORE	25
#define DIAGONAL_SCORE		50
#define KILL_SCORE		10

/* Changes below would need changes elsewhere into source too... */

#define BLOCKS		8	/* block bitmaps (0-5 are normal ones) */
#define BLK_JEWEL	6	/* index of the last of the drop blocks! */
#define BLK_BORDER	7
/* these pieces are drawn by window.c */
#define BLK_BACK	8	/* background */
#define BLK_FLASH	9	/* flashed before block's deleted */

#define MASK_BLOCKS	63	/* mask for block ids (~FLAG_*) */
#define FLAG_DELETE	64	/* these blocks are marked for deletion */


/* option toggle flags for jewel.c & window.c */
#define OPT_NEXT	1	/* show next block */
#define OPT_BORDER	2	/* show border around game area */


/* structure for the bitmap pieces */
typedef struct
{
  int blk_w;			/* one block size */
  int blk_h;
  unsigned char *data;		/* bitmap data */
} Blocks;

typedef struct
{
  int width;			/* width in blocks */
  int height;			/* height in blocks */
  int options;			/* game options */
  long score;			/* user score */
  int level;			/* game level */
  int random;			/* random rows */
  int column[COLUMN];		/* current column */
  int next[COLUMN];		/* next column */
  char **area;			/* blocks on the area */
} PlayField;


/* prototypes */

/* bitmaps */
extern Blocks SmallBlocks;
extern Blocks LargeBlocks;

/* jewel.c */
extern void	jewel_exit(const char *error);

/* game.c */
extern int	do_game(PlayField *field);

/* window.c */
extern const char *init_win(int width, int height, int options, Blocks *pieces);
extern int	check_input(long timeout);
extern void	iconify(void);
extern void	draw_level(int level);
extern void	show_block(int type, int x, int y);
extern void	copy_blocks(int fromx, int fromy, int tox, int toy, int blocks);
extern void	show_next(int *column);
extern void	show_score(long score);
extern long	time_frame(int msecs);
extern void	debugging_on(void);
