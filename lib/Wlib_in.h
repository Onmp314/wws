/*
 * Wlib.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 */

#ifndef __W_WLIB_H
#define __W_WLIB_H

#include <sys/types.h>

#ifndef uchar
#define	uchar unsigned char
#endif
#ifndef ushort
#define	ushort unsigned short
#endif
#ifndef ulong
#define	ulong unsigned long
#endif

/* maaan, am I sick & tired to always have to define these... :( */
#undef ABS
#define	ABS(x) (((x)<0)?-(x):(x))
#undef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct {
  short vmaj;			/* server version */
  short vmin;
  short pl;
  short type;			/* graphics type */
  short width;
  short height;
  short sharedcolors;		/* # of unchanging server colors */
  short planes;			/* screen bit-depth */
  ushort flags;
  short fsize;			/* default font size */
  const char *fname;		/* default font family */
} WSERVER;

/* some flags for the 'flags' field of WSERVER */
#define WSERVER_SHM		0x0001
#define WSERVER_KEY_MAPPING	0x0002

/*
 * some bitmap types (used inside the server)
 */

#define BM_PACKEDCOLORMONO	0	/* bitplanes, monochrome */
#define BM_PACKEDMONO		1	/* 1 bitplane, monochrome */
#define BM_DIRECT8		2	/* 8-bit chunky (direct) colors */
#define BM_PACKEDCOLOR		3	/* bitplanes, colors settable */
#define BM_DIRECT24		4	/* 24-bit (rgb) image */

typedef struct {
	uchar red;
	uchar green;
	uchar blue;
	/* here could be rest of the 32 color bits... */
} rgb_t;

typedef struct {
  short width;
  short height;
  short type;
  short unitsize;
  short upl;		/* units / line */
  short planes;		/* bits / pixel */
  void *data;
#ifndef __WSERVER	/* these are used only on client side */
  short colors;
  rgb_t *palette;	/* needed only for DIRECT8 and PACKEDCOLOR */
#endif
} BITMAP;

typedef struct {
  ulong ip_addr;
  long pakets;
  long bytes;
  short totalWin;
  short openWin;
} STATUS;

typedef struct _wfont {
  long magic;			/* MAGIC_F */
  short handle;

/* font type */
  short height;			/* font size */
  char *family;			/* font family name */
  ushort flags;			/* proportional, CHARSET etc. */
  ushort styles;		/* styles implemented in font */

/* font cell information */
  short baseline;		/* baseline offset for vertical aligning */
  short maxwidth;		/* width of the widest char */

  uchar widths[256];

  /* some elements to implement a linked list */
  struct _wfont *prev;
  struct _wfont *next;
  int used;
} WFONT;

/* mouse pointer stuff */

typedef struct {
  short xDrawOffset;
  short yDrawOffset;
  ushort mask[16];
  ushort icon[16];
} WMOUSE;

#define MOUSE_ARROW 0
#define MOUSE_UPDOWN 1
#define MOUSE_LEFTRIGHT 2
#define MOUSE_UPPERLEFTLOWERRIGHT 3
#define MOUSE_LOWERLEFTUPPERRIGHT 4
#define MOUSE_MOVE 5
#define MOUSE_BUSY 6
#define MOUSE_USER 7		/* last one */

typedef struct {
  long magic;			/* MAGIC_W */
  /*
   * a place where the user can store something (eg. a pointer
   * to an additional structure associated with that window).
   */
  long user_val;

  ushort handle;
  short width;
  short height;
  short x0;			/* for button.c */
  short y0;
  short type;

  /* graphics context */
  short fg;
  short bg;
  short linewidth;
  ushort drawmode;
  ushort pattern;
  ushort textstyle;
  WFONT *font;

  short colors;			/* number of allocated/inherited colors */
} WWIN;

/* window type */
#define WWIN_ROOT	0
#define WWIN_SUB	1
#define WWIN_FAKE	2	/* w_winFromID() */


typedef struct {
  WWIN *win;
  short type;
  uchar reserved[2];		/* padding & used by server */
  long time;
  short x;
  short y;
  short w;
  short h;
  long key;
} WEVENT;


/*
 *  various flags
 */

/* drawing modes */
#define	M_CLEAR		0		/* to background color */
#define	M_DRAW		1		/* to foreground color */
#define	M_INVERS	2		/* inverted, default */
#define M_TRANSP	3		/* transparent (patterns) */

