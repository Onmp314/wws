/*
 * Reversi / Othello
 *
 * Doesn't show last nor support editing.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <unistd.h>			/* usleep() */
#include "game.h"			/* includes standard stuff */
#include "messages.h"			/* message IDs */
#include "common.h"			/* go board games */
#include "id.h"

#define VERSION	"v0.6"

#define DIRS	8			/* turn directions */

typedef struct
{
  int pieces;
  color_t color;
  void (*piece)(int x, int y);
} Player;

static int Passed;
static char Board[BOARD_SIZE][BOARD_SIZE];
static char Backup[BOARD_SIZE][BOARD_SIZE];	/* for undo */
static Player Mine, Your;
static GAME *Game;



/* game engine (GUI independent) functions */
/* function prototypes. */
void my_move(int x, int y);				/* from Go GUI */
static int messages(short type, uchar x, uchar y);
static void make_move(Player *a, Player *b, int x, int y);
static int  flipper(int x, int y, Player *plr, color_t color, int dir, int flip);
static int do_pass(player_t player);
static void set_side(player_t player);
static void initialize_game(void);
static int do_undo(void);


/* information for the frontend. */
void get_configuration(GAME *game)
{
  /* variables */
  game->name    = "Reversi " VERSION;
  game->game_id = REVERSI_ID;
  game->width   = BoardWidth;
  game->height  = BoardHeight;

  /* functions */
  game->message = messages;
  game->start   = initialize_game;
  game->side    = set_side;
  game->undo    = do_undo;				/* there is undo */
  game->pass    = do_pass;

  Game = game;
}

static void set_side(player_t player)
{
  if(player == Player0)
  {
    Mine.color = WHITE;
    Your.color = BLACK;
    Mine.piece = draw_white;
    Your.piece = draw_black;
  }
  else
  {
    Mine.color = BLACK;
    Your.color = WHITE;
    Mine.piece = draw_black;
    Your.piece = draw_white;
  }
}

static void initialize_game(void)
{
  int x, y;

  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if(Board[x][y] != EMPTY)
      {
        Board[x][y] = EMPTY;
        draw_empty(x, y);
      }

  Board[3][3] = WHITE;
  Board[3][4] = BLACK;
  Board[4][3] = BLACK;
  Board[4][4] = WHITE;

  draw_white(3, 3);
  draw_black(3, 4);
  draw_black(4, 3);
  draw_white(4, 4);

  Game->o_info = Game->i_info = NULL;
  Your.pieces = Mine.pieces = 2;
  draw_pieces(BLACK, 2);
  draw_pieces(WHITE, 2);

  Passed = False;
}

/* only possible before opponent has moved, return 0 if succeeded */
static int do_undo(void)
{
  int x, y;

  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if(Board[x][y] != Backup[x][y])
      {
        Board[x][y] = Backup[x][y];
	switch(Backup[x][y])
	{
	  case BLACK:
            draw_black(x, y);
	    break;
	  case WHITE:
            draw_white(x, y);
	    break;
	  default:
            draw_empty(x, y);
	}
      }
  Passed = False;
  return True;
}

/* Message names relative to the original message sender.
 * If one trusts opponent to send only legal moves,
 * it's faster to return to the message before acting it.
 */
static int messages(short type, uchar x, uchar y)
{
  switch(type)
  {
    case PIECE_MINE:
      send_msg(RETURN_MINE, x, y);
      make_move(&Your, &Mine, x, y);
      break;
    case PIECE_YOUR:
      send_msg(RETURN_YOUR, x, y);
      make_move(&Mine, &Your, x, y);
      break;

    case RETURN_MINE:
      make_move(&Mine, &Your, x, y);
      if(Game->playing)			/* if (still) gaming */
        send_msg(MOVE_NEXT, 0, 0);
      break;
    case RETURN_YOUR:
      make_move(&Your, &Mine, x, y);
      break;

    default:
      return False;
  }
  return True;
}

/* move validity checks are done before they are sent to the round trip
 * by the event handler. Game over check is also done here.
 */
static void make_move(Player *one, Player *two, int x, int y)
{
  int dir, flips;

  memcpy(Backup, Board, sizeof(Board));
  /* flash the given stone */
  for(dir = 0; dir < 3; dir++)
  {
    one->piece(x, y);
    flush_out();
    usleep(160000);
    two->piece(x, y);
    flush_out();
    usleep(160000);
  }

  /* flip stones */
  one->pieces++;
  one->piece(x, y);
  Board[x][y] = one->color;

  for(dir = 0; dir < DIRS; dir++)
  {
    if(flipper(x, y, one, two->color, dir, 0))
    {
      flips = flipper(x, y, one, two->color, dir, 1);
      one->pieces += flips;
      two->pieces -= flips;
    }
  }
  draw_pieces(one->color, one->pieces);
  draw_pieces(two->color, two->pieces);

  Passed = False;
}

static int do_pass(player_t color)
{
  /* both players passed -> done with the thing */
  if(Passed)
  {
    if(Mine.pieces == Your.pieces)
      Game->o_info = Game->i_info = "Game over.";
    else
    {
      if(Mine.pieces > Your.pieces)
      {
        Game->o_info = Game->i_info = "You won!";
        mark_pieces(Mine.color);
      }
      else
      {
        Game->o_info = Game->i_info = "You lost...";
        mark_pieces(Your.color);
      }
    }
    game_over();
  }
  Passed = True;
  return True;
}

/* Turn all 'color' pieces between (x,y) and 'plr->color' piece in given
 * direction to 'plr->color'. If flip isn't set, just return the number
 * of such pieces.
 */
static int flipper(int x, int y, Player *plr, color_t color, int dir, int flip)
{
  static int table[DIRS][2] =
  {
    { -1, -1 },
    {  0, -1 },
    {  1, -1 },
    { -1,  0 },
    {  1,  0 },
    { -1,  1 },
    {  0,  1 },
    {  1,  1 }
  };
  int flips = 0;

  for(;;)
  {
    x += table[dir][0];
    y += table[dir][1];
    if(x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE)
      return False;		/* outside the board */

    if(Board[x][y] == color)
    {
      if(flip)
      {
        Board[x][y] = plr->color;
      	plr->piece(x, y);
      }
      flips++;
    }
    else
    {
      if(flips && Board[x][y] == plr->color)
        return flips;
      break;
    }
  }
  return 0;
}

/* called from GUI specific game part */
void my_move(int x, int y)
{
  int dir;

  /* check position validity */
  if(x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE)
  {
    if(Board[x][y] == EMPTY)
    {
      /* got to check that place is next to an opponent's piece
       * and that there's at least one own piece in horizontal,
       * vertical or diagonal direction without empty place(s)
       * in between...
       */
      for(dir = 0; dir < DIRS; dir++)
        if(flipper(x, y, &Mine, Your.color, dir, 0))
	{
	  send_msg(PIECE_MINE, x, y);
	  return;
	}
    }
  }
}
