/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes	May 1989
* W Port: Jens Kilian			February 1996
*
* draw.c - Deals with the Mah-Jongg board.  Setup and drawing.
******************************************************************************/

#include <Wlib.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "proto.h"
#include "board.h"

/*--Index into this array using a tile number in order to get the procedure
 *  that knows how to draw the face of that tile. */

typedef void (*Draw_Xyz)(int x, int y);

static void Draw_Error(int x, int y)
{
	(void)fprintf(stderr, "Drew tile face 0 at %d,%d??\n", x, y);
	return;
}

static const Draw_Xyz	Faces[1+NFACES] = {
    Draw_Error,

    Draw_Spring,  Draw_Summer,  Draw_Fall,    Draw_Winter,

    Draw_Bamboo,  Draw_Mum,     Draw_Orchid,  Draw_Plum,

    Draw_GDragon, Draw_RDragon, Draw_WDragon,

    Draw_East,    Draw_West,    Draw_North,   Draw_South,

    Draw_Bam1,    Draw_Bam2,    Draw_Bam3,    Draw_Bam4,     Draw_Bam5,
    Draw_Bam6,    Draw_Bam7,    Draw_Bam8,    Draw_Bam9,

    Draw_Dot1,    Draw_Dot2,    Draw_Dot3,    Draw_Dot4,     Draw_Dot5,
    Draw_Dot6,    Draw_Dot7,    Draw_Dot8,    Draw_Dot9,

    Draw_Crak1,   Draw_Crak2,   Draw_Crak3,   Draw_Crak4,    Draw_Crak5,
    Draw_Crak6,   Draw_Crak7,   Draw_Crak8,   Draw_Crak9
};


