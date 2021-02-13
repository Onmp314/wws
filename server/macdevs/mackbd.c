/* a keyboard driver for MacMiNT, by Jonathan Oddie */

#define	ARGS_ON_STACK
#define	P_(X)		X
#define	EXITING		volatile
#define	NORETURN

 /* these are in ../xconout2 */
#include <mint/types.h>
#include <mint/file.h>
#include <mint/locore.h>
#include <mint/atarierr.h>

 /* these are standard includes */
#include <math.h>
#include <strings.h>
#include <stdio.h> /* for sprintf; the one from kerinfo didn't seem to work. */

/* Mac vital information */
#define SIG_SVAR 0x53564152 /* 'SVAR' */

#include <Events.h>

struct sysvars *sysvar;
void *saveda5 asm("saveda5");
void *savedsp asm("savedsp");

/* kernel information */
struct kerinfo *kernel;

#define CCONWS (void)(*kernel->dos_tab[9])
#define DCNTL (*kernel->dos_tab[0x130])
#define SPRINTF sprintf
#define KMALLOC (*kernel->kmalloc)
#define KFREE (*kernel->kfree)
#define WAKESELECT (*kernel->wakeselect)
  /* DEBUG_ON lets you see debugging output all the time, not just when */
  /* the debug level is turned up */
#ifdef DEBUG_ON 
#define TRACE CCONWS
#else
#define TRACE (*kernel->trace)
#endif

/* globals */
short opened;  /* is the device open */
long selected; /* which pid has it selected, 0=none */

void (*old_timer)(void);

#define BUFSIZE 128

short bstart=0,bend=0;
char kbuf[BUFSIZE+2]; /* +2 gives a bit of slack of processing overflows */
  /* debugging messages */
char msg[128];

/* prototypes */
  /* called at startup */
DEVDRV *main(struct kerinfo *);

  /* device driver functions */
long  kbd_open  (FILEPTR *);
long  kbd_write  (FILEPTR *, char *, long);
long  kbd_read  (FILEPTR *, char *, long);
long  kbd_lseek  (FILEPTR *, long, int);
long  kbd_ioctl  (FILEPTR *, int, void *);
long  kbd_datime  (FILEPTR *, int *, int);
long  kbd_close  (FILEPTR *, int);
long  kbd_select  (FILEPTR *, long, int);
void  kbd_unselect  (FILEPTR *, long , int );
  
void   kbd_timer  (void);

/* OS structures */
DEVDRV kbd_device = {
  kbd_open, kbd_write, kbd_read, kbd_lseek, kbd_ioctl,
  kbd_datime, kbd_close, kbd_select, kbd_unselect
};

struct dev_descr kbd_devinfo = {&kbd_device,0,0,(struct tty *)0,sizeof(kbd_device)};

/**
 * Initial setup, return the device driver to the OS
**/
DEVDRV *
main(struct kerinfo *k)
{
  long *cookie;

  /* get kernel goodies */
  kernel = k;
  
  CCONWS("/dev/mackbd -- MacMiNT keyboard driver (v0.2)\r\n");
  CCONWS("(C) and GPL 04/99 Jonathan Oddie.\r\n\r\n");

  /* get sysvar struct, from SVAR cookie */
  cookie = *(long **)0x5a0L;
  while (cookie[0]) {
    if (cookie[0] == SIG_SVAR)  
      sysvar = (struct sysvars *)cookie[1];
    cookie += 2;
  }
  if (sysvar == NULL) {
    CCONWS("can't find SVAR cookie\r\n");
    return (DEVDRV *)EACCDN;
  }
  saveda5 = sysvar->saveda5;
  savedsp = sysvar->savedsp;

  DCNTL(DEV_INSTALL, "U:\\dev\\mackbd", (long)&kbd_devinfo);
  return &kbd_device;
}

