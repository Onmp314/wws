/*
 * mac.c, a part of the W Window System
 *
 * Copyright (C) 1998-2000 by Jonathan Oddie, based on mint.c:
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a graphic driver for MiNT on Mac (better known as MacMiNT)
 *
 * CHANGES
 * ++eero 5/98:
 * - added support for BMONO and DIRECT8 drivers
 * ++eero 12/98:
 * - moved linux specific code to linux.c
 * ++oddie 2/99:
 * - adapted it to macmint
 * ++oddie 6/99:
 * - you no longer have to force quit JET when wserver exits. This is a 
 *   Good Thing! (see macmint_exit() and ../main.c)
 * ++oddie 6/00:
 * - rewrote macPutCmap() to use a `clut' variable and Toolbox SetEntries() 
 *   function for palette setting, so DIRECT8 actually works now.
 */

#if !defined(__MINT__) || !defined(MAC)
#error "this is a *MacMiNT* graphics driver"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "../config.h"
#include "../types.h"
#include "gproto.h"
#include "gstructs.h"
#include "backend.h"

/* mac includes */
#include <Windows.h> /* #includes <Quickdraw.h> for us */

/* some prototypes */
void hidembar(short hsize,short vsize);
void showmbar(void);
short hasColor(void);
void macmint_exit(void);

/* in cookie.c */
void cookieinit(void);
void cookiesetup(void);
void cookiecleanup(void);

/* we draw a window over the whole screen so that no mouse clicks get sent to other people */
static WindowPtr macWindow;
/* these two are used for hiding the menubar */
static RgnHandle Gray, OldGrayRgn;

#ifdef DIRECT8
/* this is used for manipulating the device's CLUT (color lookup table) on color macs */
static ColorSpec *clut;
/* this saves the original CLUT and restores it at the end */
static ColorSpec *oldClut;

/* !!! this should be same as the define in ../color.c !!! */
#define TSTBIT(ptr,bit) (ptr[(bit)>>3] & 0x80>>((bit)&7))

/* prototype here */
static short macPutCmap(COLORTABLE *colTab, short index);

/*
 * set palette, in DIRECT8 mode
 */
static short macPutCmap(COLORTABLE *colTab, short index)
{
  uchar *used, *r, *g, *b;
  short colors,ci;
	QDErr err;

	if(!clut) { fprintf(stderr,"macPutCmap:no clut variable!\n"); return -1; }

  r = colTab->red; g = colTab->green; b = colTab->blue;
  if (index >= 0) {
    r += index; g += index; b += index;
    clut[index].rgb.red = *r<<8; clut[index].rgb.green = *g<<8; clut[index].rgb.blue = *b<<8;
    cookiesetup();
		SetEntries(index,0,clut); /* 'count' parameter is zero-based */
		err=QDError();
    cookiecleanup();
		if(err!=noErr) {fprintf(stderr,"macPutCmap: QDError=%d\n",err); return -1; }
    return 0;
  }

  used = colTab->used;
  colors = colTab->colors;
	ci = 0;
  for (index = 0; index < colors; index++) {
    if (TSTBIT(used, index)) {
	    clut[ci].rgb.red = *r<<8; clut[ci].rgb.green = *g<<8; clut[ci].rgb.blue = *b<<8;
			clut[ci].value = index;
			ci++;
    }
    r++;  g++;  b++;
  }
	cookiesetup();
	SetEntries(-1,ci-1,clut);
	err=QDError();
	cookiecleanup();
	if(err!=noErr) {fprintf(stderr,"macPutCmap: QDError=%d\n",err); return -1; }
  return 0;
}

#endif /* #ifdef DIRECT8 */

void hidembar(short hsize,short vsize)
{
 Rect r;

 Gray=*(RgnHandle *)0x9EE;                   /* Gets gray region of desktop */
 OldGrayRgn=NewRgn();                
 CopyRgn(Gray,OldGrayRgn);        
 SetRectRgn(Gray,0,0,hsize,vsize);
 r.top=0; r.left=0; r.bottom=vsize; r.right=hsize;
 if(hasColor()) 
  macWindow=NewCWindow(NULL,&r,"\pwserver",true,2,(WindowPtr)-1L,false,0L);
 else
  macWindow=NewWindow(NULL,&r,"\pwserver",true,2,(WindowPtr)-1L,false,0L);
}

