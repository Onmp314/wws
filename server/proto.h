/*
 * proto.h, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- protypes for server functions and global variables
 */

#ifndef __W_PROTO_H
#define __W_PROTO_H

#include <stdlib.h>
#include "pakets.h"


/*
 * these weird macros allocate memory regions which have one long before and
 * one long behind them accessible without memory errors and therefore save
 * us a lot of checks and thus execution time. the only drawback is: you must
 * under no circumstances mix them with the normal malloc/free calls!
 */

#define MALLOC(size) (void *)(((ulong)malloc(size+8))+4)
#define FREE(addr) free((void *)(((ulong)addr)-4))


/*
 * global stuff
 *
 * EV_KEYS and EV_MOUSE already defined in ../lib/Wlib.h
 */

#define	EV_UCONN 0x0001
#define	EV_ICONN 0x0002
#define	EV_CLIENT 0x0004

/* indexes to glob_font[] array */
#define TITLEFONT 0
#define	MENUFONT 1

#include "graph/gproto.h"


/* in client_*.c */

extern void client_do_close (WINDOW *);
extern void client_do_delete (WINDOW *);
extern void client_do_open (WINDOW *);

extern long client_create(CLIENT *cptr, short width, short height,
			  ushort flags, WWIN *libPtr);
extern long client_create2(CLIENT *cptr, short width, short height,
			   ushort flags, ushort handle, WWIN *libPtr);
extern short client_open(CLIENT *cptr, ushort handle, short x0, short y0);
extern short client_move(CLIENT *cptr, ushort handle, short x0, short y0);
extern short client_resize(CLIENT *cptr, ushort handle, short width, short height);
extern short client_close(CLIENT *cptr, ushort handle);
extern short client_delete(CLIENT *cptr, ushort handle);
extern void client_loadfont(CLIENT *cptr, const char *family, short size,
			    short styles, LFONTRETP *lfontrpaket);
extern short client_unloadfont(CLIENT *cptr, short fonthandle);
extern short client_querywinsize(CLIENT *cptr, ushort handle, short effective,
				 short *width, short *height);
extern void client_setmode(CLIENT *cptr, ushort handle, ushort mode);
extern void client_setlinewidth(CLIENT *cptr, ushort handle, short thick);
extern short client_test(CLIENT *cptr, ushort handle, short x0, short y0);
extern void client_setfont(CLIENT *cptr, ushort handle, short fonthandle);
extern void client_settextstyle(CLIENT *cptr, ushort handle, ushort flags);
extern void client_setmousepointer (CLIENT *cptr, ushort handle,
                                    short type, short xoff, short yoff,
				    ushort *mask, ushort *data);
extern short client_querymousepos(CLIENT *cptr, ushort handle, short *x0,
				  short *y0);
extern short client_querywindowpos(CLIENT *cptr, ushort handle, short effective,
				   short *x0, short *y0);
extern void client_setpattern(CLIENT *cptr, ushort handle, ushort pattern);
extern void client_setpatterndata(CLIENT *cptr, ushort handle, ushort *data);
extern void client_settitle(CLIENT *cptr, ushort handle, const char *s);
extern void client_plot(CLIENT *cptr, ushort handle, short x0, short y0);
extern void client_line(CLIENT *cptr, ushort handle, short x0, short y0,
			short xe, short ye);
extern void client_hline(CLIENT *cptr, ushort handle, short x0, short y0,
			 short xe);
extern void client_vline(CLIENT *cptr, ushort handle, short x0, short y0,
			 short ye);
extern void client_box(CLIENT *cptr, ushort handle, short x0, short y0,
		       short width, short height);
extern void client_poly(CLIENT *cptr, ushort handle, short numpoints, short *points);
extern void client_bezier(CLIENT *cptr, ushort handle, short *controls);
extern void client_bitblk(CLIENT *cptr, ushort handle, short x0, short y0,
			  short width, short height, short x1, short y1);
extern void client_bitblk2(CLIENT *cptr, ushort shandle, short x0, short y0,
			   short width, short height, ushort dhandle, short x1,
			   short y1);
extern void client_vscroll(CLIENT *cptr, ushort handle, short x0, short y0,
			   short width, short height, short y1);
extern void client_printc(CLIENT *cptr, ushort handle, short x0, short y0,
			  ushort c);
extern void client_prints(CLIENT *cptr, ushort handle, short x0, short y0,
			  const uchar *s);
extern void client_circle(CLIENT *cptr, ushort handle, short x0, short y0,
			  short r);
extern void client_ellipse(CLIENT *cptr, ushort handle, short x0, short y0,
			  short rx, short ry);
extern void client_pie(CLIENT *cptr, ushort handle, short x0, short y0,
			short rx, short ry, short ax, short ay, short bx,
			short by, short adir, short bdir);
