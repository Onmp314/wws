/* Chess logic
 *
 * The king's position (checked?  player able to counter that?)  is
 * re-evaluated after every move instead of keeping that info in memory.
 *
 * TODO
 *
 * - Fix all the bugs in check/stalemate and game movement validity
 *   checking (eg. fortification, en passant etc.).
 * - Use proper Chess terminology.
 *
 * HISTORY:
 *
 * 18.7.1996
 * - Added function for game specific messages as needed now.
 * - Corrected some english chess terms (my rules are in finnish...).
 * - Added Fortification (move which changes both king and rook positions).
 * - Added support for pawn promotion user selection.
 * - Added 'en passant' move possibility for pawns.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <stdlib.h>
#include "game.h"				/* includes standard stuff */
#include "ports.h"				/* game ports/IDs */
#include "chess.h"

#define VERSION	"v0.7"

typedef struct
{
  int kx, ky;			/* king position */
  color_t color;
  void (*piece)(int x, int y);
} Player;

static int  Debug;				/* info about checks */
static int  MoveFrom;				/* move started */
static int  PassX, PassY;			/* move for 'en passant' */
static int  FromX, FromY;			/* to select piece */
static char Board[BOARD_SIZE][BOARD_SIZE];
static char Backup[BOARD_SIZE][BOARD_SIZE];	/* for undo */
static Player Mine, Your;
static GAME *Game;				/* game in process */

/* game engine (GUI independent) functions */
/* function prototypes. */
void my_move(int x, int y);			/* from Chess GUI */
static int  messages(short type, uchar x, uchar y);
static void move_from(int x, int y);		/* mark where to move from */
static void make_move(int x, int y);		/* move the piece */
static int  still_alive(Player *plr);		/* not game over? */
static int  check_move(int x, int y);		/* a valid move? -> msg */
static int  king_ok(int x, int y);		/* king ok afterwards? */
static const char *king_checked(int kx, int ky, color_t color);
static int  chessmate(color_t color);		/* check for game end */
static int  continue_game(player_t color);	/* continue a set-up game */
static void initialize_game(void);
static int  do_undo(void);
static void done(void);
static void find_kings(void);
static void set_side(player_t color);
static int  parse_args(char *name, int argc, char *argv[]);
#if 0 /* on modern systems, comes already from stdlib.h */
static inline int abs(int x)  { return (x < 0 ? -x : x); }
#endif
static inline int sgn(int x)  { return (x == 0 ? 0 : (x < 0 ? -1 : 1)); }


/* information for the frontend. */
void get_configuration(GAME *game)
{
  /* variables */
  game->name    = "Chess " VERSION;
  game->game_id = CHESS_ID;
  game->width   = BoardWidth;
  game->height  = BoardHeight;

  /* functions */
  game->message = messages;
  game->args    = parse_args;
  game->side    = set_side;
  game->start   = initialize_game;
  game->cont    = continue_game;
  game->undo    = do_undo;				/* there is undo */
  game->over    = done;
  /* no 'pass' */

  Game = game;
}

static int parse_args(char *name, int argc, char *argv[])
{
  int idx = -1;

  while(++idx < argc)
  {
    /* one letter option */
    if(argv[idx][0] == '-' && argv[idx][1] && !argv[idx][2])
    {
      switch(argv[idx][1])
      {
	case 'h':		/* help */
	  fprintf(stderr, "\nChess " VERSION " (w) 1996 by Eero Tamminen\n");
	  fprintf(stderr, "\nOptions:\n  -d  debug\n  -h  help\n");
	  break;

	case 'd':		/* debug */
	  Debug = True;
	  break;

	default:
	  return False;
      }
    }
    else
      return False;
  }
  return True;
}

static void done(void)
{
  /* resign */
  draw_mark(FromX, FromY, 0);
  stop_timer();
}

static void set_side(player_t player)
{
  if(player == Player0)
  {
    Mine.color = WHITE;
    Your.color = BLACK;
  }
  else
  {
    Mine.color = BLACK;
    Your.color = WHITE;
  }
}

static void clear_vars(void)
{
  Mine.kx = Your.kx = -1;
  PassX = PassY = -1;
  MoveFrom = False;
}

/* continue a set-up game */
static int continue_game(player_t player)
{
  /* as only thing editable is board of a previous played game, there's no
   * need change sides according to the player color.
   */
  clear_vars();
  find_kings();
  /* if both players won't have kings, can't play */
  if(Mine.kx < 0 || Your.kx < 0)
    return False;

  start_timer();
  return True;
}

