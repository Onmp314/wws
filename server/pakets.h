/*
 * pakets.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- definitions of packet IDs and data structures
 *
 * NOTES
 * - Each paket is padded to have as size a multiple of 4 and that fields
 * are layed out so that each data type is on its appropriate boundary.
 *
 * TODO
 * Hmm...  Should these IDs be grouped so that loop.c could check the
 * window handles and window type instead of every bloody function doing it
 * itself?  There would then be three paket types:  graphics operations
 * (window, not container), window operations and others (client
 * operations).  ++eero
 */

#ifndef __W_PAKETS_H
#define __W_PAKETS_H

/*
 *	paket types
 */

/* no return code */

#define	PAK_NULL	0x0000
#define	PAK_STITLE	0x0100
#define	PAK_PLOT	0x0101
#define	PAK_LINE	0x0102
#define	PAK_HLINE	0x0103
#define	PAK_VLINE	0x0104
#define	PAK_BOX		0x0105
#define	PAK_PBOX	0x0106
#define	PAK_BITBLK	0x0107
#define	PAK_BITBLK2	0x0108
#define	PAK_VSCROLL	0x0109
#define	PAK_PRINTC	0x010a
#define	PAK_PRINTS	0x010b
#define	PAK_CIRCLE	0x010c
#define	PAK_PCIRCLE	0x010d
#define	PAK_DBOX	0x010e
#define	PAK_RAWDATA	0x010f
#define	PAK_BEEP	0x0110
#define	PAK_SMODE	0x0111
#define	PAK_SFONT	0x0112
#define	PAK_SPATTERN	0x0113
#define	PAK_STEXTSTYLE	0x0114

#define PAK_DPLOT	0x0115
#define PAK_DLINE	0x0116
#define PAK_DHLINE	0x0117
#define PAK_DVLINE	0x0118
#define PAK_DPBOX	0x0119
#define PAK_DCIRCLE	0x011a
#define PAK_DPCIRCLE	0x011b
#define PAK_POLY	0x011c
#define PAK_PPOLY	0x011d
#define PAK_DPOLY	0x011e
#define PAK_DPPOLY	0x011f

#define PAK_ALLOCCOL	0x0120
#define PAK_FREECOL	0x0121
#define PAK_CHANGECOL	0x0122
#define PAK_SETFGCOL	0x0123
#define PAK_SETBGCOL	0x0124
#define PAK_GETCOL	0x0125

#define PAK_ELLIPSE	0x0126
#define PAK_DELLIPSE	0x0127
#define PAK_PELLIPSE	0x0128
#define PAK_DPELLIPSE	0x0129
#define PAK_BEZIER	0x012a
#define PAK_DBEZIER	0x012b
#define PAK_PIE		0x012c
#define PAK_DPIE	0x012d
#define PAK_ARC		0x012e
#define PAK_DARC	0x012f

#define PAK_SLINEWIDTH		0x0130
#define PAK_SPATTERNDATA	0x0131

#define PAK_SMOUSE	0x0132

/* with return code */

#define	PAK_NULLR	0x0001
#define	PAK_INIT	0x0200
#define	PAK_EXIT	0x0201
#define	PAK_CREATE	0x0202
#define	PAK_OPEN	0x0203
#define	PAK_MOVE	0x0204
#define	PAK_CLOSE	0x0205
#define	PAK_DELETE	0x0206
#define	PAK_LOADFONT	0x0207
#define	PAK_UNLOADFONT	0x0208
#define	PAK_QWINSZ	0x0209
#define	PAK_TEST	0x020a
#define	PAK_QMPOS	0x020b
#define	PAK_QWPOS	0x020c
#define	PAK_QSTATUS	0x020d
#define	PAK_PUTBLKREQ	0x020e
#define	PAK_SSAVER	0x020f
#define	PAK_GETBLKREQ	0x0210
#define	PAK_GETBLKDATA	0x0211
#define	PAK_CREATE2	0x0212
#define PAK_RESIZE	0x0213

#define PAK_GMOUSE	0x0214

/* return codes */

#define	PAK_INITRET	0x0300
#define	PAK_LRET	0x0301
#define	PAK_SRET	0x0302
#define	PAK_S2RET	0x0303
#define	PAK_S3RET	0x0304
#define	PAK_RSTATUS	0x0305
#define PAK_LFONTRET	0x0306


/* server only pakets */

#define	PAK_EVENT	0x0400


/*
 * general layout of pakets:
 *
 * offset size contents
 * ------------------------
 *   0     2   length of pakets in bytes, always a multiple of four
 *   2     2   type of paket
 *   4     ?   paket data
 */

typedef struct {
  short len, type;
  uchar data[4000];
} PAKET;


/*
 * window related stuff
 */

typedef struct {
  short len, type;
  long uid;	/* long just in case uid_t is on some platform > short */
} INITP;

typedef struct {
  short len, type;
} EXITP;

typedef struct {
  short len, type;
  short width, height;
  ushort flags;
  ushort handle;
  WWIN *libPtr;
} CREATEP;		/* as well as CREATE2 */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, res;
} OPENP;

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, res;
} MOVEP;

typedef struct {
  short len, type;
  ushort handle;
  short width, height, res;
} RESIZEP;

typedef struct {
  short len, type;
  ushort handle;
  short res;
} CLOSEP;

