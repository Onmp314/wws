/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * $Id: Wt.h,v 1.7 2008-08-29 19:47:09 eero Exp $
 */
#ifndef _WT_H
#define _WT_H

#include <limits.h>
#include <Wlib.h>

#define WT_MAJOR_VERSION	0
#define WT_MINOR_VERSION	9
#define WT_VERSION		"0.9"

#ifdef WTINYSCREEN
#define WT_DEFAULT_XBORDER	1
#define WT_DEFAULT_YBORDER	1
#else
#define WT_DEFAULT_XBORDER	2
#define WT_DEFAULT_YBORDER	2
#endif

/* 
 * special event mask for events that wserver sends in any case
 */
#define EV_DEFAULT		0x8000

#define WT_UNSPEC		(LONG_MAX)

/*
 * additional input/output
 */

void wt_delinput (long handle);
void wt_chginput (long handle, fd_set *r, fd_set *w, fd_set *e);
long wt_addinput (fd_set *r, fd_set *w, fd_set *e,
		void (*cb) (long arg, fd_set *r, fd_set *w, fd_set *e),
		long arg);

/*
 * timeouts
 */

void wt_deltimeout (long handle);
long wt_addtimeout (long millisecs, void (*cb) (long arg), long arg);

/*
 * widget class
 */
struct _widget_t;

typedef struct _widget_class_t {
	const char *name;
	short initialized;

	long (*init) (void);

	struct _widget_t *(*create) (struct _widget_class_t *);
	long (*delete) (struct _widget_t *w);

	long (*close) (struct _widget_t *w);
	long (*open) (struct _widget_t *w);

	long (*addchild) (struct _widget_t *parent, struct _widget_t *w);
	long (*delchild) (struct _widget_t *parent, struct _widget_t *w);

	long (*realize) (struct _widget_t *w, WWIN *parent);

	long (*query_geometry) (struct _widget_t *w,
				long *x, long *y, long *wd, long *ht);
	long (*query_minsize) (struct _widget_t *w, long *wd, long *ht);
	long (*reshape) (struct _widget_t *w, long x, long y, long wd, long ht);

	long (*setopt) (struct _widget_t *w, long key, void *value);
	long (*getopt) (struct _widget_t *w, long key, void *value);

	WEVENT *(*event) (struct _widget_t *w, WEVENT *);

	long (*child_change) (struct _widget_t *w, struct _widget_t *child,
			      short what);

	long (*parent_change) (struct _widget_t *w, struct _widget_t *parent,
			      short what);

	long (*focus) (struct _widget_t *w, int enter);
} widget_class_t;

/*
 * changes mask for child_change(), parent_change()
 */
#define WT_CHANGED_SIZE		0x01
#define WT_CHANGED_POS		0x02

/*
 * widget instance
 */
typedef struct _widget_t {
	long	magic;

	struct _widget_t *next;
	struct _widget_t *prev;
	struct _widget_t *parent;
	struct _widget_t *childs;

	widget_class_t *class;

	int	x;
	int	y;
	int	w;
	int	h;
	long	usrval;
	short	event_mask;
	WWIN*	win;
	void*	geom_info;

	short	bind_event_mask;
	WEVENT	* (*bind_cb) (struct _widget_t *, WEVENT *ev);
} widget_t;

/* 
 * menu structures
 */
enum {
	MenuEnd,		/* type at the menu end (default(=0)) */
	MenuItem,		/* normal menu item */
	MenuCheck,		/* menu item is checkable */
	MenuSub			/* item opens a submenu */
};

/* if 'string' and 'icon' are NULL, menu item is a separator, if 'sub'
 * and 'select_cb' are NULL, menu is disabled
 */
typedef struct _menu_item_t {
	const char *string;			/* item text on screen */
	BITMAP *icon;				/* item icon on screen */
	void *info;				/* pointer to app data */
	struct _menu_item_t *sub;		/* submenu or check `flag' */
	void (*select_cb) (widget_t *w, struct _menu_item_t *item);
	short type;
} wt_menu_item;


/* global settings for the Wt widgets, initialized at wt_init().  values are
 * from WSERVER struct and overrode by (widget) options in ~/.wtrc.
 */
typedef struct {
	short screen_wd;	/* W screen info returned by w_init() */
	short screen_ht;
	short screen_bits;
	short screen_shared;	/* number of shared server colors */
	const char *font_fixed;	/* monospaced font for widgets needing such */
	const char *font_normal;/* font for other widgets */
	short font_size;	/* size for fonts */
/* currently unused */
	short border_offset;	/* offset from widget contents to it's border */
	short fg_color;		/* drawing color */
	short bg_color;		/* background color */
} wt_global_struct;