void Hilite_Tile(int row, int col)
/******************************************************************************
*   row	- Specifies the row of the tile to hilite
*   col - specifies the column of the tile to hilite
*
* Called to hilite a tile face.
******************************************************************************/
{
    register Board_Position	bp = &Board_Tiles[row][col];
    Point	pnts[20];
    int		pnti = 0;
    int		x, y, w, h;
    int		left, bottom, left_bottom;

#define PNT(X,Y) \
    DEBUG_ERROR(pnti >= XtNumber(pnts),"HT pnts overflow!\n"); \
    pnts[pnti].x = X;     pnts[pnti].y = Y;  ++pnti
    ;

/*--See if we are one of the very special tiles on top. */

    DEBUG_CALL(Hilite_Tile);
    if (Board_Tiles[SPEC4].level > 0) {
	if (row == 3) {
	    if (col == 6) {
		x = bp->x + Side_X * 4 + 1;
		y = bp->y - Side_Y * 4 + 1;
		w = Tile_Width / 2;
		h = Tile_Height / 2;
		PNT( x,			y );
		PNT( Tile_Width,	0 );
		PNT( 0,			h-1 );
		PNT( -(w+1),		 0 );
		PNT( 0,			h+1 );
		PNT( -(w-1),		0 );
		PNT( 0,			-Tile_Height );
		goto Hilite;
	    } else if (col == 7) {
		x = bp->x + Side_X * 4 + 1;
		y = bp->y - Side_Y * 4 + 1;
		w = Board_Tiles[3][7].x - Board_Tiles[SPEC4].x + 3 * Side_X;
		h = Tile_Height / 2;
		PNT( x,			y );
		PNT( Tile_Width,	0 );
		PNT( 0,			Tile_Height );
		PNT( -w,		0 );
		PNT( 0,			-(h+1) );
		PNT( -(Tile_Width-w),	 0 );
		PNT( 0,			-(h-1) );
		goto Hilite;
	    }
	} else if (row == 4) {
	    if (col == 6) {
		x = bp->x + Side_X * 4 + 1;
		y = bp->y - Side_Y * 4 + 1;
		w = Tile_Width / 2;
		h = Tile_Height / 2;
		PNT( x,			y );
		PNT( w-1,		0 );
		PNT( 0,			h + Side_Y );
		PNT( w+1,		0 );
		PNT( 0,			h - Side_Y );
		PNT( -Tile_Width,	0 );
		PNT( 0,			-Tile_Height );
		goto Hilite;
	    } else if (col == 7) {
		x = bp->x + Side_X * 4 + 1;
		y = bp->y - Side_Y * 4 + 1;
		w = Board_Tiles[4][7].x - Board_Tiles[SPEC4].x + 3 * Side_X;
		h = Tile_Height / 2;
		PNT( x + Tile_Width - w,	y );
		PNT( w,				0 );
		PNT( 0,				Tile_Height );
		PNT( -Tile_Width,		0 );
		PNT( 0,				-(h - Side_Y) );
		PNT( Tile_Width - w,		0 );
		PNT( 0,				-(h + Side_Y) );
		goto Hilite;
	    }
	}
    }

/*--We are a normal tile that may be partially overlapped by some other
 *  normal tile. */

    x = bp->x + Side_X * bp->level + 1;
    y = bp->y - Side_Y * bp->level + 1;
    w = Tile_Width;
    h = Tile_Height;
    if (col > 0) {
	left = Board_Tiles[row][col-1].level - bp->level;
	if (left < 0) { left = 0; }
	if (row < 7) {
	    left_bottom = Board_Tiles[row+1][col-1].level - bp->level;
	    if (left_bottom < 0) { left_bottom = 0; }
	} else {
	    left_bottom = 0;
	}
    } else {
	left = 0;
	left_bottom = 0;
    }
    if (row < 7) {
	bottom = Board_Tiles[row+1][col].level - bp->level;
	if (bottom < 0) { bottom = 0; }
    } else {
	bottom = 0;
    }
    if (bottom > left_bottom && Tile_Width == 28) { left_bottom = bottom; }
    if (left > 0) {
	w = left * Side_X;
    } else {
	w = 0;
    }
    PNT( x + w, y );
    PNT( Tile_Width - w, 0 );
    if (bottom > 0) {
	h = bottom * Side_Y;
    } else {
	h = 0;
    }
    PNT( 0, Tile_Height - h );
    if (left_bottom <= left && left_bottom <= bottom) {
	PNT( -(Tile_Width - bottom*Side_X), 0 );
	if (left != bottom) {
	    PNT( (left-bottom)*Side_X, (bottom-left)*Side_Y );
	}
	PNT( 0, -(Tile_Height - h) );
    } else if (left_bottom <= left) {	/* left_bottom > bottom */
	PNT( -(Tile_Width - left_bottom*Side_X), 0 );
	if (left_bottom != left ) {
	    PNT( 0, (bottom-left_bottom)*Side_Y );
	    PNT( (left-left_bottom)*Side_X, (left_bottom-left)*Side_Y );
	    PNT( 0, -(Tile_Height - left * Side_Y) );
	} else {
	    PNT( 0, -(Tile_Height - h) );
	}
    } else if (left_bottom <= bottom) {	/* left_bottom > left */
	if (left_bottom == bottom) {
	    PNT( -(Tile_Width-w), 0 );
	    PNT( 0, -(Tile_Height-h) );
	} else {
	    PNT( -(Tile_Width - bottom * Side_X), 0 );
	    PNT( (left_bottom-bottom)*Side_X, (bottom-left_bottom)*Side_Y );
	    PNT( -left_bottom*Side_X, 0 );
	    PNT( 0, -(Tile_Height - left_bottom * Side_Y) );
	}
    } else {		/* left_bottom > bottom && left_bottom > left */
	PNT( -(Tile_Width - left_bottom * Side_X), 0 );
	PNT( 0, (bottom-left_bottom)*Side_Y );
	PNT( (left-left_bottom)*Side_X, 0 );
	PNT( 0, -(Tile_Height - left_bottom * Side_Y) );
    }

/*--Now do it. */

  Hilite :
    wsetmode(M_INVERS);
    wpolygon(pnts, pnti);

    DEBUG_RETURN(Hilite_Tile);

} /* Hilite_Tile */


