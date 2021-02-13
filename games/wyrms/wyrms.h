/* system dependent defines and game preferences / defaults for Wyrms */

#ifndef __WYRMS_H__
#define __WYRMS_H__

#define VERSION	"v1.0"

#ifdef CURSES_WYRM
#include "chrwyrms.h"
#endif

#ifdef X11_WYRM
#include "guiwyrms.h"
#endif

#ifdef W1_WYRM
#include "guiwyrms.h"
#endif

#ifdef GEM_WYRM
#include "guiwyrms.h"
#endif

#ifdef TOS_WYRM
#include "toswyrms.h"
#endif

/* system independend defines, typedefs/structures, prototypes etc. */

/* position of the texts in OS (screen size) specific includes */
#define INFOLINE	(SCREEN_H - 1)		/* game information line */

/* default values */
#define DEF_SPEED	8	/* moves / sec */
#define DEF_DEV0	0	/* player 0 control device */
#define DEF_DEV1	3	/* player 1 control device */
#define DEF_LIVES	3	/* default amount of lives at start */
#define LB_SIZE		2	/* the lenght bonus block size */
#define LEN_INC		6	/* wyrm lenght increased by this */
#define SCORE_INC	2	/* two points from bonuses */
#define FLASH_TIME	20	/* how many moves turbo is active */

/* Maximum wyrm lenght */
#define MAX_LENGHT	64

/* device identifiers */
#define DEVICES		4
#define DEVICE_0	0
#define DEVICE_1	1
#define DEVICE_2	2
#define DEVICE_3	3

/* sound effect identifiers */
#define SND_EXPLOSION	1
#define SND_JINGLE	2
#define SND_DING	3

/* General control device directions */
#define UP		1
#define DOWN		2
#define LEFT		4
#define RIGHT		8
#define BUTTON		128

/* stuff below will need changes to the source too */

/* object identifiers (= image indeces) */
#define I_BG		0	/* has to be 0 (=FALSE) */
#define I_WALL		1	/* chrash if no BANG */
#define I_HOME		2	/* one wyrm live more to spend */
#define I_DOOR		3	/* you'll need the key */
#define I_KEY		4	/* works only once... */
#define I_LEAF		5	/* food */
#define I_GREASE	6	/* a greasy dropping */
#define I_FLASH		7	/* you found a turbo... */
#define I_BANG		8	/* hash bang explosives */
#define I_UP		9	/* directional arrows */
#define I_DOWN		10
#define I_LEFT		11
#define I_RIGHT		12
#define I_HEAD		13	/* yer' head */
#define I_BLACK		14	/* player 0 body */
#define I_WHITE		15	/* player 1 body */
#define I_GRAY		16	/* stalemate dead bodies */
#define I_ONEWAY	17
#define I_WAYONE	18
#define I_SQUARE	19
#define I_0		20	/* growth bonus numbers */
#define I_1		21
#define I_2		22
#define I_3		23
#define I_4		24
#define I_5		25
#define I_6		26
#define I_7		27
#define I_8		28
#define I_9		29

typedef struct SNAKE {	/* the world snake that eats it's tail... */
  short x;
  short y;
  struct SNAKE *next;
} Ring;

/* a structure for the wyrms */
typedef struct {
  Ring pos[MAX_LENGHT];	/* the place of every wyrm block */
  Ring *head;		/* the pointer to the wyrms head place */
  short lenght;		/* wyrm lenght */
  short speed;		/* wyrm speed */
  short timer;		/* speed timer */
  short bonus;		/* current bonus */
  short btime;		/* bonus timer */
  short dir;		/* direction */
  short pdir;		/* previous direction indicator */
  short incx;		/* wyrm position change increments */
  short incy;
  short device;		/* controlling device */
  short lives;		/* lives left */
  short score;		/* player scores */
  short id;		/* wyrm id ('color') on map */
} Wyrms;

#ifndef FALSE
#define FALSE		0
#define TRUE		!FALSE
#endif


/* prototypes */

#ifdef __SYSTEM_C__
#define ext
#else
#define ext	extern
#endif

ext void init(void);
ext void restore(int sig);
ext void do_sound(int sound);
ext void wait_frame(void);
ext int  get_key(void);
ext int  input(int device);
ext int  get_position(int *x, int *y);
ext void bitmap(int x, int y, unsigned char object);
ext void draw_map(unsigned const char *array);
ext void message(const char *string);

#undef ext

#ifdef __LEVEL_C__
#define ext
#else
#define ext	extern
#endif

ext void init_level(unsigned char *map, Wyrms wyrm[]);
ext int  write_map(const char *level_file, unsigned const char *map, Wyrms wyrm[]);
ext int  read_map(const char *path, const char *level_file);

#undef ext

#endif
