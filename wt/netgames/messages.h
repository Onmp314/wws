/*
 * Networked, turn based, two player game frontend.
 *
 * Some examplary engine specific messages.
 *
 * Engines can use these or their own messages as long
 * as normal message codes are >= ENGINE_MSG and < RETURN_CODE
 * and return codes are >= ENGINE_RET.
 *
 * Note that server versions need to include 'server.h'
 * before (defines GAME_SERVER, which has an effect
 * on 'game.h' which this file needs) this.
 *
 * (w) 1996 by Eero Tamminen
 */

/* These should be enough for most board games. If engine doesn't allow
 * board editing PIECE_MINE/PIECE_YOUR/PIECE_REMOVE could be replaced with
 * eg. PIECE_MOVE define.
 */
enum
{
  PIECE_MINE = ENGINE_MSG,		/* put my piece here */
  PIECE_YOUR,				/* put your piece here */
  PIECE_REMOVE,				/* remove piece from here */
  PIECE_SELECT,				/* select this piece */

  /* round-trip return messages */
  RETURN_MINE = ENGINE_RET,
  RETURN_YOUR,
  RETURN_REMOVE,
  RETURN_SELECT
};

