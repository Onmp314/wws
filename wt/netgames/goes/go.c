/* Go
 * 
 * Engine:
 * - Implements game logic.
 * - Marks last set button and undo.
 * - Uses a Go board.
 *
 * After the game has ended (both pass), dead groups are removed by hand.
 * Passing again signifies that all the dead groups have been removed
 * whereas computer will calculate the score.
 *
 * ATM the surrounding groups are fully checked after every move.  It would
 * be faster (and help in counting the score) if the game would keep tables
 * about all the groups on the board ie.  every piece on the board would
 * have ID of one of the groups.  However that would need quite a bit more
 * code and memory.
 *
 * Note: current game scoring is quite a pasta...
 *
 * TODO:
 * - Better rules (Super Ko).
 * - Automatic / better scoring (either japanese/chinese/AGA/GOE).
 * - Selectable (before starting game) board size (odd, 9x9 -> 19x19).
 *   Would need making separate GUI module from the one used by Pente and
 *   Gomoku.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "game.h"				/* includes standard stuff */
#include "ports.h"				/* game ports/IDs */
#include "messages.h"				/* message IDs */
#include "common.h"				/* go board games */

#define VERSION	"v0.5"

/* flag used for surround checking */
#define CHECKED	64

#ifdef DEBUG
/* file writing is started at first 'start', and stopped at first 'resign' */
#define DEBUG_FILE	"debug.out"
static FILE *Debug;
#endif

typedef struct
{
  int captures;
  color_t color;
  void (*piece)(int x, int y);
} Player;

static int  Passed, Scoring;			/* previous player passed */
static int  LastX = -1, LastY = -1;		/* for marking last piece */
static char Board[BOARD_SIZE][BOARD_SIZE];
static char Backup[BOARD_SIZE][BOARD_SIZE];	/* for undo */
static Player Mine, Your;
static GAME *Game;


/* game engine (GUI independent) functions */
/* function prototypes. */
void my_move(int x, int y);				/* from Go GUI */
static int  check4capture(int x, int y, color_t color, color_t other);
static void do_captures(int x, int y, color_t color, color_t other);
static int  capture_pieces(int x, int y, int piece);
static int  surrounded(int x, int y, int by);
static void make_move(Player *a, Player *b, int x, int y);
static int  messages(short type, uchar x, uchar y);
static int  remove_group(int x, int y);
static int  continue_game(player_t color);
static void set_side(player_t color);
static void initialize_game(void);
static void clear_vars(void);
static int  do_pass(player_t color);
static char *score_game(void);
static int  do_undo(void);

#ifdef DEBUG
static void close_debug(void)
{
  if(Debug)
  {
    fclose(Debug);
    Debug = 0;
  }
}
#endif

/* information for the frontend. */
void get_configuration(GAME *game)
{
  /* variables */
  game->name    = "Go " VERSION;
  game->game_id = GO_ID;
  game->width   = BoardWidth;
  game->height  = BoardHeight;

  /* functions */
  game->message = messages;
  game->start   = initialize_game;
  game->cont    = continue_game;
  game->side    = set_side;
  game->undo    = do_undo;				/* there is undo */
  game->pass    = do_pass;
#ifdef DEBUG
  game->over    = close_debug;
#endif

  Game = game;
}

static void set_side(player_t player)
{
  if(player == Player0)
  {
    Mine.color = BLACK;
    Your.color = WHITE;
    Mine.piece = draw_black;
    Your.piece = draw_white;
  }
  else
  {
    Mine.color = WHITE;
    Your.color = BLACK;
    Mine.piece = draw_white;
    Your.piece = draw_black;
  }
}

static void clear_vars(void)
{
  LastX = LastY = -1;
  Scoring = Passed = False;
  draw_captures(BLACK, 0);
  draw_captures(WHITE, 0);
  Mine.captures = 0;
  Your.captures = 0;

  /* There are different messages to output after game has ended but
   * the score hasn't yet been determined. See do_pass() function.
   */
  Game->o_info = Game->i_info = NULL;
}

