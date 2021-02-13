/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes	March 1989
* W Port: Jens Kilian		February 1996
*
* button.c - Deals with the command buttons on the board
******************************************************************************
*  4/24/89 GEB 	- When "Other" pressed, if "Samples" is on then turn it off
*		-  before changing button menus.
******************************************************************************/

#include <string.h>
#include <Wlib.h>
#include "main.h"
#include "proto.h"
#include "board.h"


typedef void (*ActionProc)(void);

static void New_Game(void);
static void Save_Game(void);
static void Other(void);
static void Restore_Game(void);
static void Game_Exit(void);
static void Redraw_All(void);
static void Sides(void);
static void Other(void);
static void Cheat(void);

typedef struct _Button_Rec {
    const char	*text;
    ActionProc	 action;
    Boolean	toggle;
    Boolean	hilite;
    int		 x;
    int		 y;
    int		 text_x;
    int		 text_y;
    Point	 border[6*4+4+1];
} Button_Rec, *Button;

#define Button_NULL	((Button)0)

#define NBUTTONS	8


static Button_Rec	Buttons1[NBUTTONS] = {
    { "New Game",   New_Game,	  FALSE   },
    { "Restart",    Restart_Game, FALSE  },
    { "Save",	    Save_Game,    FALSE   },
#define OTHER_BUTTON 3
    { "Other",	    Other,	  TRUE    },

    { "Hint",	    Hints,	  FALSE   },
    { NULL },
    { "Restore",    Restore_Game, FALSE  },
    { "Quit",	    Game_Exit,    FALSE },
};

Button_Rec	Buttons2[NBUTTONS] = {
    { "Redraw",	    Redraw_All,	  FALSE   },
    { "Sides",	    Sides,	  FALSE   },
#define SAMPLES_BUTTON 2
    { "Samples",    Show_Samples, TRUE    },
    { "Other",	    Other,	  TRUE    },

    { "Cheat",	    Cheat,	  TRUE    },
    { NULL },
    { NULL },
    { NULL },
};

static Button	All_Buttons	= &Buttons1[0];

static int	Button_Width  = 0;
static int	Button_Height = 0;

static int	Letter_Indent = 0;
static int	Letter_Height = 0;
static int	Letter_Width  = 0;

static WFONT	*wfont = 0;


void Complain(const char *about)
/******************************************************************************
* Pop up a dialog box and complain to the user.
******************************************************************************/
{
    WWIN *complaint;
    short w1, h1, w2, h2;

    if (!wfont) {
	/* Load font the first time through. */

	if ((wfont = w_loadfont(Dragon_Resources.Font, Dragon_Resources.Size, Dragon_Resources.Styles)) == 0) {
	    fprintf(stderr, "Can't load complainer font '%s'.\n",
		    Dragon_Resources.Font);
	    return;
	}
    }

    /* Create window. */

    if ((complaint = w_create(w_strlen(wfont, about), wfont->height,
			      (W_MOVE|EV_MOUSE))) == 0) {
	fprintf(stderr, "Can't create complainer window.\n");
	return;
    }

    /* Pop up complaint. */

    w_querywinsize(WROOT, 0, &w1, &h1);
    w_querywinsize(complaint, 0, &w2, &h2);
    if (w_open(complaint, (w1-w2)/2, (h1-h2)/2) < 0) {
	fprintf(stderr, "Can't open complainer window.\n");
	w_delete(complaint);
	return;
    }

    w_setmode(complaint, M_DRAW);
    w_setfont(complaint, wfont);
    w_printstring(complaint, 0, 0, about);
    w_flush();

    /* Complaint will be removed (and the window deleted) in the main loop. */

} /* Complain */


int	Cheating = FALSE;

static void Cheat(void)
{

    DEBUG_CALL(Cheat);
    Cheating = !Cheating;
    DEBUG_RETURN(Cheat);

} /* Cheat */


static void Game_Exit(void)
/******************************************************************************
* Called when we want to exit.
******************************************************************************/
{
    DEBUG_CALL(Game_Exit);
    w_exit();
    exit(0);

} /* Game_Exit */


static void New_Game(void)
{

    DEBUG_CALL(New_Game);
    Setup_New_Game();
    Board_Expose();
    DEBUG_RETURN(New_Game);

} /* New_Game */


