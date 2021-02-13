/* default and system specific defines for Curses Wyrms */


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

/* screen size in 8x8 pixel blocks */
#define SCREEN_W	80
#define SCREEN_H	24

/* information & it's placement on the map */
#define HEADINGS	"   Initial direction:                                   Initial direction:      "
#define INFO_MSG	"   Lives: 3   Score: 000                                Lives: 3   Score: 000   "
#define PLAYER0_DIR	22
#define PLAYER1_DIR	75
#define PLAYER0_LIVE	10
#define PLAYER1_LIVE	63
#define PLAYER0_SCORE	21
#define PLAYER1_SCORE	74


/* a character translation table for the curses interface 'images' */

#ifdef __SYSTEM_C__

#define BLOCKS 127

/* A_REVERSE could be ORed with characters... */
static unsigned char cmap[BLOCKS] =
{
  ' ',       /* background */
  '#',       /* wall  */
  'H',       /* exit  */
  'D',       /* door  */
  'K',       /* key  */
  'L',       /* leaf  */
  'T',       /* turd  */
  'F',       /* flash */
  'B',       /* bang  */
  '^',       /* arrow-up */
  'v',       /* arrow-down */
  '>',       /* arrow-left */
  '<',       /* arrow-right */
  'o',       /* head  */
  '*',       /* black */
  '+',       /* white */
  '%',       /* gray  */
  ' ',       /* - */
  ' ',       /* - */
  ' ',       /* - */
  '0',       /* 0 */
  '1',       /* 1 */
  '2',       /* 2 */
  '3',       /* 3 */
  '4',       /* 4 */
  '5',       /* 5 */
  '6',       /* 6 */
  '7',       /* 7 */
  '8',       /* 8 */
  '9',       /* 9 */
  ' ',
  ' ',
  ' ',       /* ' ' */
  '!',
  '\"',
  '#',
  '$',
  '%',
  '&',
  '\'',
  '(',
  ')',
  '*',
  '+',
  ',',
  '-',
  '.',
  '/',
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  ':',
  ';',
  '<',
  '=',
  '>',
  '?',
  '@',
  'A',
  'B',
  'C',
  'D',
  'E',
  'F',
  'G',
  'H',
  'I',
  'J',
  'K',
  'L',
  'M',
  'N',
  'O',
  'P',
  'Q',
  'R',
  'S',
  'T',
  'U',
  'V',
  'W',
  'X',
  'Y',
  'Z',
  '[',
  '\\',
  ']',
  '^',
  '_',
  '`',
  'a',
  'b',
  'c',
  'd',
  'e',
  'f',
  'g',
  'h',
  'i',
  'j',
  'k',
  'l',
  'm',
  'n',
  'o',
  'p',
  'q',
  'r',
  's',
  't',
  'u',
  'v',
  'w',
  'x',
  'y',
  'z',
  '{',
  '|',
  '}',
  '~'
};

#endif