extern wt_global_struct wt_global;


/*
 * generic options used for different widgets. to the right is the argument
 * type for use with wt_setopt() and wt_getopt().
 *
 * NOTE: new options for your own widgets should be > 500.
 */
#define WT_EOL		0	/* none  */

#define WT_WIDTH	1	/* long* */
#define	WT_HEIGHT	2	/* long* */
#define	WT_XPOS		3	/* long* */
#define	WT_YPOS		4	/* long* */

#define WT_ORIENTATION	5	/* long* */
#define	    OrientHorz		1L
#define     OrientVert		0L

#define WT_ALIGNMENT	6	/* long* */
#define     AlignLeft		(-1L)
#define     AlignTop		(-2L)
#define     AlignRight		1L
#define     AlignBottom		2L
#define     AlignCenter		0L
#define     AlignFill		(-3L)
#define	    AlignFillVert	(-4L)
#define	    AlignFillHorz	(-5L)

#define WT_LABEL	7	/* char* (getopt: char **) */
#define WT_FONT		8	/* char* (getopt: WFONT**) */
#define WT_FONTSIZE	9	/* long* */

#define WT_USRVAL	10	/* long* */
#define WT_EVENT_MASK	11	/* long* */
#define WT_DRAW_FN	12	/* void (*) (widget_t *, int, int, int, int) */

#define	WT_RESIZE_CB	13	/* void (*) (widget_t *, long, long) */
#define	WT_EVENT_CB	14	/* WEVENT* (*) (widget_t *, WEVENT *) */
#define WT_ACTION_CB	15	/* depends */
#define WT_RESIZEABLE	16	/* long* */

#define WT_STATE	17	/* long* */
#define	    ButtonStateReleased	0L
#define	    ButtonStatePressed	1L

#define PopupPersistant		0L	 /* popup waits MPRESS first */
#define PopupTransient		1L	 /* popup closes at first MRELEASE */

#define WT_MODE		18	/* long* */
#define	    ButtonModePush	0L
#define	    ButtonModeRadio	1L
#define	    ButtonModeToggle	2L

#define     LabelModeNoBorder	3L
#define     LabelModeWithBorder	ButtonModePush

#define     ShellModeMain	0L
#define     ShellModePopup	1L
#define     ShellModeNoBorder	2L

#define     ViewpModeLazyScroll	0L
#define     ViewpModeLifeScroll	1L

#define     GetstrModeNoBorder	0L
#define     GetstrModeWithBorder 1L

#define WT_COLORS		19	/* *long (getopt only) */

#define WT_HDIST		20	/* long* */
#define WT_VDIST		21	/* long* */

/*
 * shell specific
 */
#define WT_ICON_STRING		22	/* char* (getopt: char **) */

/*
 * for topwindow
 */
#define WT_WINDOW_HIDDEN	23	/* long* */

/*
 * at the moment only used by shell, but could be for others too maybe...
 */
#define WT_MIN_WIDTH	24
#define WT_MIN_HEIGHT	25

/*
 * these are iconedit-specific
 */
#define WT_ICON_WIDTH	30	/* long* */
#define WT_ICON_HEIGHT	31	/* long* */
#define WT_ICON_XPOS	32	/* long* */
#define WT_ICON_YPOS	33	/* long* */
#define WT_UNIT_SIZE	34	/* long* */
#define WT_REFRESH	35	/* NULL */

/*
 * these are HTML specific
 */
#define WT_APPEND	40	/* char* (setopt only) */
#define WT_QUERY_CB	42	/* long (*) (widget_t*, char*, int*, int*) */
#define WT_PLACE_CB	43	/* void (*) (widget_t*, long, int, int) */
#define WT_INVAL_CB	44	/* void (*) (widget_t*, int) */
#define WT_TITLE_CB	45	/* void (*) (widget_t*, char *) */

/*
 * these are `getstring' specific
 */
#define WT_STRING_ADDRESS	50	/* char* (getopt: char **) */
#define WT_STRING_LENGTH	51	/* long* */
#define WT_STRING_WIDTH		52	/* long* */
#define WT_STRING_MASK		53	/* char* (getopt: char **) */
#define WT_INKEY_CB		54	/* WEVENT* (*) (widget_t*, WEVENT*) */