static void Clear_Tile(Board_Position bp, int left, int bottom)
/******************************************************************************
*   bp	    - Specifies the Board_Position to draw
*   left    - Specifies the level of the tile on the left of this thile
*   bottom  - Specifies the level of the tile at the bottom of this tile
*
* We clear (make totally white) the space occupied by the image of this tile.
* We clear the face and the left and bottom sides.  Any shadowing caused by
* the last drawing of this tile is the responsibility of the caller.
******************************************************************************/
{
    Point	Poly[10];
    int		Polyi;

#undef PNT
#define PNT(XX,YY) \
    DEBUG_ERROR(Polyi >= XtNumber(Poly),"Tile: Poly overflow!!\n" ); \
    Poly[Polyi].x = (XX); \
    Poly[Polyi].y = (YY); \
    ++Polyi
    ;

/*--We will circle the tile outline clockwise. */

    DEBUG_CALL(Clear_Tile);
    Polyi = 0;

/*--Start with the upper left corner of the tile side. This is the "bottom"
 *  of that tile side if it has one. Leave x/y at the upper-left corner of the
 *  tile face. */

    if (left >= bp->level) {
	left = bp->level;
	PNT( bp->x + Side_X * bp->level, bp->y - Side_Y * bp->level );
    } else {
	PNT( bp->x + Side_X * left, bp->y - Side_Y * left );
	PNT( Side_X * (bp->level - left), - Side_Y * (bp->level - left) );
    }

/*--Cross the top and the right side of the tile. */

    PNT( Tile_Width + 1, 0 );
    PNT( 0, Tile_Height + 1 );

/*--Now do the bottom side of the tile. */

    if (bottom < bp->level) {
	PNT( - Side_X * (bp->level - bottom), Side_Y * (bp->level - bottom) );
    } else {
	bottom = bp->level;
    }
    PNT( -(Tile_Width + 1), 0 );

/*--Now go up the left side of the tile. */

    if (left != bottom) {
	PNT( Side_X * (left - bottom), - Side_Y * (left - bottom) );
    }
    PNT( 0, -(Tile_Height + 1) );

/*--Do the actual clearing. */

    wsetmode(M_CLEAR);
    wpolygon(Poly, Polyi);
    DEBUG_RETURN(Clear_Tile);

} /* Clear_Tile */


