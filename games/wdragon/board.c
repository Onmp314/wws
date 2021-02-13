/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes	March 1989
* W Port: Jens Kilian		February 1996
*
* board.c - Deals with the Mah-Jongg board.  Setup and execution.
******************************************************************************/

#define _BOARD_C_

#include <Wlib.h>
#include <stdlib.h>
#include "main.h"
#include "proto.h"
#include "board.h"

int		Board_Width	= 29 * 15 + 1 + 8;
int		Board_Height	= 33 * 10 + 1 + 8;
int		Score		= NTILES;

int		Board_Tile0_X	= 4;
int		Board_Tile0_Y	= 4 + 2 * 33;
unsigned int	Tile_Width	= 28;
unsigned int	Tile_Height	= 32;
int		Shadow_X	= 28 / 8;
int		Shadow_Y	= 32 / 8;
int		Side_X		= (28 / 10) & ~1;
int		Side_Y		= (32 / 10) & ~1;

Board_Position_Rec	Board_Tiles[NROWS][NCOLS];
Board_Position		Click1 = Board_Position_NULL;
int			Click1_Row;
int			Click1_Col;
Board_Position		Click2 = Board_Position_NULL;
int			Click2_Row;
int			Click2_Col;

static long		Click2_Time;
static Boolean		One_Button_Hint = FALSE;


void Write_Game(FILE *file)
/******************************************************************************
*   file    - Specifies a file open for write
*
* Called to write out the current game context for later rereading.
******************************************************************************/
{

    (void)fwrite( (char*)&Score, 1, sizeof(Score), file );
    (void)fwrite( (char*)&Board_Tiles[0][0], 1, sizeof(Board_Tiles), file );

} /* Write_Game */


void Read_Game(FILE *file)
/******************************************************************************
*   file    - Specifies a file open for reading
*
* Called to read in a new current game context.
******************************************************************************/
{

    Click1 = Board_Position_NULL;
    Click2 = Board_Position_NULL;
    (void)fread( (char*)&Score, 1, sizeof(Score), file );
    (void)fread( (char*)&Board_Tiles[0][0], 1, sizeof(Board_Tiles), file );

} /* Read_Game */


static int Pick_Tile(char *Avail)
/******************************************************************************
*   Avail	- Specifies an [NTILES] array of available tiles.  Unavailable
*		  slots contain NO_TILE.
*
* Called to pick a random tile from the Available tiles.
******************************************************************************/
{
    register char	*t;
    register int	k;

/*--Pick a random starting place. */

    k = (int)rand() % NTILES;
    t = &Avail[k];

/*--Search until we find a non-NO_TILE slot. */

    while (*t == NO_TILE) {
	++t;
	if (++k == NTILES) {
	    t = &Avail[0];
	    k = 0;
	}
    }

/*--Return the tile we found and zap the slot. */

    k = *t;
    *t = NO_TILE;
    return k;

} /* Pick_Tile */