void showmbar(void)
{
 DisposeWindow(macWindow);
 CopyRgn(OldGrayRgn,Gray);
 DisposeRgn(OldGrayRgn);
 DrawMenuBar();
}

short hasColor(void)
{
 SysEnvRec theEnvirons;
 short hasColorQD = false;

 if(SysEnvirons(curSysEnvVers,&theEnvirons) == noErr)
  hasColorQD = theEnvirons.hasColorQD;

 return hasColorQD;
}


/*
 * the real init function
 */

SCREEN *macmint_init (int forceMono)
{
  BITMAP bm;
  short hsize,vsize;
  short rowBytes;
  short pixSize;

  short forceBmono = 0; /* for debugging purposes */

  GrafPtr macscr;
  CGrafPtr macColorscr;

  cookieinit();
  cookiesetup();

  /* MacMiNT uses devices of it's own from the ../macdevs/ subdirectory */
  if (unix_input_init("/dev/mackbd", "/dev/macmouse")) {
    return NULL;
  }
	
  GetWMgrPort(&macscr);
  hsize = macscr->portBits.bounds.right;
  vsize = macscr->portBits.bounds.bottom;
  rowBytes = macscr->portBits.rowBytes & 0x3FFF; /* mask off two flag bits */

  if(hasColor()) {
   GetCWMgrPort(&macColorscr);
   pixSize = (*macColorscr->portPixMap)->pixelSize;
  } else {
   pixSize = 1;
  }

  HideCursor();
  hidembar(hsize,vsize); 
  cookiecleanup();

  printf("screen is %d x %d, %d colors, row offset %x\n",hsize,vsize,1<<pixSize,rowBytes);

  bm.width=hsize;
  bm.height=vsize;
  bm.data=macscr->portBits.baseAddr;
  bm.planes=pixSize;

#if defined(BMONO) || defined(PMONO)
  if ((bm.planes == 1)||(forceMono)) {
    bm.type = BM_PACKEDMONO;
    bm.unitsize = 4;
    bm.upl = rowBytes >> 2;
    if ((bm.width & 31)||(forceBmono)) {
#ifdef BMONO
      printf("Mac monochrome (BMONO driver) %d x %d\n",bm.width,bm.height);
      bmono_screen.bm = bm;
      return(&bmono_screen);
#endif
    } else {
#ifdef PMONO
      printf("Mac monochrome (PMONO driver) %d x %d\n",bm.width,bm.height);
      packed_mono_screen.bm = bm;
      return(&packed_mono_screen);
#endif
    }
  }
#endif /* defined(PMONO) || defined(BMONO) */

#ifdef DIRECT8
  if(bm.planes == 8) {
		short i;

    printf("Mac color (DIRECT8 driver), %d x %d\r\n",bm.width,bm.height);
    bm.type	= BM_DIRECT8;
    bm.upl	= rowBytes;
    bm.unitsize	= 1;
		clut=calloc(256,sizeof(ColorSpec));
		oldClut=calloc(256,sizeof(ColorSpec));
		memcpy(oldClut,
			(**((**(((CGrafPtr)macWindow)->portPixMap)).pmTable)).ctTable, /* i hate handles... */
			256*sizeof(ColorSpec));
    cookiesetup();
			/* This is probably not necessary but it doesn't do any harm, and if it ain't broke I
				don't fix it...*/
		for(i=0; i <= 255;i++) {
			ProtectEntry(i,false);
			ReserveEntry(i,false);
		}
    cookiecleanup();

    direct8_screen.changePalette = macPutCmap;
    direct8_screen.bm = bm;
    return &direct8_screen;
  }
#endif
  fprintf(stderr, "fatal: you didn't configure graphic driver for this mode?!\r\n");

  return NULL;
}


/* called from ../main.c when server shuts down */
void macmint_exit(void)
{
  unix_input_exit();
	
  cookiesetup();
  showmbar();
  ShowCursor();
#ifdef DIRECT8
  SetEntries(0,255,oldClut);
  free(oldClut);
  free(clut);
#endif
  cookiecleanup();
}