typedef struct {
  short len, type;
  ushort handle;
  short res;
} DELETEP;

typedef struct {
  short len, type;
  short size;
  ushort styles;
  char family[MAXFAMILYNAME];
} LOADFONTP;

typedef struct {
  short len, type;
  ushort handle;
  short fonthandle;
} UNLOADFONTP;

typedef struct {
  short len, type;
  ushort handle;
  short effective;
} QWINSZP;

typedef struct {
  short len, type;
  ushort handle;
  ushort mode;
} SMODEP;		/* as well as SLINEWIDTH */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, res;
} TESTP;

typedef struct {
  short len, type;
  ushort handle;
  short fonthandle;
} SFONTP;

typedef struct {
  short len, type;
  ushort handle;
  ushort flags;
} STEXTSTYLEP;

typedef struct {
  short len, type;
  ushort handle;
  short res;
} QMPOSP;

typedef struct {
  short len, type;
  ushort handle;
  short effective;
} QWPOSP;

typedef struct {
  short len, type;
  short index, res;
} QSTATUSP;

typedef struct {
  short len, type;
  ushort handle, pattern;
} SPATTERNP;

typedef struct {
  short len, type;
  ushort handle;
  ushort data[16];
  short res;
} SPATTERNDATAP;

typedef struct {
  short len, type;
  short width, height, x1, y1;
  ushort handle, res;
  long shmKey;
} PUTBLKREQP;

typedef struct {
  short len, type;
  short seconds, res;
} SSAVERP;

typedef struct {
  short	len, type;
  ushort handle, res;
  short	x0, y0, width, height;
  long shmKey;
} GETBLKREQP;

typedef struct {
  short len, type;
} GETBLKDATAP;

typedef struct {
  short len, type;
  ushort handle;
} GMOUSEP;

/*
 * some return pakets
 */

typedef struct {
  short	len, type;
  short	vmaj, vmin;
  short pl, screenType;
  short	width, height;
  short planes;
  ushort flags;
  short sharedcolors;
  short fsize;
  char fname[MAXFAMILYNAME];
} INITRETP;

typedef struct {
  short	len, type;
  long	ret;
} LRETP;

typedef struct {
  short	len, type;
  short	ret;
  char res[2];
} SRETP;

typedef struct {
  short len, type;
  STATUS status;
  short ret, reserved;
} RSTATUSP;

typedef struct {
  short len, type;
  short ret[2];
} S2RETP;

typedef struct {
  short len, type;
  short ret[4];
} S3RETP;

typedef struct {
  short len, type;
  WEVENT event;
} EVENTP;

typedef struct {
  short len, type;
  short handle;
  short height;
  ushort flags;
  ushort styles;
  short baseline;
  short maxwidth;
  char family[MAXFAMILYNAME];
  uchar widths[256];
} LFONTRETP;

typedef struct {
  short len, type;
  ushort handle;
  short mtype;
  short xoff, yoff;
  ushort mask[16];
  ushort icon[16];
} SMOUSEP;

typedef struct {
  short len, type;
  ushort handle;
  short red, green, blue;
  short ret, reserved;
} COLRETP;

/*
 * pakets that don't require a return code
 */

typedef struct {
  short len, type;
  ushort handle, res;
  char title[MAXTITLE];
} STITLEP;

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, res;
} PLOTP;		/* as well as DPLOT */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, xe, ye, res;
} LINEP;		/* as well as DLINE */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, e;
} HVLINEP;		/* as well as VLINE, DHLINE and DVLINE */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, width, height, res;
} BOXP;			/* as well as PBOX, DBOX and DPBOX */

typedef struct {
  short	len, type;
  ushort handle;
  short	x0, y0, width, height;
  ushort dhandle;
  short x1, y1;
} BITBLKP;		/* as well as BITBLK2 */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, width, height, y1;
} VSCROLLP;

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0;
  ushort c;
} PRINTCP;

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, res;
  uchar s[MAXPRINTS];
} PRINTSP;

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, r;
} CIRCLEP;		/* as well as PCIRCLE, DCIRCLE and DPCIRCLE */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, rx, ry, res;
} ELLIPSEP;		/* as well as PELLIPSE, DELLIPSE and DPELLIPSE */

typedef struct {
  short len, type;
  ushort handle;
  short x0, y0, rx, ry;
  short ax, ay, adir;
  short bx, by, bdir;
  short res;
} PIEP;			/* as well as DPIE, ARC and DARC */

typedef struct {
  short len, type;
  ushort handle;
  short numpoints;
  short points[MAXPOLYPOINTS<<2];
} POLYP;		/* as well as PPOLY, DPOLY and DPPOLY */

typedef struct {
  short len, type;
  ushort handle;
  short points[8];
  short res;
} BEZIERP;		/* as well as DBEZIER */

typedef struct {
  short len, type;
  ushort handle;
  short red, green, blue;
} ALLOCCOLP;

typedef struct {
  short len, type;
  ushort handle;
  short color;
} FREECOLP;

typedef struct {
  short len, type;
  ushort handle;
  short color, res;
  short red, green, blue;
} CHANGECOLP;

typedef struct {
  short len, type;
  ushort handle;
  short color;
} SETFGCOLP;   /* as well as SETBGCOL and GETCOL */


#endif /* __W_PAKETS_H */
