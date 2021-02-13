/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes	March 1989
* W Port: Jens Kilian		February 1996
*
* main.c - The mainline code.
******************************************************************************/

#define _MAIN_C_

#include <Wlib.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "proto.h"


/******************************************************************************
* Default "Resources"
******************************************************************************/

Dragon_Resources_Rec Dragon_Resources = {
	FALSE,			/* Debug printouts */
	600,			/* Time (ms) between clicks */
	"cour",			/* Font to use for complaints */
	11,			/* size */
	F_BOLD,			/* styles */
	"450,340",		/* Geometry for the board */
	FALSE,			/* Do we start as an icon? */
	FALSE,			/* Do all in reverse? */
	TRUE,			/* Want shadows? */
	"gray", 		/* What side type? */
	TRUE,			/* Is first tile sticky? */
};


/******************************************************************************
* Dragon Command Line Options
******************************************************************************/

typedef enum { BOOL_TRUE, BOOL_FALSE, STRING, INTEGER } Option_Type;

typedef struct {
  String	name;
  Option_Type	type;
  void	       *ptr;
} Option_Desc;

static Option_Desc command_line_options[] = {
  { "-debug",	BOOL_TRUE,	&Dragon_Resources.Debug },
  { "-double",	INTEGER,	&Dragon_Resources.Double_Click_Time },
  { "-font",	STRING,		&Dragon_Resources.Font },
  { "-fsize",	INTEGER,	&Dragon_Resources.Size },
  { "-styles",	INTEGER,	&Dragon_Resources.Styles },
  { "-geometry",STRING,		&Dragon_Resources.Geometry },
  { "-iconic",	BOOL_TRUE,	&Dragon_Resources.Iconic },
  { "+iconic",	BOOL_FALSE,	&Dragon_Resources.Iconic },
  { "-reverse",	BOOL_TRUE,	&Dragon_Resources.Reverse_Video },
  { "+reverse",	BOOL_FALSE,	&Dragon_Resources.Reverse_Video },
  { "-shadows",	BOOL_TRUE,	&Dragon_Resources.Tile_Shadows },
  { "+shadows",	BOOL_FALSE,	&Dragon_Resources.Tile_Shadows },
  { "-sides",	STRING,		&Dragon_Resources.Tile_Sides },
  { "-sticky",	BOOL_TRUE,	&Dragon_Resources.Sticky_Tile },
  { "+sticky",	BOOL_FALSE,	&Dragon_Resources.Sticky_Tile },
  { NULL, STRING, NULL }
};

typedef struct {
  String	name;
  String	desc;
} Option_Help;

static Option_Help command_line_help[] = {
{"-help",      		 "print out this message"},
{"-double time",	 "double-click time limit in milliseconds"},
{"-font name",		 "font to use for messages"},
{"-geometry W,H[,X,Y]",	 "size (in pixels) and position of the board"},
{"-/+iconic",            "begin program as an icon"},
{"-/+reverse",		 "turn on/off reverse video"},
{"-/+shadows",		 "turn on/off tile shadows"},
{"-sides line/black/gray","set the style for tile sides"},
{"-/+sticky",		 "first tile selected sticky"},
{ NULL, NULL }
};


static void Parse_Command_Line(int *argc, String *argv)
/******************************************************************************
*   A poor man's X-style command line parser ...
******************************************************************************/
{
  int from, to;
  Option_Desc *desc;

  for (from = to = 1; from < *argc; ++from) {
    for (desc = command_line_options; desc->name; ++desc) {
      if (strcmp(argv[from], desc->name) == 0) {
	break;
      }
    }

    if (desc->name) {
      switch (desc->type) {

      case BOOL_TRUE:
	*(Boolean *)(desc->ptr) = TRUE;
	break;

      case BOOL_FALSE:
	*(Boolean *)(desc->ptr) = FALSE;
	break;

      case STRING:
	if (++from < *argc) {
	  *(String *)(desc->ptr) = argv[from];
	} else {
	  (void)fprintf(stderr, "%s:  command line option [%s] needs an argument\n\n",
			Program_Name, argv[from-1]);
	  exit(1);
	}
	break;

      case INTEGER:
	if (++from < *argc) {
	  *(int *)(desc->ptr) = atoi(argv[from]);
	} else {
	  (void)fprintf(stderr, "%s:  command line option [%s] needs an argument\n\n",
			Program_Name, argv[from-1]);
	  exit(1);
	}
	break;
      }
    } else {
      argv[to++] = argv[from];
    }
  }

  *argc = to;

} /* Parse_Command_Line */