static void Other(void)
/******************************************************************************
* Called to change button menus.
******************************************************************************/
{

    DEBUG_CALL(Other);
    if (All_Buttons == &Buttons1[0]) {
	All_Buttons = &Buttons2[0];
    } else {
	/*--Turn off Samples mode if we are changing menus. */
	if (Buttons2[SAMPLES_BUTTON].hilite) {
	    (*(Buttons2[SAMPLES_BUTTON].action))();
	    Buttons2[SAMPLES_BUTTON].hilite = FALSE;
	}
	All_Buttons = &Buttons1[0];
    }
    All_Buttons[OTHER_BUTTON].hilite = !All_Buttons[OTHER_BUTTON].hilite;
    Button_Expose();
    DEBUG_RETURN(Other);

} /* Other */


static void Redraw_All(void)
{
    DEBUG_CALL(Redraw_All);
    Board_Expose();
    DEBUG_RETURN(Redraw_All);

} /* Redraw_All */


static void Restore_Game(void)
/******************************************************************************
* Called to restore a previous game.
******************************************************************************/
{
    extern char *getenv();
    char	 name[1024];
    FILE	*file;

    DEBUG_CALL(Restore_Game);
    (void)strcpy( &name[0], getenv( "HOME" ) );
    (void)strcat( &name[0], "/.dragon-save" );
    file = fopen( &name[0], "r" );
    if (file == (FILE*)NULL) {
	Complain( "Cannot open the $HOME/.dragon-save file." );
    } else {
	Read_Game( file );
	(void)fclose( file );
	Redraw_All();
    }
    DEBUG_RETURN(Restore_Game);

} /* Restore_Game */


static void Save_Game(void)
/******************************************************************************
* Called to save the current game.
******************************************************************************/
{
    extern char *getenv();
    char	 name[1024];
    FILE	*file;

    DEBUG_CALL(Save_Game);
    (void)strcpy( &name[0], getenv( "HOME" ) );
    (void)strcat( &name[0], "/.dragon-save" );
    file = fopen( &name[0], "w" );
    if (file == (FILE*)NULL) {
	Complain( "Cannot open the $HOME/.dragon-save file." );
    } else {
	Write_Game( file );
	(void)fclose( file );
    }
    DEBUG_RETURN(Save_Game);

} /* Save_Game */


static void Sides(void)
{
    DEBUG_CALL(Sides);

    if (Tile_Control & SHADOW) {
	if (Tile_Control & BLACKSIDE) {
	    if (Tile_Control & GRAYSIDE) {
		Tile_Control &= ~(SHADOW | BLACKSIDE | GRAYSIDE);
	    } else {
		Tile_Control &= ~(SHADOW | BLACKSIDE);
		Tile_Control |= GRAYSIDE;
	    }
	} else {
	    Tile_Control &= ~SHADOW;
	    Tile_Control |= BLACKSIDE;
	}
    } else {
	Tile_Control |= SHADOW;
    }
    Board_Expose();

    DEBUG_RETURN(Sides);

} /* Sides */