/*
 * these are (mostly) scrollbar-specific
 */
#define WT_SIZE		60	/* long* */
#define WT_POSITION	61	/* long* */
#define WT_LINE_INC	62	/* long* */
#define WT_PAGE_INC	63	/* long* */
#define WT_TOTAL_SIZE	64	/* long* */

/*
 * for listbox
 * (ACTION_CB got same arguments as CHANGE_CB
 */
#define WT_LIST_WIDTH		70	/* long* */
#define WT_LIST_HEIGHT		71	/* long* */
#define WT_LIST_ADDRESS		72	/* char** (getopt: char ***) */
#define WT_LIST_POSITION	73	/* long* */
#define WT_CHANGE_CB		74	/* void (*) (widget_t*, char*, int) */
#define WT_CURSOR		75	/* long* */
#define WT_LIST_ONLY		76

/*
 * for filesel widget
 */
#define WT_FILESEL_PATH		80	/* char* (getopt: char **) */
#define WT_FILESEL_MASK		81	/* char* (getopt: char **) */
#define WT_FILESEL_FILE		82	/* char* (getopt: char **) */

/*
 * for packer
 */
#define WT_PACK_WIDGET		90	/* widget_t* */
#define WT_PACK_INFO		91	/* widget_t* */

#define WT_ALIGN_VERT		100
#define WT_ALIGN_HORZ		101
#define WT_ALIGN_MODE		102
#define		LabelTextHorzOut	0
#define 	LabelTextVertOut	1

#define WT_ICON			103

#define WT_VT_WIDTH		110	/* long* */
#define WT_VT_HEIGHT		111	/* long* */
#define WT_VT_HISTSIZE		112	/* long* */
#define WT_VT_STRING		113	/* wt_opaque_t* */
#define WT_VT_HISTPOS		114	/* long* */
#define WT_VT_VISBELL		115	/* long* */
#define WT_VT_REVERSE		116	/* long* */
#define WT_VT_BLINK		117	/* long* */

typedef struct {
  char *cp;
  long len;
} wt_opaque_t;

/* 
 * for wedit/edittext widget
 * WT_CHANGE_CB got: void (*)(widget_t *, short, short),
 * WT_ACTION_CB has same arguments as WT_OFFSET_CB.
 */
#define WT_TEXT_CLEAR		120	/* void  */
#define WT_TEXT_APPEND		121	/* uchar* (getopt: uchar **) */
#define WT_TEXT_INSERT		122	/* uchar* (getopt: uchar **) */
#define WT_TEXT_CHAR		123	/* long* */
#define WT_TEXT_LINE		124	/* long* */
#define WT_TEXT_COLUMN		125	/* long* */
#define WT_TEXT_SELECT		126	/* long* */
#define WT_TEXT_WRAP		127	/* long* */
#define WT_TEXT_INDENT		128	/* long* */
#define WT_TEXT_TAB		129	/* long* */
#define WT_OFFSET_CB		130	/* uchar *(*)(widget_t *, uchar *, short) */
#define WT_MAP_CB		131	/* uchar (*)(widget_t *, uchar) */

/* for range widget */
#define WT_VALUE		140	/* char*, (default) value (getopt: char **) */
#define WT_VALUE_MIN		141	/* char*, minimum value, eg. "0.0" (getopt: char **) */
#define WT_VALUE_MAX		142	/* char*, maximum value (getopt: char **) */
#define WT_VALUE_DECIMALS	143	/* long*, decimals used for values */
#define WT_VALUE_STEPS		144	/* long* */

/*
 * widget support functions. needed when using widgets.
 */
extern widget_t *wt_init (void);
extern widget_t *wt_create (widget_class_t *class, widget_t *parent);
extern long	 wt_delete (widget_t *w);
extern long	 wt_open (widget_t *);
extern long	 wt_close (widget_t *);
extern long	 wt_realize (widget_t *top);
extern long	 wt_run (void);
extern long	 wt_break (long r);
extern long	 wt_getopt (widget_t *, ...);
extern long	 wt_setopt (widget_t *, ...);
extern WWIN	*wt_widget2win (widget_t *);
extern long	 wt_getfocus (widget_t *);
extern long	 wt_do_event (void);
extern void	 wt_dispatch_event (WEVENT *);
extern void	 wt_bind (widget_t *w, short eventmask,
			  WEVENT * (*cb) (widget_t *, WEVENT *));

/*
 * some useful helpers: alertbox, entrybox, fileselector
 */