/* grayscale patterns (0 - MAX_GRAYSCALES) */
#define MAX_GRAYSCALES	0x40

/* arbitrary patterns (MAX_GRAYSCALES - 0xfe) */
#define GRAY_PATTERN	0xfe		/* default */
#define W_PATTERN	0xfd		/* server background */
#define W_USERPATTERN	0xfc		/* returned for user set patt. data */

/* w_hatch() types */
#define P_HATCH		0
#define P_LINE_L	1
#define P_LINE_R	2

/* font style flags */
#define	F_NORMAL	0x0000
#define	F_REVERSE	0x0001
#define	F_UNDERLINE	0x0002
#define F_ITALIC	0x0004
#define F_BOLD		0x0008
#define F_LIGHT		0x0010

/* something to mask legal styles */
#define	F_STYLEMASK	0x001f

/* charset flags */
#define F_LATIN1	0x0001		/* ISO-latin1 (default) */
#define F_ASCII		0x0002		/* only characters 32-126 */
#define F_SYMBOL	0x0004		/* any charset */
#define F_PROP		0x0008		/* proportional font */

/* event request flags */
#define	W_MOVE		0x0001
#define	W_TOP		0x0002
#define	W_TITLE		0x0004
#define	EV_KEYS		0x0008
#define	EV_MOUSE	0x0010
#define	W_NOBORDER	0x0020
#define	W_NOMOUSE	0x0040
#define EV_ACTIVE	0x0080
#define W_CLOSE		0x0100
#define W_ICON		0x0200
#define	W_CONTAINER	0x0400
#define W_RESIZE	0x0800
#define EV_MMOVE	0x1000
#define EV_MODIFIERS	0x2000	/* adds modifier & key release events! */
#define W_SIMPLEWIN	0x011D  /* W_MOVE|W_TITLE|EV_KEYS|EV_MOUSE|W_CLOSE */

#define	W_FLAGMASK	0x3fff

/* event types */
#define	EVENT_KEY	0x0001
#define	EVENT_MMOVE	0x0002
#define	EVENT_MPRESS	0x0003
#define	EVENT_MRELEASE	0x0004
#define	EVENT_BUTTON	0x0005
#define	EVENT_GADGET	0x0006
#define	EVENT_SAVEON	0x0007
#define	EVENT_SAVEOFF	0x0008
#define EVENT_ACTIVE	0x0009
#define EVENT_INACTIVE	0x000a
#define EVENT_RESIZE	0x000b
#define EVENT_KRELEASE	0x000c

#define	GADGET_EXIT	0x0001
#define	GADGET_CLOSE	0x0002
#define GADGET_ICON	0x0003
#define GADGET_SIZE	0x0004

/* don't change, some server stuff depends on what these values are */
#define	BUTTON_LEFT	4
#define	BUTTON_MID	2
#define	BUTTON_RIGHT	1

/* special key IDs */
#define IS_WKEY(x)	((x) & 0xff00)
#define WKEY_FN(x)	(((x) >> 8) & 0xf)
#define WKEY_F1		0x100
#define WKEY_F2		0x200
#define WKEY_F3		0x300
#define WKEY_F4		0x400
#define WKEY_F5		0x500
#define WKEY_F6		0x600
#define WKEY_F7		0x700
#define WKEY_F8		0x800
#define WKEY_F9		0x900
#define WKEY_F10	0xa00
#define WKEY_F11	0xb00
#define WKEY_F12	0xc00
#define WKEY_UP		0x1100
#define WKEY_DOWN	0x1200
#define WKEY_LEFT	0x1300
#define WKEY_RIGHT	0x1400
#define WKEY_PGUP	0x1500
#define WKEY_PGDOWN	0x1600
#define WKEY_HOME	0x1700
#define WKEY_END	0x1800
#define WKEY_INS	0x1900
#define WKEY_DEL	0x1a00

/* modifier masks */
#define IS_WMOD(x)	((x) & 0xfe0000)
#define WMOD_SIDE(x)	((x) & 0x010000)
#define WMOD_LEFT	0x000000
#define WMOD_RIGHT	0x010000

/* modifier flags */
#define WMOD_SHIFT	0x020000
#define WMOD_CTRL	0x040000
#define WMOD_ALT	0x060000
#define WMOD_META	0x080000
#define WMOD_SUPER	0x0a0000
#define WMOD_HYPER	0x0c0000
#define WMOD_ALTGR	0x0e0000
#define WMOD_CAPS	0x100000
#define WMOD_NUM	0x120000
#define WMOD_SCROLL	0x140000