void Do_Button_Configuration(void)
/******************************************************************************
* Called when the Board changes Tile sizes.
******************************************************************************/
{
#define BUTTONS_PER_LINE 4
#undef PNT
#define PNT(X,Y)	pnt->x = X; pnt->y = Y; ++pnt;
    int		 indent;
    int		 x, dx;
    int		 y, dy;
    int		 i, j;
    int		 f, s, t, l;
    Point	*pnt;
    register Button	but;

/*--Buttons will be three tiles wide and 2/3'rd of a tile high. */

    DEBUG_CALL(Do_Button_Configuration);
    Button_Width = 3 * Tile_Width;
    dx = Button_Width + Tile_Width / 2;
    Button_Height = 5 * Tile_Height / 9;
    dy = Tile_Height * 8 / 10;

/*--Letters are as large as can fit within the buttons. */

    indent = Button_Height / 5;
    Letter_Indent = indent + 2;
    s = Letter_Indent * 3 / 4;
    t = s / 2;
    f = indent - s + t;
    Letter_Height = Button_Height - (2 * indent) - 2;
    if ((Letter_Height & 1) == 0) { --Letter_Height; }
    Letter_Width  = (Button_Width - (2 * indent) - 2) / 8 - 2;
    if ((Letter_Width & 1) == 0) { --Letter_Width; }
    
/*--Now place the buttons. */

    for (j = 1; j >= 0; --j) {
	if (j == 1) {
	    but = &Buttons1[0];
	} else {
	    but = &Buttons2[0];
	}
	x = indent + 12;
	y = indent + 4;
	for (i = 0; i < NBUTTONS; ++i) {
	    but[i].x = x;
	    but[i].y = y;

	    if (but[i].text == NULL) { goto Next_Button; }
	    l = strlen(but[i].text);
	    if (l > 8) {
		(void)fprintf( stderr, "Button name too long: %s\n",
			       but[i].text );
		l = 8;
	    }
	    but[i].text_x = (Button_Width - l * (Letter_Width+2) + 2) / 2;
	    but[i].text_y = indent + 1;

	    pnt = &but[i].border[0];
	    PNT( x,				y + indent		);
	    PNT( f,				0			);
	    PNT( 0,				-s			);
	    PNT( -t,				0			);
	    PNT( 0,				t			);
	    PNT( s,				0			);
	    PNT( 0,				-f			);
	    PNT( Button_Width - 2 * indent,	0			);
	    PNT( 0,				f			);
	    PNT( s,				0			);
	    PNT( 0,				-t			);
	    PNT( -t,				0			);
	    PNT( 0,				s			);
	    PNT( f,				0			);
	    PNT( 0,				Button_Height - 2*indent);
	    PNT( -f,				0			);
	    PNT( 0,				s			);
	    PNT( t,				0			);
	    PNT( 0,				-t			);
	    PNT( -s,				0			);
	    PNT( 0,				f			);
	    PNT( -(Button_Width - 2*indent),	0			);
	    PNT( 0,				-f			);
	    PNT( -s,				0			);
	    PNT( 0,				t			);
	    PNT( t,				0			);
	    PNT( 0,				-s			);
	    PNT( -f,				0			);
	    PNT( 0,				-Button_Height+2*indent	);

	  Next_Button :
	    if (i % BUTTONS_PER_LINE + 1 == BUTTONS_PER_LINE) {
		x = indent + 12;
		y += dy;
	    } else {
		x += dx;
	    }
	}
    }
    DEBUG_RETURN(Do_Button_Configuration);

} /* Do_Button_Configuration */