static void Tile(int row, int col)
/******************************************************************************
*   row		  - Specifies the tile to draw
*   col		  - Specifies the tile to draw
*
* Called to draw a tile.  We draw the face, the sides, and the shadow.
******************************************************************************/
{
    register Board_Position	bp= &Board_Tiles[row][col];
    Point			Poly[100];
    int				Polyi;
    int				left;
    int				bottom;
    int				curx;
    int				cury;
    int				sidex;
    int				sidey;
    int				i, j, k, l, m;

#undef PNT
#define PNT(XX,YY) \
    DEBUG_ERROR(Polyi >= XtNumber(Poly), "Tile: Poly overflow!!\n" ); \
    Poly[Polyi].x = (XX); \
    Poly[Polyi].y = (YY); \
    ++Polyi;

/*--This tile no longer needs drawing. */

    DEBUG_CALL(Tile);
    bp->draw = FALSE;

/*--Determine the level of the tile on the left of this tile. */

    if (col > 0) {
	if (col == SPEC1col && row == SPEC1row) {
	    left = Board_Tiles[SPEC2].level;
	} else if (col == SPEC2col && row == SPEC2row) {
	    if (Board_Tiles[3][12].level == 0 ||
		Board_Tiles[4][12].level == 0) {
		left = 0;
	    } else {
		left = 1;
	    }
	} else {
	    left = Board_Tiles[row][col-1].level;
	}
    } else {
	left = 0;
    }

/*--Determine the level of the tile at the bottom of this tile. */

    if (row < 7) {
	bottom = Board_Tiles[row+1][col].level;
    } else {
	bottom = 0;
    }

/*--Clear the area that will be covered by this tile. */

    Clear_Tile( bp, left, bottom );

/*--Draw the tile face. */

    (*(Faces[(int)(bp->tiles[bp->level-1])]))(bp->x + bp->level * Side_X + 1,
					      bp->y - bp->level * Side_Y + 1);

/*--Now draw the tile edges. */

    if (Tile_Control & BLACKSIDE) {

/*--We want black/gray sides. */

	wsetmode(M_DRAW);
	w_box(Board, bp->x + bp->level * Side_X,
		     bp->y - bp->level * Side_Y,
		     Tile_Width + 1, Tile_Height + 1 );

	if (left < bp->level) {
	    Polyi = 0;
	    PNT( bp->x + left * Side_X, bp->y - left * Side_Y );
	    PNT( (bp->level - left) * Side_X, (left - bp->level) * Side_Y );
	    PNT( 0, Tile_Height + 1 );
	    PNT( (left - bp->level) * Side_X, (bp->level - left) * Side_Y );
	    PNT( 0, -(Tile_Height + 1) );

	    wsetmode(M_DRAW);
	    if (Tile_Control & GRAYSIDE) {
	      wsetpattern(MAX_GRAYSCALES*4/9);
	      wdpolygon(Poly, Polyi);
	    } else {
	      wpolygon(Poly, Polyi);
	    }
	    wpolyline(Poly, Polyi);
	}
	if (bottom < bp->level) {
	    Polyi = 0;
	    PNT( bp->x + bp->level * Side_X,
		 bp->y - bp->level * Side_Y + Tile_Height + 1 );
	    PNT( Tile_Width + 1, 0 );
	    PNT( (bottom - bp->level) * Side_X, (bp->level - bottom) * Side_Y);
	    PNT( -(Tile_Width + 1), 0 );
	    PNT( (bp->level - bottom) * Side_X, (bottom - bp->level) * Side_Y);

	    wsetmode(M_DRAW);
	    if (Tile_Control & GRAYSIDE) {
	      wsetpattern(MAX_GRAYSCALES*4/9);
	      wdpolygon(Poly, Polyi);
	    } else {
	      wpolygon(Poly, Polyi);
	    }
	    wpolyline(Poly, Polyi);
	}

/*--We want line'ed sides. */

    } else {

	Polyi = 0;
	if (left >= bp->level) {
	    PNT( bp->x + Side_X * bp->level, bp->y - Side_Y * bp->level );
	} else {

/*--First we draw the left side.  We leave x/y at the bottom left corner of
 *  the tile face when we are done. */

#define LSEGS 7 /* keep this an odd number */

	    sidex = Side_X * (bp->level - left);
	    sidey = Side_Y * (bp->level - left);
	    j = sidex;
	    if (Tile_Width == 28 && bp->level - left == 1) {
		PNT( bp->x + Side_X * left, bp->y - Side_Y * left - sidey );
		PNT( 0, Tile_Height + 1 + sidey );
		k = 0;
	    } else {
		PNT( bp->x + Side_X * left, bp->y - Side_Y * left );
		PNT(0, Tile_Height + 1 );
		k = sidey;
	    }
	    PNT( sidex, -sidey );
	    i = Tile_Height / (LSEGS+1);
	    m = Tile_Height - i * (LSEGS+1);
	    for (l = LSEGS; l > 0; --l) {
		cury = -i;
		if (m > 0) { cury -= 1; --m; }
		PNT( 0, cury );
		PNT( -j, k );
		j = -j;
		k = -k;
	    }
	    PNT( 0, -i-1 );
	    PNT( sidex, k );
	}
	PNT( 0, Tile_Height + 1 );

/*--Draw the left edge of the tile and then draw the bottom side of the tile.
 *  We leave x/y at the bottom right corner of the tile face when we are done.
 */

#define RSEGS 6	/* keep this an even number */

	if (bottom < bp->level) {
	    sidex = Side_X * (bp->level - bottom);
	    sidey = Side_Y * (bp->level - bottom);
	    i = Tile_Width / (RSEGS+1);
	    m = Tile_Width - i * (RSEGS+1);
	    if (Tile_Width == 28 && bp->level - bottom == 1) {
		j = 0;
	    } else {
		j = sidex;
	    }
	    k = sidey;
	    for (l = RSEGS; l > 0; --l) {
		curx = i;
		if (m > 0) { curx += 1; --m; }
		PNT( curx, 0 );
		PNT( -j, k );
		j = -j;
		k = -k;
	    }
	    PNT( i+1, 0 );
	    PNT( -j, sidey );
	    PNT( -(Tile_Width + 1 + sidex - j), 0 );
	    PNT( sidex, -sidey );
	}
	PNT( Tile_Width + 1, 0 );

/*--Draw the right side. */

	PNT( 0, -(Tile_Height + 1) );

/*--Draw the top side. */

	PNT( -(Tile_Width + 1), 0 );

/*--Draw all of those edges. */

	if (Tile_Control & GRAYSIDE) {
	  wsetpattern(MAX_GRAYSCALES*4/9);
	  wdpolyline(Poly, Polyi);
	} else {
	  wsetmode(M_DRAW);
	  wpolyline(Poly, Polyi);
	}
    }

/*--Now draw the tile shadow. */

    if (Tile_Control & SHADOW) {
	int	top, right;
	Boolean	top_right;

/*--Determine the level of the tile on the right of this tile. */

	if (col == SPEC1col) {
	    if (row == SPEC2row) {
		right = Board_Tiles[SPEC1].level;
	    } else if (row == SPEC3row) {
		right = 0;
	    } else {
		right = 0;
	    }
	} else {
	    right = Board_Tiles[row][col+1].level;
	}

/*--Determine the level of the tile at the top of this tile. */

	if (row > 0) {
	    top = Board_Tiles[row-1][col].level;
	} else {
	    top = 0;
	}

/*--Do we have an upper-right tile? */

	if (row > 0 &&
	    Board_Tiles[row-1][col+1].level >= bp->level) {
	    top_right = TRUE;
	} else if (row == SPEC3row && col == SPEC3col &&
		   Board_Tiles[3][1].level > 0) {
	    top_right = TRUE;
	} else if (row == 4 && col == 12 &&
		   Board_Tiles[SPEC2].level > 0) {
	    top_right = TRUE;
	} else {
	    top_right = FALSE;
	}

/*--Draw the upper shadow if necessary. */

	if (top < bp->level) {
	    Polyi = 0;
	    PNT( bp->x + bp->level * Side_X - 1,
		 bp->y - bp->level * Side_Y );
	    PNT( Shadow_X, -Shadow_Y );
	    if (top_right) {
		i = Shadow_X;
	    } else {
		i = 0;
	    }
	    PNT( Tile_Width + 3 - i, 0 );
	    PNT( -(Shadow_X - i), Shadow_Y );
	    PNT( -(Tile_Width + 3), 0 );

	    wsetmode(M_TRANSP);
	    wsetpattern(GRAY_PATTERN);
	    wdpolygon(Poly, Polyi);
	}

/*--Now work on the right shadow.  It may need to be drawn in pieces. */

	Polyi = 0;

/*--If SPEC3 has both neighbors then don't draw the right shadow. */

	if (row == SPEC3row && col == SPEC3col) {
	    if (Board_Tiles[3][1].level > 0) {
		if (Board_Tiles[4][1].level > 0) {
		    right = bp->level;

/*--If SPEC3 has only the upper neighbor then draw just the lower shadow. */

		} else {
		    i = bp->y - Board_Tiles[3][1].y;
		    PNT( Board_Tiles[4][1].x + Side_X, Board_Tiles[4][1].y );
		    PNT( Shadow_X, 0 );
		    PNT( 0, i - Shadow_Y);
		    PNT( -Shadow_X, Shadow_Y );
		    PNT( 0, -i );
		    right = bp->level;
		}

/*--If SPEC3 has only the lower neighbor then draw just the upper shadow. */

	    } else if (Board_Tiles[4][1].level > 0) {
		i = Board_Tiles[4][1].y - bp->y;
		PNT( bp->x + bp->level * Side_X + Tile_Width + 1 + 1,
		     bp->y - bp->level * Side_Y );
		PNT( Shadow_X, -Shadow_Y );
		PNT( 0, i + Shadow_Y );
		PNT( -Shadow_X, 0 );
		PNT( 0, -i );
		right = bp->level;
	    }

/*--If SPEC2's upper neighbor is there then draw that tile's upper shadow. */

	} else if (row == 3 && col == 12 && Board_Tiles[SPEC2].level > 0) {
	    i = Board_Tiles[SPEC2].y - bp->y;
	    PNT( bp->x + bp->level * Side_X + Tile_Width + 1 + 1,
		 bp->y - bp->level * Side_Y );
	    PNT( Shadow_X, -Shadow_Y );
	    PNT( 0, i + Shadow_Y );
	    PNT( -Shadow_X, 0 );
	    PNT( 0, -i );
	    right = bp->level;

/*--If SPEC2's lower neighbor is there then draw that tile's lower shadow. */

	} else if (row == 4 && col == 12 && Board_Tiles[SPEC2].level > 0) {
	    i = bp->y - Board_Tiles[SPEC2].y;
	    PNT( Board_Tiles[SPEC2].x + Side_X,
		 Board_Tiles[SPEC2].y + Tile_Height + 1);
	    PNT( Shadow_X, 0 );
	    PNT( 0, i - Shadow_Y);
	    PNT( -Shadow_X, Shadow_Y );
	    PNT( 0, -i );
	    right = bp->level;
	}

/*--If required, draw a normal right shadow that may be truncated by an upper
 *  right neighbor. */

	if (right < bp->level) {
	    Polyi = 0;
	    if (top_right) {
		i = Shadow_Y;
	    } else {
		i = 0;
	    }
	    PNT( bp->x + bp->level * Side_X + Tile_Width + 1 + 1,
		 bp->y - bp->level * Side_Y );
	    PNT( Shadow_X, -(Shadow_Y-i) );
	    PNT( 0, Tile_Height + 1 - i );
	    PNT( -Shadow_X, Shadow_Y );
	    PNT( 0, -(Tile_Height + 1) );
	}

/*--Draw any right shadow that may have been requested. */

	if (Polyi > 0) {
	    wsetmode(M_TRANSP);
	    wsetpattern(GRAY_PATTERN);
	    wdpolygon(Poly, Polyi);
	}
    }

/*--Now check for hiliting. */

    if (Board_State != s_Sample) {
	if (Click1 == bp) {
	    Hilite_Tile( Click1_Row, Click1_Col );
	} else if (Click2 == bp) {
	    Hilite_Tile( Click2_Row, Click2_Col );
	}
    }
    DEBUG_RETURN(Tile);

} /* Tile */