#define	MAGIC_W	0x15263748
#define	MAGIC_F	0x15263749

/* special flag for w_open() */
#define UNDEF -32768


/*
 *
 */

extern WWIN *WROOT;
/* extern WWIN *WSCREEN;   not yet implemented */

/*
 * some functions
 */

extern WSERVER	*w_init(void);
extern void	w_exit(void);
extern WWIN	*w_create(short width, short height, ushort flags);
extern WWIN	*w_createChild(WWIN *parent, short width, short height, ushort flags);
extern short	w_open(WWIN *win, short x0, short y0);
extern short	w_move(WWIN *win, short x0, short y0);
extern short	w_resize(WWIN *win, short width, short height);
extern short	w_close(WWIN *win);
extern short	w_delete(WWIN *win);

extern WEVENT	*w_queryevent(fd_set *rdp, fd_set *wdp, fd_set *xdp, long timeout);
extern short	w_querywindowpos(WWIN *win, short effective, short *x0, short *y0);
extern short	w_querywinsize(WWIN *win, short effective, short *width, short *height);
extern short	w_querymousepos(WWIN *win, short *x0, short *y0);
extern short	w_querystatus(STATUS *st, short index);

extern short	w_settitle(WWIN *win, const char *s);
extern short	w_setmode(WWIN *win, ushort mode);
extern short	w_setlinewidth(WWIN *win, short width);
extern ushort	w_setpattern(WWIN *win, ushort pattern);
extern ushort	w_setpatterndata(WWIN *win, ushort *data);
extern WFONT	*w_loadfont(const char *family, short size, ushort stylemask);
extern short	w_settextstyle(WWIN *win, ushort flags);
extern WFONT	*w_setfont(WWIN *win, WFONT *font);
extern short	w_unloadfont(WFONT *font);
extern short	w_setsaver(short seconds);
extern short	w_setmousepointer (WWIN *win, short type, WMOUSE *data);
extern short	w_getmousepointer (WWIN *win);

extern short	w_beep(void);
extern short	w_null(void);
extern void	w_flush(void);
extern short	w_test(WWIN *win, short x0, short y0);
extern short	w_bitblk(WWIN *win, short x0, short y0, short width, short height, short x1, short y1);
extern short	w_bitblk2(WWIN *swin, short x0, short y0, short width, short height, WWIN *dwin, short x1, short y1);
extern short	w_vscroll(WWIN *win, short x0, short y0, short width, short height, short y1);
extern short	w_printchar(WWIN *win, short x0, short y0, short c);
extern short	w_printstring(WWIN *win, short x0, short y0, const char *s);

extern short	w_plot(WWIN *win, short x0, short y0);
extern short	w_hline(WWIN *win, short x0, short y0, short xe);
extern short	w_vline(WWIN *win, short x0, short y0, short ye);
extern short	w_line(WWIN *win, short x0, short y0, short xe, short ye);
extern short	w_box(WWIN *win, short x0, short y0, short width, short height);
extern short	w_pbox(WWIN *win, short x0, short y0, short width, short height);
extern short	w_circle(WWIN *win, short x0, short y0, short r);
extern short	w_pcircle(WWIN *win, short x0, short y0, short r);
extern short	w_ellipse(WWIN *win, short x0, short y0, short rx, short ry);
extern short	w_pellipse(WWIN *win, short x0, short y0, short rx, short ry);
extern short	w_arc(WWIN *win, short x0, short y0, short rx, short ry, float start, float end);
extern short	w_pie(WWIN *win, short x0, short y0, short rx, short ry, float start, float end);
extern short	w_poly(WWIN *win, short numpoints, short *points);
extern short	w_ppoly(WWIN *win, short numpoints, short *points);
extern short	w_bezier(WWIN *win, short *controls);