void Draw_Text(const char *str, int x, int y)
/******************************************************************************
* Called to draw the vector text in some button.
******************************************************************************/
{
#ifdef WANTDEBUG
    static char *pntserr = "DT pnts overflow!\n";
#endif
    Point	pnts[50];
    int		pnti;
    int		h1;
    int		w1;

#undef PNT
#define PNT(X,Y) \
    DEBUG_ERROR( pnti >= Number(pnts), pntserr ); \
    pnts[pnti].x = X; pnts[pnti].y = Y; ++pnti
    ;

/*--Position ourselves for the first letter. */

    DEBUG_CALL(Draw_Text);
    h1 = Letter_Height - 1;
    w1 = Letter_Width - 1;

/*--Loop over all letters in the text. */

    for ( ; *str != '\000'; x+=Letter_Width+2,++str) {
	pnti = 0;
	switch (*str) {

/*--The letters we have. */

	  case '0' :
	    {	int	w6, h6, h26;

		h6 = h1 / 6;
		h26 = h1 - 4 * h6;
		w6 = w1 / 6;

		PNT( x,		y+h6+h6	);
		PNT( 0,		h26	);
		PNT( w6,	h6	);
		PNT( w6,	h6	);
		PNT( w1-4*w6,	0	);
		PNT( w6,	-h6	);
		PNT( w6,	-h6	);
		PNT( 0,		-h26	);
		PNT( -w6,	-h6	);
		PNT( -w6,	-h6	);
		PNT( -(w1-4*w6),0	);
		PNT( -w6,	h6	);
		PNT( -w6,	h6	);
		break;
	    }

	  case '1' :
	    {
		PNT( x+(w1+1)/2,y	);
		PNT( 0,		h1	);
		break;
	    }

	  case '2' :
	    {	int	w6, w46, h6, h6r;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h6r = (h1 - h6 * 6) / 2;

		PNT( x,		y + h6	);
		PNT( w6,	-h6	);
		PNT( w46,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h6+h6r	);
		PNT( -w6,	h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h1-5*h6-h6r);
		PNT( 0,		h6	);
		PNT( w1,	0	);
		break;
	    }

	  case '3' :
	    {	int	w6, w46, h6, h6r;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h6r = (h1 - h6 * 6) / 2;

		PNT( x,		y + h6	);
		PNT( w6,	-h6	);
		PNT( w46,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h6+h6r	);
		PNT( -w6,	h6	);
		PNT( -w46/2,	0	);
		PNT( w46/2,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h1-5*h6-h6r);
		PNT( -w6,	h6	);
		PNT( -w46,	0	);
		PNT( -w6,	-h6	);
		break;
	    }

	  case '4' :
	    {	int	w23, h23;

		w23 = w1 * 2 / 3;
		h23 = h1 * 2 / 3;

		PNT( x + w1,	y + h23	);
		PNT( -w1,	0	);
		PNT( w23,	-h23	);
		PNT( 0,		h1	);
		break;
	    }

	  case '5' :
	    {	int	w6, w46, h6, h6r;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h6r = (h1 - h6 * 6) / 2;

		PNT( x + w1,	y	);
		PNT( -w1,	0	);
		PNT( 0,		h1-3*h6-h6r);
		PNT( w1-w6,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h6+h6r	);
		PNT( -w6,	h6	);
		PNT( -w46,	0	);
		PNT( -w6,	-h6	);
		break;
	    }

	  case '6' :
	    {	int	w6, w46, h6, h6r;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		if (((h6-w6) & 1) == 1) { --h6; }
		h6r = (h1 - h6 * 6) / 2;

		PNT( x + w1,	y+h6	);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h1-2*h6);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	-h6	);
		PNT( 0,		-h6-h6r );
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		break;
	    }

	  case '7' :
	    {
		PNT( x,		y	);
		PNT( w1,	0	);
		PNT( -w1,	h1	);
		break;
	    }

	  case '8' :
	    {	int	w6, w46, h6, h6r;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		if (((h6-w6) & 1) == 1) { --h6; }
		h6r = (h1 - h6 * 6) / 2;

		PNT( x + w1,	y + h6	);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h6+h6r	);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h1-5*h6-h6r);
		PNT( -w6,	h6	);
		PNT( -w46,	0	);
		PNT( -w6,	-h6	);
		PNT( 0,		-h1+5*h6+h6r);
		PNT( w6,	-h6	);
		PNT( w46,	0	);
		PNT( w6,	-h6	);
		PNT( 0,		-h6-h6r	);
		break;
	    }

          case '9' :
	    {	int	w6, w46, h6, h6r;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h6r = (h1 - h6 * 6) / 2;

		PNT( x,		y+h1-h6	);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	-h6	);
		PNT( 0,		-h1+2*h6);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h6+h6r);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	-h6	);
		break;
	    }


	  case 'a' : case 'A' :
	    {	int	h2, w2, w5l, w5r;

		h2 = h1 * 12 / 20;
		w2 = (w1+1) / 2;
		w5l = w1 / 5;
		w5r = w1 - w5l;

		PNT( x,		y + h1	);
		PNT( w5l,	-(h1-h2));
		PNT( w5r - w5l,	0	);
		PNT( w2 - w5r,	-h2	);
		PNT( w5l - w2,	h2	);
		PNT( w5r - w5l,	0	);
		PNT( w5l,	h1-h2	);
		break;
	    }

	  case 'b' : case 'B' :
	    {	int	w6, h6;
		int	h2;

		h2 = (h1+1) / 2;
		w6 = w1 / 6;
		h6 = h1 / 6;

		PNT( x,		y + h1  );
		PNT( 0,		-h1	);
		PNT( w1-w6,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h2-h6-h6);
		PNT( -w6,	h6	);
		PNT( -(w1-w6),	0	);
		PNT( w1-w6,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h1-h2-h6-h6);
		PNT( -w6,	h6	);
		PNT( -(w1-w6),	0	);
		break;
	    }

	  case 'c' : case 'C' :
	    {	int	w6, w46, h6, h46;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h46 = h1 - h6 - h6;

		PNT( x + w1,	y + h6	);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h46	);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	-h6	);
		break;
	    }

	  case 'd' : case 'D' :
	    {	int	w6, w56, h6, h46;

		w6 = w1 / 6;
		w56 = w1 - w6;
		h6 = h1 / 6;
		h46 = h1 - h6 - h6;

		PNT( x,		y	);
		PNT( w56,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h46	);
		PNT( -w6,	h6	);
		PNT( -w56,	0	);
		PNT( 0,		-h1	);
		break;
	    }

	  case 'e' : case 'E' :
	    {	int	h2, w2;

		h2 = (h1+1) / 2;
		w2 = (w1+1) / 2;

		PNT( x + w1,	y );
		PNT( -w1,	0 );
		PNT( 0,		h2 );
		PNT( w2,	0 );
		PNT( -w2,	0 );
		PNT( 0,		h1-h2 );
		PNT( w1,	0 );
		break;
	    }

	  case 'f' : case 'F' :
	    {	int	h2, w2;

		h2 = (h1+1) / 2;
		w2 = (w1+1) / 2;

		PNT( x + w1,	y );
		PNT( -w1,	0 );
		PNT( 0,		h2 );
		PNT( w2,	0 );
		PNT( -w2,	0 );
		PNT( 0,		h1-h2 );
		break;
	    }

	  case 'g' : case 'G' :
	    {	int	w6, w46, h6, h46;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h46 = h1 - h6 - h6;

		PNT( x + w1,	y + h6	);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h46	);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	-h6	);
		PNT( 0,		h6-(h1+1)/2);
		PNT( -(w1+1)/2,	0	);
		break;
	    }

	  case 'h' : case 'H' :
	    {	int	h2 = (h1+1) / 2;

		PNT( x,		y	);
		PNT( 0,		h1	);
		PNT( 0,		-h2	);
		PNT( w1,	0	);
		PNT( 0,		h2	);
		PNT( 0,		-h1	);
		break;
	    }

	  case 'i' : case 'I' :
	    {	int	w2 = (w1+1) / 2;

		PNT( x,		y	);
		PNT( w1,	0	);
		PNT( -w2,	0	);
		PNT( 0,		h1	);
		PNT( -w2,	0	);
		PNT( w1,	0	);
		break;
	    }

	  case 'k' : case 'K' :
	    { 	int	h2 = (h1+1) / 2;

		PNT( x,		y	);
		PNT( 0,		h1	);
		PNT( 0,		-h2	);
		PNT( w1,	-(h1-h2));
		PNT( -w1,	h1-h2	);
		PNT( w1,	h2	);
		break;
	    }

	  case 'l' : case 'L' :
	    {	PNT( x,		y );
		PNT( 0,		h1 );
		PNT( w1,	0 );
		break;
	    }

	  case 'm' : case 'M' :
	    {	int	w2 = (w1+1) / 2;

		PNT( x,		y + h1	);
		PNT( 0,		-h1	);
		PNT( w2,	h1	);
		PNT( w1 - w2,	-h1	);
		PNT( 0,		h1	);
		break;
	    }

	  case 'n' : case 'N' :
	    PNT( x,	y + h1	);
	    PNT( 0,	-h1	);
	    PNT( w1,	h1	);
	    PNT( 0,	-h1	);
	    break;

	  case 'o' : case 'O' :
	    {	int	w6, w46, h6, h46;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h46 = h1 - h6 - h6;

		PNT( x + w1,	y + h6	);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h46	);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	-h6	);
		PNT( 0,		-h46	);
		break;
	    }

	  case 'p' : case 'P' :
	    {	int	w6, h6;
		int	h2;

		h2 = (h1+1) / 2;
		w6 = w1 / 6;
		h6 = h1 / 6;

		PNT( x,		y + h1  );
		PNT( 0,		-h1	);
		PNT( w1-w6,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h2-h6-h6);
		PNT( -w6,	h6	);
		PNT( -(w1-w6),	0	);
		break;
	    }

	  case 'q' : case 'Q' :
	    {	int	w6, w46, h6, h46;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h46 = h1 - h6 - h6;

		PNT( x + w1,	y + h6	);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h46	);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( -w6-w6,	-h6	);
		PNT( w6+w6,	h6	);
		PNT( w6,	h6	);
		PNT( -w6,	-h6	);
		PNT( w6,	-h6	);
		PNT( 0,		-h46	);
		break;
	    }

	  case 'r' : case 'R' :
	    {	int	w6, h6;
		int	h2;

		h2 = (h1+1) / 2;
		w6 = w1 / 6;
		h6 = h1 / 6;

		PNT( x,		y + h1  );
		PNT( 0,		-h1	);
		PNT( w1-w6,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h2-h6-h6);
		PNT( -w6,	h6	);
		PNT( -(w1-w6),	0	);
		PNT( w1-w6,	0	);
		PNT( w6,	h2	);
		break;
	    }

	  case 's' : case 'S' :
	    {	int	w6, w46, h6, h6r;

		w6 = w1 / 6;
		w46 = w1 - w6 - w6;
		h6 = h1 / 6;
		h6r = (h1 - h6 * 6) / 2;

		PNT( x + w1,	y + h6	);
		PNT( -w6,	-h6	);
		PNT( -w46,	0	);
		PNT( -w6,	h6	);
		PNT( 0,		h6+h6r	);
		PNT( w6,	h6	);
		PNT( w46,	0	);
		PNT( w6,	h6	);
		PNT( 0,		h6+h6r	);
		PNT( -w6,	h6	);
		PNT( -w46,	0	);
		PNT( -w6,	-h6	);
		break;
	    }

	  case 't' : case 'T' :
	    {	int	w2 = (w1+1) / 2;

		PNT( x,		y	);
		PNT( w1,	0	);
		PNT( -w2,	0	);
		PNT( 0,		h1	);
		break;
	    }

	  case 'u' : case 'U' :
	    {	int	w6, h6, h56;

		w6 = w1 / 6;
		h6 = h1 / 6;
		h56 = h1 - h6;

		PNT( x,		y	);
		PNT( 0,		h56	);
		PNT( w6,	h6	);
		PNT( w1-w6-w6,	0	);
		PNT( w6,	-h6	);
		PNT( 0,		-h56	);
		break;
	    }

	  case 'v' : case 'V' :
	    {	int	w2;

		w2 = (w1+1) / 2;

		PNT( x,		y	);
		PNT( w2,	h1	);
		PNT( w1-w2,	-h1	);
		break;
	    }

	  case 'w' : case 'W' :
	    {	int	w4, w2;

		w4 = w1 / 4;
		w2 = (w1+1) / 2;

		PNT( x,		y	);
		PNT( w4,	h1	);
		PNT( w2-w4,	-h1	);
		PNT( w1-w2-w4,	h1	);
		PNT( w4,	-h1	);
		break;
	    }

	  case 'y' : case 'Y' :
	    {	int	h2, w2;

		h2 = (h1+1) / 2;
		w2 = (w1+1) / 2;

		PNT( x,		y	);
		PNT( w2,	h2	);
		PNT( w1-w2,	-h2	);
		PNT( -(w1-w2),	h2	);
		PNT( -w2,	h1-h2	);
		break;
	    }

