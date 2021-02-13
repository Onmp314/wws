/*
 * Networked, turn based, two player game frontend.
 *
 * A programming interface between game frontend and engine(s).
 * Generic include with messages etc.
 *
 * (w) 1996 by Eero Tamminen
 */

#ifndef __GAME_H
#define __GAME_H

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef DEBUG
/* convenience function... */
#define DEBUG_PRINT(x)	fprintf(stderr, "%s\n", x)
#else
#define DEBUG_PRINT(x)
#endif

/* Port used by all the framework based games. */
#define GAME_PORT	18245

#ifndef uchar
#define uchar	unsigned char
#endif

typedef enum { Player0 = 0, Player1 } player_t;
#define False	0
#define True	(!False)

/* frontend functions for the game engine */

extern void send_msg(short type, uchar x, uchar y);
extern int confirm(const char *operation);	/* True if user confirmed operation */
extern void show_info(const char *msg);	/* show (eg. status) strings to user */
extern void game_over(void);		/* tell framework game's finished */


/* Network messages types (short):
 *
 * All of the messages need to be acknowledged by the other
 * end, before new ones are sent. This should garantee that
 * a broken connection can be resumed later to the opponent
 * without any confusions.
 *
 * note: Comments are from the receiver's perspective.
 * Sender of a message receives the return code.
 */
#define MSG_SIZE 4		/* short id + two bytes */
enum MESSAGES
{
  /* messages that framework uses */
  GAME_START = 0,		/* start game */
  GAME_RESIGN,			/* hopeless... */
  GAME_CONT,			/* continue from setup situation */
  GAME_LEVEL,			/* select (computer) opponent level */
  MOVE_UNDO,			/* undo opponents last move */
  MOVE_PASS,			/* your turn, opponent passed */
  MOVE_NEXT,			/* your turn, sent by engine */

  /* engine specific message IDs start from this value */
  ENGINE_MSG = 0x100,

  /* rest are acknowledgements (framework allow only one
   * message to be in transit at the time)
   */
  RETURN_CODE = 0x200,		/* if greater -> last message acknowledged */
  RETURN_ILLEGAL,		/* illegal message (incompatible games) */
  RETURN_FAIL,			/* operation failed (try again later) */
  RETURN_NEXT,			/* opponent's turn */
  RETURN_PASS,			/* you pass */
  RETURN_UNDO,			/* undo your last move */
  RETURN_LEVEL,			/* game server level accepted */
  RETURN_CONT,			/* continue accepted */
  RETURN_RESIGN,		/* you resigned the game */
  RETURN_START,			/* start game (confirmation) */

  /* engine specific return message IDs start from this value */
  ENGINE_RET = 0x300
};


/* game configuration / state structure */
typedef struct
{
  /* set by the framework */
  int turn;			/* turn number */
  int my_move;			/* whose move it is */
  int playing;			/* game 'on' */
  player_t player;		/* player side (PLAYER0 starts) */

  /* set by all game engines */
  long game_id;			/* game ID (see ports.h) */
  void (*start)(void);		/* (re)start game */
  void (*side)(player_t color);	/* Use this color (PLAYER0 starts) */

  /* this function will get any unidentified (game specific) messages */
  int (*message)(short id, uchar x, uchar y);

#ifdef GAME_SERVER
  /* game server 'brain' specific, level is optional */
  void (*comp)(void);		/* calculate computer's move */
  int (*move)(void);		/* send a move (more moves -> True) */
  int (*level)(int level);	/* set computer level (value ok -> True) */
#else
  /* set by interactive game engines */
  const char *name;		/* eg. to show in game window title */
  long width;			/* game area size */
  long height;
#endif

  /* All the members below are optional */

  int (*cont)(player_t color);	/* continue a set-up game (ok -> True) */
  int (*pass)(player_t color);	/* pass this move (ok -> True) */
  int (*undo)(void);		/* undo last own move (ok -> True) */
  void (*over)(void);		/* opponent resigned, game over */
  const char *i_info;		/* message to show when it's your turn */
  const char *o_info;		/* message to show after your move/pass */

  /* parse program's command line arguments, game name and
   * framework specific arguments are not counted into argc/argv
   */
  int (*args)(char *name, int argc, char *argv[]);

} GAME;


/* Game engine  */

/* functions for extending interface, intializing the board and
 * processing events (might) need GUI specific arguments so each
 * GUI has it's own header for them.
 */

/* fills the game specific stuff into the structure */
extern void get_configuration(GAME *game);

#endif	/* __GAME_H */