void Set_Tile_Controls(void)
/******************************************************************************
* Called whenever the board has been reset or resized.  We recalculate all of
* the drawing controls for the tiles.
******************************************************************************/
{
    register Board_Position	bp;
    int				row, col;

/*--Now set up the control information for all of the tiles.  The special
 *  tiles are easy. */

    DEBUG_CALL(Set_Tile_Controls);
    if (Board_Tiles[SPEC4].level > 0) {
	Board_Tiles[SPEC4].x	 	= Board_Tile0_X + 6 * (Tile_Width + 1)
	  				  + (Tile_Width + 1) / 2 + 4 * Side_X;
	Board_Tiles[SPEC4].y	 	= Board_Tile0_Y + 3 * (Tile_Height + 1)
					  + (Tile_Height + 1) / 2 - 3 * Side_Y;
    }

    if (Board_Tiles[SPEC3].level > 0) {
	Board_Tiles[SPEC3].x	 	= Board_Tile0_X + 0 * (Tile_Width + 1);
	Board_Tiles[SPEC3].y	 	= Board_Tile0_Y + 3 * (Tile_Height + 1)
					  + (Tile_Height + 1) / 2;
    }

    if (Board_Tiles[SPEC2].level > 0) {
	Board_Tiles[SPEC2].x	 	= Board_Tile0_X + 13 * (Tile_Width+1);
	Board_Tiles[SPEC2].y	 	= Board_Tile0_Y +  3 * (Tile_Height+1)
					  + (Tile_Height + 1) / 2;
    }

    if (Board_Tiles[SPEC1].level > 0) {
	Board_Tiles[SPEC1].x	 	= Board_Tile0_X + 14 * (Tile_Width+1);
	Board_Tiles[SPEC1].y	 	= Board_Tile0_Y +  3 * (Tile_Height+1)
	      				  + (Tile_Height + 1) / 2;
    }

/*--Do the more regular tiles. */

    for (row = 0; row <= 7; ++row) {
	for (col = 12; col >= 1; --col) {
	    bp = &Board_Tiles[row][col];

/*--Skip any tiles that don't exist. */

	    if (bp->level == 0) { continue; }

/*--Set up the face x/y coordinates. */

	    bp->x = Board_Tile0_X + col * (Tile_Width + 1);
	    bp->y = Board_Tile0_Y + row * (Tile_Height + 1);

	}
    }
    DEBUG_RETURN(Set_Tile_Controls);

} /* Set_Tile_Controls */


static void Pick1(Board_Position bp, char *Avail)
{
    bp->tiles[0] = Pick_Tile( Avail );
    bp->level = 1;
}

static void Pick2(Board_Position bp, char *Avail)
{
    bp->tiles[0] = Pick_Tile( Avail );
    bp->tiles[1] = Pick_Tile( Avail );
    bp->level = 2;
}

static void Pick3(Board_Position bp, char *Avail)
{
    bp->tiles[0] = Pick_Tile( Avail );
    bp->tiles[1] = Pick_Tile( Avail );
    bp->tiles[2] = Pick_Tile( Avail );
    bp->level = 3;
}

static void Pick4(Board_Position bp, char *Avail)
{
    bp->tiles[0] = Pick_Tile( Avail );
    bp->tiles[1] = Pick_Tile( Avail );
    bp->tiles[2] = Pick_Tile( Avail );
    bp->tiles[3] = Pick_Tile( Avail );
    bp->level = 4;
}


void Setup_New_Game(void)
/******************************************************************************
* Called to generate an all-new game.
******************************************************************************/
{
    register Board_Position	bp;
    char			Avail[NTILES];
    int				row, col, i;

/*--Clear the board. */

    DEBUG_CALL(Setup_New_Game);
    bp = &Board_Tiles[0][0];
    for (row = 0; row < NROWS; ++row) {
	for (col = 0; col < NCOLS; ++col) {
	    bp->tiles[0] = NO_TILE;
	    bp->tiles[1] = NO_TILE;
	    bp->tiles[2] = NO_TILE;
	    bp->tiles[3] = NO_TILE;
	    bp->level = 0;
	}
    }

/*--Mark all tiles as available. */

    i = 0;
    for (row = 0; row < 4; ++row) {
	Avail[i++] = row + 1;
	Avail[i++] = row + 5;
	for (col = 8; col < NFACES; ++col) {
	    Avail[i++] = 1 + col % NFACES;
	}
    }
    if (i != NTILES) { (void)fprintf( stderr, "NTILES gak!\n" ); }

/*--Fill in the "odd" tile slots. */

    Pick1( &Board_Tiles[SPEC1], Avail );
    Pick1( &Board_Tiles[SPEC2], Avail );
    Pick1( &Board_Tiles[SPEC3], Avail );
    Pick1( &Board_Tiles[SPEC4], Avail );

    for (col = 1; col <= 12; ++col) {
	Pick1( &Board_Tiles[0][col], Avail );
	Pick1( &Board_Tiles[7][col], Avail );
    }
    for (row = 1; row <= 6; ++row) {
	Pick1( &Board_Tiles[row][ 3], Avail );
	Pick1( &Board_Tiles[row][10], Avail );
    }
    for (row = 2; row <= 5; ++row) {
	Pick1( &Board_Tiles[row][ 2], Avail );
	Pick1( &Board_Tiles[row][11], Avail );
    }
    for (row = 3; row <= 4; ++row) {
	Pick1( &Board_Tiles[row][ 1], Avail );
	Pick1( &Board_Tiles[row][12], Avail );
    }

/*--Now do the next square at level 2. */

    for (col = 4; col <= 9; ++col) {
	Pick2( &Board_Tiles[1][col], Avail );
	Pick2( &Board_Tiles[6][col], Avail );
    }
    for (row = 2; row <= 5; ++row) {
	Pick2( &Board_Tiles[row][4], Avail );
	Pick2( &Board_Tiles[row][9], Avail );
    }

/*--Now do the next square at level 3. */

    for (col = 5; col <= 8; ++col) {
	Pick3( &Board_Tiles[2][col], Avail );
	Pick3( &Board_Tiles[5][col], Avail );
    }
    for (row = 3; row <= 4; ++row) {
	Pick3( &Board_Tiles[row][5], Avail );
	Pick3( &Board_Tiles[row][8], Avail );
    }

/*--Now do the final square at level 4. */

    for (row = 3; row <= 4; ++row) {
	for (col = 6; col <= 7; ++col) {
	    Pick4( &Board_Tiles[row][col], Avail );
	}
    }

/*--Now set up the control information for all of the tiles. */

    Set_Tile_Controls();
    Score = NTILES;
    DEBUG_RETURN(Setup_New_Game);

} /* Setup_New_Game */


