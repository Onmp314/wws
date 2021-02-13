/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes		March 1989
* W Port: Jens Kilian			February 1996
*
* tile.c - Deals with the individual Mah-Jongg tiles.
******************************************************************************/

#include <stdlib.h>
#include <Wlib.h>
#include "main.h"
#include "proto.h"

/*--SEASONS */

extern BITMAP	spring;
extern BITMAP	summer;
extern BITMAP	fall;
extern BITMAP	winter;

/*--FLOWERS */

extern BITMAP	bamboo;
extern BITMAP	mum;
extern BITMAP	orchid;
extern BITMAP	plum;

/*--DRAGONS */

extern BITMAP	gdragon;
extern BITMAP	rdragon;
extern BITMAP	wdragon;

/*--WINDS */

extern BITMAP	east;
extern BITMAP	west;
extern BITMAP	north;
extern BITMAP	south;

/*--NUMBERS */

extern BITMAP	one;
extern BITMAP	two;
extern BITMAP	three;
extern BITMAP	four;
extern BITMAP	five;
extern BITMAP	six;
extern BITMAP	seven;
extern BITMAP	eight;
extern BITMAP	nine;

/*--OTHER */

extern BITMAP	bam;
extern BITMAP	crak;
extern BITMAP	dot;

extern BITMAP_Init Sizes[N_BITMAP_SIZES][28];

static int		Tile_X11		= 28 / 2;				/* one column */
static int		Tile_X21		= 28 / 3;				/* two columns */
static int		Tile_X22		= 28 * 2 / 3;
static int		Tile_X31		= 28 / 4;				/* three columns */
static int		Tile_X32		= 28 * 2 / 4;
static int		Tile_X33		= 28 * 3 / 4;

static int		Tile_Y11		= 32 / 2;				/* one row */
static int		Tile_Y21		= 32 / 3;				/* two rows */
static int		Tile_Y22		= 32 * 2 / 3;
static int		Tile_Y31		= 32 / 5;				/* three rows */
static int		Tile_Y32		= 32 * 2 / 5;
static int		Tile_Y33		= 32 * 3 / 5;

static int		Number_X		= 28 - 7;
static int		Number_Y		= 0;
static int		SubTile_X		= 7 / 2;
static int		SubTile_Y		= 8 / 2;


static void Setup_Image(BITMAP *image, uchar *bits, int width, int height)
/******************************************************************************
*	image		- Specifies the BITMAP object to initialize
*	bits		- Specifies the image data
*	width		- Specifies the width of the image
*	height		- Specifies the height of the image
*
* Called to init a BITMAP so that it points at a particular bitmap image.
******************************************************************************/
{
	image->width = width;
	image->height = height;
	image->type = BM_PACKEDMONO;
	image->unitsize = 4;
	image->upl = (width + 31) / 32;
	image->planes = 1;
	image->data = bits;
} /* Setup_Image */


void Configure_Tiles(int size)
/******************************************************************************
*	size		- Specifies the tile size to use; 1..5
*
* Called when the Board receives a ConfigureNotify event.  We check to see if
* the size of the board/tiles have changed.  If so then we reconfigure the
* images of the tiles.
******************************************************************************/
{
	BITMAP_Init *table;
	int 		 w;
	int 		 h;

/*--Make sure that our caller is rational. */

	if (size < 1 || size > N_BITMAP_SIZES) {
		(void)fprintf( stderr,
					   "Bad size given to Configure_Tiles in tile.c [%d].\n",
					   size );
		size = 1;
	}

/*--Set up all of the various images. */

	for (table = &Sizes[size-1][0]; table->image != (BITMAP*)NULL; ++table) {
		Setup_Image( table->image, table->bits, table->width, table->height );
	}

/*--Set up the sub-tile positions. */

	w = spring.width;
	h = spring.height;

	Tile_X11	= w / 2;				/* one column */
	Tile_X21	= w / 3;				/* two columns */
	Tile_X22	= w - Tile_X21;
	Tile_X31	= w / 4;				/* three columns */
	while (Tile_X31 + bam.width/2 + bam.width*2 >= spring.width) {
		--Tile_X31;
	}
	Tile_X32	= Tile_X31 + bam.width;
	Tile_X33	= Tile_X32 + bam.width;

	Tile_Y11	= h / 2;				/* one row */
	Tile_Y21	= h / 3;				/* two rows */
	Tile_Y22	= h - Tile_Y21;
	Tile_Y31	= h * 2 / 5;			/* three rows */
	while (Tile_Y31 + bam.height/2 + bam.height*2 >= spring.height) {
		--Tile_Y31;
	}
	Tile_Y32	= Tile_Y31 + bam.height;
	Tile_Y33	= Tile_Y32 + bam.height;

	Number_X	= w - one.width - 1;
	Number_Y	= 1;
	SubTile_X	= bam.width  / 2;
	SubTile_Y	= bam.height / 2;

} /* Configure_Tiles */


