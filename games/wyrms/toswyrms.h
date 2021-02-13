/* some user changable defines for TOS Wyrms */

/* how many times screen is refereshed in a second (1 Vsync()) */
#define FRAME_RATE	72

/* keyboard controls (ASCII values), DEV0 & DEV1 are joysticks */
#define DEV2_UP		'a'
#define DEV2_DN		'z'
#define DEV2_LF		'x'
#define DEV2_RT		'c'
#define DEV2_BUTTON	's'
#define DEV3_UP		'8'
#define DEV3_DN		'2'
#define DEV3_LF		'4'
#define DEV3_RT		'6'
#define DEV3_BUTTON	'5'


#ifdef BIG_BITMAPS

#ifdef __SYSTEM_C__
#include "16x16.h"
#endif

/* screen size in 8x8 pixel blocks */
#define SCREEN_W	40
#define SCREEN_H	25

/* information & it's placement on the map */
#define HEADINGS	"     Dir:                   Dir:        "
#define INFO_MSG	"     3 / 000                3 / 000     "
#define PLAYER0_DIR	10
#define PLAYER1_DIR	33
#define PLAYER0_LIVE	5
#define PLAYER1_LIVE	28
#define PLAYER0_SCORE	9
#define PLAYER1_SCORE	32

#else	/* small bitmaps */

#ifdef __SYSTEM_C__
#include "8x8.h"
#endif

/* full screen size in bitmap blocks */
#define SCREEN_W	80
#define SCREEN_H	50

/* information & it's place (column) on the map */
#define HEADINGS	"   Initial direction:                                   Initial direction:      "
#define INFO_MSG	"   Lives: 3   Score: 000                                Lives: 3   Score: 000   "
#define PLAYER0_DIR	22
#define PLAYER1_DIR	75
#define PLAYER0_LIVE	10
#define PLAYER1_LIVE	63
#define PLAYER0_SCORE	21
#define PLAYER1_SCORE	74

#endif
