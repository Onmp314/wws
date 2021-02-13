/* a mouse driver for MacMiNT, by Jonathan Oddie */

#define	ARGS_ON_STACK
#define	P_(X)		X
#define	EXITING		volatile
#define	NORETURN

 /* these are in ../xconout2/ */
#include <mint/types.h>
#include <mint/file.h>
#include <mint/locore.h>
#include <mint/atarierr.h>
 /* these are standard includes */
#include <math.h>
#include <strings.h>
#include <stdio.h> /* for sprintf; the one from kerninfo didn't seem to work. */

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

/* this is unsafe but quicker version of max macro. I only use it for comparing integer */
/* variables anyway (not anything like max(x++,y++) ) */
#if defined(max)
#undef max
#endif
#define max(x,y) (x > y ? x : y)

/* globals */
short opened;  /* is the device open */
long selected; /* which pid has it selected, 0=none */

short timerflag = 0; 
void (*old_timer)(void); /* old timer handler */

  /* how many events to keep in buffer * (sizeof a mouse event==3) */
#define BUFSIZE 256*3

short mstart=0,mend=0;
char mbuf[BUFSIZE + 3]; /* the +3 gives a bit of slack to make processing overflows easier */
short oldx=0,oldy=0,oldb=0;
  /* debugging messages */
char msg[128];

/* prototypes */
  /* called at startup */
DEVDRV *main(struct kerinfo *);

  /* device driver functions */
long  mouse_open  (FILEPTR *);
long  mouse_write  (FILEPTR *, char *, long);
long  mouse_read  (FILEPTR *, char *, long);
long  mouse_lseek  (FILEPTR *, long, int);
long  mouse_ioctl  (FILEPTR *, int, void *);
long  mouse_datime  (FILEPTR *, int *, int);
long  mouse_close  (FILEPTR *, int);
long  mouse_select  (FILEPTR *, long, int);
void  mouse_unselect  (FILEPTR *, long , int );
  
  /* timer function */
void   mouse_timer  (void);

  /* utility function, compresses all mouse shifts in the buffer into one event */
void stuffevents(char ret[3]);

/* OS structures */
DEVDRV mouse_device = {
  mouse_open, mouse_write, mouse_read, mouse_lseek, mouse_ioctl,
  mouse_datime, mouse_close, mouse_select, mouse_unselect
};

struct dev_descr mouse_devinfo = {&mouse_device,0,0,(struct tty *)0,sizeof(mouse_device)};

/**
 * Initial setup, return the device driver to the OS
**/
DEVDRV *
main(struct kerinfo *k)
{
  long *cookie;

  /* get kernel goodies */
  kernel = k;
  
  CCONWS("/dev/macmouse -- MacMiNT mouse driver\r\n");
  CCONWS("(C) and GPL 04/99 Jonathan Oddie.\r\n\r\n");

  /* get sysvar struct, from SVAR cookie */
  /* isn't there some easier way to do this?? */
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

  SPRINTF(msg,"mbuf at %p\r\n",mbuf);
  TRACE(msg);

  DCNTL(DEV_INSTALL, "U:\\dev\\macmouse", (long)&mouse_devinfo);
  return &mouse_device;
}

long mouse_open(FILEPTR *f)
{
  TRACE("mouse_open\r\n");
  if(!opened) {
    opened = 1;

    /* initialize some stuff */
    oldx = oldy = 0;
    mstart = mend = 0;

    /* install timer routine */
    old_timer = sysvar->timer;
    sysvar->timer = mouse_timer;
    SPRINTF(msg,"old_timer=%p, mouse_timer=%p\r\n",old_timer,mouse_timer);
    TRACE(msg);

    return 0;
  } else {
    TRACE("mouse_open: already open!\r\n");
    return EACCDN; /* probably not the best error code */
  }
}

long mouse_lseek(FILEPTR *f, long where, int whence)
{
  TRACE("mouse_lseek\r\n");
  return 0;
}

long mouse_datime(FILEPTR *f, int *timeptr, int rwflag)
{
  TRACE("mouse_datime\r\n");
  return 0;
}

