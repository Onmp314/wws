/* The game objects and alphabet bitmap handling for W-Wyrms.
 *
 * At the bottom are some ifdefs for Atari MiNT
 * (XBios Dosound() effects and screen Vsync()).
 *
 * Copyright (C) 1996 by Eero Tamminen
 */

#include <Wlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __SYSTEM_C__
#include "wyrms.h"

/* window size */
#define WIN_W		(SCREEN_W * BLK_W)
#define WIN_H		(SCREEN_H * BLK_H)
#define WIN_NAME	" W-Wyrms " VERSION " "
/* only the level editor needs mouse events... */
#define WIN_PROPERTIES	(W_MOVE | W_TITLE | W_CLOSE | W_ICON | EV_MOUSE | EV_KEYS)
#define ICON_PROPERTIES	(W_MOVE | EV_MOUSE)
#define KEY_EXIT	'q'

static WWIN *Win, *Images, *Icon;
static int MouseDown;		/* for the editor */

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
  int used;			/* how many buffer items in use */
  int last;			/* last input key */
  Next *in;			/* next input char put here */
  Next *out;			/* next input char read from here */
  Next buf[DEVBUF_SIZE];
} Buf;

/* buffers for 'device' input */
static Buf dev_buf[DEVICES];


/* library inside prototypes */
static const char *gfx_init(void);
static int  process_events(WEVENT *ev);
static int  check_events(long timeout);
static void buffer_IO(int key);
static void ToStack(int dev, int key);


/* init graphics, convert bitmaps and open a window */
void init(void)
{
  int dev, idx;
  const char *error;

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

  /* open a window */
  if((error = gfx_init()))
  {
    message(error);
    restore(1);
  }
}

const char *gfx_init(void)
{
  int i;

  if(!w_init())
    return "can't connect to wserver";

  if(!(Icon = w_create(BLK_W * 3, BLK_H * 3, ICON_PROPERTIES)))
    return "can't create icon";

  if(!(Images = w_create(BLOCKS_W, BLOCKS_H, 0)))
    return "can't create bitmap window";

  {
    BITMAP pieces;
    /* transmit the bitmaps to the bitmap window */
    memset(&pieces, 0, sizeof(pieces));
    pieces.type    = BM_PACKEDMONO;
    pieces.width   = BLOCKS_W;
    pieces.height  = BLOCKS_H;
    pieces.upl     = (BLOCKS_W + 31) / 32;
    pieces.unitsize= 4;
    pieces.planes  = 1;
    pieces.data    = (short*)Bitmaps;

    w_putblock(&pieces, Images, 0, 0);
  }
  /* setup icon (used images should be on the first row) */
  w_bitblk2(Images, I_HEAD * BLK_W, 0, BLK_W, BLK_H, Icon, 0, 0);
  w_bitblk2(Images, I_BG   * BLK_W, 0, BLK_W, BLK_H, Icon, 2*BLK_W, BLK_H);
  w_bitblk2(Images, I_HEAD * BLK_W, 0, BLK_W, BLK_H, Icon, 2*BLK_W, 2*BLK_H);
  for(i = 0; i < 2; i++)
  {
    w_bitblk2(Images, I_WHITE * BLK_W, 0, BLK_W, BLK_H, Icon, (i+1) * BLK_W, 0);
    w_bitblk2(Images, I_BG    * BLK_W, 0, BLK_W, BLK_H, Icon, i * BLK_W, BLK_H);
    w_bitblk2(Images, I_BLACK * BLK_W, 0, BLK_W, BLK_H, Icon, i * BLK_W, 2*BLK_H);
  }

  if(!(Win = w_create(WIN_W, WIN_H, WIN_PROPERTIES)))
    return "can't create window";

  w_settitle(Win, WIN_NAME);
  if(w_open(Win, UNDEF, UNDEF) < 0)
    return "can't open window";

  return 0;
}

/* wait and return a key (ASCII). no buffer clearing nor updates */
int get_key(void)
{
  /* wait for a key */
  return check_events(-1L);
}

/* input / message processing functions */