extern short	w_dplot(WWIN *win, short x0, short y0);
extern short	w_dhline(WWIN *win, short x0, short y0, short xe);
extern short	w_dvline(WWIN *win, short x0, short y0, short ye);
extern short	w_dline(WWIN *win, short x0, short y0, short xe, short ye);
extern short	w_dbox(WWIN *win, short x0, short y0, short width, short height);
extern short	w_dpbox(WWIN *win, short x0, short y0, short width, short height);
extern short	w_dcircle(WWIN *win, short x0, short y0, short r);
extern short	w_dpcircle(WWIN *win, short x0, short y0, short r);
extern short	w_dellipse(WWIN *win, short x0, short y0, short rx, short ry);
extern short	w_dpellipse(WWIN *win, short x0, short y0, short rx, short ry);
extern short	w_darc(WWIN *win, short x0, short y0, short rx, short ry, float start, float end);
extern short	w_dpie(WWIN *win, short x0, short y0, short rx, short ry, float start, float end);
extern short	w_dpoly(WWIN *win, short numpoints, short *points);
extern short	w_dppoly(WWIN *win, short numpoints, short *points);
extern short	w_dbezier(WWIN *win, short *controls);

extern short	w_allocColor(WWIN *win, uchar red, uchar green, uchar blue);
extern short	w_changeColor(WWIN *win, short color, uchar red, uchar green, uchar blue);
extern short	w_getColor(WWIN *win, short color, uchar *red, uchar *green, uchar *blue);
extern short	w_setForegroundColor(WWIN *win, short color);
extern short	w_setBackgroundColor(WWIN *win, short color);
extern short	w_freeColor(WWIN *win, short color);

extern short	w_putblock(BITMAP *bm, WWIN *win, short x1, short y1);
extern BITMAP	*w_getblock(WWIN *win, short x0, short y0, short width, short height);

/* 
 * rest of the functions are implemented completely on the client side
 */
extern short	w_bmheader(BITMAP *bm);
extern BITMAP	*w_allocbm(short width, short height, short type, short colors);
extern BITMAP	*w_convertBitmap(BITMAP *src, short dst_type, short dst_colors);
extern BITMAP	*fs_direct2mono(BITMAP *bm, int in_place, const char **error);
extern uchar	*(*w_convertFunction(BITMAP *src, BITMAP *dst, int row))();
extern void	w_ditherOptions(uchar *graymap, int expand);
extern void	w_freebm(BITMAP *bm);

extern uchar	*w_allocMap(WWIN *win, short colors, rgb_t *palette, uchar *oldmap);
extern short	w_mapData(BITMAP *bm, uchar *colmap);

extern short	w_fill(WWIN *win, short x, short y);

extern BITMAP	*w_xpm2bm(const char **xpm);
extern BITMAP	*w_readimg(const char *fname, short *width, short *height);
extern short	w_writepbm(const char *path, BITMAP *bm);
extern BITMAP	*w_readpbm(const char *path);
extern BITMAP	*w_copybm(BITMAP *bm);

extern short	w_showButton(WWIN *button);
extern short	w_hideButton(WWIN *button);
extern WWIN	*w_createButton(WWIN *parent, short x0, short y0, short width, short height);
extern WEVENT	*w_querybuttonevent(fd_set *rdp, fd_set *wdp, fd_set *xdp, long timeout);
extern short	w_centerPrints(WWIN *win, WFONT *font, const char *s);

extern ulong	w_winID(WWIN *win);
extern WWIN	*w_winFromID(ulong id);

extern void	w_trace(short flag);
extern int	w_strlen(WFONT *font, const char *s);
extern ushort	w_hatch(int type, int width, int times);
extern const char	*w_fonttype(const char *filename, short *size, ushort *styles);
extern void	scan_geometry(const char *geometry, short *col, short *lin, short *xp, short *yp);
extern void	limit2screen(WWIN *win, short *xp, short *yp);

extern float	qsin(float angle);
extern float	qcos(float angle);
extern ulong	isqrt(ulong N);

extern long	w_gettime (void);


#define W_SEL_TEXT	"text"		/* '\t' || '\n' || >= 32 */
#define W_SEL_VALUE	"value"		/* '0' - '9' */
#define W_SEL_DATA	"data"		/* 0-255 */
#define W_SEL_FILE	"file"		/* file path / name */
#define W_SEL_TYPE	"type"		/* clip type for type requests */

typedef struct {
  char  type[16];
  char *data;
  long len;
} w_selection_t;

typedef int w_clipboard_t;	/* ATM file descriptor */

extern w_clipboard_t  w_selopen (const char *type);
extern w_clipboard_t  w_selappend (w_clipboard_t, const char *data, long len);
extern void           w_selclose (w_clipboard_t);
extern w_selection_t* w_getselection (const char *type);
extern int 	      w_putselection (const char *type, const char *data, long len);
extern void	      w_freeselection (w_selection_t *);

#endif /* __W_WLIB_H */
