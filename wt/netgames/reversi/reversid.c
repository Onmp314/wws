/*
 * Reversid, a reversi server
 *
 * I used TeSche's wreversi program as a starting point for this GameFrame
 * Reversi server, so I'd like to thank Torsten for it! :)
 *
 * Changes:
 * - Adapted wreversi as GameFrame server by discarding the user interface
 *   stuff and changing the source into callbacks.  8/96
 * - Reversid puts more weight for corner/edge positions. 8/96
 * - Fixed previous errors and changed/speeded up the brain slightly. 9/96
 * - Remade the flipping code and included it into best() function
 *   for speed. 9/96
 *
 * Other ideas for speeding up the game brain:
 * - Calculating moves while opponent is thinking and keeping the already
 *   calculated (still relevant) move trees in memory and using them when
 *   it's time to move.
 * - Calculating ready made patterns for optimum plays.
 * - Abort recursion of bad moves (may not work too well, if none
 *   of the legal moves is good).
 *
 * Recursion is what makes this code quite unusable at levels below 5.
 * Kreversid game included into the KDE package should be under GNU
 * copyleft, maybe it's engine could be loaned here?  It's a heck of a lot
 * faster and better than this one.
 *
 * (w) 1996 by Eero Tamminen (puu)
 * puujalka@modeemi.cs.tut.fi
 */

#include "server.h"
#include "messages.h"
#include "id.h"

#ifdef __MINT__
long _stksize = 8192;
#endif


#define VERSION	"0.9"

#define	BWIDTH		8
#define	BHEIGHT		8
#define	MAXLOOSE	(-32768)		/* worst possible move */

enum { EMPTY, WHITE, BLACK };
#define OPPONENT(x) (3 - (x))

typedef	struct
{
  char field[BWIDTH][BHEIGHT];
  int passed;
} BOARD;


/* how many stones the sides are worth on the lowest level (2) */
#define SIDE_BONUS	8

#define DEF_LEVEL	3

static int ComputerColor, HumanColor;
static int MaxDepth, SideBonus = SIDE_BONUS;	/* bound to level */
static int StartLevel = DEF_LEVEL;
static int Moving, CompX, CompY;
static BOARD Board, Backup;
static GAME *Game;


/* engine function prototypes. */
static void help(void);
static int set_level(int level);
static void set_side(player_t player);
static void initialize_game(void);
static void compute_move(void);
static int send_move(void);
static int do_undo(void);
static int pass_move(player_t color);
static int messages(short type, uchar x, uchar y);
static int parse_args(char *name, int argc, char *argv[]);

/* brain */
static int best(BOARD *bp, int color, int depth);
static int flips(BOARD *bp, int color, int x, int y);



/* information for the frontend. */
void get_configuration(GAME *game)
{
  /* startup variables */
  game->game_id = REVERSI_ID;

  /* function */
  game->level   = set_level;
  game->args    = parse_args;
  game->message = messages;
  game->start   = initialize_game;
  game->comp    = compute_move;
  game->move    = send_move;
  game->pass	= pass_move;
  game->side    = set_side;
  game->undo    = do_undo;

  Game = game;
}


static void help(void)
{
  fprintf(stderr, "\nReversi game brain (w) 1996 by Torsten Scherer\n");
  fprintf(stderr, "Reversi server " VERSION " (w) 1996 by Eero Tamminen\n\n");

  fprintf(stderr, "Options: reversid [-h] [-l #]\n");
  fprintf(stderr, "  -h    help\n");
  fprintf(stderr, "  -l #  recursion level (2-5)\n");
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
        /* level */
        case 'l':
	  if(++idx >= argc || !set_level(atoi(argv[idx])))
	  {
	    help();
	    return False;
	  }
	break;

	case 'h':
	  help();
	  break;

	default:
	  help();
	  return False;
      }
    }
    else
    {
      help();
      return False;
    }
  }
  /* ok */
  return True;
}