extern short client_status(CLIENT *cptr, CLIENT *clients, short index,
			   STATUS *status);
extern long client_putblockreq (CLIENT *cptr, short width, short height,
				ushort handle, short x0, short y0, long shmKey);
extern void client_putblockdata(CLIENT *cptr, uchar *rawdata, short space);
extern long client_getblockreq(CLIENT *cptr, ushort handle, short x0, short y0,
			       short width, short height, long shmKey);
extern short client_getblockdata(CLIENT *cptr, uchar *rawdata, short space);
extern short client_getmousepointer (CLIENT *cptr, ushort handle);
extern void client_beep(CLIENT *cptr);

extern short clientAllocColor(CLIENT *cptr, ushort handle,
			      ushort red, ushort green, ushort blue);
extern short clientFreeColor(CLIENT *cptr, ushort handle, short color);
extern short clientGetColor(CLIENT *cptr, ushort handle, short color,
			    ushort *red, ushort *green, ushort *blue);
extern short clientChangeColor(CLIENT *cptr, ushort handle, short color,
			       ushort red, ushort green, ushort blue);
extern short clientSetColor(CLIENT *cptr, ushort handle, short color);


/* in color.c */

extern int glob_sharedcolors;
extern COLORTABLE *glob_colortable;
extern void colorInitShared (char *colorspec);
extern void colorGetMask (short color, ushort *mask);
extern short colorCopyVirtualTable (WINDOW *win, WINDOW *parent);
extern short colorFreeTable (WINDOW *win);
extern short colorInit (void);
extern short colorSetColorTable (COLORTABLE *colTab);
extern short colorAllocColor (WINDOW *win,
			      ushort red, ushort green, ushort blue);
extern short colorFreeColor (WINDOW *win, short color);
extern short colorGetColor (WINDOW *win, short color,
			    ushort *red, ushort *green, ushort *blue);
extern short colorChangeColor (WINDOW *win, short color,
			       ushort red, ushort green, ushort blue);
extern short colorSetFGColor (WINDOW *win, short color);
extern short colorSetBGColor (WINDOW *win, short color);


/* in font.c */

extern FONT glob_font[MAXFONTS];
extern int font_init(const char *title_font, const char *menu_font);
extern short font_loadfont(const char *fontname);
extern short font_unloadfont(FONT *fp);
extern int fontStrLen(FONT *f, const uchar *s);


/* in graph/init.c */
extern SCREEN *screen_init(int forceMono);
extern REC *glob_clip0, *glob_clip1;
extern GCONTEXT *gc0;


/* in main.c */

extern SCREEN *glob_screen;
extern int glob_unixh, glob_ineth;
extern uid_t glob_uid;
extern int glob_debug;
extern int glob_mouse_accel;
extern char *glob_fontpath;
extern char *glob_fontfamily;
extern short glob_fontsize;
extern short is_terminating;
extern void terminate(int sig, const char *msg);
extern void wserver_exit(int sig, const char *msg);
extern void *mysignal(int signum, void *);


/* in mouse.c */

extern MOUSE glob_mouse;
extern void wmouse_init (void);
extern void mouse_show (void);
extern void mouse_hide (void);
extern void mouse_move (void);
extern short mouse_rcintersect (short x0, short y0, short width, short height);


/* in loop.c */

extern int get_rectangle(short width, short height, short *x0, short *y0,
			  char pressed, char released);
extern WINDOW *glob_leftmousepressed, *glob_rightmousepressed;
extern long glob_bytes, glob_pakets;
extern CLIENT *glob_clients;
extern fd_set glob_crfd;
extern short glob_pakettype;
extern short glob_loopmove;
extern long glob_evtime;
extern void loop (void);


/* in menu.c */

extern void menu_domenu (void);
extern MENU menu;


/* in misc.c */

extern short intersect(short x0, short y0, short width0, short height0,
		       short x1, short y1, short width1, short height1);
extern REC *wrec_intersect(REC *rec1, REC *rec2);
extern void set_defaultgc(WINDOW *win);
extern void init_defmasks(void);
extern GCONTEXT *glob_inversgc;
extern GCONTEXT *glob_cleargc;
extern GCONTEXT *glob_drawgc;


/* in recs.c */

#if 0
extern short glob_recsused;
extern WREC glob_rec[MAXRECS];
extern void recs_rebuild(void);
extern void recUpdateDirty(REC *r, int x0, int y0, int width, int height);
#endif


/* in wfuncs_???.c */

extern void w_topDown(WINDOW *win);

#ifdef CLICK_TO_FOCUS
extern void w_changeActiveWindowTo(WINDOW *win);
extern void w_maybeActiveWindowClosing(WINDOW *win);
#else
extern void w_changeActiveWindow(void);
#endif

#endif /* __W_PROTO_H */