static void initialize_game(void)
{
  int x, y;

  Board[0][0] = ROOK;
  Board[1][0] = KNIGHT;
  Board[2][0] = BISHOP;
  Board[5][0] = BISHOP;
  Board[6][0] = KNIGHT;
  Board[7][0] = ROOK;

  for(x = 0; x < 8; x++)
    Board[x][7] = Board[x][0];

  for(x = 0; x < 8; x++)
  {
    Board[x][1] = PAWN;
    Board[x][6] = PAWN;
    for(y = 2; y < 6; y++)
      Board[x][y] = EMPTY;
  }

  /* player is always at the 'bottom' */
  if(Mine.color == WHITE)
  {
    Board[3][0] = KING;
    Board[4][0] = QUEEN;
    Board[3][7] = QUEEN;
    Board[4][7] = KING;
    for(x = 0; x < 8; x++)
    {
      Board[x][0] |= BLACK;
      Board[x][1] |= BLACK;
      Board[x][6] |= WHITE;
      Board[x][7] |= WHITE;
    }
  }
  else
  {
    Board[3][0] = QUEEN;
    Board[4][0] = KING;
    Board[3][7] = KING;
    Board[4][7] = QUEEN;
    for(x = 0; x < 8; x++)
    {
      Board[x][0] |= WHITE;
      Board[x][1] |= WHITE;
      Board[x][6] |= BLACK;
      Board[x][7] |= BLACK;
    }
  }
  clear_vars();
  find_kings();

  for(x = 0; x < 8; x++)
    for(y = 0; y < 8; y++)
      draw_piece(x, y, Board[x][y]);

  start_timer();
}

/* only possible before opponent has moved, return 0 if succeeded */
static int do_undo(void)
{
  int x, y;

  draw_mark(FromX, FromY, 0);
  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if(Board[x][y] != Backup[x][y])
      {
        Board[x][y] = Backup[x][y];
	draw_piece(x, y, Board[x][y]);
      }
  return True;
}

/* set the players' king positions */
static void find_kings(void)
{
  int x, y;

  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if(PIECE(Board[x][y]) == KING) {
        if(COLOR(Board[x][y]) == Mine.color)
	{
	  Mine.kx = x;
	  Mine.ky = y;
	}
	else
	{
	  Your.kx = x;
	  Your.ky = y;
	}
      }
}

static void fortificate(int x, int y, int rook)
{
  int dir, tx;

  /* move king */
  make_move(x, y);

  /* move rook */
  dir = sgn(x - FromX);
  tx = (dir > 0 ? 7 : 0);

  Board[tx][y] = EMPTY;
  draw_piece(tx, y, EMPTY);

  Board[x-dir][y] = rook;
  draw_piece(x-dir, y, rook);
}

static inline void promotion(int x, int y, int to)
{
  /* new piece (queen/rook/bishop/knight) with old attribute flags */
  Board[x][y] = to | (Board[x][y] & ~PIECES);
  draw_piece(x, y, Board[x][y]);
}

/* Message names relative to the original message sender.
 * This trusts opponent to send only legal moves!!! As
 * it's faster to return to the message before acting it.
 */