/*--Letters we don't have, and also blank. */

	  default :
	    ; /* do nothing */
	}

/*--Draw the letter. */

	DEBUG_OTHER(Draw_Text,1 char);
	if (pnti > 0) {
	    wpolyline(pnts, pnti);
	}
    }
    DEBUG_RETURN(Draw_Text);

} /* Draw_Text */


void Draw_Score(int score, int x, int y)
/******************************************************************************
* Called to draw the score at x/y.
******************************************************************************/
{
    char		scr[4];
    unsigned int	w = (Letter_Width+2) * 3 + 4;
    unsigned int	h = Letter_Height + 6;
    int			thickness;

    if (score == 0) {
	int	i, lx, ly;
	short	width, height;
	Letter_Height *= 3;
	Letter_Width *= 3;
	w_querywinsize(Board, 0, &width, &height);
	lx = width/2-7*Letter_Width/2;
	ly = height/2 - Letter_Height/2;
	for (i = 5; i > 0; --i) {
	    wsetmode(M_DRAW);
	    Draw_Text( "You Win", lx++, ly++ );
	}
	Letter_Height /= 3;
	Letter_Width /= 3;
    } else {

 /* check for possible play, issue message if no play found */

	int	Found_Tile;
    
        Click1_Row = 0;
        Click1_Col = 0;
	Click1 = Board_Position_NULL;
        Found_Tile = 0;
        for (;;) {
	    Next_Tile( 1, &Click1_Row, &Click1_Col);
	    if (Click1_Col == 0) { break; }
	    Click1 = &Board_Tiles[Click1_Row][Click1_Col];
	    Click2_Row = Click1_Row;
	    Click2_Col = Click1_Col;
	    Next_Tile( 2, &Click2_Row, &Click2_Col);
	    if (Click2_Col != 0) {
	       Found_Tile = 1;
	       break;
	    }
        }
        if (Found_Tile == 0) {
	   short width, height;
	   w_querywinsize(Board, 0, &width, &height);
	   for(thickness=0; thickness<3; thickness++) {
	      wsetmode(M_DRAW);
	      Draw_Text( "No Free Tiles", 
	                 width/2-13*Letter_Width/2+thickness, 
	                 height/2-Letter_Height/2+thickness);
	   }
        }
        Click1 = Board_Position_NULL;
        Click2 = Board_Position_NULL;
    }

    y -= h + 2;
    wsetmode(M_CLEAR);
    w_pbox(Board, x, y, w, h);
    wsetmode(M_DRAW);
    w_box(Board, x-2, y-2, w+4, h+4 );
    w_box(Board, x-1, y-1, w+2, h+2 );
    w_box(Board, x, y, w, h );
    scr[0] = (score > 99 ? '0' + score / 100     : ' ');
    scr[1] = (score >  9 ? '0' + score / 10 % 10 : ' ');
    scr[2] = 		   '0' + score % 10;
    scr[3] = '\000';
    Draw_Text( scr, x+3, y+3 );
    wsetmode(M_INVERS);
    w_pbox(Board, x, y, w, h);

} /* Draw_Score */