void Draw_All_Tiles(void)
/******************************************************************************
* Draws all visible tiles.
******************************************************************************/
{
    int		i,j;

/*--Draw the rightmost special tiles. */

    DEBUG_CALL(Draw_All_Tiles);
    if (Board_Tiles[SPEC1].draw && Board_Tiles[SPEC1].level > 0) {
	Tile( SPEC1row, SPEC1col );
    }
    if (Board_Tiles[SPEC2].draw && Board_Tiles[SPEC2].level > 0) {
	Tile( SPEC2row, SPEC2col );
    }

/*--Draw the current game.  Draw the normally placed tiles. */

    for (i = 0; i <= 7; ++i) {
	for (j = 12; j >= 1; --j) {
	    if (Board_Tiles[i][j].draw && Board_Tiles[i][j].level > 0) {
		Tile( i, j );
	    }
	}
    }

/*--Now draw the other special tiles. */

    if (Board_Tiles[SPEC4].draw && Board_Tiles[SPEC4].level > 0) {
	Tile( SPEC4row, SPEC4col );
    }
    if (Board_Tiles[SPEC3].draw && Board_Tiles[SPEC3].level > 0) {
	Tile( SPEC3row, SPEC3col );
    }
    Draw_Score( Score,
	        (int)(Board_Tile0_X + 14 * (Tile_Width  + 1)),
	        (int)(Board_Tile0_Y +  8 * (Tile_Height + 1)) );
    DEBUG_RETURN(Draw_All_Tiles);

} /* Draw_All_Tiles */