static int continue_game(player_t player)
{
  if(Scoring)
    return False;		/* needs clearing */

  /* remove last mark */
  if(LastX >= 0 || LastY >= 0)
    remove_mark(LastX, LastY, Board[LastX][LastY]);

  set_side(player);
  clear_vars();

#ifdef DEBUG
  if(!Debug)
    Debug = fopen(DEBUG_FILE, "wb");
#endif
  return True;
}

static void initialize_game(void)
{
  int x, y;

  for(x = 0; x < BOARD_SIZE; x++)
    for(y = 0; y < BOARD_SIZE; y++)
      if(Board[x][y] != EMPTY)
      {
        Board[x][y] = EMPTY;
        draw_empty(x, y);
      }
  clear_vars();

#ifdef DEBUG
  if(!Debug)
    Debug = fopen(DEBUG_FILE, "wb");
#endif
}

/* only possible before opponent has moved, return 0 if succeeded */
static int do_undo(void)
{
  int x, y;

  if(LastX < 0 || LastY < 0)
    return False;

  remove_mark(LastX, LastY, Board[LastX][LastY]);

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
  LastX = LastY = -1;
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
    case PIECE_REMOVE:
      if(remove_group(x, y))		/* confirm before doing it */
        send_msg(RETURN_REMOVE, x, y);
      else
        send_msg(RETURN_FAIL, 0, 0);
      break;

    case RETURN_MINE:
      make_move(&Mine, &Your, x, y);
      if(Game->playing)			/* if (still) gaming */
        send_msg(MOVE_NEXT, 0, 0);
      break;
    case RETURN_YOUR:
      make_move(&Your, &Mine, x, y);
      break;
    case RETURN_REMOVE:
      remove_group(x, y);
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
  memcpy(Backup, Board, sizeof(Board));
  Board[x][y] = one->color;
  one->piece(x, y);
  if(Game->playing)
  {
    if(LastX >= 0 || LastY >= 0)
      remove_mark(LastX, LastY, two->color);
    draw_mark(x, y, one->color);
    LastX = x; LastY = y;

    do_captures(x, y, one->color, two->color);
    Passed = False;

#ifdef DEBUG
    if(Debug)
    {
      int x, y, c;
      /* print current game board to a debug file */
      fprintf(Debug, "Move: %d\n", Game->turn);
      for(y = 0; y < BOARD_SIZE; y++)
      {
        for(x = 0; x < BOARD_SIZE; x++)
	{
	  switch(Board[x][y])
	  {
	    case EMPTY:
	      c = '.';
	      break;
	    case BLACK:
	      c = '#';
	      break;
	    case WHITE:
	      c = 'O';
	      break;
	    default:
	      c = '*';
	  }
	  fprintf(Debug, " %c", c);
	}
	fputc('\n', Debug);
      }
    }
#endif
  }
}

static int remove_group(int x, int y)
{
  color_t color = Board[x][y];

  if(Scoring)
  {
    if(color == Mine.color)
    {
      draw_mark(x, y, color);
      if(Game->my_move || confirm("Remove marked group"))
      {
	Your.captures += capture_pieces(x, y, color);
	draw_captures(Your.color, Your.captures);
      }
      else
      {
	remove_mark(x, y, color);
	return False;
      }
    }
    else
    {
      Mine.captures += capture_pieces(x, y, color);
      draw_captures(Mine.color, Mine.captures);
    }
  }
  else
    capture_pieces(x, y, color);

  show_info("group removed.");
  return True;
}

static int do_pass(player_t color)
{
  /* both players passed -> done with the thing */
  if(Passed)
  {
    /* Even scoring done? */
    if(Scoring)
    {
      game_over();
      /* has to be done this way so that normal messages wouldn't overwrite
       * the game over / score text.
       */
      Game->o_info = Game->i_info = score_game();
      Passed = Scoring = False;
      return True;
    }

    /* game done -> Scoring */
    Game->i_info = "Remove dead groups...";
    Scoring = True;
    Passed = False;
#ifdef DEBUG
    close_debug();
#endif
    return True;
  }

  /* wait until other player passed too */
  Passed = True;
  return True;
}

