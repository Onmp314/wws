/* Bitmap / IO handling for TOS Wyrms objects and alphabet
 *
 * Bitmaps have to be byte aligned!!!
 *
 * Wyrms Functions init(), restore(), get_key(), delay() and JoyISR()
 * contain ST spesific functions like Logbase and Physbase (get screen
 * addresses), Get_rez (the current rezolution), Kbdvbase (get keyboard
 * vector table address), Ikbdws (ask IKBD to report joystick state
 * updates), Cursconf (dis/enable text cursor), Supexec (exec function in
 * processor Supervisor mode to access system vector(s)), Cconis (check for
 * a keypress), Crawcin (get a 'raw' keycode), Setscreen (swap logical and
 * physical screens), Vsync (wait for vertical sync (screen refresh)),
 * Dosound (sound effects with the sound chip) system calls.
 */

/* some Atari specific low level things... */
#define _hz_200	(unsigned long *)0x4baL
#ifdef __SOZOBONX__
  #include <bios.h>
  typedef KBDVECS _KBDVECS;
#else
  #include <ostruct.h>	/* OS structures (_KBDVECS) */
#endif
#include <osbind.h>	/* OS functions */
#include <mintbind.h>	/* signal handling */
#include <signal.h>	/* signal IDs */
#include <stdlib.h>	/* srand() */
#include "dosound.h"	/* sound effects */
#define __SYSTEM_C__
#include "wyrms.h"


/* assembly code for receiving IKBD packets (joystick) */
extern void JoyISR(void *packet_addr);


/* for the assembly joystick handler */
volatile long JoyFlags[2] = {0, 0};

/* a temp for restoring */
static void (*joy_handler);


#define DEVBUF_SIZE	8

/* for ring-buffered input */
typedef struct NEXT
{
  int item;			/* input character */
  struct NEXT *next;		/* next char in the ring */
} Next;

/* if the structure IO input happens in the background thread,
 * you'll probably need a lock for accessing the structure.
 */
typedef struct
{
  int used;
  int last;
  Next *in;
  Next *out;
  Next buf[DEVBUF_SIZE];
} Buf;

/* buffers for 'device' input */
Buf dev_buf[DEVICES];


/* logical screen address */
static unsigned char *Screen;


/* Systen specific functions */
static void check_IO(void);
void ToStack(int dev, int key);


/* get / set system variables at start */
void init(void)
{
  _KBDVECS *table_addr;
  int dev, idx;

  Cconws("\r\nTOS-Wyrms " VERSION " (C) 1995 by Eero Tamminen\r\n\r\n");
  Cconws("A mono, two player game for the ST-hirez.\r\n");
  Cconws("Control with joysticks, exit with ESC.\r\n");

  /* initialize the input buffer links */
  for(dev = 0; dev < DEVICES; dev++)
  {
    dev_buf[dev].used = 0;
    for(idx = 0; idx < (DEVBUF_SIZE - 1); idx++)
      dev_buf[dev].buf[idx].next = &dev_buf[dev].buf[idx + 1];
    dev_buf[dev].buf[idx].next = &dev_buf[dev].buf[0];
    dev_buf[dev].out = &dev_buf[dev].buf[0];
    dev_buf[dev].in = &dev_buf[dev].buf[0];
  }

  /* get the screen address */
  Screen = (unsigned char *)Logbase();

  /* Install a new joystick packet handler into keyboard vector table.  Tell
   * IKBD to send joystick packets.
   */
  table_addr = Kbdvbase();
  joy_handler = table_addr->joyvec;
  table_addr->joyvec = JoyISR;
  Ikbdws(0, "\024");		/* report joystick state changed */

  /* cursor off */
  (void)Cursconf(0, 0);

  /* MiNT: on a case of an exiting request, restore system variables */
  signal(SIGHUP, restore);
  signal(SIGINT, restore);
  signal(SIGQUIT, restore);
  signal(SIGTERM, restore);
}

/* restore system variables etc. */
void restore(int sig)
{
  _KBDVECS *table_addr;

  Dosound(snd_off);
  table_addr = Kbdvbase();
  table_addr->joyvec = joy_handler;
  Ikbdws(0, "\010");			/* normal (checking mouse) */

  /* was this little wyrm killed? ;-) */
  if(sig)
    Cconws("SPLAT\r\n");

  (void)Cursconf(1, 0);
  exit(0);
}