static void Hilite(Button button)
/******************************************************************************
* Xor the hilite pattern on the button indicated.
******************************************************************************/
{
    wsetmode(M_INVERS);
    w_pbox(Board, button->x + 1,
		  button->y + 1,
		  (unsigned int)(Button_Width - 1),
		  (unsigned int)(Button_Height - 1) );
} /* Hilite */


void Button_Expose(void)
/******************************************************************************
* Called when the Board receives an Expose event.
******************************************************************************/
{
    int		i;

    DEBUG_CALL(Button_Expose);

/*--Loop over all buttons and display all "real" buttons. */

    for (i = 0; i < NBUTTONS; ++i) {

/*--Clear the space for the button and then draw the outline. */

	wsetmode(M_CLEAR);
	w_pbox(Board, All_Buttons[i].x,
		      All_Buttons[i].y,
		      Button_Width + 1,
		      Button_Height + 1);

	if (All_Buttons[i].text == NULL) { continue; }

	wsetmode(M_DRAW);
	wpolyline(All_Buttons[i].border, Number(All_Buttons[0].border));

/*--Draw the text of the button and then do hiliting if we need it. */

	Draw_Text( All_Buttons[i].text,
		   All_Buttons[i].x + All_Buttons[i].text_x,
		   All_Buttons[i].y + All_Buttons[i].text_y );
	if (All_Buttons[i].hilite) {
	    Hilite( &All_Buttons[i] );
	}
    }
    DEBUG_RETURN(Button_Expose);

} /* Button_Expose */