long kbd_open(FILEPTR *f)
{
  TRACE("kbd_open\r\n");
  if(!opened) {
    opened = 1;
    bstart = bend = 0;

    /* install new timer routine */
    old_timer = sysvar->timer;
    sysvar->timer = kbd_timer;
    SPRINTF(msg,"old timer at %p, new timer at %p\r\n",old_timer,kbd_timer);
    TRACE(msg);

    return 0;
  } else {
    TRACE("kbd_open: already open!\r\n");
    return EACCDN; /* probably not the best error code */
  }
}

long kbd_lseek(FILEPTR *f, long where, int whence)
{
  TRACE("kbd_lseek\r\n");
  return 0;
}

long kbd_datime(FILEPTR *f, int *timeptr, int rwflag)
{
  TRACE("kbd_datime\r\n");
  return 0;
}

long kbd_close (FILEPTR *f, int pid)
{
  TRACE("kbd_close\r\n");
  if(f->links <= 0) {
    TRACE("kbd_close: really closing\r\n");
    if(opened) {
      opened = 0;
      /* restore old timer routine */
      sysvar->timer = old_timer;

      return 0;
    } else {
      TRACE("kbd_close:not open!\r\n");
      return EACCDN;  /* hmmm... */
    }
  }
  return 0;
}

long kbd_select (FILEPTR *f, long proc, int mode)
{
  TRACE("kbd_select\r\n");
  if(!selected) {
  /* this process is obviously misguided :) */
    if(mode==O_WRONLY) {
      TRACE("kbd_select for O_WRONLY\r\n");
      return 1;
    }
    if(bstart-bend) {
      TRACE("kbd_select:returning 1\r\n");
      return 1;
    }
    selected = proc;
    return 0;
  } else {
    TRACE("kbd_select:already selected\r\n");
    return 2; /* selected!=0, someone is already selecting us */
  }
}

void kbd_unselect (FILEPTR *f, long proc, int mode)
{
  TRACE("kbd_unselect\r\n");
  if(selected && (selected==proc)) {
    TRACE("kbd_unselect:setting selected=0\r\n");
    selected = 0;
  }
  return;
}

long kbd_read  (FILEPTR *f, char *buf, long bytes)
{
  SPRINTF(msg,"kbd_read: bytes=%i\r\n",(int)bytes);
  TRACE(msg);
  if(!(bend-bstart)) {
    TRACE("kbd_read: nothing to read!\r\n");
    return 0;
  }
  
  if(bytes>(bend-bstart)) bytes=bend-bstart;

  /* copy the stuff over */
  memcpy(buf,&kbuf[bstart],bytes);

  /* update bstart & bend */
  if(bytes==bend-bstart)
    bstart = bend = 0;
  else
    bstart += bytes;
  SPRINTF(msg,"kbd_read:bstart=%i; bend=%i\r\n",bstart,bend);
  TRACE(msg);

  return bytes;
}

long kbd_write (FILEPTR *f, char *buf, long bytes)
{
  TRACE("kbd_write\r\n");
  return EINVFN;
}

long kbd_ioctl (FILEPTR *f, int mode, void *buf)
{
  TRACE("kbd_ioctl\r\n");
  if(mode==FIONREAD) {
    SPRINTF(msg,"kbd_ioctl:FIONREAD:result=%i\r\n",bend-bstart);
    TRACE(msg);
    *(long *)buf = bend-bstart;
  } else if(mode==FIONWRITE) {
    TRACE("kbd_ioctl:FIONWRITE:result=0\r\n");
    *(long *)buf = 0;
  } else
    return EINVFN;
  return 0;
}

void kbd_timer(void)
{
	EventRecord theEvent;
	Boolean gotEvent;
	char key;

	TRACE("kbd_timer\r\n");
	gotEvent = GetNextEvent(keyDownMask | autoKeyMask,&theEvent);
	if(gotEvent) {
		key = theEvent.message & charCodeMask;
		kbuf[bend++] = key;
		if(bend>BUFSIZE) bstart = bend = 0;
		if(selected) WAKESELECT(selected);
	}
	(*old_timer)();
}  