static char *score_game(void)
{
  int color, x, y, area, my_score, your_score;
  static char info[32];

  area = 0;

  /* calculate score */
  my_score = Mine.captures;
  your_score = Your.captures;

  for(y = 0; y < BOARD_SIZE; y++)
  {
    color = EMPTY;
    for(x = 0; x < BOARD_SIZE; x++)
    {
      if(Board[x][y] == EMPTY)
      {
        if(x > 0 && color != EMPTY)
	  area = surrounded(x, y, color);
      }
      else
        if(color == EMPTY)
	{
          color = Board[x][y];
	  if(color == BLACK || color == WHITE)
	    area = surrounded(x-1, y, color);
	}
      if(area)
      {
        if(color == Mine.color)
	  my_score += area;
	else
	  your_score += area;
	area = 0;
      }
      color = Board[x][y];
    }
  }

  /* clear check marks from the board for editing */
  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if(Board[x][y] & CHECKED)
        Board[x][y] ^= CHECKED;

  /* score message */
  if(my_score > your_score)
    sprintf(info, "You won %d/%d!", my_score, your_score);
  else
    sprintf(info, "You lost %d/%d...", my_score, your_score);

  return info;
}

/* if any of the surrounding 'other' color group(s) is surrounded by
 * 'other' group(s) (ie. it doesn't have freedoms left), remove it...
 */
static void do_captures(int x, int y, color_t color, color_t other)
{
  int *place, table[] = { -1, 0, 2, 0, -1, -1, 0, 2 };
  int idx, *captures;

  if(color == Mine.color)
    captures = &Mine.captures;
  else
    captures = &Your.captures;

  place = table;
  for(idx = 0; idx < 4; idx++)
  {
    x += *(place++);
    y += *(place++);
    if(x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE
    && Board[x][y] == other)
    {
      if(surrounded(x, y, color))
        *captures += capture_pieces(x, y, other|CHECKED);
      else
        surrounded(x, y, color);	/* remove check marks */
    }
  }
  draw_captures(color, *captures);
}     

/* piece is surrounded, check whether any of the surrounding groups
 * would be captured by the move.
 */
static int check4capture(int x, int y, color_t color, color_t other)
{
  int *place, table[] = { -1, 0, 2, 0, -1, -1, 0, 2 };
  int idx, dead, captures = 0;

  place = table;
  for(idx = 0; idx < 4; idx++)
  {
    x += *(place++);
    y += *(place++);
    if(x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE
    && Board[x][y] == other)
    {
      dead = surrounded(x, y, color);
      if(dead && !(dead == 1 && x == LastX && y == LastY))
      {
        /* legal capture, not Ko (repeated situation) */
        captures += dead;
      }
      /* remove check marks */
      surrounded(x, y, color);
    }
  }
  if(captures)
    return True;
  return False;
}

#define STACK	120
static int Stack[STACK], StkIdx;

static inline void push(int x, int y)
{
  if(StkIdx <= STACK-2)
  {
    Stack[StkIdx++] = x;
    Stack[StkIdx++] = y;
  }
}

static inline int pop(int *x, int *y)
{
  if(StkIdx > 1)
  {
    /* last-in, first-out! */
    *y = Stack[--StkIdx];
    *x = Stack[--StkIdx];
    return True;
  }
  return False;
}

/* remove all the pieces connected into piece at (x,y). On normal case
 * all of them are supposed to have 'CHECKED' flag on.
 */