static int messages(short type, uchar x, uchar y)
{
  /* default turn change message */
  Game->i_info = Game->o_info = NULL;

  switch(type)
  {
    /* which piece to move... */
    case PIECE_SELECT:
      send_msg(RETURN_SELECT, x, y);
      move_from(x, 7-y);
      break;

    case RETURN_SELECT:
      move_from(x, y);
      break;

    /* normal move */
    case PIECE_MOVE:
      send_msg(RETURN_MOVE, x, y);
      make_move(x, 7-y);
      still_alive(&Mine);
      break;

    case RETURN_MOVE:
      make_move(x, y);
      if(still_alive(&Your))		/* if (still) playing */
      {
        /* promotion for a pawn? */
        if(y == 0 && COLORPIECE(Board[x][y]) == (PAWN | Mine.color))
	{
	  draw_mark(x, y, 1);
          send_msg(PIECE_PROMOTE, x, promote_to(Mine.color));
	  draw_mark(x, y, 0);
	}
        else
          send_msg(MOVE_NEXT, 0, 0);
      }
      break;

    /* pawn reached the other side and got promoted */
    case PIECE_PROMOTE:
      send_msg(RETURN_PROMOTE, x, y);
      promotion(x, 7, y);
      still_alive(&Mine);
      break;

    case RETURN_PROMOTE:
      promotion(x, 0, y);
      if(still_alive(&Your))
        send_msg(MOVE_NEXT, 0, 0);
      break;

    /* pawn ate other pawn 'in passing' */
    case EN_PASSANT:
      send_msg(RETURN_PASSANT, x, y);
      make_move(x, 7-y);
      Game->i_info = "'En passant' move.";
      draw_piece(x, 6-y, EMPTY);
      Board[x][6-y] = EMPTY;
      still_alive(&Mine);
      break;

    case RETURN_PASSANT:
      send_msg(MOVE_NEXT, 0, 0);
      make_move(x, y);
      Game->o_info = "En passant.";
      draw_piece(x, y+1, EMPTY);
      Board[x][y-1] = EMPTY;
      if(still_alive(&Your))
        send_msg(MOVE_NEXT, 0, 0);
      break;

    /* king fortified itself behind the rook */
    case HIDE_KING:
      send_msg(RETURN_KING, x, y);
      fortificate(x, 7-y, ROOK|MOVED|Your.color);
      Game->i_info = "Opponent fortified.";
      still_alive(&Mine);
      break;

    case RETURN_KING:
      fortificate(x, y, ROOK|MOVED|Mine.color);
      Game->o_info = "Fortified.";
      if(still_alive(&Your))
        send_msg(MOVE_NEXT, 0, 0);
      break;

    default:
      return False;
  }
  return True;
}

/* move validity checks are done before they are sent to the round trip
 * by the event handler.
 */
static void make_move(int x, int y)
{
  MoveFrom = False;
  memcpy(Backup, Board, sizeof(Board));

  /* can this piece can be captured 'en passant'? */
  if(PIECE(Board[FromX][FromY]) == PAWN
  && FromX == x && abs(FromY - y) == 2
  && !(Board[FromX][FromY] & MOVED))
  {
    PassX = x;
    PassY = y;
  }
  else
    PassX = PassY = -1;

  /* make the move */
  Board[x][y] = Board[FromX][FromY] | MOVED;
  Board[FromX][FromY] = EMPTY;

  /* show the move */
  draw_piece(FromX, FromY, EMPTY);
  draw_piece(x, y, Board[x][y]);

  /* update king position? */
  if(PIECE(Board[x][y]) == KING) {
    if(COLOR(Board[x][y]) == Mine.color)
    {
      Mine.kx = x;
      Mine.ky = y;
    }
    else
    {
      Your.kx = x;
      Your.ky = y;
    }
  }
}

/* check whether I did nasty move from the opponent point of view... */
static int still_alive(Player *plr)
{
  static char msg[32];
  const char *checked_by;

  if(!Game->playing)
    return False;

  /* game message if any */
  if((checked_by = king_checked(plr->kx, plr->ky, plr->color)))
  {
    if(Game->my_move)
      Game->o_info = "Check.";
    else
    {
      sprintf(msg, "Checked with %s.", checked_by);
      Game->i_info = msg;
    }
    if(Debug)
      printf("king at (%d,%d) checked with %s\n", plr->kx, plr->ky, checked_by);
  }

  if(chessmate(plr->color))
  {
    stop_timer();
    if(checked_by)
    {
      if(Game->my_move)
        show_info("Chessmate!");
      else
        show_info("Chessmated...");
    }
    else
      show_info("Stalemate.");
    game_over();
    return False;
  }
  else if(Debug)
    printf("Continue...\n");

  return True;
}

static void move_from(int x, int y)
{
  if(MoveFrom)
  {
    /* cancel */
    draw_mark(FromX, FromY, 0);
    MoveFrom = 0;
  }
  else
  {
    draw_mark(x, y, 1);
    MoveFrom = 1;
    FromX = x;
    FromY = y;
  }
}

/* called from GUI specific game part */
void my_move(int x, int y)
{
  int msg = PIECE_MOVE;

  /* check position validity */
  if(x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE)
  {
    if(MoveFrom)
    {
      /* cancel? */
      if(x == FromX && y == FromY)
        send_msg(PIECE_SELECT, x, y);
      else
	/* legal move? */
	if(!Game->playing || (msg = check_move(x, y)))
	  send_msg(msg, x, y);
    }
    else
    {
      /* selecting my own piece for moving? */
      if(!Game->playing || (Board[x][y] != EMPTY &&
         COLOR(Board[x][y]) == Mine.color))
	send_msg(PIECE_SELECT, x, y);
    }
  }
}

