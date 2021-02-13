/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes	March 1989
* W Port: Jens Kilian		February 1996
*
* main.h - #include'ed into all source files.
******************************************************************************/

#include <stdio.h>

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifdef _MAIN_C_
#define VAR(A)		A
#define VARI(A,B)	A = B
#else
#define VAR(A)		extern A
#define VARI(A,B)	extern A
#endif


/******************************************************************************
* Types & macros (from X)
******************************************************************************/

typedef int Boolean;
typedef const char *String;
typedef struct {
    short x, y;
} Point;

#define Number(foo) (sizeof(foo)/sizeof(foo[0]))

/******************************************************************************
* Dragon Resources
******************************************************************************/

typedef struct _Dragon_Resources_Rec {
    Boolean		Debug;			/* Debug printouts */
    int			Double_Click_Time;	/* Time (ms) between clicks */
    String		Font;			/* What font? */
    int			Size;			/* in what size */
    int			Styles;			/* in what style(s) */
    String		Geometry;		/* Geometry for the board */
    Boolean		Iconic;			/* Do we start as an icon? */
    Boolean		Reverse_Video;		/* Do all in reverse? */
    Boolean		Tile_Shadows;		/* Want shadows? */
    String		Tile_Sides;		/* What side type? */
    Boolean		Sticky_Tile;		/* Is first tile sticky? */
} Dragon_Resources_Rec, *Dragon_Resources_Ptr;

#ifdef WANTDEBUG
#define DEBUG_CALL(Name) \
  if (Dragon_Resources.Debug) { \
    (void)fprintf( stderr, "Name call\n" ); (void)fflush(stderr); \
  }
#define DEBUG_RETURN(Name) \
  if (Dragon_Resources.Debug) { \
    (void)fprintf( stderr, "Name return\n" ); (void)fflush(stderr); \
  }
#define DEBUG_OTHER(Name,Other) \
  if (Dragon_Resources.Debug) { \
    (void)fprintf( stderr, "Name Other\n" ); (void)fflush(stderr); \
  }
#define DEBUG_ERROR(Test,Msg) \
  if (Test) { (void)fprintf(stderr,Msg); (void)fflush(stderr); }
#else
#define DEBUG_CALL(Name)
#define DEBUG_RETURN(Name)
#define DEBUG_OTHER(Name,Other)
#define DEBUG_ERROR(Test,Msg)
#endif /* WANTDEBUG */

/******************************************************************************
* Bitmap/Image Initialization Structure
******************************************************************************/

typedef struct _BITMAP_Init {
    BITMAP	*image;
    uchar	*bits;
    int		 width;
    int		 height;
} BITMAP_Init;



/******************************************************************************
* Global State
******************************************************************************/

typedef enum { s_Normal, s_Sample } Display_State;

VARI( Display_State	Board_State,		s_Normal );

VAR( Dragon_Resources_Rec  Dragon_Resources );	/* Application resources */

VAR( WWIN		  *Board );		/* Our main drawable */
VAR( WWIN		  *Icon );		/* Our icon */

VAR( String		   Program_Name );	/* Name of program in cmd */

VAR( int		   Tile_Control );	/* Control bits */

#define SHADOW		(1<<0)
#define BLACKSIDE	(1<<1)
#define GRAYSIDE	(1<<2)

#undef VAR
#undef VARI