static int capture_pieces(int x, int y, int piece)
{
  int captures = 0, above, below;

#ifdef DEBUG
  if(Debug)
    fprintf(Debug, "Capture at (%d,%d)\n", x, y);
#endif
  StkIdx = 0;
  for(;;)
  {
    /* search left edge of connected pieces */
    while(--x >= 0 && Board[x][y] == piece);

    above = below = 1;

    /* until right edge */
    while(++x < BOARD_SIZE && Board[x][y] == piece)
    {
      if(y > 0)
      {
        y--;
	if(Board[x][y] == piece)
	{
	  if(above)
	  {
	    push(x, y);
	    above = 0;
	  }
	}
	else
	  above = 1;
	y++;
      }
      if(y < BOARD_SIZE-1)
      {
        y++;
	if(Board[x][y] == piece)
	{
	  if(below)
	  {
	    push(x, y);
	    below = 0;
	  }
	}
	else
	  below = 1;
        y--;
      }
      Board[x][y] = EMPTY;
      draw_empty(x, y);
      captures++;
    }

    /* while something in stack, pop(x,y) */
    if(!pop(&x, &y))
      return captures;
  }
}

/* Check all the pieces connected into piece at (x,y) for freedoms.
 * If group isn't completely surrounded, return False, otherwise
 * return number of pieces in the group.
 * The checked pieces need to be flagged with 'CHECKED' flag so
 * that no piece is checked twice.
 */
static int surrounded(int x, int y, int by)
{
  int piece, above, below, pieces = 0;

  piece = Board[x][y];
  StkIdx = 0;
  for(;;)
  {
    /* search left edge of connected pieces */
    while(--x >= 0 && (Board[x][y]|CHECKED) == (piece|CHECKED));

    /* not bordered */
    if(x >= 0 && Board[x][y] != by)
      return False;

    above = below = 1;

    /* until right edge */
    while(++x < BOARD_SIZE && (Board[x][y]|CHECKED) == (piece|CHECKED))
    {
      if(y > 0)
      {
	y--;
	if(Board[x][y] == piece)
	{
	  if(above)
	  {
	    push(x, y);
	    above = 0;
	  }
	}
	else
	{
	  if(Board[x][y] != by && Board[x][y] != (piece^CHECKED))
	    return False;
	  above = 1;
	}
	y++;
      }
      if(y < BOARD_SIZE-1)
      {
	y++;
	if(Board[x][y] == piece)
	{
	  if(below)
	  {
	    push(x, y);
	    below = 0;
	  }
	}
	else
	{
	  if(Board[x][y] != by && Board[x][y] != (piece^CHECKED))
	    return False;
	  below = 1;
	}
	y--;
      }
      Board[x][y] ^= CHECKED;
      pieces++;
    }

    /* not bordered */
    if(x < BOARD_SIZE && Board[x][y] != by)
      return False;

    /* while something in stack, pop(x,y) */
    if(!pop(&x, &y))
      return pieces;
  }
}

/* called from GUI specific game part */
void my_move(int x, int y)
{
  /* check position validity */
  if(x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE)
  {
    if(Game->playing)
    {
      if(Scoring)
      {
	if(Board[x][y] != EMPTY)
	  send_msg(PIECE_REMOVE, x, y);
      }
      else
      {
	if(Board[x][y] == EMPTY)
	{
	  int suicide;

	  Board[x][y] = Mine.color;
	  suicide = surrounded(x, y, Your.color);
	  /* remove check marks */
	  surrounded(x, y, Your.color);

	  /* if place/group has freedoms or placing the stone there will
	   * make them by capturing opponent's stones, allow the move.
	   */
	  if(!suicide || check4capture(x, y, Mine.color, Your.color))
	    send_msg(PIECE_MINE, x, y);
	  Board[x][y] = EMPTY;
	}
      }
    }
    else
    {
      if(Board[x][y] == EMPTY)
        send_msg(PIECE_MINE, x, y);
      else
        if(Board[x][y] == Mine.color)
          send_msg(PIECE_YOUR, x, y);
	else
          send_msg(PIECE_REMOVE, x, y);
    }
  }
}