static void Sample(int face, int x, int y)
/******************************************************************************
* Draw one sample tile.
******************************************************************************/
{
    w_box(Board, x, y, Tile_Width+2, Tile_Height+2);
    (*(Faces[face]))( x+1, y+1 );

} /* Sample */


static void Tile_Samples(void)
/******************************************************************************
* Called when we want to display all tiles as a sampler.
******************************************************************************/
{
    int		x = Board_Tile0_X + 2 * Tile_Width;
    int		y = Board_Tile0_Y;
    short	w, h;

    DEBUG_CALL(Tile_Samples);

/*--Clear the board. */

    w_querywinsize(Board, 0, &w, &h);
    wsetmode(M_CLEAR);
    w_pbox(Board, 0, Board_Tile0_Y - Side_Y - Shadow_Y,
	          w, h - (Board_Tile0_Y - Side_Y - Shadow_Y));

/*--Draw sample tiles. */

    wsetmode(M_DRAW);
    Draw_Text( "Flower", Board_Tile0_X, y );
    Draw_Text( "Flower", Board_Tile0_X+1, y );
    Sample(  5, (int)(x + (Tile_Width + 1)*0), y );/*Draw_Bamboo*/
    Sample(  6, (int)(x + (Tile_Width + 1)*1), y );/*Draw_Mum*/
    Sample(  7, (int)(x + (Tile_Width + 1)*2), y );/*Draw_Orchid*/
    Sample(  8, (int)(x + (Tile_Width + 1)*3), y );/*Draw_Plum*/

    y += (int)Tile_Height + 1;
    Draw_Text( "Season", Board_Tile0_X, y );
    Draw_Text( "Season", Board_Tile0_X+1, y );
    Sample(  1, (int)(x + (Tile_Width + 1)*0), y );/*Draw_Spring*/
    Sample(  2, (int)(x + (Tile_Width + 1)*1), y );/*Draw_Summer*/
    Sample(  3, (int)(x + (Tile_Width + 1)*2), y );/*Draw_Fall*/
    Sample(  4, (int)(x + (Tile_Width + 1)*3), y );/*Draw_Winter*/

    y += (int)Tile_Height + 1;
    Draw_Text( "Dragon", Board_Tile0_X, y );
    Draw_Text( "Dragon", Board_Tile0_X+1, y );
    Sample( 10, (int)(x + (Tile_Width + 1)*1), y );/*Draw_RDragon*/
    Sample( 11, (int)(x + (Tile_Width + 1)*2), y );/*Draw_WDragon*/
    Sample(  9, (int)(x + (Tile_Width + 1)*0), y );/*Draw_GDragon*/

    y += (int)Tile_Height + 1;
    Draw_Text( "Wind", Board_Tile0_X, y );
    Draw_Text( "Wind", Board_Tile0_X+1, y );
    Sample( 12, (int)(x + (Tile_Width + 1)*0), y );/*Draw_East*/
    Sample( 13, (int)(x + (Tile_Width + 1)*1), y );/*Draw_West*/
    Sample( 14, (int)(x + (Tile_Width + 1)*2), y );/*Draw_North*/
    Sample( 15, (int)(x + (Tile_Width + 1)*3), y );/*Draw_South*/

    y += (int)Tile_Height + 1;
    Draw_Text( "Bam", Board_Tile0_X, y );
    Draw_Text( "Bam", Board_Tile0_X+1, y );
    Sample( 16, (int)(x + (Tile_Width + 1)*0), y );/*Draw_Bam1*/
    Sample( 17, (int)(x + (Tile_Width + 1)*1), y );/*Draw_Bam2*/
    Sample( 18, (int)(x + (Tile_Width + 1)*2), y );/*Draw_Bam3*/
    Sample( 19, (int)(x + (Tile_Width + 1)*3), y );/*Draw_Bam4*/
    Sample( 20, (int)(x + (Tile_Width + 1)*4), y );/*Draw_Bam5*/
    Sample( 21, (int)(x + (Tile_Width + 1)*5), y );/*Draw_Bam6*/
    Sample( 22, (int)(x + (Tile_Width + 1)*6), y );/*Draw_Bam7*/
    Sample( 23, (int)(x + (Tile_Width + 1)*7), y );/*Draw_Bam8*/
    Sample( 24, (int)(x + (Tile_Width + 1)*8), y );/*Draw_Bam9*/

    y += (int)Tile_Height + 1;
    Draw_Text( "Dot", Board_Tile0_X, y );
    Draw_Text( "Dot", Board_Tile0_X+1, y );
    Sample( 25, (int)(x + (Tile_Width + 1)*0), y );/*Draw_Dot1*/
    Sample( 26, (int)(x + (Tile_Width + 1)*1), y );/*Draw_Dot2*/
    Sample( 27, (int)(x + (Tile_Width + 1)*2), y );/*Draw_Dot3*/
    Sample( 28, (int)(x + (Tile_Width + 1)*3), y );/*Draw_Dot4*/
    Sample( 29, (int)(x + (Tile_Width + 1)*4), y );/*Draw_Dot5*/
    Sample( 30, (int)(x + (Tile_Width + 1)*5), y );/*Draw_Dot6*/
    Sample( 31, (int)(x + (Tile_Width + 1)*6), y );/*Draw_Dot7*/
    Sample( 32, (int)(x + (Tile_Width + 1)*7), y );/*Draw_Dot8*/
    Sample( 33, (int)(x + (Tile_Width + 1)*8), y );/*Draw_Dot9*/

    y += (int)Tile_Height + 1;
    Draw_Text( "Crak", Board_Tile0_X, y );
    Draw_Text( "Crak", Board_Tile0_X+1, y );
    Sample( 34, (int)(x + (Tile_Width + 1)*0), y );/*Draw_Crak1*/
    Sample( 35, (int)(x + (Tile_Width + 1)*1), y );/*Draw_Crak2*/
    Sample( 36, (int)(x + (Tile_Width + 1)*2), y );/*Draw_Crak3*/
    Sample( 37, (int)(x + (Tile_Width + 1)*3), y );/*Draw_Crak4*/
    Sample( 38, (int)(x + (Tile_Width + 1)*4), y );/*Draw_Crak5*/
    Sample( 39, (int)(x + (Tile_Width + 1)*5), y );/*Draw_Crak6*/
    Sample( 40, (int)(x + (Tile_Width + 1)*6), y );/*Draw_Crak7*/
    Sample( 41, (int)(x + (Tile_Width + 1)*7), y );/*Draw_Crak8*/
    Sample( 42, (int)(x + (Tile_Width + 1)*8), y );/*Draw_Crak9*/

    w_flush();
    DEBUG_RETURN(Tile_Samples);

} /* Tile_Samples */