void Restart_Game(void)
/******************************************************************************
* Called when the RESTART button is pressed.  Restart the game.
******************************************************************************/
{
    int				row;
    int				col;
    register Board_Position	bp;

/*--Reset levels and remove hilites. */

    DEBUG_CALL(Restart_Game);
    Click1 = Board_Position_NULL;
    Click2 = Board_Position_NULL;
    Score = NTILES;
    bp = &Board_Tiles[0][0];
    for (row = 0; row < NROWS; ++row) {
	for (col = 0; col < NCOLS; ++bp,++col) {
	    if      (bp->tiles[3] != NO_TILE) { bp->level = 4; }
	    else if (bp->tiles[2] != NO_TILE) { bp->level = 3; }
	    else if (bp->tiles[1] != NO_TILE) { bp->level = 2; }
	    else if (bp->tiles[0] != NO_TILE) { bp->level = 1; }
	    else { bp->level = 0; }
	}
    }

/*--Finish setting up and then redraw everything. */

    Set_Tile_Controls();
    Board_Expose();
    DEBUG_RETURN(Restart_Game);

} /* Restart_Game */


static void Set_Tile_Draw(int row, int col)
/******************************************************************************
*   row	- Specifies the row of the tile
*   col - Specifies the column of the tile
*
* Called to set the "draw" flag on a tile.  We also recursively set the
* draw flag on anyone that needs to be redrawn because we are being redrawn.
******************************************************************************/
{
    register Board_Position	bp = &Board_Tiles[row][col];

/*--If we don't exist or if we are already being redrawn then stop. */

    DEBUG_CALL(Set_Tile_Draw);
    if (bp->level == 0 || bp->draw) {
	return;
    }

/*--Redraw us.  Redraw anyone to our left that has a height greater than ours
 *  because their shadow/tile-face overlaps us. */

    bp->draw = TRUE;
    if (col > 0 &&
	Board_Tiles[row][col-1].level > bp->level) {
	Set_Tile_Draw( row, col-1 );
    }

/*--Redraw anyone below us that has a level greater than ours because their
 *  shadow/tile-face overlaps us. */

    if (row < 7 &&
	Board_Tiles[row+1][col].level > bp->level) {
	Set_Tile_Draw( row+1, col );
    }

/*--Redraw anyone below-to-the-left of us that has a level greater or equal
 *  to ours because their shadow/tile-face overlays us */

    if (row < 7 &&
	col > 0 &&
	Board_Tiles[row+1][col-1].level >= bp->level) {
	Set_Tile_Draw( row+1, col-1 );
    }

/*--If we are certain specific tiles then we may need to set specific other
 *  tiles. */

    if (row == 3 || row == 4) {
	if (col == 6 || col == 7) {
	    Set_Tile_Draw( SPEC4row, SPEC4col );
	} else if (col == 1) {
	    Set_Tile_Draw( SPEC3row, SPEC3col );
	}
    }
    DEBUG_RETURN(Set_Tile_Draw);

} /* Set_Tile_Draw */


