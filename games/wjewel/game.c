/*
 * game.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- Wjewel game logic
 */

#include <stdlib.h>		/* rand() */
#include "jewel.h"		/* defines and prototypes */


/* local prototypes */
static int guide_down(PlayField *field);
static int key_action(PlayField *field, int key, int x, int y, int *nodrop);
static int vertical(PlayField *field, int x);
static int horizontal(PlayField *field, int y);
static int diagonal(PlayField *field, int x, int y, int incy);
static int check_blocks(PlayField *field);
static int kill_blocks(PlayField *field);


/* do the game... */
int do_game(PlayField *field)
{
  int x, y, scores, todo = BLOCKS_FIRST + field->level * BLOCKS_INC;

  /* clear the field */
  for(x = 0; x < field->width; x++)
    for(y = 0; y < field->height; y++)
      field->area[x][y] = BLK_BACK;

  /* randomize the first column */
  for(x = 0; x < COLUMN; x++)
    field->next[x] = rand() % BLK_JEWEL;

  /* level ready, draw it... */
  draw_level(field->level);

  /* randomize bottom of the level if needed */
  for(y = field->height - field->random; y < field->height; y++)
    for(x = 0; x < field->width; x++)
    {
      field->area[x][y] = rand() % BLK_JEWEL;
      show_block(field->area[x][y], x, y);
    }

  do
  {
    /* current column blocks */
    for(x = 0; x < COLUMN; x++)
      field->column[x] = field->next[x];

    /* next column blocks (at random intervals there will be jewels... */
    if(!(rand() % JEWEL_INTERVAL))
      for(x = 0; x < COLUMN; x++)
        field->next[x] = BLK_JEWEL;
    else
      for(x = 0; x < COLUMN; x++)
        field->next[x] = rand() % BLK_JEWEL;

    if(field->options & OPT_NEXT)
      show_next(field->next);

    /* let user guide the block down */
    if(guide_down(field))
      return 0;			/* game over */

    /* check and remove blocks of similar type in rows */
    do
    {
      scores = check_blocks(field);
      if(scores)
	field->score += (field->level / BLK_JEWEL + 1) * scores;
    } while(scores);
    show_score(field->score);

  } while(--todo);

  return 1;			/* next level */
}

/* let the player guide the current column to the 'well' bottom */
static int guide_down(PlayField *field)
{
  int idx,
    nodrop = 1,
    timeout = BASE_SPEED - field->level * SPEED_INCREASE,
    x = field->width / 2,
    y = 0;
  long msecs;

  /* set time frame (msecs) */
  msecs = time_frame(timeout);

  /* draw the initial block */
  for(idx = 0; idx < COLUMN; idx++)
  {
    /* space all filled up? -> game over */
    if(field->area[x][idx] != BLK_BACK)
      return 1;
    show_block(field->column[idx], x, idx);
  }

  for(;;)
  {
    /* timeout for block move while checking for user input... */
    while(nodrop && msecs)
    {
      msecs = time_frame(0);
      x = key_action(field, check_input(msecs), x, y, &nodrop);
    }
    /* set new time frame (msecs) */
    msecs = time_frame(timeout);

    /* down one row... reached the bottom? => next column */
    if(y+COLUMN >= field->height || field->area[x][y+COLUMN] != BLK_BACK)
      break;

    /* copy column one row downwards */
    copy_blocks(x, y, x, y+1, COLUMN);
    /* remove the highest block */
    show_block(BLK_BACK, x, y);
    y++;
  }

  /* this column wasn't the 'jewel' one? */
  if(field->column[0] != BLK_JEWEL)
  {
    /* put the column onto the game area */
    for(idx = 0; idx < COLUMN; idx++)
      field->area[x][y + idx] = field->column[idx];
  }
  else
  {
    /* remove the jewels */
    for(idx = 0; idx < COLUMN; idx++)
      show_block(BLK_BACK, x, y+idx);

    /* something under the jewels? */
    if(y+idx < field->height)
    {
      /* block type that got under jewels */
      idx = field->area[x][y+idx];

      /* mark them for deletion */
      for(x = 0; x < field->width; x++)
	for(y = 0; y < field->height; y++)
	  if(field->area[x][y] == idx)
	    field->area[x][y] |= FLAG_DELETE;
    }
  }
  /* => next column */
  return 0;
}

/* do what the user wants... */
static int key_action(PlayField *field, int key, int x, int y, int *nodrop)
{
  int idx, oldx = x;

  switch(key)
  {
    case KEY_LEFT:
      if(x) x--;
      break;
    case KEY_RIGHT:
      if(++x == field->width) x--;
      break;
    case KEY_TURN:
      oldx = field->column[0];
      for(idx = 0; idx < COLUMN-1; idx++)
      {
        field->column[idx] = field->column[idx+1];
	show_block(field->column[idx], x, y+idx);
      }
      field->column[idx] = oldx;
      show_block(oldx, x, y+idx);
      return x;
    case KEY_DROP:
      *nodrop = 0;
      return x;
    case KEY_PAUSE:
      iconify();
      return x;
    case KEY_EXIT:
      jewel_exit(0);
  }
  if(x != oldx)
  {
    for(idx = 0; idx < COLUMN; idx++)
      /* can't move there? */
      if(field->area[x][y+idx] != BLK_BACK)
        return oldx;

    copy_blocks(oldx, y, x, y, COLUMN);
    /* remove blocks on the old positions... */
    for(idx = 0; idx < COLUMN; idx++)
      show_block(BLK_BACK, oldx, y+idx);
  }
  return x;
}


