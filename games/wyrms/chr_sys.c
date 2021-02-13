/* curses specific functions for Wyrms */

#include <curses.h>
/* other includes */
#include <stdlib.h>
#include <stdio.h>		/* IO stuff */
#include <signal.h>		/* signal stuff */
#include <unistd.h>		/* usleep() */
#include <sys/types.h>		/* FD_* stuff */
#include <sys/time.h>		/* for gettimeofday() */
#define __SYSTEM_C__
#include "wyrms.h"		/* misc info & prototypes */

/* for ring-buffered input */
typedef struct NEXT
{
  int item;			/* input character */
  struct NEXT *next;		/* next char in the ring */
} Next;

#define DEVBUF_SIZE	8

/* if the structure IO input happens in the background thread,
 * you'll probably need a lock for accessing the structure.
 */
typedef struct
{
  int used;
  Next *in;
  Next *out;
  Next buf[DEVBUF_SIZE];
} Buf;

/* buffers for 'device' input */
Buf dev_buf[DEVICES];


/* Systen specific functions */
static int CheckKey(void);
static void ManageBuffer(void);

/* initialize system */
void init(void)
{
  int dev, idx;

  puts("\nCurses-Wyrms " VERSION " (C) 1995 by Eero Tamminen\n");
  puts("A two player worm duel for the text terminals.");

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

  /* initialize curses */
  initscr();
  cbreak();
  noecho();

  /* in a case of an exiting request, restore... */
  signal(SIGHUP, restore);
  signal(SIGINT, restore);
  signal(SIGQUIT, restore);
  signal(SIGTERM, restore);
}

/* restore system variables etc. */
void restore(int sig)
{
  endwin();
  if(sig)
    puts("Interrupted (use 'q' for quit).");
  exit(0);
}

/* make soundeffects */
void do_sound(int sound)
{
}

/* wait until next 'frame' */
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

/* wait and return a key (ASCII) */
int get_key(void)
{
  refresh();
  return getch();
}

/* return device input */
int input(int dev)
{
  int key;

  /* flush screen */
  refresh();

  /* check keys */
  ManageBuffer();

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

/* return postion... */
int get_position(int *x, int *y)
{
  int key;

  refresh();
  key = getch();

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

/* check for a key, if available buffer it */
static int CheckKey(void)
{
  fd_set in;
  struct timeval tv;
  char key = 0;

  FD_ZERO(&in);
  FD_SET(0, &in);			/* input from stdio */
  tv.tv_sec = tv.tv_usec = 0;		/* no timeout */
  select(1, &in, NULL, NULL, &tv);	/* check input */
  if(FD_ISSET(0, &in))			/* yes: */
    read(0, &key, 1);

  if(key == 'q' || key == 'Q')		/* quit */
    restore(0);

  return key;
}

/* map keys into control values and buffer them */
static void ManageBuffer(void)
{
  int dev = -1, key = 0;

  /* push the key into corresponding ring buffer */
  switch(CheckKey())
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
  {
    dev_buf[dev].in->item = key;
    dev_buf[dev].in = dev_buf[dev].in->next;
    if(dev_buf[dev].out == dev_buf[dev].in)
      dev_buf[dev].out = dev_buf[dev].out->next;
    else
      dev_buf[dev].used++;
  }
}

/* map blocks onto the window and redraw backup pixmap */
void bitmap(int x, int y, unsigned char obj)
{
  if(obj >= BLOCKS)
    obj = 0;
  mvaddch(y, x, cmap[(int)obj]);
}

/* draw the whole map */
void draw_map(unsigned const char *array)
{
  int x, y;

  for(x = 0; x < SCREEN_W; x++)
    for(y = 0; y < SCREEN_H; y++)
      bitmap(x, y, *(array++));
}

/* a message... */
void message(const char *string)
{
  mvaddstr(INFOLINE, 0, string);
}