static void Remove_Tile(Board_Position bp, int row, int col)
/******************************************************************************
* Called to remove the top tile of the indicated Board_Position.
******************************************************************************/
{

/*--If the tile just went away then clear the area and allow the window
 *  background to shine through. */

    DEBUG_CALL(Remove_Tiles);
    if (bp->level == 1) {
	if (Tile_Control & SHADOW) {
	    wsetmode(M_CLEAR);
	    w_pbox(Board, bp->x, bp->y - Side_Y - Shadow_Y,
		          Tile_Width + Side_X + 3 + Shadow_X,
		          Tile_Height + Side_Y + 2 + Shadow_Y);
	} else {
	    wsetmode(M_CLEAR);
	    w_pbox(Board, bp->x, bp->y - Side_Y,
		          Tile_Width + Side_X + 2,
		          Tile_Height + Side_Y + 2);
	}
    } else {
	int	sidex = Side_X * bp->level;
	int	sidey = Side_Y * bp->level;
	if (Tile_Control & SHADOW) {
	    wsetmode(M_CLEAR);
	    w_pbox(Board, bp->x + sidex, bp->y - sidey - Shadow_Y,
		          Tile_Width + 3 + Shadow_X,
		          Tile_Height+ 2 + Shadow_Y);
	} else {
	    wsetmode(M_CLEAR);
	    w_pbox(Board, bp->x + sidex, bp->y - sidey,
		          Tile_Width + 2,
		          Tile_Height+ 2);
	}
	Set_Tile_Draw( row, col );
    }
    --bp->level;

/*--Schedule the surrounding tiles for redrawing. */

    if (col == SPEC1col) {
	if (row == SPEC4row) {
	    Set_Tile_Draw( 3, 6 );
	    Set_Tile_Draw( 3, 7 );
	    Set_Tile_Draw( 4, 6 );
	    Set_Tile_Draw( 4, 7 );
	    return;
	} else if (row == SPEC3row) {
	    Set_Tile_Draw( 3, 1 );
	    Set_Tile_Draw( 4, 1 );
	    return;
	} else if (row == SPEC2row) {
	    Set_Tile_Draw( SPEC1row, SPEC1col );
	    Set_Tile_Draw( 3, 12 );
	    Set_Tile_Draw( 4, 12 );
	    return;
	} else {
	    Set_Tile_Draw( SPEC2row, SPEC2col );
	    Set_Tile_Draw( 3, 12 );
	    Set_Tile_Draw( 4, 12 );
	    return;
	}
    }
    if (col == 1 && (row == 3 || row == 4)) {
	Set_Tile_Draw( SPEC3row, SPEC3col );
    }
    if (col == 12 && (row == 3 || row == 4)) {
	Set_Tile_Draw( SPEC2row, SPEC2col );
    }
    if (row > 0) {
	Set_Tile_Draw( row - 1, col + 1 );
	Set_Tile_Draw( row - 1, col     );
	if (col > 0 &&
	    Board_Tiles[row-1][col].level == 0) {
	    Set_Tile_Draw( row - 1, col - 1 );
	}
    }
    Set_Tile_Draw( row, col+1 );
    if (col > 0) {
	Set_Tile_Draw( row,     col - 1 );
    }
    if (row < 7) {
	Set_Tile_Draw( row + 1, col     );
	if (col > 0) {
	    Set_Tile_Draw( row + 1, col - 1 );
	}
    }
    DEBUG_RETURN(Remove_Tile);

} /* Remove_Tile */