/* return device input */
int input(int dev)
{
  int key;

   /* manage game control input */
   check_events(0L);

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

/* for the level editor, returns only when mouse is down or key pressed... */
int get_position(int *x, int *y)
{
  short mx, my;
  int key = 0;
  WEVENT *ev;

  if((ev = w_queryevent(NULL, NULL, NULL, 100)))
    key = process_events(ev);

  if(MouseDown)
  {
    w_querymousepos(Win, &mx, &my);
    *x = mx / BLK_W;
    *y = my / BLK_H;
  }

  return key;
}

/* W event parsing, wait for timeout, key or exit */
static int check_events(long timeout)
{
  WEVENT *ev;
  int key;

  for(;;)
  {
    if(!(ev = w_queryevent(NULL, NULL, NULL, timeout)))
      return 0;
    key = process_events(ev);

    if(timeout < 0)			/* waiting for a key -> return it */
      return key;
    buffer_IO(key);
  }
}

static int process_events(WEVENT *ev)
{
  short x, y;

  switch(ev->type)
  {
    /* user pressed  a key */
    case EVENT_KEY:
      if((ev->key & 0xFF) == KEY_EXIT)
	restore(0);
	return ev->key & 0xFF;
      break;

    /* mouse button down */
    case EVENT_MPRESS:
      if(ev->win == Icon)
      {
	w_querywindowpos(Icon, 1, &x, &y);
	w_close(Icon);
	w_open(Win, x, y);
      }
      else
        MouseDown = 1;
      break;

    /* mouse button up */
    case EVENT_MRELEASE:
      if(ev->win == Win)
        MouseDown = 0;
      break;

    /* handle GUI gadgets */
    case EVENT_GADGET:
      switch(ev->key)
      {
	case GADGET_EXIT:
	case GADGET_CLOSE:
	  restore(0);

	case GADGET_ICON:
	  w_querywindowpos(Win, 1, &x, &y);
	  w_close(Win);
	  w_open(Icon, x, y);
	  break;
      }
  }
  return 0;
}

/* check IO status and fill buffers accordingly */
static void buffer_IO(int key)
{
  int dev = -1;

  switch(key)
  {
    case DEV0_UP:
      key = UP;
      dev = 0;
      break;
    case DEV0_DN:
      key = DOWN;
      dev = 0;
      break;
    case DEV0_LF:
      key = LEFT;
      dev = 0;
      break;
    case DEV0_RT:
      key = RIGHT;
      dev = 0;
      break;
    case DEV0_BUTTON:
      key = BUTTON;
      dev = 0;
      break;

    case DEV1_UP:
      key = UP;
      dev = 1;
      break;
    case DEV1_DN:
      key = DOWN;
      dev = 1;
      break;
    case DEV1_LF:
      key = LEFT;
      dev = 1;
      break;
    case DEV1_RT:
      key = RIGHT;
      dev = 1;
      break;
    case DEV1_BUTTON:
      key = BUTTON;
      dev = 1;
      break;

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

/* put the key into the device's IO buffer,
 * ensure that there are no repeats
 */
static void ToStack(int dev, int key)
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


/* map blocks into the window */
void bitmap(int x, int y, unsigned char obj)
{
  x *= BLK_W;
  y *= BLK_H;

  w_bitblk2(Images, (obj % BLK_ROW) * BLK_W, (obj / BLK_ROW) * BLK_H,
            BLK_W, BLK_H, Win, x, y);
}

/* draw the bitmap onto the screen */
void draw_map(const unsigned char *array)
{
  int x, y;

  /* draw it */
  for(x = 0; x < SCREEN_W; x++)
    for(y = 0; y < SCREEN_H; y++)
      bitmap(x, y, *(array++));
}

/* currently just prints the message, later could be and alert... */
void message(const char *msg)
{
  fprintf(stderr, "%s\n", msg);
}

/* -----------
 * OS dependent stuff
 */

#ifdef __MINT__

#include <osbind.h>		/* Vsync(), Dosound() */
#include "dosound.h"		/* soundchip effects */

/* manage IO buffers and wait for the screen refresh */
void wait_frame(void)
{
  Vsync();		/* sync with screen refresh to avoid flickering */
}

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

void restore(int sig)
{
  Dosound(snd_off);
  exit(sig);
}

#else	/* unix */

/* no sounds...*/
void do_sound(int sound)
{ }
void restore(int sig)
{
  exit(sig);
}

#include <sys/time.h>

void wait_frame(void)
{
  static long until_sec = 0, until_usec = 0;
  struct timezone tzp;
  struct timeval now;

  /* get time */
  gettimeofday(&now, &tzp);

  if(now.tv_sec < until_sec ||
    (now.tv_sec == until_sec && now.tv_usec < until_usec))
  {
    /* wait while for 'frame' time left */
    now.tv_usec = until_usec - now.tv_usec;
    now.tv_sec  = until_sec - now.tv_sec;
    if(now.tv_usec < 0)
    {
      now.tv_usec += 1000000L;
      now.tv_sec--;
    }
    select(0, NULL, NULL, NULL, &now);
  }
  gettimeofday(&now, &tzp);

  /* calc next frame */
  until_sec  = now.tv_sec;
  until_usec = now.tv_usec + (1000000L / FRAME_RATE);
  if(until_usec >= 1000000L)
  {
    until_usec -= 1000000L;
    until_sec++;
  }
}

#endif