/* make soundeffects */
void do_sound(int sound)
{
  switch(sound)
  {
    case SND_EXPLOSION:
      Dosound(snd_explosion);
      break;
    case SND_JINGLE:
      Dosound(snd_jingle);
      break;
    case SND_DING:
      Dosound(snd_ding);
      break;
  }
}

/* manage IO buffers and wait for the screen refresh */
void wait_frame(void)
{
  check_IO();
  Vsync();
}

/* wait and return a key (ASCII). no buffer clearing */
int get_key(void)
{
  int key;
  /* wait for a key */
  key = Crawcin() & 0xFF;
  if(key == 27)			/* ESC quits */
    restore(0);
  return key;
}

/* return device input */
int input(int dev)
{
  int key;

   /* manage game control input */
   check_IO();

  /* return key if in buffer */
  if(dev_buf[dev].used)
  {
    key = dev_buf[dev].out->item;
    dev_buf[dev].out = dev_buf[dev].out->next;
    dev_buf[dev].used--;
    return key;
  }
  return 0;
}

/* return new postion... */
int get_position(int *x, int *y)
{
  int key;

  key = get_key();

  switch(key)
  {
    case '8':
      (*y)--;
      break;
    case '9':
      (*x)++;
      (*y)--;
      break;
    case '6':
      (*x)++;
      break;
    case '3':
      (*x)++;
      (*y)++;
      break;
    case '2':
      (*y)++;
      break;
    case '1':
      (*x)--;
      (*y)++;
      break;
    case '4':
      (*x)--;
      break;
    case '7':
      (*x)--;
      (*y)--;
      break;
    default:
      return key;
  }
  return 0;
}

/* check IO status and fill buffers accordingly */
static void check_IO(void)
{
  if(JoyFlags[0])
    ToStack(0, (int)JoyFlags[0]);

  if(JoyFlags[1])
    ToStack(1, (int)JoyFlags[1]);

  if(Cconis())
  {
    int key = 0, dev = -1;

    switch(Crawcin() & 0xFF)		/* ASCII */
    {
      /* ESC quits */
      case 27:
        restore(0);

      case DEV2_UP:
	key = UP;
	dev = 2;
	break;
      case DEV2_DN:
	key = DOWN;
	dev = 2;
	break;
      case DEV2_LF:
	key = LEFT;
	dev = 2;
	break;
      case DEV2_RT:
	key = RIGHT;
	dev = 2;
	break;
      case DEV2_BUTTON:
	key = BUTTON;
	dev = 2;
	break;

      case DEV3_UP:
	key = UP;
	dev = 3;
	break;
      case DEV3_DN:
	key = DOWN;
	dev = 3;
	break;
      case DEV3_LF:
	key = LEFT;
	dev = 3;
	break;
      case DEV3_RT:
	key = RIGHT;
	dev = 3;
	break;
      case DEV3_BUTTON:
	key = BUTTON;
	dev = 3;
	break;
    }
    if(dev >= 0)
      ToStack(dev, key);
  }
}

/* put the key into the device's IO buffer,
 * ensure that there are no repeats
 */
void ToStack(int dev, int key)
{
  if(dev_buf[dev].last != key || !dev_buf[dev].used)
  {
    dev_buf[dev].in->item = key;
    dev_buf[dev].in = dev_buf[dev].in->next;
    /* discard last if there would be an overrun */
    if(dev_buf[dev].out == dev_buf[dev].in)
      dev_buf[dev].out = dev_buf[dev].out->next;
    else
      dev_buf[dev].used++;
    dev_buf[dev].last = key;
  }
}


/* map blocks onto screen. */
void bitmap(int x, int y, unsigned char obj)
{
  unsigned char *from, *to;
  int byte;

  if(obj >= BLOCKS)
    obj = 0;

  from = Bitmaps +
         (obj / BLK_ROW) * (BLOCKS_W / 8 * BLK_H) +
	 (obj % BLK_ROW) * (BLK_W / 8);
  to   = Screen + (y * (SCREEN_W * BLK_H) + x) * (BLK_W / 8);

  for(y = 0; y < BLK_H; y++)
  {
    byte = (BLK_W / 8);
    while(byte-- > 0)
      to[byte] = from[byte];
    from += (BLOCKS_W / 8);
    to   += (SCREEN_W * BLK_W / 8);
  }
}

void draw_map(unsigned const char *array)
{
  int x, y;

  for(x = 0; x < SCREEN_W; x++)
    for(y = 0; y < SCREEN_H; y++)
      bitmap(x, y, *(array++));
}

/* a message */
void message(const char *string)
{
  Cconws(string);
  Cconws("\r\n");
}
