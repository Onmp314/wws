/* Gomoku
 * 
 * A five-in-a-row board game.
 * - Uses a Go board.
 * - Implements game logic.
 * - shows last set button.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "game.h"				/* includes standard stuff */
#include "ports.h"				/* game ports/IDs */
#include "messages.h"				/* message IDs */
#include "common.h"				/* go board games */

#define VERSION	"v0.9"
#define ABS(x)	((x) < 0 ? -(x) : (x))

typedef struct
{
  color_t color;
  void (*piece)(int x, int y);
} Player;

static int  LastX = -1, LastY = -1;		/* for undo & last mark */
static char Board[BOARD_SIZE][BOARD_SIZE];
static Player Mine, Your;
static int NeedClearing;			/* five-in-a-row */
static GAME *Game;



/* game engine (GUI independent) functions */
/* function prototypes. */
void my_move(int x, int y);				/* from Go GUI */
static int messages(short type, uchar x, uchar y);
static void make_move(Player *a, Player *b, int x, int y);
static int check5row(color_t color, int x, int y);
static int continue_game(player_t player);
static void set_side(player_t player);
static void initialize_game(void);
static int do_undo(void);

static inline void make_mark(int x, int y)
{
  draw_mark(x, y, Board[x][y]);
  LastX = x;
  LastY = y;
}

static inline void del_old_mark(void)
{
  if(LastX >= 0)
  {
    remove_mark(LastX, LastY, Board[LastX][LastY]);
    LastX = -1;
  }
}


/* information for the frontend. */
void get_configuration(GAME *game)
{
  /* variables */
  game->name    = "Gomoku " VERSION;
  game->game_id = GOMOKU_ID;
  game->width   = BoardWidth;
  game->height  = BoardHeight;

  /* functions */
  game->message = messages;
  game->start   = initialize_game;
  game->cont    = continue_game;
  game->side    = set_side;
  game->undo    = do_undo;				/* there is undo */
  game->over    = del_old_mark;
  /* no 'pass' */

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

static int continue_game(player_t player)
{
  if(NeedClearing)
    return False;

  del_old_mark();
  set_side(player);

  return True;
}

static void initialize_game(void)
{
  int x, y;

  NeedClearing = False;
  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if(Board[x][y] != EMPTY)
      {
        Board[x][y] = EMPTY;
        draw_empty(x, y);
      }
  del_old_mark();
}

/* only possible before opponent has moved, return 0 if succeeded */
static int do_undo(void)
{
  if(LastX < 0)
    return False;

  Board[LastX][LastY] = EMPTY;
  draw_empty(LastX, LastY);
  LastX = -1;
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
      send_msg(RETURN_REMOVE, x, y);
      Board[x][y] = EMPTY;
      draw_empty(x, y);
      break;

    case RETURN_MINE:
      make_move(&Mine, &Your, x, y);
      if(Game->playing)			/* if (still) playing */
        send_msg(MOVE_NEXT, 0, 0);
      break;
    case RETURN_YOUR:
      make_move(&Your, &Mine, x, y);
      break;
    case RETURN_REMOVE:
      Board[x][y] = EMPTY;
      draw_empty(x, y);
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
  Board[x][y] = one->color;
  one->piece(x, y);
  if(Game->playing)
  {
    del_old_mark();
    make_mark(x, y);

    if(check5row(one->color, x, y))
    {
      if(one == &Mine)			/* my move won? */
        show_info("You won!");
      else
        show_info("You lost...");
      game_over();
    }
  }
}

/* return true for win */
static int check5row(color_t color, int x, int y)
{
  static int table[] = { 1, 0, 0, 1, 1, 1, 1, -1 };
  register int a, b, c, d, w, h;
  int idx, *dir = table;

  for(idx = 0; idx < 4; idx++)
  {
    a = b = x;
    c = d = y;
    w = *dir++;
    h = *dir++;
    do
    {
      a -= w;
      c -= h;
    }
    while(a >= 0 && c >= 0 && a < BOARD_SIZE && c < BOARD_SIZE
    && Board[a][c] == color);

    do
    {
      b += w;
      d += h;
    }
    while(b >= 0 && d >= 0 && b < BOARD_SIZE && d < BOARD_SIZE
    && Board[b][d] == color);

    if(ABS(b-a) > 5 || ABS(d-c) > 5)
    {
      /* mark the winning 'straight' */
      NeedClearing = True;
      for(;;)
      {
	a += w;
	c += h;
	if(a == b && c == d)
	  break;

        if(!(a == x && c == y))
	  draw_mark(a, c, color);
      }
    }
  }
  return NeedClearing;
}

/* called from GUI specific game part */
void my_move(int x, int y)
{
  /* check position validity */
  if(x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE)
  {
    if(Game->playing)
    {
      if(Board[x][y] == EMPTY)
        send_msg(PIECE_MINE, x, y);
    }
    else
    {
      if(NeedClearing)
        return;

      /* switch the piece */
      if(Board[x][y] == EMPTY)
        send_msg(PIECE_MINE, x, y);
      else if(Board[x][y] == Mine.color)
        send_msg(PIECE_YOUR, x, y);
      else
        send_msg(PIECE_REMOVE, x, y);
    }
  }
}