/* check that there are no other pieces on the way... */
static int clear2move(int x1, int y1, int x2, int y2)
{
  int xoff, yoff;

  xoff = sgn(x2 - x1);
  yoff = sgn(y2 - y1);

  for(;;)
  {
    x1 += xoff;
    y1 += yoff;
    /* skip first and last (checked earlier that it's not own) */
    if((xoff && x1 == x2) || (yoff && y1 == y2))
      return True;

    /* check that nothing is on the path */
    if(Board[x1][y1] != EMPTY)
      return False;
  }
}

/* check whether move is valid (-> True) according to the Chess rules. */
static int check_move(int tox, int toy)
{
  int w, h, old, new, ok = False;
  color_t color;

  if(tox < 0 || toy < 0 || tox >= BOARD_SIZE || toy >= BOARD_SIZE)
    return False;

  new = Board[tox][toy];  
  old = Board[FromX][FromY];
  color = COLOR(old);

  /* can't eat my own pieces */
  if(new != EMPTY && color == COLOR(new))
    return False;

  w = abs(tox - FromX);
  h = abs(toy - FromY);

  switch(PIECE(old))
  {
    case KING:
      /* one square off? */
      if(w < 2 && h < 2)
        break;

      /* maybe it's fortification? */
      if(h == 0 && w == 2 && !(old & MOVED))
      {
	int dir, tx;

	/* rook position */
	dir = sgn(tox - FromX);
	tx = (dir > 0 ? 7 : 0);

	/* my unmoved rook, way clear and king not checked? */
	if(Board[tx][toy] == (ROOK | color)
	&& clear2move(FromX, toy, tx, toy)
	&& !king_checked(FromX, toy, color))
	{
	  /* check whether the square king moves over
	   * and to are being checked...
	   */
	  Board[FromX][toy] = Board[tx][toy] = EMPTY;
	  Board[FromX+dir][toy] = (KING|color);
	  if(!king_checked(FromX, toy, color))
	  {
	    Board[FromX+dir][toy] = (ROOK|color);
	    Board[FromX+2*dir][toy] = (KING|color);
	    ok = !king_checked(FromX, toy, color);
	    Board[FromX+2*dir][toy] = EMPTY;
	  }
	  Board[FromX+dir][toy] = EMPTY;
	  Board[FromX][toy] = (KING|color);
	  Board[tx][toy] = (ROOK|color);
	  /* fortify */
	  if(ok)
	    return HIDE_KING;
	}
      }
      return False;

    case ROOK:
      /* not diagonal nor something in the way? */
      if(!(w && h) && clear2move(FromX, FromY, tox, toy))
        break;
      return False;

    case BISHOP:
      /* diagonal and nothing in the way? */
      if(w == h || clear2move(FromX, FromY, tox, toy))
        break;
      return False;

    case QUEEN:
      /* either proper rook or bishop move? */
      if((!(w && h) && clear2move(FromX, FromY, tox, toy))
      || (w == h && clear2move(FromX, FromY, tox, toy)))
        break;
      return False;

    case KNIGHT:
      if(w && h && w+h == 3)
        break;
      return False;

    case PAWN:
      /* not upwards? */
      if((color == Mine.color && FromY <= toy) ||
         (color == Your.color && FromY >= toy))
        return False;

      /* diagonally? */
      if(w == 1 && h == 1)
      {
        if(new == EMPTY)
        {
	  /* 'en passant' move? */
	  if(tox == PassX && FromY == PassY)
	  {
	    int piece = Board[PassX][PassY];
	    Board[PassX][PassY] = EMPTY;
	    ok = king_ok(tox, toy);
	    Board[PassX][PassY] = piece;
	    if(ok)
	      return EN_PASSANT;
	  }
	  return False;
	}
	/* eat */
	break;
      }
      else
        /* moving to an empty square straight above? */
        if(new == EMPTY && w == 0 && (h == 1 || (h == 2 &&
	   Board[tox][toy+sgn(FromY - toy)] == EMPTY && !(old & MOVED))))
	  break;
      return False;
  }

  /* king should not be checked after this move... */
  if(king_ok(tox, toy))
    return PIECE_MOVE;

  return False;
}