static void Draw_Image(BITMAP *image, int x, int y)
/******************************************************************************
* Do a w_putblock on the image.
******************************************************************************/
{
	wsetmode(M_DRAW);
	wputblock(image, x, y);
} /* Draw_Image */


static void Draw_Blank(int x, int y)
/******************************************************************************
* Draw an empty tile; our caller will fill it in with little images.
******************************************************************************/
{
	wsetmode(M_CLEAR);
	w_pbox(Board, x, y, spring.width, spring.height);
} /* Draw_Blank */


static void Draw_Number(BITMAP *image, int x, int y)
/******************************************************************************
* Called to draw the number in the upper right corner of a numbered tile.
******************************************************************************/
{
	wsetmode(M_DRAW);
	wputblock(image, x + Number_X, y + Number_Y);
} /* Draw_Number */


void Draw_Spring(int x, int y)
{
	Draw_Image( &spring, x, y );
} /* Draw_Spring */

void Draw_Summer(int x, int y)
{
	Draw_Image( &summer, x, y );
} /* Draw_Summer */

void Draw_Fall(int x, int y)
{
	Draw_Image( &fall, x, y );
} /* Draw_Fall */

void Draw_Winter(int x, int y)
{
	Draw_Image( &winter, x, y );
} /* Draw_Winter */


void Draw_Bamboo(int x, int y)
{
	Draw_Image( &bamboo, x, y );
} /* Draw_Bamboo */

void Draw_Mum(int x, int y)
{
	Draw_Image( &mum, x, y );
} /* Draw_Mum */

void Draw_Orchid(int x, int y)
{
	Draw_Image( &orchid, x, y );
} /* Draw_Orchid */

void Draw_Plum(int x, int y)
{
	Draw_Image( &plum, x, y );
} /* Draw_Plum */


void Draw_GDragon(int x, int y)
{
	Draw_Image( &gdragon, x, y );
} /* Draw_GDragon */

void Draw_RDragon(int x, int y)
{
	Draw_Image( &rdragon, x, y );
} /* Draw_RDragon */

void Draw_WDragon(int x, int y)
{
	Draw_Image( &wdragon, x, y );
} /* Draw_WDragon */


void Draw_East(int x, int y)
{
	Draw_Image( &east, x, y );
} /* Draw_East */

void Draw_West(int x, int y)
{
	Draw_Image( &west, x, y );
} /* Draw_West */

void Draw_North(int x, int y)
{
	Draw_Image( &north, x, y );
} /* Draw_North */

void Draw_South(int x, int y)
{
	Draw_Image( &south, x, y );
} /* Draw_South */


void Draw_Bam1(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x,  y);

	Draw_Image( &bam, bx + Tile_X11, by + Tile_Y11 );

	Draw_Number( &one, x, y );

} /* Draw_Bam1 */

void Draw_Bam2(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X11, by + Tile_Y21 );
	Draw_Image( &bam, bx + Tile_X11, by + Tile_Y22 );

	Draw_Number( &two, x, y );

} /* Draw_Bam2 */

void Draw_Bam3(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X11, by + Tile_Y21 );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y22 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y22 );

	Draw_Number( &three, x, y );

} /* Draw_Bam3 */

void Draw_Bam4(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y21 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y21 );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y22 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y22 );

	Draw_Number( &four, x, y );

} /* Draw_Bam4 */

void Draw_Bam5(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X11, by + Tile_Y31 );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y32 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y32 );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y33 );

	Draw_Number( &five, x, y );

} /* Draw_Bam5 */

void Draw_Bam6(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y31 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y31 );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y32 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y32 );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y33 );

	Draw_Number( &six, x, y );

} /* Draw_Bam6 */

void Draw_Bam7(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y31 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y31 );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y32 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y32 );

	Draw_Image( &bam, bx + Tile_X31, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X32, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X33, by + Tile_Y33 );

	Draw_Number( &seven, x, y );

} /* Draw_Bam7 */

void Draw_Bam8(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X21, by + Tile_Y31 );
	Draw_Image( &bam, bx + Tile_X22, by + Tile_Y31 );

	Draw_Image( &bam, bx + Tile_X31, by + Tile_Y32 );
	Draw_Image( &bam, bx + Tile_X32, by + Tile_Y32 );
	Draw_Image( &bam, bx + Tile_X33, by + Tile_Y32 );

	Draw_Image( &bam, bx + Tile_X31, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X32, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X33, by + Tile_Y33 );

	Draw_Number( &eight, x, y );

} /* Draw_Bam8 */