static void Touch_Tile(Board_Position bp, int row, int col, long time)
/******************************************************************************
* Called when we click on a specific tile.  We decide what to do.  For a
* single click we hilite the tile unless we already have two tiles hilited.
* For a "double" click with two tiles hilited we will remove both of the
* tiles.
******************************************************************************/
{

/*--If there is no Click1 then this guy becomes it. */

    DEBUG_CALL(Touch_Tile);
    if (Click1 == Board_Position_NULL) {
	Click1 = bp;
	Click1_Row = row;
	Click1_Col = col;
	Hilite_Tile( row, col );
	DEBUG_RETURN(Touch_Tile);
	return;
    }

/*--If there is no Click2 then this guy becomes it unless he is already Click1.
 */

    if (Click1 != bp) {
	if (Click2_Row == row &&
	    Click2_Col == col &&
	    Click2_Time + Dragon_Resources.Double_Click_Time >= time) {
	    Click2 = bp;
	}
	if( Click2 == Board_Position_NULL) {
	    Click2 = bp;
	    Click2_Row = row;
	    Click2_Col = col;
	    Click2_Time = time;
	    Hilite_Tile( row, col );
	    DEBUG_RETURN(Touch_Tile);
	    return;
	}

/*--If this guy is not one Click1 and not Click2 then we have an error. */

	if (Click2 != bp) {
	    w_beep();
	    DEBUG_RETURN(Touch_Tile);
	    return;
	}
    }

/*-- double-click removes both tiles. */

    if (Click2 != Board_Position_NULL &&
	Click2_Time + Dragon_Resources.Double_Click_Time >= time) {
	One_Button_Hint = FALSE;
	Remove_Tile( Click1, Click1_Row, Click1_Col );
	Click1 = Board_Position_NULL;
	Remove_Tile( Click2, Click2_Row, Click2_Col );
	Click2 = Board_Position_NULL;
	Score -= 2;
	Draw_All_Tiles();
	DEBUG_RETURN(Touch_Tile);
	return;
    }

/*--2nd click on any tile means turn-it-off. */

    if (Click1 == bp) {
	int	s;
	Hilite_Tile( Click1_Row, Click1_Col );
	Click1 = Click2;
	s = Click1_Row;
	Click1_Row = Click2_Row;
	Click2_Row = s;
	s = Click1_Col;
	Click1_Col = Click2_Col;
	Click2_Col = s;;
	Click2 = Board_Position_NULL;
    } else {
	Click2 = Board_Position_NULL;
	Hilite_Tile( Click2_Row, Click2_Col );
    }
    Click2_Time = time;
    DEBUG_RETURN(Touch_Tile);

} /* Touch_Tile */


void Tile_Remove(void)
/******************************************************************************
* Called when the remove-selected-tile-pair mouse button is pressed.
******************************************************************************/
{

    DEBUG_CALL(Tile_Remove);
    if (Click1 != Board_Position_NULL &&
	Click2 != Board_Position_NULL) {
	Click2_Time = w_gettime();
	Touch_Tile( Click2, Click2_Row, Click2_Col, Click2_Time );
    }
    DEBUG_RETURN(Tile_Remove);

} /* Tile_Remove */


static Boolean Touch(Board_Position bp, WEVENT *event)
/******************************************************************************
* Return TRUE if this WEVENT touched this Board_Position.
******************************************************************************/
{
    int		face_x = bp->x + bp->level * Side_X;
    int		face_y = bp->y - bp->level * Side_Y;

/*--Does this tile exist? */

    DEBUG_CALL(Touch);
    if (bp->level == 0) {
	DEBUG_RETURN(Touch);
	return FALSE;
    }

/*--Did we touch the face? */

    if (event->x >= face_x && event->x <= face_x + Tile_Width + 1 &&
	event->y >= face_y && event->y <= face_y + Tile_Height + 1) {
	DEBUG_RETURN(Touch);
	return TRUE;
    }

/*--Did we touch the side? */

    if (event->x >= bp->x && event->x <= bp->x + Tile_Width + 1 &&
	event->y >= bp->y && event->y <= bp->y + Tile_Height + 1) {
	DEBUG_RETURN(Touch);
	return TRUE;
    }

/*--Guess not. */

    DEBUG_RETURN(Touch);
    return FALSE;

} /* Touch */