static int king_ok(int tox, int toy)
{
  int kx, ky, new, old, color;
  const char *check;

  new = Board[tox][toy];  
  old = Board[FromX][FromY];
  color = COLOR(old);

  if(PIECE(old) == KING)
  {
    kx = tox;
    ky = toy;
  }
  else
  {
    if(color == Mine.color)
    {
      kx = Mine.kx;
      ky = Mine.ky;
    }
    else
    {
      kx = Your.kx;
      ky = Your.ky;
    }
  }

  /* test move */
  Board[tox][toy] = old;
  Board[FromX][FromY] = EMPTY;
  check = king_checked(kx, ky, color);

  /* restore */
  Board[FromX][FromY] = old;
  Board[tox][toy] = new;

  if(Debug)
    printf("move: (%d,%d) to (%d,%d), king at (%d,%d) -> ",
        FromX, FromY, tox, toy, kx, ky);

  if(check)
  {
    if(Debug)
      printf("%s checking king\n", check);
    return False;
  }
  if(Debug)
    printf("ok\n");
  return True;
}

/* check whether there are any of the opponent's pieces threatening king on
 * the given position.  (COLORPIECE macro masks other than piece id and
 * color flags off)
 */
static const char *king_checked(int kx, int ky, color_t color)
{
  color_t o_color = OPPONENT(color);
  int o_piece, o_piece2, x, y;

  /* check knight */
  
  o_piece = KNIGHT | o_color;
  if((ky > 0   &&
      ((kx > 1  && COLORPIECE(Board[kx-2][ky-1]) == o_piece)  ||
       (kx < 6  && COLORPIECE(Board[kx+2][ky-1]) == o_piece)  ||
       (ky > 1 &&
       ((kx > 0 && COLORPIECE(Board[kx-1][ky-2]) == o_piece) ||
        (kx < 7 && COLORPIECE(Board[kx+1][ky-2]) == o_piece))))) ||
     (ky < 7   &&
      ((kx > 1  && COLORPIECE(Board[kx-2][ky+1]) == o_piece)  ||
       (kx < 6  && COLORPIECE(Board[kx+2][ky+1]) == o_piece)  ||
       (ky < 6 &&
       ((kx > 0 && COLORPIECE(Board[kx-1][ky+2]) == o_piece) ||
        (kx < 7 && COLORPIECE(Board[kx+1][ky+2]) == o_piece))))))
    return "knight";

  /* check for queen and rook */
  o_piece = QUEEN | o_color;
  o_piece2 = ROOK | o_color;

  x = kx;
  while(x > 0 && Board[--x][ky] == EMPTY);
  if(COLORPIECE(Board[x][ky]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[x][ky]) == o_piece2)
    return "rook";

  x = kx;
  while(x < 7 && Board[++x][ky] == EMPTY);
  if(COLORPIECE(Board[x][ky]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[x][ky]) == o_piece2)
    return "rook";

  y = ky;
  while(y > 0 && Board[kx][--y] == EMPTY);
  if(COLORPIECE(Board[kx][y]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[kx][y]) == o_piece2)
    return "rook";

  y = ky;
  while(y < 7 && Board[kx][++y] == EMPTY);
  if(COLORPIECE(Board[kx][y]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[kx][y]) == o_piece2)
    return "rook";

  /* and bishop */
  o_piece2 = BISHOP | o_color;

  x = kx; y = ky;
  while(x > 0 && y > 0 && Board[--x][--y] == EMPTY);
  if(COLORPIECE(Board[x][y]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[x][y]) == o_piece2)
    return "bishop";

  x = kx; y = ky;
  while(x < 7 && y < 7 && Board[++x][++y] == EMPTY);
  if(COLORPIECE(Board[x][y]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[x][y]) == o_piece2)
    return "bishop";

  x = kx; y = ky;
  while(x > 0 && y < 7 && Board[--x][++y] == EMPTY);
  if(COLORPIECE(Board[x][y]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[x][y]) == o_piece2)
    return "bishop";

  x = kx; y = ky;
  while(x < 7 && y > 0 && Board[++x][--y] == EMPTY);
  if(COLORPIECE(Board[x][y]) == o_piece)
    return "queen";
  if(COLORPIECE(Board[x][y]) == o_piece2)
    return "bishop";

  /* check pawn */

  o_piece = PAWN | o_color;
  if(color == Mine.color)
  {
    /* opponent is always at top */
    if(ky > 0   &&
       ((kx > 0 && COLORPIECE(Board[kx-1][ky-1]) == o_piece) ||
        (kx < 7 && COLORPIECE(Board[kx+1][ky-1]) == o_piece)))
      return "pawn";
  }
  else
    if(ky < 7   &&
       ((kx > 0 && COLORPIECE(Board[kx-1][ky+1]) == o_piece) ||
        (kx < 7 && COLORPIECE(Board[kx+1][ky+1]) == o_piece)))
      return "pawn";

  /* check king */  

  o_piece = KING | o_color;
  if((kx > 0 && COLORPIECE(Board[kx-1][ky]) == o_piece) ||
     (kx < 7 && COLORPIECE(Board[kx+1][ky]) == o_piece) ||
     (ky > 0 &&
      (COLORPIECE(Board[kx][ky-1]) == o_piece ||
      (kx > 0 && COLORPIECE(Board[kx-1][ky-1]) == o_piece) ||
      (kx < 7 && COLORPIECE(Board[kx+1][ky-1]) == o_piece))) ||
     (ky < 7 &&
      (COLORPIECE(Board[kx][ky+1]) == o_piece ||
      (kx > 0 && COLORPIECE(Board[kx-1][ky+1]) == o_piece) ||
      (kx < 7 && COLORPIECE(Board[kx+1][ky+1]) == o_piece))))
    return "king";

  /* not checked! */
  return NULL;
}