/* check vertically (down) */
static int vertical(PlayField *field, int x)
{
  int y, score = 0, blocks = 1;

  for(y = 1; y < field->height; y++)
    if(field->area[x][y] != BLK_BACK &&
       (field->area[x][y] & MASK_BLOCKS) == (field->area[x][y-1] & MASK_BLOCKS))
      blocks++;
    else
    {
      /* need at least three-in-a-row... */
      if(blocks >= SCORE_WIDTH)
      {
	score += (VERTICAL_SCORE << (blocks - SCORE_WIDTH));
        for(; blocks > 0; blocks--)
	  field->area[x][y-blocks] |= FLAG_DELETE;
      }
      blocks = 1;
    }
  if(blocks >= SCORE_WIDTH)
  {
    score += (VERTICAL_SCORE << (blocks - SCORE_WIDTH));
    for(; blocks > 0; blocks--)
      field->area[x][y-blocks] |= FLAG_DELETE;
  }
  return score;  
}

/* check horizontally (right) */
static int horizontal(PlayField *field, int y)
{
  int x, score = 0, blocks = 1;

  for(x = 1; x < field->width; x++)
    if(field->area[x][y] != BLK_BACK &&
       (field->area[x][y] & MASK_BLOCKS) == (field->area[x-1][y] & MASK_BLOCKS))
      blocks++;
    else
    {
      if(blocks >= SCORE_WIDTH)
      {
	score += (HORIZONTAL_SCORE << (blocks - SCORE_WIDTH));
	for(; blocks > 0; blocks--)
	  field->area[x-blocks][y] |= FLAG_DELETE;
      }
      blocks = 1;
    }
  if(blocks >= SCORE_WIDTH)
  {
    score += (HORIZONTAL_SCORE << (blocks - SCORE_WIDTH));
    for(; blocks > 0; blocks--)
      field->area[x-blocks][y] |= FLAG_DELETE;
  }
  return score;
}

/* check diagonally left->right, up or down (incy +1 or -1)
 * that way there are some redundant checks, but shouldn't matter
 */
static int diagonal(PlayField *field, int x, int y, int incy)
{
  int type, blocks, score;

  type = field->area[x][y] & MASK_BLOCKS;
  score = 0;
  blocks = 1;

  while(++x, y += incy, x < field->width && y >= 0 && y < field->height)
  {
    if(field->area[x][y] != BLK_BACK &&
       (field->area[x][y] & MASK_BLOCKS) == type)
      blocks++;
    else
    {
      if(blocks >= SCORE_WIDTH)
      {
	score += (DIAGONAL_SCORE << (blocks - SCORE_WIDTH));
	for(; blocks; blocks--)
	  field->area[x-blocks][y-blocks*incy] |= FLAG_DELETE;
      }
      /* next possible 'row' start place */
      type = field->area[x][y] & MASK_BLOCKS;
      blocks = 1;
    }
  }
  if(blocks >= SCORE_WIDTH)
  {
    score += (DIAGONAL_SCORE << (blocks - SCORE_WIDTH));
    for(; blocks; blocks--)
      field->area[x-blocks][y-blocks*incy] |= FLAG_DELETE;
  }
  return score;
}

/* mark rows of same type, kill them and return the resulting score */
static int check_blocks(PlayField *field)
{
  int x, y, score = 0;

  y = field->height - 1;

  /* check and mark rows + collect the scores */
  for(x = 0; x < field->width; x++)
    score += vertical(field, x);

  for(y = 0; y < field->height; y++)
  {
    score += horizontal(field, y);
    score += diagonal(field, 0, y,  1);
    score += diagonal(field, 0, y, -1);
  }

  /* check rest of the diagonal directions (top & bottom) */
  y = field->height - 1;
  for(x = 1; x <= field->width - SCORE_WIDTH; x++)
  {
    score += diagonal(field, x, 0,  1);
    score += diagonal(field, x, y, -1);
  }

  /* remove the marked blocks */
  score += kill_blocks(field);

  return score;
}

/* remove marked blocks */
static int kill_blocks(PlayField *field)
{
  int x, y, top, score, blocks = 0;

  /* show_marked blocks */
  for(x = 0; x < field->width; x++)
    for(y = 0; y < field->height; y++)
      if(field->area[x][y] & FLAG_DELETE)
      {
	show_block(BLK_FLASH, x, y);
        blocks++;
      }
  /* no blocks to remove? */
  if(!blocks)
    return 0;

  score = blocks * KILL_SCORE;

  /* flush & wait 1/8 sec */
  check_input(128L);

  /* remove blocks */
  for(x = 0; x < field->width; x++)
  {
    top = 0;
    for(y = 0; y < field->height; y++)
    {
      if(field->area[x][y] == BLK_BACK)
      {
        /* the topmost (possible) block */
        top = y+1;
	continue;
      }
      if(field->area[x][y] & FLAG_DELETE)
      {
        blocks = y - top;
	if(blocks > 0)
	{
          copy_blocks(x, top, x, top + 1, blocks);
	  /* move blocks above deleted one down */
	  for(blocks = y; blocks > top; blocks--)
	    field->area[x][blocks] = field->area[x][blocks-1];
	}
        show_block(BLK_BACK, x, top);
        field->area[x][top] = BLK_BACK;
	top++;
      }
    }
  }
  /* flush & wait 1/8 sec */
  check_input(128L);

  return score;
}