long mouse_close (FILEPTR *f, int pid)
{
  TRACE("mouse_close\r\n");
  if(f->links <= 0) {
    TRACE("mouse_close: really closing\r\n");
    if(opened) {
      opened = 0;
      /* restore old timer handler */
      sysvar->timer = old_timer;

      return 0;
    } else {
      TRACE("mouse_close:not open!\r\n");
      return EACCDN;  /* hmmm... */
    }
  }
  return 0;
}

long mouse_select (FILEPTR *f, long proc, int mode)
{
  TRACE("mouse_select\r\n");
  if(!selected) {
  /* this process is obviously misguided :) */
    if(mode==O_WRONLY) {
      TRACE("mouse_select for O_WRONLY\r\n");
      return 1;
    }
    if(mstart-mend) {
      TRACE("mouse_select:returning 1\r\n");
      return 1;
    }
    selected = proc;
    return 0;
  } else {
    TRACE("mouse_select:already selected\r\n");
    return 2; /* selected!=0, someone is already selecting us */
  }
}

void mouse_unselect (FILEPTR *f, long proc, int mode)
{
  TRACE("mouse_unselect\r\n");
  if(selected && (selected==proc)) {
    TRACE("mouse_unselect:setting selected=0\r\n");
    selected = 0;
  }
  return;
}

long mouse_read  (FILEPTR *f, char *buf, long bytes)
{
  SPRINTF(msg,"mouse_read: bytes=%i\r\n",(int)bytes);
  TRACE(msg);
  if(!(mend-mstart)) {
    TRACE("mouse_read: nothing to read!\r\n");
    return 0;
  }
  
  /* make sure bytes is a reasonable value */
  if(bytes > (mend-mstart)) bytes = mend-mstart;
  if(bytes%3 != 0) bytes=floor(bytes/3)*3;
  SPRINTF(msg,"mouse_read: bytes fixed = %i\r\n",(int)bytes);
  TRACE(msg);
  
  /* copy the stuff over */
  memcpy(buf,&mbuf[mstart],bytes);

  /* update mstart & mend */
  if(bytes==mend-mstart)
    mstart = mend = 0;
  else
    mstart = mstart + bytes;
  SPRINTF(msg,"mouse_read:mstart=%i; mend=%i\r\n",mstart,mend);
  TRACE(msg);

  return bytes;
}

long mouse_write (FILEPTR *f, char *buf, long bytes)
{
  TRACE("mouse_write\r\n");
  return EINVFN;
}

long mouse_ioctl (FILEPTR *f, int mode, void *buf)
{
  TRACE("mouse_ioctl\r\n");
  if(mode==FIONREAD) {
    SPRINTF(msg,"mouse_ioctl:FIONREAD:result=%i\r\n",mend-mstart);
    TRACE(msg);
    *(long *)buf = mend-mstart;
  } else if(mode==FIONWRITE) {
    TRACE("mouse_ioctl:FIONWRITE:result=0\r\n");
    *(long *)buf = 0;
  } else
    return EINVFN;
  return 0;
}  

/* tiemr routine */