void Tile_Press(WEVENT *event)
/******************************************************************************
* Called when the Board receives an EVENT_MPRESS.
******************************************************************************/
{
    register Board_Position	bp;
    int		x;
    int		y;
    int		row;
    int		col;
    long	time = w_gettime ();

/*--Figure out a rough row/col coordinate for the click. */

    DEBUG_CALL(Tile_Press);
    y = event->y - Board_Tile0_Y;
    if (y < 0) { return; }
    row = y / (Tile_Height + 1);
    if (row > 7) { return; }
    x = event->x - Board_Tile0_X;
    if (x < 0) { return; }
    col = x / (Tile_Width + 1);
    if (col < 0 || row > 14) { goto Touched; }

/*--See if we are a special tile. */

    if (col == 0) {
	if (Touch( bp = &Board_Tiles[SPEC3], event )) {
	    Touch_Tile( bp, SPEC3row, SPEC3col, time );
	    goto Touched;
	}
	goto Touched;
    } else if (col == 13) {
	if (Touch( bp = &Board_Tiles[SPEC2], event )) {
	    Touch_Tile( bp, SPEC2row, SPEC2col, time );
	    goto Touched;
	}
	if (Touch( bp = &Board_Tiles[4][12], event )) {
	    Touch_Tile( bp, 4, 12, time );
	    goto Touched;
	}
	if (Touch( bp = &Board_Tiles[3][12], event )) {
	    Touch_Tile( bp, 3, 12, time );
	    goto Touched;
	}
	goto Touched;
    } else if (col == SPEC1col) {
	if (Touch( bp = &Board_Tiles[SPEC1], event )) {
	    Touch_Tile( bp, SPEC1row, SPEC1col, time );
	    goto Touched;
	}
	if (Touch( bp = &Board_Tiles[SPEC2], event )) {
	    Touch_Tile( bp, SPEC2row, SPEC2col, time );
	    goto Touched;
	}
	goto Touched;
    } else if ((row == 3 || row == 4) && (col == 6 || col == 7)) {
	if (Touch( bp = &Board_Tiles[SPEC4], event )) {
	    Touch_Tile( bp, SPEC4row, SPEC4col, time );
	    goto Touched;
	}
    }

/*--See if the x/y falls exactly into somebody else's tile face. */

    if (col > 0 && row < 7) {
	if (Touch( bp = &Board_Tiles[row+1][col-1], event )) {
	    Touch_Tile( bp, row+1, col-1, time );
	    goto Touched;
	}
    }
    if (row < 7) {
	if (Touch( bp = &Board_Tiles[row+1][col], event )) {
	    Touch_Tile( bp, row+1, col, time );
	    goto Touched;
	}
    }
    if (col > 0) {
	if (Touch( bp = &Board_Tiles[row][col-1], event )) {
	    Touch_Tile( bp, row, col-1, time );
	    goto Touched;
	}
    }

/*--We don't have a touch on a neighbor so it must be us. */

    if (Touch( bp = &Board_Tiles[row][col], event )) {
	Touch_Tile( bp, row, col, time );
	goto Touched;
    }

  Touched :
    DEBUG_RETURN(Tile_Press);

} /* Tile_Press */


static Boolean Tile_Not_Free(int row, int col)
/******************************************************************************
* Returns TRUE if the tile has neither a left nor a right side free.
******************************************************************************/
{

/*--The 4 in the center can be covered by SPEC4. */

    if (row == 3 || row == 4) {
	if ((col == 6 || col == 7) &&
	    Board_Tiles[SPEC4].level > 0) { return TRUE; }
	else if (col == 1 &&
		 Board_Tiles[SPEC3].level > 0 &&
		 Board_Tiles[row][col+1].level > 0) { return TRUE; }
	else if (col == 12 &&
		 Board_Tiles[SPEC2].level > 0 &&
		 Board_Tiles[row][col-1].level > 0) { return TRUE; }
    }
	
/*--If a tile has a neighbor then he isn't free. */

    if (Board_Tiles[row][col-1].level >= Board_Tiles[row][col].level &&
	Board_Tiles[row][col+1].level >= Board_Tiles[row][col].level) {
	return TRUE;
    }

/*--Check the special tiles. */

    if (col == SPEC1col) {

/*--Tiles 1, 3, and 4 are always free. */

	if (row != SPEC2row) { return FALSE; }

/*--Tile 2 is free if tile 1 is gone or if its two normal neighbors are gone.*/

	if (Board_Tiles[SPEC1].level > 0 &&
	    (Board_Tiles[3][12].level > 0 ||
	     Board_Tiles[4][12].level > 0)) { return TRUE; }
    }
    return FALSE;

} /* Tile_Not_Free */