static void set_side(player_t player)
{
  if(player == Player0)
  {
    ComputerColor = WHITE;
    HumanColor = BLACK;
  }
  else
  {
    ComputerColor = BLACK;
    HumanColor = WHITE;
  }
}


static void initialize_game(void)
{
  int i, j;

  for (i = 0; i < BWIDTH; i++)
    for (j = 0; j < BHEIGHT; j++)
      Board.field[i][j] = EMPTY;

  Board.field[BWIDTH/2-1][BHEIGHT/2-1] = WHITE;
  Board.field[BWIDTH/2][BHEIGHT/2]     = WHITE;
  Board.field[BWIDTH/2][BHEIGHT/2-1]   = BLACK;
  Board.field[BWIDTH/2-1][BHEIGHT/2]   = BLACK;

  MaxDepth = StartLevel;
}


/* only possible before opponent has moved, return 0 if succeeded */
static int do_undo(void)
{
  Board = Backup;
  return True;
}


/* first send the computers move to opponent, and then inform
 * frameworks about the game turn change
 */
static int send_move(void)
{
  if(Moving)
  {
    Moving = False;
    if(CompX >= 0 && CompY >= 0)
    {
      send_msg(PIECE_MINE, CompX, CompY);
      return True;
    }
    else
      send_msg(MOVE_PASS, 0, 0);
  }
  else
    send_msg(MOVE_NEXT, 0, 0);

  /* turn over, next player */
  return False;
}


static int pass_move(player_t color)
{
  /* Game done (both passed)? */
  if(Board.passed)
    game_over();

  Board.passed = True;
  return True;
}


static int messages(short type, uchar x, uchar y)
{
  if(!Game->playing)
  {
    /* no board editing */
    send_msg(RETURN_FAIL, 0, 0);
    return True;
  }

  Backup = Board;
  switch(type)
  {
    case PIECE_MINE:
      if(!flips(&Board, HumanColor, x, y))
      {
        send_msg(RETURN_FAIL, 0, 0);
        return True;
      }
      send_msg(RETURN_MINE, x, y);
      Board.passed = False;
      break;

    case RETURN_MINE:
      if(!flips(&Board, ComputerColor, x, y))
        return False;

      Board.passed = False;
      break;

    /* unknown game message -> error */  
    default:
      return False;
  }
  return True;
}


/* 
 * the brainy part...
 */

static int set_level(int level)
{
  if(level < 2 || level > 20)
    level = DEF_LEVEL;

  /* the significance of bonuses decreases with more recursion */
  SideBonus = (20 - level) * SIDE_BONUS / 20 + 1;

  StartLevel = level;
  return True;
}


/* calculate the computer move */
static void compute_move(void)
{
  /* calculate the best move (changes CompX / CompY) */
  CompX = CompY = -1;
  best(&Board, ComputerColor, 0);
  Moving = True;
}


#define DIRS	8

static int Dirs[DIRS*2] =
  { -1, -1, 0, -1, 1, -1, -1, 0, 1, 0, -1, 1, 0, 1, 1, 1 };


/* For speeds sake, does all game board evaluation this here (without
 * further function calls compiler might then have also better changes at
 * optimizing the code).  There are also some gotos for exiting nested
 * loops...
 */