static void De_Hilite_All(void)
/******************************************************************************
* Called to take the hilite off of any and all buttons everywhere.
******************************************************************************/
{
    int		i;

    for (i = 0; i < NBUTTONS; ++i) {
	if (All_Buttons[i].text == NULL) { continue; }
	if (All_Buttons[i].hilite && !All_Buttons[i].toggle) {
	    All_Buttons[i].hilite = FALSE;
	    Hilite( &All_Buttons[i] );
	    w_flush();
	}
    }

} /* De_Hitlite_All */


void Button_Press(WEVENT *event)
/******************************************************************************
* Called when the Board receives an EVENT_MPRESS.
******************************************************************************/
{
    int		i;

/*--First we make sure that nobody is hilited. */

    DEBUG_CALL(Button_Press);
    De_Hilite_All();

/*--See if some button just got clicked. */

    for (i = 0; i < NBUTTONS; ++i) {
	if (All_Buttons[i].text == NULL) { continue; }
	if (event->x >= All_Buttons[i].x &&
	    event->x <= All_Buttons[i].x + Button_Width &&
	    event->y >= All_Buttons[i].y &&
	    event->y <= All_Buttons[i].y + Button_Height) {

/*--Hilite this button and then do whatever it is supposed to do. */

	    All_Buttons[i].hilite = !All_Buttons[i].hilite;
	    Hilite( &All_Buttons[i] );
	    w_flush();
	    (*(All_Buttons[i].action))();
	    break;
	}
    }
    DEBUG_RETURN(Button_Press);

} /* Button_Press */


void Button_Release(void)
/******************************************************************************
* Called when the Board receives an EVENT_MRELEASE.
******************************************************************************/
{

/*--Turn off any and all button hilites. */

    DEBUG_CALL(Button_Release);
    De_Hilite_All();
    DEBUG_RETURN(Button_Release);

} /* Button_Release */