void Tile_Release(void)
/******************************************************************************
* Called when the Board receives an EVENT_MRELEASE.
******************************************************************************/
{
    extern int	Cheating;

/*--If there is a Click2 and if the tile type does not match with Click1 then
 *  unhilite Click2. */

    DEBUG_CALL(Tile_Release);
    if (!Cheating &&
	Click1 != Board_Position_NULL &&
	Click2 != Board_Position_NULL) {
	int		tile1, tile2;

	tile1 = Click1->tiles[Click1->level-1];
	tile2 = Click2->tiles[Click2->level-1];
	if (/* Do tile faces match for those types that must match exactly? */
	    ((tile1 > 8 || tile2 > 8) && tile1 != tile2) ||
	    /* Are both tiles seasons? */
	    (tile1 <= 4 && tile2 > 4) ||
	    /* Are both tiles flowers? */
	    (tile1 >= 5 && tile1 <= 8 && (tile2 < 5 || tile2 > 8))) {
	    /* They don't match. */
	    if (Dragon_Resources.Sticky_Tile) {
		/* Simply remove tile 2 from selected tiles. */
		Hilite_Tile( Click2_Row, Click2_Col );
	    } else {
		/* Remove tile 1 from selection and make tile 2 => tile 1.*/
		Hilite_Tile( Click1_Row, Click1_Col );
 		Click1 	    = Click2;
 		Click1_Row  = Click2_Row;
 		Click1_Col  = Click2_Col;
	    }
	    Click2      = Board_Position_NULL;
	    Click2_Col  = 0;	/* Prevent dbl-clk removing 1 tile. */
	    Click2_Time = 0;
	}
    }

/*--If this tile has a left or a right neighbor then he isn't allowed. */

    if (!Cheating) {
	if (Click2 != Board_Position_NULL &&
	    Tile_Not_Free( Click2_Row, Click2_Col)) {
	    Hilite_Tile( Click2_Row, Click2_Col );
	    Click2 = Board_Position_NULL;
	    Click2_Time = 0;
	}
	if (Click1 != Board_Position_NULL &&
	    Tile_Not_Free( Click1_Row, Click1_Col)) {
	    Hilite_Tile( Click1_Row, Click1_Col );
	    Click1 = Board_Position_NULL;
	}
    }

    DEBUG_RETURN(Tile_Release);

} /* Tile_Release */


void Next_Tile(int Click, int *row, int *col)
/******************************************************************************
* Returns the "next" tile past row/col that exists and is "free".  Returns 0,0
* when we run out of tiles.
******************************************************************************/
{
    int		tile1, tile2;

/*--Loop until we give up.  Advance the column.  Advance the row on column
 *  overflow.  Give up on row overflow. */

    DEBUG_CALL(Next_Tile);
    for (;;) {
	++*col;
	if (*col > 14) {
	    *col = 1;
	    ++*row;
	    if (*row > 7) {
		*row = 0;
		*col = 0;
		break;
	    }
	}

/*--Check this tile.  If it doesn't exist or isn't free then ignore it. */

	if (Board_Tiles[*row][*col].level == 0) { continue; }
	if (Tile_Not_Free( *row, *col )) { continue; }

/*--If moving Click1 then return now. */

	if (Click == 1) { break; }

/*--Continue the search if this tile does not match Click1. */

	tile1 =  Click1->tiles[Click1->level-1];
	tile2 = Board_Tiles[*row][*col].tiles[Board_Tiles[*row][*col].level-1];
	if (/* Do tile faces match for those types that must match exactly? */
	    ((tile1 > 8 || tile2 > 8) && tile1 != tile2) ||
	    /* Are both tiles seasons? */
	    (tile1 <= 4 && tile2 > 4) ||
	    /* Are both tiles flowers? */
	    (tile1 >= 5 && tile1 <= 8 && (tile2 < 5 || tile2 > 8))) {
	    /* They don't match. */
	    continue;
	}
	break;
    }
    DEBUG_RETURN(Next_Tile);

} /* Next_Tile */