void mouse_timer(void)
{
  Point p;
  Boolean b;
  KeyMap keys;

  char *mevent;
  char temp[3]; 

  int dx,dy;
  int nEvents;
  int i;

  /* conversion table: converts a 0 (no buttons), 1 (left), or 2 (right) to 
      a sun format packet */
  static char cnvrt[3] = {0x87,0x83,0x86};

  TRACE("mouse_timer\r\n");
  GetMouse(&p);
  b=Button();
  GetKeys(keys);
  if(b&&(keys[1]&0x1)) b=2; /* if shift down, fake a right mouse button */
  SPRINTF(msg,"mouse_timer: p.h=%i; p.v=%i; b=%i\r\n",p.h,p.v,b);
  TRACE(msg);
  dx = p.h - oldx;
  dy = oldy - p.v;
  b = cnvrt[b];
  if(dx||dy||(b!=oldb)) {
    mevent = &mbuf[mend];
    if((dx<127)&&(dx>-127)&&(dy<127)&&(dy>-127)) {
      *mevent++ = b;
      *mevent++ = dx;
      *mevent = dy;
      mend += 3;
      if(mend >= BUFSIZE) { /* aack!! buffer overflow! */
        TRACE("mouse_timer: buffer overflow!\r\n");
        stuffevents(temp);
        memcpy(mbuf,temp,3);
        mstart = 0;
        mend = 3;
      } else {
        SPRINTF(msg,"mouse_timer:mend=%i\r\n",mend);
        TRACE(msg);
      }
    } else {
      /* shoot, these deltas won't fit into a char. */
      /* we have to split them up into several events. */
      SPRINTF(msg,"mouse_timer: big deltas: dx=%i, dy=%i\r\n",dx,dy);
      TRACE(msg);
      nEvents = max(abs(dx),abs(dy)) / 127 + 1;
      SPRINTF(msg,"mouse_timer: nevents=%i\r\n",nEvents);
      TRACE(msg);
      if((mend+nEvents*3)>BUFSIZE) {
        TRACE("mouse_timer: buffer overflow\r\n");
        /* aargh! buffer overflow! */
        stuffevents(temp);
        memcpy(mbuf,temp,3);
        mstart = 0;
        mend = 3;
      } 
	/* clear garbage out of the way */
      for(i=mend;i<=mend+(nEvents*3);i++) mbuf[i]=0;

      if((dx>127)||(dx<-127)) {
        mevent = &mbuf[mend];
        if(dx>127) {
          while(dx>127) {
            *mevent++ = b;
            *mevent++ = 127;
            mevent++;
            dx -= 127;
            SPRINTF(msg,"writing dx=127 event at %p:%x,%i,%i\r\n",mevent-3,*(mevent-3),*(mevent-2),*(mevent-1));
            TRACE(msg);
          }
        } else { /* dx<-127 */
          while(dx<-127) {
            *mevent++ = b;
            *mevent++ = -127;
            mevent++;
            dx += 127;
            SPRINTF(msg,"writing dx=-127 event at %p:%x,%i,%i\r\n",mevent-3,*(mevent-3),*(mevent-2),*(mevent-1));
            TRACE(msg);
          }
        }
        *mevent++ = b;
        *mevent = dx;
        SPRINTF(msg,"writing dx event at %p:%x,%i,%i\r\n",mevent-1,*(mevent-1),*mevent,*(mevent+1));
        TRACE(msg);
      } else {
        mevent = &mbuf[mend];
        *mevent++ = b;
        *mevent = dx;
      }

      if((dy>127)||(dy<-127)) {
        mevent = &mbuf[mend];
        if(dy>127) {
          while(dy>127) {
            *mevent++ = b;
            mevent++;
            *mevent++ = 127;
            dy -= 127;
            SPRINTF(msg,"writing dy=127 event at %p:%x,%i,%i\r\n",mevent-3,*(mevent-3),*(mevent-2),*(mevent-1));
            TRACE(msg);
          }
        } else { /* dy<-127 */
          while(dy<-127) {
            *mevent++ = b;
            mevent++;
            *mevent++ = -127;
            dy += 127;
            SPRINTF(msg,"writing dy=-127 event at %p:%x,%i,%i\r\n",mevent-3,*(mevent-3),*(mevent-2),*(mevent-1));
            TRACE(msg);
          }
        }
        *mevent++ = b;
        mevent++;
        *mevent++ = dy;
        SPRINTF(msg,"writing dy event at %p:%x,%i,%i\r\n",mevent-3,*(mevent-3),*(mevent-2),*(mevent-1));
        TRACE(msg);
      } else {
        mevent = &mbuf[mend];
        *mevent++ = b;
        mevent++;
        *mevent = dy;
      }
      mend += nEvents*3;
      /* if you've managed to create another buffer overflow at this */
      /* point, I envy your monitor size :) */
    }
    oldx = p.h;
    oldy = p.v;
    oldb = b;
    if(selected)
      WAKESELECT(selected);
    }
  (*old_timer)();
}

/* utility function */
/* since we don't want to lose any events, this compresses all the events in the buffer */
/* into one (only mouse moves, doesn't care about clicks) */

void stuffevents(char ret[3]) {
  char *mevent;

  ret[0] = ret[1] = ret[2] = 0;

  TRACE("stuffevents\r\n");
  mevent = &mbuf[mstart];
  while(mevent < &mbuf[mend]) {
    ret[1] += mevent[1];
    ret[2] += mevent[2];
    mevent += 3;
  }
  SPRINTF(msg,"stuffevents:compressed to %x,%i,%i\r\n",ret[0],ret[1],ret[2]);
  TRACE(msg);
}