void Show_Samples(void)
/******************************************************************************
* Called when the Samples button is presses.  Display or un-display the sample
* tiles.
******************************************************************************/
{
    if (Board_State == s_Normal) {
	Board_State = s_Sample;
	Tile_Samples();
    } else {
	Board_State = s_Normal;
	Board_Expose();
    }

} /* Show_Samples */


void Board_Expose(void)
/******************************************************************************
* Called when the Board must be drawn.
******************************************************************************/
{
    int		i,j;
    short w, h;

    DEBUG_CALL(Board_Expose);

    w_querywinsize(Board, 0, &w, &h);
    wsetmode(M_CLEAR);
    w_pbox(Board, 0, Board_Tile0_Y - Side_Y - Shadow_Y,
	          w, h - (Board_Tile0_Y - Side_Y - Shadow_Y));

/*--Draw the correct stuff.  We might not want the current game. */

    if (Board_State == s_Sample) {
	Tile_Samples();
	return;
    }

/*--Draw the entire board. */

    for (i = 0; i < NROWS; ++i) {
	for (j = 0; j < NCOLS; ++j) {
	    if (Board_Tiles[i][j].level > 0) {
		Board_Tiles[i][j].draw = TRUE;
	    }
	}
    }
    Draw_All_Tiles();

/*--Make sure that it all goes out to the server. */

    w_flush();
    DEBUG_RETURN(Board_Expose);

} /* Board_Expose */