void Hints(void)
/******************************************************************************
* If Click1 not present then search for the "first" remaining tile otherwise
* use Click1 as our current "base" tile.
* If Click1 present but not Click2 then search for any match for Click1.
* If Click2 not present either then search for the first remaining tile past 
* Click1 otherwise search for the first remaining tile past Click2.
* Keep searching for a new Click2 until we hit a matching tile or until we
* run out.  Exit on match with new tile as Click2.
* Advance Click1 and start a new search for Click2.  If we run out on Click1
* then remove Click1.
******************************************************************************/
{

/*--If we have a Click1 but no Click2 then search for a Click2. */

    if (Click1 != Board_Position_NULL &&
	Click2 == Board_Position_NULL) {
	One_Button_Hint = TRUE;
	Click2_Row = 0;
	Click2_Col = 0;
	for (;;) {
	    Next_Tile( 2, &Click2_Row, &Click2_Col );
	    if (Click2_Col == 0) {
		One_Button_Hint = FALSE;
		Hilite_Tile( Click1_Row, Click1_Col );
		Click1 = Board_Position_NULL;
		DEBUG_RETURN(Hints);
		return;
	    }
	    if (Click2_Row != Click1_Row ||
		Click2_Col != Click1_Col) {
		Click2 = &Board_Tiles[Click2_Row][Click2_Col];
		Hilite_Tile( Click2_Row, Click2_Col );
		DEBUG_RETURN(Hints);
		return;
	    }
	}
    }

/*--Find a Click1 to work with if we don't already have one. */

    DEBUG_CALL(Hints);
    if (Click1 == Board_Position_NULL) {
	Click1_Row = 0;
	Click1_Col = 0;
	Next_Tile( 1, &Click1_Row, &Click1_Col );
	if (Click1_Col == 0) { 
	    DEBUG_RETURN(Hints);
	    return;
	}
	Hilite_Tile( Click1_Row, Click1_Col );
	Click1 = &Board_Tiles[Click1_Row][Click1_Col];
    }

/*--Find our starting position for Click2 if we don't have one. */

    if (Click2 == Board_Position_NULL) {
	Click2_Row = Click1_Row;
	Click2_Col = Click1_Col;
    } else {
	Hilite_Tile( Click2_Row, Click2_Col );
	Click2 = Board_Position_NULL;
    }

/*--Loop until we get something. */

    for (;;) {
	Next_Tile( 2, &Click2_Row, &Click2_Col );
	if (Click2_Col != 0) {
	    if (Click2_Row != Click1_Row ||
		Click2_Col != Click1_Col) {
		Click2 = &Board_Tiles[Click2_Row][Click2_Col];
		Hilite_Tile( Click2_Row, Click2_Col );
		DEBUG_RETURN(Hints);
		return;
	    }
	} else {
	    Hilite_Tile( Click1_Row, Click1_Col );
	    Click1 = Board_Position_NULL;
	    if (One_Button_Hint) {
		One_Button_Hint = FALSE;
		return;
	    }
	    Next_Tile( 1, &Click1_Row, &Click1_Col );
	    if (Click1_Col == 0) {
		DEBUG_RETURN(Hints);
		return;
	    }
	    Hilite_Tile( Click1_Row, Click1_Col );
	    Click1 = &Board_Tiles[Click1_Row][Click1_Col];
	    Click2_Row = Click1_Row;
	    Click2_Col = Click1_Col;
	}
    }

} /* Hints */