/* Go through all the pieces and their moves to check (with check_move())
 * whether there are any* moves which don't lead to a checked position.  If
 * none -> game over.
 */
static int chessmate(color_t color)
{
  int x, y, a, b, piece;

  if(Debug)
    printf("Checking...\n");

  /* go through all the player's pieces */
  for(y = 0; y < BOARD_SIZE; y++)
    for(x = 0; x < BOARD_SIZE; x++)
      if(Board[x][y] != EMPTY && COLOR(Board[x][y]) == color)
      {
        FromX = x; FromY = y;
	piece = PIECE(Board[x][y]);

	if(piece == QUEEN || piece == ROOK)
	{
	  if(Debug)
	    printf("Move queen/rook:\n");
	  a = x;
	  while(--a >= 0)
	    if(check_move(a, y))  return False;
	  a = x;
	  while(++a <= 7)
	    if(check_move(a, y))  return False;
	  b = y;
	  while(--b >= 0)
	    if(check_move(x, b))  return False;
	  b = y;
	  while(++b <= 7)
	    if(check_move(x, b))  return False;
	  if(piece == ROOK)
	    continue;
	}

	if(piece == QUEEN || piece == BISHOP)
	{
	  if(Debug)
	    printf("Move queen/bishop:\n");
	  a = x; b = y;
	  while(++a <= 7 && ++b <= 7)
	    if(check_move(a, b))  return False;
	  a = x; b = y;
	  while(--a >= 0 && --b >= 0)
	    if(check_move(a, b))  return False;
	  a = x; b = y;
	  while(++a <= 7 && --b >= 0)
	    if(check_move(a, b))  return False;
	  a = x; b = y;
	  while(--a >= 0 && ++b <= 7)
	    if(check_move(a, b))  return False;
	  continue;
	}

	if(piece == KNIGHT)
	{
	  if(Debug)
	    printf("Move knight:\n");
	  if(check_move(x-2, y-1))  return False;
	  if(check_move(x-2, y+1))  return False;
	  if(check_move(x-1, y-2))  return False;
	  if(check_move(x-1, y+2))  return False;
	  if(check_move(x+1, y-2))  return False;
	  if(check_move(x+1, y+2))  return False;
	  if(check_move(x+2, y-1))  return False;
	  if(check_move(x+2, y+1))  return False;
	  continue;
	}

	if(piece == KING)
	{
	  if(Debug)
	    printf("Move king:\n");
	  if(check_move(x-1, y-1))  return False;
	  if(check_move(x-1, y))    return False;
	  if(check_move(x-1, y+1))  return False;
	  if(check_move(x+1, y-1))  return False;
	  if(check_move(x+1, y))    return False;
	  if(check_move(x+1, y+1))  return False;
	  if(check_move(x,   y-1))  return False;
	  if(check_move(x,   y+1))  return False;
	  continue;
	}

	if(piece == PAWN)
	{
	  if(Debug)
	    printf("Move pawn:\n");
	  if(color == Mine.color)
	  {
	    if(check_move(x-1, y-1))  return False;
	    if(check_move(x+1, y-1))  return False;
	    if(check_move(x,   y-1))  return False;
	    if(check_move(x,   y-2))  return False;
	  }
	  else
	  {
	    if(check_move(x-1, y+1))  return False;
	    if(check_move(x+1, y+1))  return False;
	    if(check_move(x,   y+1))  return False;
	    if(check_move(x,   y+2))  return False;
	  }
	  continue;
	}
      }
  if(Debug)
    printf("Game Over\n");
  /* can't move -> game over */
  return True;
}