void Board_Configure(short width, short height)
/******************************************************************************
* Called when the Board is resized.
******************************************************************************/
{
    int		old_height = Tile_Height;

/*--Calculate the new Board size. */

    DEBUG_CALL(Board_Configure);
    Board_Width   = width;
    Board_Height  = height;
    Tile_Width    = (Board_Width-9) / 15 - 1;
    Tile_Height   = (Board_Height-9) / 10 - 1;

/*--Pick a tile size based upon the size of the board. */

#if N_BITMAP_SIZES > 4
    if (Tile_Width >= 80 && Tile_Height >= 96) {
	Tile_Width  = 80;
	Tile_Height = 96;
	Configure_Tiles( 5 );
    } else
#endif
#if N_BITMAP_SIZES > 3
    if (Tile_Width >= 68 && Tile_Height >= 80) {
	Tile_Width  = 68;
	Tile_Height = 80;
	Configure_Tiles( 4 );
    } else
#endif
#if N_BITMAP_SIZES > 2
    if (Tile_Width >= 56 && Tile_Height >= 64) {
	Tile_Width  = 56;
	Tile_Height = 64;
	Configure_Tiles( 3 );
    } else
#endif
#if N_BITMAP_SIZES > 1
    if (Tile_Width >= 40 && Tile_Height >= 48) {
	Tile_Width  = 40;
	Tile_Height = 48;
	Configure_Tiles( 2 );
    } else
#endif
    {
	Tile_Width  = 28;
	Tile_Height = 32;
	Configure_Tiles( 1 );
    }

/*--Figure the real 0,0 coordinate. */

    Board_Tile0_X = 4;
    Board_Tile0_Y = 4 + 2 * Tile_Height;

/*--Figure the Shadow and Side sizes. */

    Shadow_X = Tile_Width  / 8;
    Shadow_Y = Tile_Height / 8;
    Side_X   = (Tile_Width  / 10) & ~1;
    Side_Y   = (Tile_Height / 10) & ~1;

/*--See if we need to repaint. */

    if (old_height != Tile_Height) {
	Do_Button_Configuration();
	Set_Tile_Controls();
	Board_Expose();
    }
    DEBUG_RETURN(Board_Configure);

} /* Board_Configure */


void Board_Setup(void)
/******************************************************************************
* Called to set up and create the Board window.
******************************************************************************/
{
    short x, y, w, h;

/*--Get the geometry to use. */

    scan_geometry(Dragon_Resources.Geometry, &w, &h, &x, &y);
    if (w == UNDEF) w = 450;
    if (h == UNDEF) h = 340;

/*--Now actually create the board. */

    if ((Board = w_create(w, h, (W_MOVE|W_TITLE|W_CLOSE|W_ICON|EV_MOUSE)))
	== 0) {
	fprintf(stderr, "Can't create window.\n");
	exit(1);
    }

    w_settitle(Board, " Dragon ");

    if (!Dragon_Resources.Iconic) {
      /* Immediately show the board. */

      w_open(Board, x, y);
    }

/*--Give the tiles a default initial size. */

    {	short width, height;
	w_querywinsize(Board, 0, &width, &height);
	Board_Configure(width, height);
    }

/*--Give the buttons a default initial size based upon the Tile sizes. */

    Do_Button_Configuration();

/*--Set up the random number generator. */

    srand(time(0));

/*--Set up the initial game. */

    Setup_New_Game();

    DEBUG_RETURN(Board_Setup);

} /* Board_Setup */