static void Command_Line_Usage(String Bad_Option)
/******************************************************************************
*   Bad_Option	- Specifies the unfathomable command line option we received
*
* Called when we find an unrecognized command line option.  We spit out a
* Unix-style usage message and die.
******************************************************************************/
{
    Option_Help	*opt;
    int		 col;

    (void)fprintf( stderr, "%s:  unrecognized command line option [%s]\n\n",
		   Program_Name, Bad_Option );

    (void)fprintf( stderr, "usage:  %s", Program_Name );
    col = 8 + strlen(Program_Name);
    for (opt = command_line_help; opt->name; opt++) {
	int len = 3 + strlen(opt->name);	/* space[string] */
	if (col + len > 79) {
	    (void)fprintf (stderr, "\n   ");	/* 3 spaces */
	    col = 3;
	}
	(void)fprintf (stderr, " [%s]", opt->name);
	col += len;
    }

    (void)fprintf( stderr, "\n\nType %s -help for more help.\n",
		   Program_Name );
    exit( 1 );

} /* Command_Line_Usage */


static void Command_Line_Help(void)
/******************************************************************************
* Called when the -help switch is used.  Put out our extended help message.
******************************************************************************/
{
    Option_Help	*opt;

    (void)fprintf( stderr, "usage:\n        %s [options]\n\n",
		   Program_Name);
    (void)fprintf( stderr, "Where the options are:\n");
    for (opt = command_line_help; opt->name; opt++) {
	(void)fprintf( stderr, "    %-24s %s\n", opt->name, opt->desc );
    }
    (void)fprintf( stderr, "\n" );
    exit( 0 );

} /* Command_Line_Help */


static void Event_Loop(void)
/******************************************************************************
* Main event loop - returns when user closes the window.
******************************************************************************/
{
    WEVENT *wevent;

    for (;;) {
	if ((wevent = w_queryevent(NULL, NULL, NULL, -1))) {
	    switch (wevent->type) {

	    case EVENT_MPRESS:
		if (wevent->win == Board) {
		    switch (wevent->key) {

		    case BUTTON_LEFT:
			Tile_Press(wevent);
			Button_Press(wevent);
			break;

		    case BUTTON_MID:
			Hints();
			break;

		    case BUTTON_RIGHT:
			Tile_Remove();
			break;
		    }

		} else if (wevent->win == Icon) {
		    Iconify();

		} else /* Complaint */ {
		    w_close(wevent->win);
		    w_delete(wevent->win);
		}
		break;

	    case EVENT_MRELEASE:
		if ((wevent->win == Board) && (wevent->key == BUTTON_LEFT)) {
			Tile_Release();
			Button_Release();
		}
		break;

	    case EVENT_GADGET:
		switch (wevent->key) {

		case GADGET_EXIT:
		case GADGET_CLOSE:
		    return;

		case GADGET_ICON:
		    Iconify();
		    break;

		case GADGET_SIZE:
		    /* ### ... Board_Configure(...); ... ### */
		    break;
		}
		break;
	    }
	}
    }
} /* Event_Loop */


int main (int argc, String *argv)
/******************************************************************************
* Our Main-Line Code.
******************************************************************************/
{
    Program_Name = argv[0];

    if (!w_init()) {
	fprintf(stderr, "%s: Can't open server connection.\n", Program_Name);
	return 1;
    }

/*--Parse the command line. */

    Parse_Command_Line(&argc, argv);

/*--See if there is anything left on the command line. */

    for (++argv, --argc; argc > 0; ++argv, --argc) {
	if (strcmp( argv[0], "-help" ) == 0) { Command_Line_Help(); }
	Command_Line_Usage( argv[0] );
    }

/*--Set up our icon. */

    DEBUG_OTHER(main,Icon_Setup);
    Icon_Setup();

/*--Check the tile controls. */

    Tile_Control = 0;
    if (Dragon_Resources.Tile_Shadows) { Tile_Control |= SHADOW; }
    if (strcmp( Dragon_Resources.Tile_Sides, "black" ) == 0) {
	Tile_Control |= BLACKSIDE;
    } else if (strcmp( Dragon_Resources.Tile_Sides, "gray" ) == 0) {
	Tile_Control |= BLACKSIDE | GRAYSIDE;
    } else if (strcmp( Dragon_Resources.Tile_Sides, "line" ) != 0) {
	(void)fprintf( stderr,
		       "-side option not given line, gray, or black value.\n");
    }

/*--Create the board that we will be using. */

    DEBUG_OTHER(main,Board_Setup);
    Board_Setup();

/*--Draw initial board. */

    Board_Expose();
    Button_Expose();

/*--Process events until done. */

    Event_Loop();

    w_exit();
    return 0;

} /* main */
