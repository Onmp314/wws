/* 
 * Curses specific Reversi callbacks for the frontend functions.
 *
 * Keypad is used to move the cursor and space to set a piece on
 * current position.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "c_game.h"		/* includes standard stuff */
#include "common.h"

#define OFFSET	2		/* letter, space, line, space */

/* used by the frontend */
int BoardWidth  = BOARD_SIZE*2+6;
int BoardHeight = BOARD_SIZE+6;

static int CursorX, CursorY, TextX, TextY, Captured;

/* callback function prototypes. */
static void set_cursor(int x, int y);


/* GUI specific functions */

/* Draw the initial board.
 * Could also load and transmit play piecesto W server.
 */
int initialize_board(void)
{
  int x, y;

  for(y = 'A', x = 0; x < BOARD_SIZE*2; y++, x += 2)
  {
    mvaddch(OFFSET+1+BOARD_SIZE, OFFSET+1+x, y);
    mvaddch(OFFSET+BOARD_SIZE, OFFSET+x, '-');
    addch('-');
    mvaddch(OFFSET-1, OFFSET+x, '-');
    addch('-');
  }
  mvaddch(OFFSET+BOARD_SIZE, OFFSET + BOARD_SIZE*2, '-');
  addch('+');
  mvaddch(OFFSET-1, OFFSET + BOARD_SIZE*2, '-');
  addch('+');
  mvaddch(OFFSET+BOARD_SIZE, OFFSET-1, '+');
  mvaddch(OFFSET-1, OFFSET-1, '+');

  for(y = 0; y < BOARD_SIZE; y++)
  {
    mvaddch(OFFSET + y, OFFSET-1, '|');
    for(x = 0; x < BOARD_SIZE*2; x += 2)
    {
      mvaddch(OFFSET + y, OFFSET+1 + x, '.');
    }
    move(OFFSET + y, OFFSET+1+BOARD_SIZE*2);
    addch('|');
    addch(' ');
    addch('A' + y);
  }

  CursorX = BOARD_SIZE/2;
  CursorY = BOARD_SIZE/2;
  set_cursor(CursorX, CursorY);
  refresh();

  return True;
}

void add_text(int x, int y)
{
  mvaddstr(y, x, "Pieces:");
  Captured = 0;
  TextX = x;
  TextY = y;
}

void draw_pieces(color_t color, int pieces)
{
  char number[12];

  if(color == BLACK)
  {
    sprintf(number, "[#]: %03d", pieces);
    mvaddstr(TextY+1, TextX, number);
  }
  else
  {
    sprintf(number, "[O]: %03d", pieces);
    mvaddstr(TextY+2, TextX, number);
  }
  Captured = pieces;
  refresh();
}

void mark_pieces(color_t color)
{
  standout();
  /* should be the last set value */
  draw_pieces(color, Captured);
  standend();
}

static void set_cursor(int x, int y)
{
  move(OFFSET+y, OFFSET+1 + x*2);
  refresh();
}

void draw_empty(int x, int y)
{
  mvaddch(OFFSET+y, OFFSET+1 + x*2, '.');
  refresh();
}

void draw_white(int x, int y)
{
  mvaddch(OFFSET+y, OFFSET+1 + x*2, 'O');
  refresh();
}

void draw_black(int x, int y)
{
  mvaddch(OFFSET + y, OFFSET+1 + x*2, '#');
  refresh();
}

/* return 0 when turn is over */
void process_key(int key)
{
  switch(key)
  {
    case ' ':
      my_move(CursorX, CursorY);
      return;

    case '8':
      CursorY--;
      break;
    case '9':
      CursorX++;
      CursorY--;
      break;
    case '6':
      CursorX++;
      break;
    case '3':
      CursorX++;
      CursorY++;
      break;
    case '2':
      CursorY++;
      break;
    case '1':
      CursorX--;
      CursorY++;
      break;
    case '4':
      CursorX--;
      break;
    case '7':
      CursorX--;
      CursorY--;
      break;
    case '5':
      CursorX = BOARD_SIZE/2;
      CursorY = BOARD_SIZE/2;
      break;

    default:
      return;
  }

  if(CursorX < 0)
    CursorX = 0;

  if(CursorY < 0)
    CursorY = 0;

  if(CursorX >= BOARD_SIZE)
    CursorX = BOARD_SIZE-1;

  if(CursorY >= BOARD_SIZE)
    CursorY = BOARD_SIZE-1;

  /* move cursor */
  set_cursor(CursorX, CursorY);

  return;
}

void flush_out(void)
{
  refresh();
}