extern int wt_entrybox (widget_t *appshell,
			char *buf, long buflen,
			const char *title, const char *caption,
			...);

extern int wt_dialog (widget_t *appshell,
			const char *msg, int type, const char *title,
			...);

#define WT_DIAL_INFO	0
#define WT_DIAL_ERROR	1
#define WT_DIAL_QUEST	2
#define WT_DIAL_WARN	3

extern int wt_popup (const char *entries,
	void (*draw_fn) (widget_t *, int, long, long, long, long));

extern char *wt_fileselect (widget_t *appshell, const char *title,
			    const char *path, const char *mask,
			    const char *file);

/* handle popup with `menu' according to `pressed' */
extern void wt_popup_cb(widget_t *parent, wt_menu_item *menu, int pressed);

/* WT_DRAW_FN callback for drawing button/label and menu/popup icons */
extern void wt_drawbm_fn(widget_t *w, short x, short y, BITMAP *bm);


/*
 * available widget classes
 */
extern widget_class_t *wt_top_class;
extern widget_class_t *wt_shell_class;
extern widget_class_t *wt_topwindow_class;

extern widget_class_t *wt_box_class;
extern widget_class_t *wt_pane_class;
extern widget_class_t *wt_form_class;
extern widget_class_t *wt_viewport_class;
extern widget_class_t *wt_packer_class;

extern widget_class_t *wt_pushbutton_class;
extern widget_class_t *wt_radiobutton_class;
extern widget_class_t *wt_checkbutton_class;
extern widget_class_t *wt_label_class;
#define wt_button_class	wt_pushbutton_class

extern widget_class_t *wt_drawable_class;
extern widget_class_t *wt_scrollbar_class;
extern widget_class_t *wt_iconedit_class;
extern widget_class_t *wt_html_class;
extern widget_class_t *wt_getstring_class;
extern widget_class_t *wt_edittext_class;
extern widget_class_t *wt_text_class;
extern widget_class_t *wt_list_class;
extern widget_class_t *wt_listbox_class;
extern widget_class_t *wt_filesel_class;
extern widget_class_t *wt_vt_class;
extern widget_class_t *wt_popup_class;
extern widget_class_t *wt_menu_class;
extern widget_class_t *wt_range_class;
extern widget_class_t *wt_select_class;
extern widget_class_t *wt_arrow_class;

/*
 * helper functions. these are needed when implementing new widgets.
 */
extern void	wt_box3d (WWIN *, int x, int y, int wd, int ht);
extern void	wt_box3d_press (WWIN *, int x, int y, int wd, int ht, int);
extern void	wt_box3d_release (WWIN *, int x, int y, int wd, int ht, int);
extern void	wt_box3d_mark (WWIN *, int x, int y, int wd, int ht);
extern void	wt_box3d_unmark (WWIN *, int x, int y, int wd, int ht);
extern void	wt_circle3d (WWIN *, int x, int y, int r);
extern void	wt_circle3d_press (WWIN *, int x, int y, int r);
extern void	wt_circle3d_release (WWIN *, int x, int y, int r);
extern void	wt_circle3d_mark (WWIN *, int x, int y, int r);
extern void	wt_circle3d_unmark (WWIN *, int x, int y, int r);
extern void	wt_arrow3d (WWIN *, int x, int y, int w, int h, int dir);
extern void	wt_arrow3d_press (WWIN *, int x, int y, int w, int h, int dir);
extern void	wt_arrow3d_release (WWIN *, int x, int y, int w, int h, int d);
extern void	wt_text (WWIN *, WFONT *fp, const char *str,
			 int x, int y, int w, int h, int align);

extern void	wt_remove (widget_t *);
extern void	wt_add_before (widget_t *par, widget_t *w, widget_t *new);
extern void	wt_add_after (widget_t *par, widget_t *w, widget_t *new);

extern void	wt_ungetfocus (widget_t *);
extern void	wt_change_notify (widget_t *, short changes);
extern WWIN	*wt_create_window (WWIN *parent, short wd, short ht,
			short flags, widget_t *w);

extern long	wt_geometry(widget_t *w, long *x, long *y, long *wd, long *ht);
extern long	wt_minsize (widget_t *w, long *wd, long *ht);
extern long	wt_reshape (widget_t *w, long x, long y, long wd, long ht);

extern WFONT	*wt_loadfont(const char *fname, int fsize, int flags, int fixed);
extern const char *wt_variable(const char *variable);

#endif /* _WT_H */