void Draw_Bam9(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &bam, bx + Tile_X31, by + Tile_Y31 );
	Draw_Image( &bam, bx + Tile_X32, by + Tile_Y31 );
	Draw_Image( &bam, bx + Tile_X33, by + Tile_Y31 );

	Draw_Image( &bam, bx + Tile_X31, by + Tile_Y32 );
	Draw_Image( &bam, bx + Tile_X32, by + Tile_Y32 );
	Draw_Image( &bam, bx + Tile_X33, by + Tile_Y32 );

	Draw_Image( &bam, bx + Tile_X31, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X32, by + Tile_Y33 );
	Draw_Image( &bam, bx + Tile_X33, by + Tile_Y33 );

	Draw_Number( &nine, x, y );

} /* Draw_Bam9 */


void Draw_Dot1(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X11, by + Tile_Y11 );

	Draw_Number( &one, x, y );

} /* Draw_Dot1 */

void Draw_Dot2(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X11, by + Tile_Y21 );
	Draw_Image( &dot, bx + Tile_X11, by + Tile_Y22 );

	Draw_Number( &two, x, y );

} /* Draw_Dot2 */

void Draw_Dot3(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X11, by + Tile_Y21 );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y22 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y22 );

	Draw_Number( &three, x, y );

} /* Draw_Dot3 */

void Draw_Dot4(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y21 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y21 );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y22 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y22 );

	Draw_Number( &four, x, y );

} /* Draw_Dot4 */

void Draw_Dot5(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X11, by + Tile_Y31 );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y32 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y32 );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y33 );

	Draw_Number( &five, x, y );

} /* Draw_Dot5 */

void Draw_Dot6(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y31 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y31 );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y32 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y32 );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y33 );

	Draw_Number( &six, x, y );

} /* Draw_Dot6 */

void Draw_Dot7(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y31 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y31 );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y32 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y32 );

	Draw_Image( &dot, bx + Tile_X31, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X32, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X33, by + Tile_Y33 );

	Draw_Number( &seven, x, y );

} /* Draw_Dot7 */

void Draw_Dot8(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X21, by + Tile_Y31 );
	Draw_Image( &dot, bx + Tile_X22, by + Tile_Y31 );

	Draw_Image( &dot, bx + Tile_X31, by + Tile_Y32 );
	Draw_Image( &dot, bx + Tile_X32, by + Tile_Y32 );
	Draw_Image( &dot, bx + Tile_X33, by + Tile_Y32 );

	Draw_Image( &dot, bx + Tile_X31, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X32, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X33, by + Tile_Y33 );

	Draw_Number( &eight, x, y );

} /* Draw_Dot8 */

void Draw_Dot9(int x, int y)
{
	int 		bx = x - SubTile_X;
	int 		by = y - SubTile_Y;

	Draw_Blank( x, y );

	Draw_Image( &dot, bx + Tile_X31, by + Tile_Y31 );
	Draw_Image( &dot, bx + Tile_X32, by + Tile_Y31 );
	Draw_Image( &dot, bx + Tile_X33, by + Tile_Y31 );

	Draw_Image( &dot, bx + Tile_X31, by + Tile_Y32 );
	Draw_Image( &dot, bx + Tile_X32, by + Tile_Y32 );
	Draw_Image( &dot, bx + Tile_X33, by + Tile_Y32 );

	Draw_Image( &dot, bx + Tile_X31, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X32, by + Tile_Y33 );
	Draw_Image( &dot, bx + Tile_X33, by + Tile_Y33 );

	Draw_Number( &nine, x, y );

} /* Draw_Dot9 */


void Draw_Crak1(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &one, x, y );
} /* Draw_Crak1 */

void Draw_Crak2(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &two, x, y );
} /* Draw_Crak2 */

void Draw_Crak3(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &three, x, y );
} /* Draw_Crak3 */

void Draw_Crak4(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &four, x, y );
} /* Draw_Crak4 */

void Draw_Crak5(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &five, x, y );
} /* Draw_Crak5 */

void Draw_Crak6(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &six, x, y );
} /* Draw_Crak6 */

void Draw_Crak7(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &seven, x, y );
} /* Draw_Crak7 */

void Draw_Crak8(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &eight, x, y );
} /* Draw_Crak8 */

void Draw_Crak9(int x, int y)
{
	Draw_Image( &crak, x, y );
	Draw_Number( &nine, x, y );
} /* Draw_Crak9 */