static int best(register BOARD *bp, int color, int depth)
{
  int dir, i, j, count, new, max, mx = 0, my = 0, opponent = OPPONENT(color);
  register char *field, piece;
  register int x, y, dx, dy;
  BOARD tmp;

  max = MAXLOOSE;

  i = BWIDTH;
  while(--i >= 0)
  {
    field = bp->field[i+1];

    j = BHEIGHT;
    while(--j >= 0)
    {
      /* not empty position? */
      if(*--field)
        continue;

      /* check if there's something to flip */
      dir = DIRS*2;

      while (dir > 0)
      {
	dx = Dirs[--dir];
	dy = Dirs[--dir];
	x = i;
	y = j;

	for (count = 0;; count++)
	{
	  x += dx;
	  y += dy;

	  if(x < 0 || x >= BWIDTH || y < 0 || y >= BHEIGHT)
	    break;

	  piece = bp->field[x][y];
	  if (!piece)
	    break;

	  if(piece == color)
	  {
	    if(count)
	      goto do_flips;
	    break;
	  }
	}
      }

      /* none of the directions for (i,j) a legal move */
	continue;


do_flips:

      /* copy current board and make the move (i,j) */
      tmp = *bp;
      tmp.field[i][j] = color;
      new = 0;

      for(;;)
      {
flip_this:
	/* flip the counted pieces */
	new += count;
	while(count--)
	{
	  x -= dx;
	  y -= dy;
	  tmp.field[x][y] = color;
	}

	for(;;)
	{
	  /* no more directions? => out */
	  if(dir <= 0)
	    goto flipping_done;

	  /* check direction for flips */
	  dx = Dirs[--dir];
	  dy = Dirs[--dir];
	  x = i;
	  y = j;

	  for(count = 0;; count++)
	  {
	    x += dx;
	    y += dy;

	    if(x < 0 || x >= BWIDTH || y < 0 || y >= BHEIGHT)
	      break;

	    piece = tmp.field[x][y];
	    if (!piece)
	      break;

	    if(piece == color)
	    {
	      if(count)
		goto flip_this;		/* yikes, loop exit backwards... */
	      break;
	    }
	  }
	  /* loop while nothing to flip */
	}
      }

flipping_done:

      /* add weight to the sides... */
      if(i == 0 || i == BWIDTH-1)
	new += SideBonus;
      if(j == 0 || j == BHEIGHT-1)
	new += SideBonus;

      /* recurse */
      if(depth < MaxDepth)
	new -= best(&tmp, opponent, depth+1);

      /* better alternative than previous ones? */
      if(new > max)
      {
	mx = i;
	my = j;
	max = new;
      }
    }
  }

  /* top level and move possible? */
  if(depth == 0 && max > MAXLOOSE)
  {
    /* new computer move */
    CompX = mx;
    CompY = my;
  }

  return max;
}

/* do opponent's flip
 */
static int flips(BOARD *bp, int color, int mx, int my)
{
  int dir, count, dx, dy, x, y, flipped;
  char piece;

#ifdef DEBUG
  fprintf(stderr, "\n  0 1 2 3 4 5 6 7\n");
  for(y = 0; y < BHEIGHT; y++)
  {
    fprintf(stderr, "%d", y);
    for(x = 0; x < BWIDTH; x++)
      switch(Board.field[x][y])
      {
	case BLACK:
	  fprintf(stderr, " #");
	  break;
	case WHITE:
	  fprintf(stderr, " O");
	  break;
	default:
	  fprintf(stderr, " .");
      }
    fprintf(stderr, "\n");
  }
#endif

  dir = DIRS*2;
  flipped = 0;

  while(dir > 0)
  {
    /* check direction for flips */
    dx = Dirs[--dir];
    dy = Dirs[--dir];
    x = mx;
    y = my;

    for(count = 0;; count++)
    {
      x += dx;
      y += dy;

      if(x < 0 || x >= BWIDTH || y < 0 || y >= BHEIGHT)
	break;

      piece = bp->field[x][y];
      if(!piece)
        break;

      if(piece == color)
      {
	/* do the flip */
	flipped += count;
	while(count--)
	{
	  x -= dx;
	  y -= dy;
	  bp->field[x][y] = color;
	}
	break;
      }
    }
    /* loop while nothing to flip */
  }

  if(flipped)
  {
    bp->field[mx][my] = color;
#ifdef DEBUG
  if(color == WHITE)
    fprintf(stderr, "WHITE moved to (%d,%d)\n", mx, my);
  else
    fprintf(stderr, "BLACK moved to (%d,%d)\n", mx, my);
#endif
  }

  return flipped;
}
