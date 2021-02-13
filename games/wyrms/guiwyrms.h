/* default and system specific defines for GUI Wyrms */

/* keyboard controls (ASCII values) */
#define DEV0_UP		'a'
#define DEV0_DN		'z'
#define DEV0_LF		'x'
#define DEV0_RT		'c'
#define DEV0_BUTTON	's'
#define DEV1_UP		'u'
#define DEV1_DN		'j'
#define DEV1_LF		'k'
#define DEV1_RT		'l'
#define DEV1_BUTTON	'i'
#define DEV2_UP		'-'
#define DEV2_DN		'+'
#define DEV2_LF		'('
#define DEV2_RT		')'
#define DEV2_BUTTON	'*'
#define DEV3_UP		'8'
#define DEV3_DN		'2'
#define DEV3_LF		'4'
#define DEV3_RT		'6'
#define DEV3_BUTTON	'5'


/* screen refresh rate (/sec) */
#define FRAME_RATE	72

#ifdef BIG_BITMAPS

#ifdef __SYSTEM_C__
  #ifdef X11_WYRM		/* has different bitorder */
    #include "x16x16.h"
  #else
    #include "16x16.h"
  #endif
#endif

/* screen size in 16x16 pixel blocks */
#define SCREEN_W	30
#define SCREEN_H	20

/* information & it's placement on the map */
#define HEADINGS	"   Dir:             Dir:      "
#define INFO_MSG	"   3 / 000          3 / 000   "
#define PLAYER0_DIR	8
#define PLAYER1_DIR	25
#define PLAYER0_LIVE	3
#define PLAYER1_LIVE	20
#define PLAYER0_SCORE	7
#define PLAYER1_SCORE	24

#else	/* small bitmaps */

#ifdef __SYSTEM_C__
  #ifdef X11_WYRM		/* has different bitorder */
    #include "x8x8.h"
  #else
    #include "8x8.h"
  #endif
#endif

/* screen size in 8x8 pixel blocks */
#define SCREEN_W	60
#define SCREEN_H	40

/* information & it's placement on the map */
#define HEADINGS	"   Initial direction:               Initial direction:      "
#define INFO_MSG	"   Lives: 3   Score: 000            Lives: 3   Score: 000   "
#define PLAYER0_DIR	22
#define PLAYER1_DIR	55
#define PLAYER0_LIVE	10
#define PLAYER1_LIVE	43
#define PLAYER0_SCORE	21
#define PLAYER1_SCORE	54

#endif
