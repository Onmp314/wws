/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1996, Kay Roemer.
 *
 * mostly complete HTML 1.0 widget.
 *
 * CHANGES
 * ++eero 10/97:
 * - modfied for new w_loadfont() syntax.
 * - uses fonts specified in Wt configuration file (html_normal / html_fixed
 *   / html_fsize) or if they are unavailable the toolkit defaults.
 * ++eero 5/98:
 * - Aliases tags: ADDRESS, DFN, SAMP to styles BLOCKQUOTE, I, TT.
 * - Changed Kay's own 'area' and 'font' tags to 'textarea' and 'wfont'
 *   as the previous names are now used in the standard HTML.
 * - Added HEAD/TITLE handling.  Markers will now have header/title
 *   variables and doprint() will call window title callback when they
 *   are both set.
 *
 * TODO:
 * - Parse and then *skip* the HEAD element.  This would mean separate
 *   tag 'dictionaries' for HTML header and body (as they should have).
 *
 * $Id: html.c,v 1.3 2009-08-23 20:27:33 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

/*
 * distance (in pixels) between markers. Increasing this value saves memory
 * but decreases scrolling preformance.
 */
#define MARKER_DISTANCE	400

/*
 * we have two font types, a proportional font (FONT_N*) for normal text and
 * and a monospaced one (FONT_C*) for <TT> and the like.
 *
 * for each of them are four variants: roman, bold, italic and bold-italic.
 */
#define FONT_BFLAG	1
#define FONT_IFLAG	2

/* class defaults, initialized in html_init() */
static const char *html_fixed;
static const char *html_normal;
static int html_fsize;

#define FONT_N		(0)
#define FONT_NB		(FONT_N | FONT_BFLAG)
#define FONT_NI		(FONT_N | FONT_IFLAG)
#define FONT_NBI	(FONT_N | FONT_BFLAG | FONT_IFLAG)

#define FONT_C		(4)
#define FONT_CB		(FONT_C | FONT_BFLAG)
#define FONT_CI		(FONT_C | FONT_IFLAG)
#define FONT_CBI	(FONT_C | FONT_BFLAG | FONT_IFLAG)

#define FONT_MAX	(8)

/*
 * how deep lists can be nested
 */
#define LIST_STACK_SZ	(8)

/*
 * tokens for the text parser.
 */
#define TOK_SPACE	' '
#define TOK_TAB		'\t'
#define TOK_NL		'\n'
#define TOK_LT		'<'
#define TOK_GT		'>'
#define TOK_AMP		'&'
#define TOK_SEMICO	';'
#define TOK_QUOT	'"'
#define TOK_EQU		'='
#define TOK_EOF		(-1)
#define TOK_INDENT	' '	/* char which width is used for indenting */

/************************* Lots of Typedefs **************************/

/*
 * list stack entry
 */
typedef struct {
	long n;
	long y;
	short indent;
} liststack_t;

/*
 * a rectangle
 */
typedef struct _rect_t {
	long x, y;
	short wd, ht;
	short ismap;
	struct _rect_t *next;
} rect_t;

/*
 * an anchor
 */
typedef struct _anchor_t {
	short is_href;
	char *url;
	rect_t *recs;
	struct _anchor_t *next;
} anchor_t;

/*
 * a marker
 */
typedef struct _marker_t {
	struct _marker_t *next;
	long x, y;			/* position at which output for this
					   marker begins */

	long code_idx;			/* index into w->html_code */

	uchar last_char_was_space;	/* !=0 if last char was TOK_SPACE */
	uchar anchor_is_href;		/* !=0 if current anchor is HREF */
	uchar preformat;		/* #<pre> - #</pre> seen */
	uchar underline;		/* dito for <u> */
	uchar italic, bold, tt;		/* dito for <i>, <b>, <tt> */
	uchar header, title;		/* HTML header <head></head> */

	short indent;			/* x indentation */
	short line_ht;			/* max. height of all objects in
					   current line */

	liststack_t lstack[LIST_STACK_SZ]; /* stack for lists */
	short lsp;			   /* list stack pointer, index into
					      'lstack'. */
} marker_t;

/*
 * html widget
 */
typedef struct {
	widget_t w;
	uchar is_open;
	uchar is_realized;
	uchar is_locked;

	long new_y;

	char *html_code;		/* html source */

	WWIN  *parent_win;
	short parent_h;			/* height of the parent window */

	long yoffs;			/* offset of our window from w->w.y */
	long ymin, ymax;		/* output only inbetween these */

	WFONT *fonts[FONT_MAX];		/* fonts */
	short cur_f;			/* current font, index into 'fonts' */
	short line_dist;		/* distance between lines */

	char *cp, *cp2;			/* used by the scanner */

	marker_t *markers;		/* list of markers */
	marker_t *cur_m;		/* current marker */
	marker_t m0;			/* marker at end of text */
	marker_t m1;			/* used when redrawing */

	long area_x, area_y, area_w;	/* area into which code is rendered */
	long rborder;			/* right border */
	long marker_y;			/* y-pos of last marker */

	anchor_t *anchors;		/* list of anchors */
	anchor_t *cur_a;		/* current anchor */
	anchor_t *sel_a;		/* anchor onto which user clicked */
	long sel_x, sel_y;		/* rel. offsets in the anchor rect
					   into which user clicked */

	long (*img_query_cb) (widget_t *w, char *url, long *wd, long *ht);
	void (*img_place_cb) (widget_t *w, long handle, long x, long y);
	void (*img_inval_cb) (widget_t *w, int discard);
	void (*win_title_cb) (widget_t *w, char *title);

	void (*anchor_cb) (widget_t *w, char *url, long x, long y, int pressed);

	short usrwd, usrht;
} html_widget_t;

/*
 * entry of table of special (ie escaped) characters
 */
typedef struct {
	const char *name;
	short val;
} special_char_t;

/*
 * entry of argument vector for tag functions
 */
typedef struct {
	char *arg;
	char *val;
} arg_t;

/*
 * entry of tag function table
 */
typedef struct {
	const char *name;
	long (*fn) (html_widget_t *, int argc, arg_t *argv);
} ftab_t;

/****************************** Forwards ****************************/

static void html_vmove (html_widget_t *, long y);
static void html_hresize (html_widget_t *, long area_w);

/*************** Tag Function And Special Char Tables ***************/

/*
 * tag function prototypes
 */
static long tag_head		(html_widget_t *, int argc, arg_t *);
static long tag_title		(html_widget_t *, int argc, arg_t *);
static long tag_not_title	(html_widget_t *, int argc, arg_t *);
static long tag_not_head	(html_widget_t *, int argc, arg_t *);
static long tag_body		(html_widget_t *, int argc, arg_t *);

static long tag_h		(html_widget_t *, int argc, arg_t *);
static long tag_not_h		(html_widget_t *, int argc, arg_t *);

static long tag_hr		(html_widget_t *, int argc, arg_t *);
static long tag_br		(html_widget_t *, int argc, arg_t *);
static long tag_p		(html_widget_t *, int argc, arg_t *);

static long tag_pre		(html_widget_t *, int argc, arg_t *);
static long tag_not_pre		(html_widget_t *, int argc, arg_t *);
static long tag_b		(html_widget_t *, int argc, arg_t *);
static long tag_not_b		(html_widget_t *, int argc, arg_t *);
static long tag_i		(html_widget_t *, int argc, arg_t *);
static long tag_not_i		(html_widget_t *, int argc, arg_t *);
static long tag_u		(html_widget_t *, int argc, arg_t *);
static long tag_not_u		(html_widget_t *, int argc, arg_t *);
static long tag_tt		(html_widget_t *, int argc, arg_t *);
static long tag_not_tt		(html_widget_t *, int argc, arg_t *);

static long tag_ul		(html_widget_t *, int argc, arg_t *);
static long tag_not_ul		(html_widget_t *, int argc, arg_t *);
static long tag_li		(html_widget_t *, int argc, arg_t *);
static long tag_dt		(html_widget_t *, int argc, arg_t *);
static long tag_dd		(html_widget_t *, int argc, arg_t *);

static long tag_textarea	(html_widget_t *, int argc, arg_t *);
static long tag_wfont		(html_widget_t *, int argc, arg_t *);

static long tag_img		(html_widget_t *, int argc, arg_t *);

static long tag_a		(html_widget_t *, int argc, arg_t *);
static long tag_not_a		(html_widget_t *, int argc, arg_t *);

static long tag_bq		(html_widget_t *, int argc, arg_t *);
static long tag_not_bq		(html_widget_t *, int argc, arg_t *);

/*
 * tag function table. must be alphabetically sorted.
 */
static const ftab_t tag_funcs[] = {
	{ "/a",		tag_not_a },
	{ "/address",	tag_not_bq },
	{ "/b",		tag_not_b },
	{ "/blockquote",tag_not_bq },
	{ "/cite",	tag_not_i },
	{ "/code",	tag_not_tt },
	{ "/dir",	tag_not_ul },
	{ "/dfn",	tag_not_i },
	{ "/dl",	tag_not_ul },
	{ "/em",	tag_not_i },
	{ "/h1",	tag_not_h },
	{ "/h2",	tag_not_h },
	{ "/h3",	tag_not_h },
	{ "/h4",	tag_not_h },
	{ "/h5",	tag_not_h },
	{ "/h6",	tag_not_h },
	{ "/head",	tag_not_head },
	{ "/i",		tag_not_i },
	{ "/kbd",	tag_not_tt },
	{ "/menu",	tag_not_ul },
	{ "/ol",	tag_not_ul },
	{ "/pre",	tag_not_pre },
	{ "/samp",	tag_not_tt },
	{ "/strong",	tag_not_b },
	{ "/title",	tag_not_title },
	{ "/tt",	tag_not_tt },
	{ "/u",		tag_not_u },
	{ "/ul",	tag_not_ul },
	{ "/var",	tag_not_i },

	{ "a",		tag_a },
	{ "address",	tag_bq },
	{ "b",		tag_b },
	{ "blockquote",	tag_bq },
	{ "body",	tag_body },
	{ "br",		tag_br },
	{ "cite",	tag_i },
	{ "code",	tag_tt },
	{ "dd",		tag_dd },
	{ "dfn",	tag_i },
	{ "dir",	tag_ul },
	{ "dl",		tag_ul },
	{ "dt",		tag_dt },
	{ "em",		tag_i },
	{ "h1",		tag_h },
	{ "h2",		tag_h },
	{ "h3",		tag_h },
	{ "h4",		tag_h },
	{ "h5",		tag_h },
	{ "h6",		tag_h },
	{ "head",	tag_head },
	{ "hr",		tag_hr },
	{ "i",		tag_i },
	{ "img",	tag_img },
	{ "kbd",	tag_tt },
	{ "li",		tag_li },
	{ "menu",	tag_ul },
	{ "ol",		tag_ul },
	{ "p",		tag_p },
	{ "pre",	tag_pre },
	{ "samp",	tag_tt },
	{ "strong",	tag_b },
	{ "textarea",	tag_textarea },
	{ "title",	tag_title },
	{ "tt",		tag_tt },
	{ "u",		tag_u },
	{ "ul",		tag_ul },
	{ "var",	tag_i },
	{ "wfont",	tag_wfont }
};

/*
 * special character table. made complete from HTML-draft. /simon
 * must be alphabetically sorted.
 */
static const special_char_t special_chars[] = {
	{ "AElig",  	198	}, /* capital AE diphthong (ligature) */
	{ "Aacute", 	193	}, /* capital A, acute accent */
	{ "Acirc",  	194	}, /* capital A, circumflex accent */
	{ "Agrave", 	192	}, /* capital A, grave accent */
	{ "Aring",  	197	}, /* capital A, ring */
	{ "Atilde", 	195	}, /* capital A, tilde */
	{ "Auml",   	196	}, /* capital A, dieresis or umlaut mark */
	{ "Ccedil", 	199	}, /* capital C, cedilla */
	{ "ETH",    	208	}, /* capital Eth, Icelandic */
	{ "Eacute", 	201	}, /* capital E, acute accent */
	{ "Ecirc",  	202	}, /* capital E, circumflex accent */
	{ "Egrave", 	200	}, /* capital E, grave accent */
	{ "Euml",   	203	}, /* capital E, dieresis or umlaut mark */
	{ "Iacute", 	205	}, /* capital I, acute accent */
	{ "Icirc",  	206	}, /* capital I, circumflex accent */
	{ "Igrave", 	204	}, /* capital I, grave accent */
	{ "Iuml",   	207	}, /* capital I, dieresis or umlaut mark */
	{ "Ntilde", 	209	}, /* capital N, tilde */
	{ "Oacute", 	211	}, /* capital O, acute accent */
	{ "Ocirc",  	212	}, /* capital O, circumflex accent */
	{ "Ograve", 	210	}, /* capital O, grave accent */
	{ "Oslash", 	216	}, /* capital O, slash */
	{ "Otilde", 	213	}, /* capital O, tilde */
	{ "Ouml",   	214	}, /* capital O, dieresis or umlaut mark */
	{ "THORN",  	222	}, /* capital THORN, Icelandic */
	{ "Uacute", 	218	}, /* capital U, acute accent */
	{ "Ucirc",  	219	}, /* capital U, circumflex accent */
	{ "Ugrave", 	217	}, /* capital U, grave accent */
	{ "Uuml",   	220	}, /* capital U, dieresis or umlaut mark */
	{ "Yacute", 	221	}, /* capital Y, acute accent */
	{ "aacute", 	225	}, /* small a, acute accent */
	{ "acirc",  	226	}, /* small a, circumflex accent */
	{ "acute",  	180	}, /* acute accent */
	{ "aelig",  	230	}, /* small ae diphthong (ligature) */
	{ "agrave", 	224	}, /* small a, grave accent */
	{ "amp",	'&'	}, /* ampersand */
	{ "aring",  	229	}, /* small a, ring */
	{ "atilde", 	227	}, /* small a, tilde */
	{ "auml",   	228	}, /* small a, dieresis or umlaut mark */
	{ "brvbar", 	166	}, /* broken (vertical) bar */
	{ "ccedil", 	231	}, /* small c, cedilla */
	{ "cedil",  	184	}, /* cedilla */
	{ "cent",   	162	}, /* cent sign */
	{ "copy",   	169	}, /* copyright sign */
	{ "curren", 	164	}, /* general currency sign */
	{ "deg",    	176	}, /* degree sign */
	{ "divide", 	247	}, /* divide sign */
	{ "eacute", 	233	}, /* small e, acute accent */
	{ "ecirc",  	234	}, /* small e, circumflex accent */
	{ "egrave", 	232	}, /* small e, grave accent */
	{ "eth",    	240	}, /* small eth, Icelandic */
	{ "euml",   	235	}, /* small e, dieresis or umlaut mark */
	{ "frac12", 	189	}, /* fraction one-half */
	{ "frac14", 	188	}, /* fraction one-quarter */
	{ "frac34", 	190	}, /* fraction three-quarters */
	{ "gt",		'>'	}, /* greater than sign */
	{ "iacute", 	237	}, /* small i, acute accent */
	{ "icirc",  	238	}, /* small i, circumflex accent */
	{ "iexcl",  	161	}, /* inverted exclamation mark */
	{ "igrave", 	236	}, /* small i, grave accent */
	{ "iquest", 	191	}, /* inverted question mark */
	{ "iuml",   	239	}, /* small i, dieresis or umlaut mark */
	{ "laquo",  	171	}, /* angle quotation mark, left */
	{ "lt",		'<'	}, /* less than sign */
	{ "macr",   	175	}, /* macron */
	{ "micro",  	181	}, /* micro sign */
	{ "middot", 	183	}, /* middle dot */
	{ "nbsp",   	160	}, /* no-break space */
	{ "not",    	172	}, /* not sign */
	{ "ntilde", 	241	}, /* small n, tilde */
	{ "oacute", 	243	}, /* small o, acute accent */
	{ "ocirc",  	244	}, /* small o, circumflex accent */
	{ "ograve", 	242	}, /* small o, grave accent */
	{ "ordf",   	170	}, /* ordinal indicator, feminine */
	{ "ordm",   	186	}, /* ordinal indicator, masculine */
	{ "oslash", 	248	}, /* small o, slash */
	{ "otilde", 	245	}, /* small o, tilde */
	{ "ouml",   	246	}, /* small o, dieresis or umlaut mark */
	{ "para",   	182	}, /* pilcrow (paragraph sign) */
	{ "plusmn", 	177	}, /* plus-or-minus sign */
	{ "pound",  	163	}, /* pound sterling sign */
	{ "quot",	'"'	}, /* double quote sign */
	{ "raquo",  	187	}, /* angle quotation mark, right */
	{ "reg",    	174	}, /* registered sign */
	{ "sect",   	167	}, /* section sign */
	{ "shy",    	173	}, /* soft hyphen */
	{ "sup1",   	185	}, /* superscript one */
	{ "sup2",   	178	}, /* superscript two */
	{ "sup3",   	179	}, /* superscript three */
	{ "szlig",  	223	}, /* small sharp s, German (sz ligature) */
	{ "thorn",  	254	}, /* small thorn, Icelandic */
	{ "times",  	215	}, /* multiply sign */
	{ "uacute", 	250	}, /* small u, acute accent */
	{ "ucirc",  	251	}, /* small u, circumflex accent */
	{ "ugrave", 	249	}, /* small u, grave accent */
	{ "uml",    	168	}, /* umlaut (dieresis) */
	{ "uuml",   	252	}, /* small u, dieresis or umlaut mark */
	{ "yacute", 	253	}, /* small y, acute accent */
	{ "yen",    	165	}, /* yen sign */
	{ "yuml",   	255	}  /* small y, dieresis or umlaut mark */
};

/************************ Utility Funtions ********************************/

static void
setfont (html_widget_t *w)
{
	marker_t *m = w->cur_m;
	short idx;

	idx = m->tt ? FONT_C : FONT_N;
	if (m->bold)
		idx |= FONT_BFLAG;
	if (m->italic)
		idx |= FONT_IFLAG;

	w->cur_f = idx;
	w_setfont (w->w.win, w->fonts[idx]);
}

static void
settextstyle (html_widget_t *w)
{
	short style;

	style = w->cur_m->underline ? F_UNDERLINE : 0;
	w_settextstyle (w->w.win, style);
}

static int
loadfonts (html_widget_t *w, const char *fbase, WFONT **fp)
{
	if (!(fp[FONT_N] = wt_loadfont (fbase, html_fsize, 0, 0)))
		return -1;

	if (!(fp[FONT_NB] = wt_loadfont (fbase, html_fsize, F_BOLD, 0)))
		fp[FONT_NB] = wt_loadfont (fbase, html_fsize, 0, 0);

	if (!(fp[FONT_NI] = wt_loadfont (fbase, html_fsize, F_ITALIC, 0)))
		fp[FONT_NI] = wt_loadfont (fbase, html_fsize, 0, 0);

	if (!(fp[FONT_NBI] = wt_loadfont (fbase, html_fsize, F_BOLD | F_ITALIC, 0)))
		fp[FONT_NBI] = wt_loadfont (fbase, html_fsize, 0, 0);

	return 0;
}

static void
unloadfonts (WFONT **fp)
{
	int i;

	for (i = 0; i < 4; ++i) {
		if (fp[i]) {
			w_unloadfont (fp[i]);
		}
	}
}

/*
 * find the entry in tag_funcs the belongs to the tag 'name'.
 */
static const ftab_t *
lookup_tag_fn (const char *name)
{
	int l, r, m, i;

	l = 0;
	r = sizeof (tag_funcs) / sizeof (*tag_funcs) - 1;
	do {
		m = (l+r) >> 1;
		i = strcmp (name, tag_funcs[m].name);
		if (i == 0)
			return &tag_funcs[m];
		if (i < 0) {
			r = m-1;
		} else {
			l = m+1;
		}
	} while (l <= r);
	return NULL;
}

/*
 * find the entry in special_chars the belongs to the special character
 * 'name'.
 */
static const special_char_t *
lookup_special_char (const char *name)
{
	int l, r, m, i;

	l = 0;
	r = sizeof (special_chars) / sizeof (*special_chars) - 1;
	do {
		m = (l+r) >> 1;
		i = strcmp (name, special_chars[m].name);
		if (i == 0)
			return &special_chars[m];
		if (i < 0) {
			r = m-1;
		} else {
			l = m+1;
		}
	} while (l <= r);
	return NULL;
}

/*
 * determine whether part of the rectangle at (0,y0) with size (w->w.w, ht)
 * is visible.
 */
static inline int
isvisible (html_widget_t *w, long y0, long ht)
{
	return (y0+ht > w->ymin &&
		y0 < MIN (w->ymax, w->yoffs + 3*w->parent_h));
}

/******************** Anchor Support Functions **********************/

/*
 * add a new anchor to 'w'.
 */
static void
anchor_add (html_widget_t *w, const char *url, int is_href)
{
	anchor_t *ap;

	ap = malloc (sizeof (anchor_t));
	if (ap) {
		ap->url = strdup (url);
		if (!url) {
			free (ap);
			return;
		}
		ap->is_href = is_href;
		ap->recs = NULL;
		ap->next = w->anchors;
		w->cur_a = w->anchors = ap;
	}
}

/*
 * add a rectangle to the current anchor (w->cur_a)
 */
static void
anchor_add_rect (html_widget_t *w, int x, int y, int wd, int ht, int ismap)
{
	rect_t *rp;

	if (!w->cur_a)
		return;
	if ((rp = w->cur_a->recs) && !ismap && !rp->ismap &&
	    rp->x + rp->wd == x && rp->y == y && rp->ht == ht) {
		/*
		 * can join the rectangles
		 */
		rp->wd += wd;
		return;
	}
	rp = malloc (sizeof (rect_t));
	if (rp) {
		rp->x  = x;
		rp->y  = y;
		rp->wd = wd;
		rp->ht = ht;
		rp->ismap = ismap;
		rp->next = w->cur_a->recs;
		w->cur_a->recs = rp;
	}
}

/*
 * free all the anchors of the html widget 'w'
 */
static void
anchor_free_all (html_widget_t *w)
{
	anchor_t *ap, *next_ap;
	rect_t *rp, *next_rp;

	for (ap = w->anchors; ap; ap = next_ap) {
		for (rp = ap->recs; rp; rp = next_rp) {
			next_rp = rp->next;
			free (rp);
		}
		next_ap = ap->next;
		free (ap->url);
		free (ap);
	}
	w->anchors = NULL;
	w->cur_a = NULL;
	w->sel_a = NULL;
}

/*
 * Find the position of a NAME-anchor whose fragment identifier matches the
 * one given in 'url'.
 */
static anchor_t *
anchor_position (html_widget_t *w, const char *url, int *x, int *y)
{
	int xmin, ymin;
	anchor_t *ap;
	rect_t *rp;
	const char *cp;

	if ((cp = strchr (url, '#'))) {
		url = ++cp;
	}
	for (ap = w->anchors; ap; ap = ap->next) {
		if (ap->is_href)
			continue;
		cp = strchr (ap->url, '#');
		cp = cp ? cp+1 : ap->url;
		if (!strcasecmp (cp, url))
			break;
	}
	if (ap && ap->recs) {
		xmin = INT_MAX;
		ymin = INT_MAX;
		for (rp = ap->recs; rp; rp = rp->next) {
			if (rp->x < xmin)
				xmin = rp->x;
			if (rp->y < ymin)
				ymin = rp->y;
		}
		*x = xmin;
		*y = ymin;
		return ap;
	}
	return NULL;
}

/*
 * find the HREF-anchor (and rectangle) that contains point (x,y)
 */
static anchor_t *
anchor_find (html_widget_t *w, int x, int y, rect_t **rect)
{
	anchor_t *ap;
	rect_t *rp;

	for (ap = w->anchors; ap; ap = ap->next) {
		if (!ap->is_href)
			continue;
		for (rp = ap->recs; rp; rp = rp->next) {
			if ((ulong)(x - rp->x) < (ulong)rp->wd &&
			    (ulong)(y - rp->y) < (ulong)rp->ht) {
				if (rect)
					*rect = rp;
				return ap;
			}
		}
	}
	return NULL;
}

/*
 * highlight the anchor (highlighting it twice must return to
 * non-highlighted state).
 */
static void
anchor_highlight (html_widget_t *w, anchor_t *ap)
{
	rect_t *rp;

	w_setmode (w->w.win, M_INVERS);
	for (rp = ap->recs; rp; rp = rp->next) {
		w_pbox (w->w.win, rp->x, rp->y - w->yoffs, rp->wd, rp->ht);
	}
}

/*
 * return nonzero if we need to add anchors (ie when the html code is
 * parsed the first time).
 */
static inline int
anchor_needed (html_widget_t *w)
{
	return (w->cur_m == &w->m0);
}

/********************* Marker Support Functions ***********************/

/*
 * check if we need to save a new marker
 */
static inline int
marker_needed (html_widget_t *w)
{
	return (w->cur_m->y - w->marker_y >= MARKER_DISTANCE);
}

/*
 * save a marker
 */
static void
marker_save (html_widget_t *w, marker_t *mp)
{
	marker_t *newm;

	newm = malloc (sizeof (marker_t));
	if (newm) {
		*newm = *mp;
		newm->code_idx = w->cp - w->html_code;
		newm->next = w->markers;
		w->markers = newm;
		w->marker_y = mp->y;
	}
}

/*
 * "context switch" to marker 'mp'
 */
static void
marker_set (html_widget_t *w, marker_t *mp)
{
	if (mp != &w->m0) {
		w->m1 = *mp;
		mp = &w->m1;
	}
	w->cur_m = mp;
	w->cp = w->cp2 = &w->html_code[mp->code_idx];
	setfont (w);
	settextstyle (w);

	w->ymin = 0;
	w->ymax = LONG_MAX;
}

/*
 * delete all markers of an html widget
 */
static void
marker_free_all (html_widget_t *w)
{
	marker_t *mp, *next;

	for (mp = w->markers; mp; mp = next) {
		next = mp->next;
		free (mp);
	}
	w->markers = NULL;
	w->cur_m = &w->m0;
}

/*
 * find the first marker that is useable to redraw the widget in the area
 * starting at (0, y0) with size (w->w.w, +oo).
 */
static marker_t *
marker_find (html_widget_t *w, long y0)
{
	marker_t *mp;

	for (mp = w->markers; mp; mp = mp->next) {
		if (mp->y + mp->line_ht <= y0 || !mp->next)
			return mp;
	}
	return NULL;
}

/*
 * initialize a marker so default values
 */
static void
marker_init (html_widget_t *w, marker_t *m)
{
	m->code_idx = 0;

	m->preformat = 0;
	m->italic = 0;
	m->bold = 0;
	m->underline = 0;
	m->tt = 0;

	m->header = 0;
	m->last_char_was_space = 0;
	m->anchor_is_href = 0;

	m->line_ht = w->line_dist;
	m->indent = 0;
	m->x = w->area_x;
	m->y = w->area_y;

	m->lsp = 0;
}

/************************* HTML Parser ********************************/

/*
 * get the next lookahead-token from the input string
 */
static int
gettoken (html_widget_t *w, int preformat)
{
	char *cp = w->cp;

	if (!*cp) {
		return TOK_EOF;
	}
	if (!isspace (*cp) || preformat) {
		w->cp2 = w->cp + 1;
		return (uchar)*cp;
	}
	while (*cp && isspace (*cp)) {
		++cp;
	}
	w->cp2 = cp;
	return TOK_SPACE;
}

/*
 * eat the lookahead token
 */
static inline void
eattoken (html_widget_t *w)
{
	w->cp = w->cp2;
}

/*
 * parse an encoded character:
 *	&something;
 */
static int
parse_special (html_widget_t *w)
{
	char name[20];
	const special_char_t *spc;
	char *save_cp, *cp = name;
	int c;

	save_cp = w->cp;

	while ((c = gettoken (w, 0)) != TOK_EOF && c != TOK_SEMICO) {
		eattoken (w);
		if (cp - name < sizeof (name)-1)
			*cp++ = c;
	}
	if (c != TOK_SEMICO) {
		/*
		 * missing `;'. unget all the characters
		 */
		w->cp = w->cp2 = save_cp;
		return TOK_AMP;
	}
	eattoken (w);
	*cp = '\0';

	if (name[0] == '#' || isdigit (name[0])) {
		c = atoi (&name[name[0] == '#' ? 1 : 0]) & 0xff;
		return c ? c : TOK_SPACE;
	}
	if ((spc = lookup_special_char (name)))
		return spc->val;
	/*
	 * no such special char. Unget all the characters.
	 */
	w->cp = w->cp2 = save_cp;
	return TOK_AMP;
}

static inline int
convert_special (html_widget_t *w, int c)
{
	return (c != TOK_AMP) ? c : parse_special (w);
}

/*
 * parse a tag name/argument:
 *	"something with spaces"
 *	something
 */
static char *
parse_name (html_widget_t *w, char *cp, int sz)
{
	int c, end;

	if (sz <= 0)
		return NULL;

	while ((c = gettoken (w, 0)) == TOK_SPACE)
		eattoken (w);

	switch (c) {
	case TOK_EOF:
		eattoken (w);
		return NULL;

	case TOK_GT:
		return cp;

	case TOK_QUOT:
		eattoken (w);
		end = TOK_QUOT;
		break;

	default:
		eattoken (w);
		*cp++ = convert_special (w, c);
		--sz;
		end = TOK_SPACE;
		break;
	}
	while ((c = gettoken (w, 0)) != TOK_EOF &&
	       c != TOK_GT && c != TOK_EQU && c != end) {
		eattoken (w);
		c = convert_special (w, c);
		if (sz > 0) {
			*cp++ = c;
			--sz;
		}
	}
	if (c != TOK_GT && c != TOK_EQU)
		eattoken (w);

	if (sz > 0) {
		*cp++ = '\0';
		return cp;
	}
	return NULL;
}

/*
 * eat everything until we see '-->' But between '-' and '>' may be
 * whitespace.
 */
static int
parse_comment (html_widget_t *w)
{
	int c, lastc = 0;

	while (42) {
		while ((c = gettoken (w, 0)) != TOK_EOF) {
			eattoken (w);
			if (lastc == '-' && c == '-')
				break;
			lastc = c;
		}
		eattoken (w);
		if (c == EOF)
			return -1;

		while ((c = gettoken (w, 0)) != TOK_EOF && isspace (c))
			eattoken (w);
		eattoken (w);
		if (c == EOF)
			return -1;

		if (c == TOK_GT)
			return 0;
	}
}

#define ARGBUF	1024
#define ARGMAX	10

/*
 * parse and evaluate a tag: <tag name1[=val1] name2[=val2] ...>
 */
static int
parse_tag (html_widget_t *w)
{
	char buf[ARGBUF];
	arg_t args[ARGMAX], *argp = args;
	char *cp2, *cp = buf;
	const ftab_t *ftab;
	int c, i;

	for (i = 0; i < ARGMAX; ++argp) {
		argp->arg = cp;
		argp->val  = NULL;

		if (!(cp2 = parse_name (w, cp, ARGBUF-(cp-buf))))
			goto bad;

		if (cp2 != cp) {
			cp = cp2;
			++i;
		}
		if (i == 1 && !strncmp ("!--", argp->arg, 3)) {
			/*
			 * comments are special...
			 */
			return parse_comment (w);
		}
		while ((c = gettoken (w, 0)) == TOK_SPACE)
			eattoken (w);

		if (c == TOK_EOF)
			goto bad;
		if (c == TOK_GT) {
			eattoken (w);
			break;
		}
		if (c == TOK_EQU) {
			eattoken (w);
			argp->val = cp;
			if (!(cp = parse_name (w, cp, ARGBUF-(cp-buf))))
				goto bad;
		}
	}
	if (i <= ARGMAX) {
		for (cp = args[0].arg; *cp; ++cp)
			*cp = tolower (*cp);
		if ((ftab = lookup_tag_fn (args[0].arg)))
			return (*ftab->fn) (w, i, args);
		return -1;
	}

	/*
	 * something went wrong: eat the rest of the tag
	 */
bad:
	while ((c = gettoken (w, 0)) != TOK_EOF && c != TOK_GT)
		eattoken (w);
	eattoken (w);
	return -1;
}

/*
 * skip leading whitspace in 'cp'
 */
static inline char *
skipspace (char *cp)
{
	while (*cp == TOK_SPACE)
		++cp;
	return cp;
}

/*
 * do a linebreak
 */
static inline void
dolinebreak (html_widget_t *w)
{
	marker_t *m = w->cur_m;

	m->x = w->area_x + m->indent;
	m->y += MAX (w->line_dist, m->line_ht);
	m->line_ht = w->line_dist;
}

/*
 * print out what is in the buffer and advance x and y position
 * accordingly.
 */
static void
doprint (html_widget_t *w, char *buf)
{
	marker_t *m = w->cur_m;
	int y, wd, ht;

	if (m->header) {
		if (m->title && w->win_title_cb) {
			w->win_title_cb ((widget_t *)w, buf);
		}
		return;
	}

	if (!m->preformat && m->x <= w->area_x + m->indent) {
		/*
		 * skip leading space if at beginning of line and
		 * not in <PRE> mode.
		 */
		buf = skipspace (buf);
	}
	if (*buf) {
		wd = w_strlen (w->fonts[w->cur_f], buf);
		ht = w->fonts[w->cur_f]->height;
		y = m->y + w->line_dist - ht;
		/*
		 * dont print if its not inside the window
		 */
		if (isvisible (w, m->y, ht)) {
			w_printstring (w->w.win, m->x, y - w->yoffs, buf);
		}
		if (w->cur_a && anchor_needed (w)) {
			anchor_add_rect (w, m->x, m->y, wd, w->line_dist, 0);
		}
		m->x += wd;
	}
}

/*
 * parse and print out the html text.
 */
static int
parse_html (html_widget_t *w)
{
	char buf[100], *cp, *breakchar;
	marker_t *m = w->cur_m;
	int c, i, wd, owd = w->w.w;

	if (!w->html_code)
		return 0;

	++w->is_locked;

	wd = 0;
	breakchar = cp = buf;
	while (m->y <= w->ymax && (c = gettoken (w, m->preformat)) != TOK_EOF){
		eattoken (w);
		if (c == TOK_LT) {
			/*
			 * a tag
			 */
			*cp = 0;
			doprint (w, buf);
			breakchar = cp = buf;
			wd = 0;
			parse_tag (w);
		} else switch ((c = convert_special (w, c))) {
		case TOK_TAB:
			/*
			 * horizontal tab. will only occure in <PRE> mode.
			 */
			*cp = 0;
			doprint (w, buf);
			breakchar = cp = buf;

			wd = 8*w->fonts[w->cur_f]->widths[TOK_INDENT];
			i = m->x - w->area_x + wd;
			m->x = w->area_x + i - i % wd;

			while (m->x > w->area_x + w->area_w) {
				m->x -= (w->area_w - m->indent);
				m->y += MAX (w->line_dist, m->line_ht);
				m->line_ht = w->line_dist;
			}
			wd = 0;
			break;

		case TOK_NL:
			/*
			 * linefeed. will only occure in <PRE> mode.
			 */
			*cp = 0;
			doprint (w, buf);
			breakchar = cp = buf;
			wd = 0;
			dolinebreak (w);
			break;

		case TOK_SPACE:
			/*
			 * space.
			 */
			if (!m->preformat && m->last_char_was_space) {
				/*
				 * compress multiple spaces when not in <PRE>
				 * mode
				 */
				break;
			}
			m->last_char_was_space = 1;
			if (m->x + wd <= w->area_x + w->area_w) {
				breakchar = cp;
			}
			*cp++ = TOK_SPACE;
			wd += w->fonts[w->cur_f]->widths[TOK_SPACE];
			break;

		default:
			/*
			 * nonspace character
			 */
			m->last_char_was_space = 0;
			*cp++ = c;
			wd += w->fonts[w->cur_f]->widths[(uchar)c];
			break;
		}
		if (m->x + wd > w->area_x + w->area_w) {
			/*
			 * exceeded the right border. need a line-break.
			 */
			*cp = 0;
			if (breakchar > buf && breakchar < cp) {
				/*
				 * up to 'breakchar' everything in the
				 * buffer will fit into current line.
				 *
				 * 'breakchar' points to a character where
				 * we can nicely break the line (ie a space
				 * between two words).
				 */
				*breakchar = 0;
				doprint (w, buf);
				*breakchar = TOK_SPACE;
				if (!m->preformat) {
					breakchar = skipspace (breakchar);
				}
				wd = w_strlen (w->fonts[w->cur_f], breakchar);
				memmove (buf, breakchar, cp-breakchar);
				cp = buf + (cp-breakchar);
				breakchar = buf;
				dolinebreak (w);
			} else if (m->x > w->area_x + m->indent) {
				/*
				 * we have already something written to the
				 * current line and current word doesn't
				 * fit. break line and then print what we've
				 * got.
				 */
				dolinebreak (w);
				doprint (w, buf);
				breakchar = cp = buf;
				wd = 0;
			} else {
				/*
				 * the line is empty but the word is so long
				 * that it doesn't fit into one line. print
				 * it anyway and perform a linebreak
				 * afterwards.
				 */
				html_hresize (w, m->x + wd - w->area_x);
				doprint (w, buf);
				breakchar = cp = buf;
				wd = 0;
				dolinebreak (w);
			}
		} else if (cp - buf >= sizeof (buf) - 2) {
			/*
			 * buffer full. flush it
			 */
			*cp = 0;
			doprint (w, buf);
			breakchar = cp = buf;
			wd = 0;
		}
		if (cp == buf && m == &w->m0 && marker_needed (w)) {
			/*
			 * we have to place a marker.
			 */
			marker_save (w, w->cur_m);
		}
	}
	*cp = 0;
	doprint (w, buf);

	/*
	 * set the size of the widget so the whole output fits inside it.
	 * and remember the current pos in the text.
	 */
	if (m == &w->m0) {
		long ht = m->y + MAX (m->line_ht, w->line_dist) + w->line_dist;
		if (ht > w->w.h || w->w.w != owd) {
			if (ht > w->w.h)
				w->w.h = ht;
			wt_change_notify ((widget_t *)w, WT_CHANGED_SIZE);
		}
		m->code_idx = w->cp - w->html_code;
	}
	if (--w->is_locked == 0 && w->new_y != w->w.y) {
		/*
		 * do the move that was deferred until we leave parse_html()
		 */
		html_vmove (w, w->new_y);
	}
	return 0;
}

/************************ Tag Functions ********************************/


/* 
 * <head></head> HTML header contents won't be printed
 */
static long
tag_head (html_widget_t *w, int argc, arg_t *argv)
{
	w->cur_m->header++;
	return 0;
}

static long
tag_not_head (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->header)
		w->cur_m->header--;
	return 0;
}

static long
tag_title (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->header)
		w->cur_m->title++;
	return 0;
}

static long
tag_not_title (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->title)
		w->cur_m->title--;
	return 0;
}

static long
tag_body (html_widget_t *w, int argc, arg_t *argv)
{
	/* in case there's an unterminated header */
	w->cur_m->header = 0;
	w->cur_m->title = 0;
	return 0;
}

/*
 * process <h1>, ..., <h6>
 */
static long
tag_h (html_widget_t *w, int argc, arg_t *argv)
{
	int level = argv[0].arg[1];
	marker_t *m = w->cur_m;

	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist;

	switch (level) {
	case '1':
		if (++m->underline == 1) {
			settextstyle (w);
		}
	case '2':
		if (++m->bold == 1) {
			setfont (w);
		}
		break;
	case '3':
		if (++m->italic == 1) {
			setfont (w);
		}
		break;
	case '4':
	case '5':
	case '6':
		break;
	}
	return 0;
}

/*
 * process </h1>, ..., </h6>
 */
static long
tag_not_h (html_widget_t *w, int argc, arg_t *argv)
{
	char level = argv[0].arg[2];
	marker_t *m = w->cur_m;

	switch (level) {
	case '1':
		if (m->underline > 0 && --m->underline == 0) {
			settextstyle (w);
		}
	case '2':
		if (m->bold > 0 && --m->bold == 0) {
			setfont (w);
		}
		break;
	case '3':
		if (m->italic > 0 && --m->italic == 0) {
			setfont (w);
		}
		break;
	case '4':
	case '5':
	case '6':
		break;
	}
	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist/2;
	return 0;
}

static long
tag_hr (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;
	int margin;

	margin = (w->area_w - m->indent)*5/100;
	if (margin < 0)
		margin = 0;

	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist;
	if (isvisible (w, m->y, w->line_dist)) {
		w_setmode (w->w.win, M_DRAW);
		w_hline (w->w.win, w->area_x + m->indent + margin,
			 m->y - w->yoffs, w->area_x + w->area_w - margin - 1);
	}
	m->y += w->line_dist;
	return 0;
}

static long
tag_br (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->x > w->area_x + w->cur_m->indent) {
		dolinebreak (w);
	}
	return 0;
}

static long
tag_p (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;

	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist;
	return 0;
}

static long
tag_pre (html_widget_t *w, int argc, arg_t *argv)
{
	++w->cur_m->preformat;
	return 0;
}

static long
tag_not_pre (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->preformat > 0)
		--w->cur_m->preformat;
	return 0;
}

static long
tag_b (html_widget_t *w, int argc, arg_t *argv)
{
	if (++w->cur_m->bold == 1)
		setfont (w);
	return 0;
}

static long
tag_not_b (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->bold > 0 && --w->cur_m->bold == 0)
		setfont (w);
	return 0;
}

static long
tag_i (html_widget_t *w, int argc, arg_t *argv)
{
	if (++w->cur_m->italic == 1)
		setfont (w);
	return 0;
}

static long
tag_not_i (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->italic > 0 && --w->cur_m->italic == 0)
		setfont (w);
	return 0;
}

static long
tag_u (html_widget_t *w, int argc, arg_t *argv)
{
	if (++w->cur_m->underline == 1)
		settextstyle (w);
	return 0;
}

static long
tag_not_u (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->underline > 0 && --w->cur_m->underline == 0)
		settextstyle (w);
	return 0;
}

static long
tag_tt (html_widget_t *w, int argc, arg_t *argv)
{
	if (++w->cur_m->tt == 1)
		setfont (w);
	return 0;
}

static long
tag_not_tt (html_widget_t *w, int argc, arg_t *argv)
{
	if (w->cur_m->tt > 0 && --w->cur_m->tt == 0)
		setfont (w);
	return 0;
}

/*
 * set the area in which to print out text (HTML extension)
 */
static long
tag_textarea (html_widget_t *w, int argc, arg_t *argv)
{
	int i, j, changed = 0;
	long ax = 0, ay = 0, aw = 0;

	for (i = 1; i < argc; ++i) {
		j = argv[i].val ? atol (argv[i].val) : 0;
		if (!strcasecmp ("x", argv[i].arg)) {
			ax = j;
			changed |= 1;
		} else if (!strcasecmp ("y", argv[i].arg)) {
			ay = j;
			changed |= 2;
		} else if (!strcasecmp ("width", argv[i].arg)) {
			/*
			 * width is relative to the widget width
			 */
			aw = j;
			changed |= 4;
		}
	}
	if (changed) {
		if (changed & 1)
			w->area_x = MAX (0, ax);
		if (changed & 2)
			w->area_y = MAX (0, ay);
		if (changed & 4) {
			w->area_w = w->w.w + MIN (0, aw);
			w->rborder = MAX (0, -aw);
		}
		w->cur_m->x = w->area_x + w->cur_m->indent;
		w->cur_m->y = w->area_y;
	}
	return 0;
}

/*
 * set fonts (HTML extension)
 */
static long
tag_wfont (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;
	WFONT *fp[4];
	int i, j;
	const char *cp;

	for (i = 1; i < argc; ++i) {
		if (!strcasecmp ("teletype", argv[i].arg)) {
			j = FONT_C;
		} else if (!strcasecmp ("normal", argv[i].arg)) {
			j = FONT_N;
		} else {
			continue;
		}
		cp = (j == FONT_C) ? html_fixed : html_normal;
		cp = argv[i].val ? argv[i].val : cp;

#if 0
		/*
		 * work around 8 fonts-per-client-limit :(
		 */
		unloadfonts (&w->fonts[j]);
		if (loadfonts (w, cp, fp)) {
			loadfonts (w, j == FONT_C ? html_fixed : html_normal, fp);
			continue;
		}
#else
		if (loadfonts (w, cp, fp))
			continue;
		unloadfonts (&w->fonts[j]);
#endif
		w->fonts[j+0] = fp[0];
		w->fonts[j+1] = fp[1];
		w->fonts[j+2] = fp[2];
		w->fonts[j+3] = fp[3];
	}
	i = MAX (w->fonts[FONT_N]->height, w->fonts[FONT_C]->height);
	if (i < w->line_dist && m->x > w->area_x + m->indent) {
		/*
		 * line distance is smaller for the new fonts
		 * and the current line is not empty.
		 */
		if (w->line_dist > m->line_ht)
			m->line_ht = w->line_dist;
	}
	w->line_dist = i;
	return 0;
}

/*
 * process <ul>, <ol>, <dl>, <menu> or <dir>.
 */
static long
tag_ul (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;
	liststack_t *lsp;

	if (++m->lsp >= LIST_STACK_SZ)
		return 0;

	lsp = &m->lstack[m->lsp];

	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist/4;

	if (!strcmp ("ol", argv[0].arg)) {
		lsp->indent = 5*w->fonts[w->cur_f]->widths[TOK_INDENT];
		lsp->n = 1;
	} else if (!strcmp ("dl", argv[0].arg)) {
		lsp->indent = MIN ((w->area_w - m->indent)/4, 20);
		lsp->n = 0;
		m->x -= lsp->indent;
	} else {
		lsp->indent = 3*w->fonts[w->cur_f]->widths[TOK_INDENT];
		lsp->n = 0;
	}
	lsp->y = m->y-1; /* must be != m->y */

	m->indent += lsp->indent;
	m->x += lsp->indent;
	return 0;
}

/*
 * process </ul>, </ol>, </dl>, </menu> or </dir>.
 */
static long
tag_not_ul (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;

	if (m->lsp <= 0)
		return 0;

	m->indent -= m->lstack[m->lsp].indent;
	m->x  -= m->lstack[m->lsp].indent;

	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist/4;
	--m->lsp;
	return 0;
}

/*
 * process <li> inside <ol>, <ul>, <menu> or <dir>.
 */
static long
tag_li (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;
	liststack_t *lsp;
	int x, y, r;

	if (m->lsp >= LIST_STACK_SZ)
		return 0;

	lsp = &m->lstack[m->lsp];

	if (m->x > w->area_x + m->indent || lsp->y == m->y) {
		dolinebreak (w);
	}
	m->y += w->line_dist/4;
	lsp->y = m->y;

	if (lsp->n == 0) {
		/*
		 * <UL>, <MENU> or <DIR>
		 */
		x = m->x - lsp->indent/2;
		y = m->y + w->line_dist - w->fonts[w->cur_f]->height/2 - 1;
		r = MIN (lsp->indent, w->fonts[w->cur_f]->height)/4;

		if (isvisible (w, y, r+r)) {
			w_setmode (w->w.win, M_DRAW);
			w_pcircle (w->w.win, x, y - w->yoffs, r);
		}
	} else {
		/*
		 * <OL>
		 */
		char num[10];
		sprintf (num, "%3ld. ", lsp->n);

		x = m->x - w_strlen (w->fonts[w->cur_f], num);
		y = m->y + w->line_dist - w->fonts[w->cur_f]->height;

		if (isvisible (w, y, w->line_dist)) {
			w_printstring (w->w.win, x, y - w->yoffs, num);
		}
		++lsp->n;
	}
	return 0;
}

/*
 * process <dt> inside <dl>.
 */
static long
tag_dt (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;
	liststack_t *lsp;
	
	if (m->lsp >= LIST_STACK_SZ)
		return 0;

	lsp = &m->lstack[m->lsp];

	if (m->x > w->area_x + m->indent - lsp->indent ||
	    lsp->y == m->y) {
		dolinebreak (w);
	}
	m->y += w->line_dist/4;
	m->x  = w->area_x + m->indent - lsp->indent;

	lsp->n = m->y-1; /* must be != m->y */
	lsp->y = m->y;
	return 0;
}

/*
 * process <dd> inside <dl>.
 */
static long
tag_dd (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;
	liststack_t *lsp;

	if (m->lsp >= LIST_STACK_SZ)
		return 0;

	lsp = &m->lstack[m->lsp];

	if (m->x > w->area_x + m->indent || lsp->n == m->y) {
		dolinebreak (w);
	}
	m->x   = w->area_x + m->indent;
	lsp->n = m->y;
	return 0;
}

/*
 * ignores ALIGN (does always top-alignment).
 */
static long
tag_img (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;
	char *alt = NULL, *src = NULL;
	long x, y, wd, ht, i, ismap = 0;
	long handle;

	if (!w->img_query_cb)
		return 0;
	for (i = 1; i < argc; ++i) {
		if (!strcasecmp ("src", argv[i].arg)) {
			src = argv[i].val;
		} else if (!strcasecmp ("alt", argv[i].arg)) {
			alt = argv[i].val;
		} else if (!strcasecmp ("ismap", argv[i].arg)) {
			ismap = 1;
		}
	}
	if (src) {
		/*
		 * ask user for the size of the picture
		 */
		handle = (*w->img_query_cb) ((widget_t *)w, src, &wd, &ht);
		if (!handle)
			return 0;
		/*
		 * break the line if picture does not fit and something is
		 * in the line already
		 */
		if (m->x + wd + 2 > w->area_x + w->area_w &&
		    m->x > w->area_x + m->indent) {
			dolinebreak (w);
		}
		if (m->x + wd + 2 > w->area_x + w->area_w)
			html_hresize (w, m->x + wd + 2 - w->area_x);

		x = m->x + 1;
		y = m->y + 1;

		if (w->cur_a && anchor_needed (w)) {
			anchor_add_rect (w, x, y, wd, ht, ismap);
		}
		if (isvisible (w, y, ht)) {
			/*
			 * draw an "outline" of the picture.
			 */
			w_setmode (w->w.win, M_DRAW);
			w_box (w->w.win, x, y - w->yoffs, wd, ht);
			/*
			 * draw the ALT string (if present and fits) into
			 * the outline
			 */
			if (alt && w->fonts[FONT_N]->height <= ht-4 &&
			    w_strlen (w->fonts[FONT_N], alt) <= wd-4) {
				w_setfont (w->w.win, w->fonts[FONT_N]);
				w_settextstyle (w->w.win, 0);
				wt_text (w->w.win, w->fonts[FONT_N], alt,
					x, y - w->yoffs, wd, ht, AlignCenter);
				setfont (w);
				settextstyle (w);
			}
			/*
			 * tell user where the picture is placed (he is
			 * responsible for getting the picture and
			 * putblock()ing it to the position we tell him).
			 */
			if (w->img_place_cb) {
				(*w->img_place_cb) ((widget_t *)w, handle,
					x, y - w->yoffs);
			}
		}
		m->last_char_was_space = 0;

		ht += 2;
		wd += 2;
		if (ht > m->line_ht)
			m->line_ht = ht;
		if ((m->x += wd) >= w->area_x + w->area_w)
			dolinebreak (w);
	}
	return 0;
}

/*
 * an anchor. turn on underlining for HREF's.
 */
static long
tag_a (html_widget_t *w, int argc, arg_t *argv)
{
	char *url = NULL;
	int i, is_href = 0;

	for (i = 1; i < argc; ++i) {
		if (!strcasecmp ("name", argv[i].arg)) {
			url = argv[i].val;
			is_href = 0;
		} else if (!strcasecmp ("href", argv[i].arg)) {
			url = argv[i].val;
			is_href = 1;
		}
	}
	if (url) {
		w->cur_m->anchor_is_href = is_href;
		if (anchor_needed (w))
			anchor_add (w, url, is_href);
		if (is_href && ++w->cur_m->underline == 1)
			settextstyle (w);
	}
	return 0;
}

static long
tag_not_a (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;

	if (anchor_needed (w)) {
		if (!w->cur_a) {
			/* Hmmm. Probably a missing or nested <A ...> */
			return 0;
		}
		if (!w->cur_a->recs) {
			/*
			 * an empty anchor. add a dummy rectangle so
			 * we know where the anchor is.
			 */
			anchor_add_rect (w, m->x, m->y, 0, 0, 0);
		}
		w->cur_a = NULL;
	}
	if (m->anchor_is_href && m->underline > 0 && --m->underline == 0)
		settextstyle (w);
	return 0;
}

/*
 * BLOCKQUOTE
 */
static long
tag_bq (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;

	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist;
	m->indent += 10;
	m->x = w->area_x + m->indent;

	if (++m->italic == 1)
		setfont (w);
	return 0;
}

static long
tag_not_bq (html_widget_t *w, int argc, arg_t *argv)
{
	marker_t *m = w->cur_m;

	if (m->x > w->area_x + m->indent) {
		dolinebreak (w);
	}
	m->y += w->line_dist;
	m->indent -= 10;
	m->x = w->area_x + m->indent;

	if (m->italic > 0 && --m->italic == 0)
		setfont (w);
	return 0;
}

/************************** Widget Code ******************************/


static long html_query_geometry (widget_t *, long *, long *, long *, long *);

/*
 * the html code has changed. render it.
 */
static void
html_newcode (html_widget_t *w)
{
	if (w->w.w != w->area_x + w->area_w + w->rborder) {
		w->w.w = w->area_x + w->area_w + w->rborder;
		w_resize (w->w.win, w->w.w, 3*w->parent_h);
		wt_change_notify ((widget_t *)w, WT_CHANGED_SIZE);
	}
	w_setmode (w->w.win, M_CLEAR);
	w_pbox (w->w.win, 0, 0, w->w.w, 3*w->parent_h);

	marker_free_all (w);
	anchor_free_all (w);

	marker_init (w, &w->m0);
	w->marker_y = w->m0.y;

	marker_set (w, &w->m0);
	marker_save (w, &w->m0);

	w->w.h = 0;
	parse_html (w);
}

static void
html_hresize (html_widget_t *w, long area_w)
{
	if (w->area_x + area_w > w->w.w) {
		w->w.w = w->area_x + area_w + w->rborder;
		w->area_w = area_w;
		w_resize (w->w.win, w->w.w, 3*w->parent_h);
	}
	/*
	 * caller is responsible for calling wt_change_notify()
	 */
}

/*
 * move the html widget to a new y position. this is tricky since our window's
 * height is only 3 times the height of the parent window...
 */
static void
html_vmove (html_widget_t *w, long y)
{
	long yoffs, ymin, ymax, delta;
	marker_t *mp;

	if (w->is_locked) {
		/*
		 * parse_html() is currently active. so we must defer
		 * the move until we leave it.
		 */
		w->new_y = y;
		return;
	}
	if (y + w->yoffs <= 0 && y + w->yoffs + 3*w->parent_h >= w->parent_h) {
		/*
		 * can just move the window without redraw
		 */
		w->w.y = w->new_y = y;
		w_move (w->w.win, w->w.x, y + w->yoffs);
		wt_change_notify ((widget_t *)w, WT_CHANGED_POS);
		return;
	}
	if (w->img_inval_cb)
		(*w->img_inval_cb) ((widget_t *)w, 0);

	/*
	 * our window is three times as high as the parent window.
	 * our window has been moved so that part of the parent would
	 * be come visible:
	 *
	 * +---+     +-+
	 * |+-+| or  | |   <- our window
	 * ++-++     | |
	 *  | |     ++-++
	 *  | |     |+-+|  <- parent
	 *  +-+     +---+
	 *
	 * we move it (except when the *widget* is at the bottom or the top
	 * of the parent) so that the parent is in the middle of our window:
	 *
	 *  +-+
	 * ++-++
	 * || ||
	 * ++-++
	 *  +-+
	 *
	 * and redraw the contents of our window.
	 */
	 
	yoffs = - y - w->parent_h;
	if (yoffs < 0) {
		/*
		 * at top
		 */
		yoffs = 0;
	} else if (yoffs > w->w.h - 3*w->parent_h) {
		/*
		 * at bottom
		 */
		yoffs = w->w.h - 3*w->parent_h;
	}

	delta = abs (yoffs - w->yoffs);
	if (delta < 3*w->parent_h) {
		/*
		 * we can copy some part of the window
		 * contents. this is faster than redrawing.
		 */
		if (w->yoffs < yoffs) {
			w_vscroll (w->w.win, 0, delta,
				w->w.w, 3*w->parent_h - delta, 0);
			ymin = w->yoffs + 3*w->parent_h;
			ymax = yoffs + 3*w->parent_h;
		} else {
			w_vscroll (w->w.win, 0, 0,
				w->w.w, 3*w->parent_h - delta, delta);
			ymin = yoffs;
			ymax = w->yoffs;
		}
	} else {
		/*
		 * have to redraw the whole window
		 */
		ymin = yoffs;
		ymax = yoffs + 3*w->parent_h;
	}
	w_setmode (w->w.win, M_CLEAR);
	w_pbox (w->w.win, 0, ymin - yoffs, w->w.w, ymax - ymin);

	w->yoffs = yoffs;
	w->w.y = w->new_y = y;
	w_move (w->w.win, w->w.x, y + yoffs);
	wt_change_notify ((widget_t *)w, WT_CHANGED_POS);

	/*
	 * find the marker whose y position is just above the pane that
	 * we have to redraw and use it to reconstruct the contents of
	 * the widget
	 */
	mp = marker_find (w, ymin);
	if (mp) {
		marker_set (w, mp);
		w->ymin = ymin;
		w->ymax = ymax;
		parse_html (w);
	}
}

static long
html_init (void)
{
	const char *size;

	html_fixed = wt_variable("html_fixed");
	html_normal = wt_variable("html_normal");
	size = wt_variable("html_fsize");
	if (!html_fixed) {
		html_fixed = wt_global.font_fixed;
	}
	if (!html_normal) {
		html_normal = wt_global.font_normal;
	}
	if (size) {
		html_fsize = atoi(size);
	} else {
		html_fsize = wt_global.font_size;
	}
	return 0;
}

static widget_t *
html_create (widget_class_t *cp)
{
	html_widget_t *wp;
	int i;

	wp = malloc (sizeof (html_widget_t));
	if (!wp)
		return NULL;
	memset (wp, 0, sizeof (html_widget_t));
	wp->w.class = wt_html_class;

	if (loadfonts (wp, html_normal, &wp->fonts[FONT_N])) {
		free (wp);
		return NULL;
	}
	/* fixed width font */
	if (loadfonts (wp, html_fixed, &wp->fonts[FONT_C])) {
		unloadfonts (&wp->fonts[FONT_N]);
		free (wp);
		return NULL;
	}
	i = MAX (wp->fonts[FONT_N]->height, wp->fonts[FONT_C]->height);
	wp->line_dist = i;

	return (widget_t *)wp;
}

static long
html_delete (widget_t *_w)
{
	html_widget_t *w = (html_widget_t *)_w;

	if (w->is_realized)
		w_delete (w->w.win);
	wt_ungetfocus (_w);

	unloadfonts (&w->fonts[FONT_N]);
	unloadfonts (&w->fonts[FONT_C]);

	anchor_free_all (w);
	marker_free_all (w);
	free (w);
	return 0;
}

static long
html_close (widget_t *_w)
{
	html_widget_t *w = (html_widget_t *)_w;

	if (w->is_realized && w->is_open) {
		w_close (w->w.win);
		w->is_open = 0;
	}
	return 0;
}

static long
html_open (widget_t *_w)
{
	html_widget_t *w = (html_widget_t *)_w;

	if (w->is_realized && !w->is_open) {
		w_open (w->w.win, w->w.x, w->w.y + w->yoffs);
		w->is_open = 1;
	}
	return 0;
}

static long
html_addchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
html_delchild (widget_t *parent, widget_t *w)
{
	return -1;
}

static long
html_realize (widget_t *_w, WWIN *parent)
{
	html_widget_t *w = (html_widget_t *)_w;
	long x, y, wd, ht;
	short dummy;

	if (w->is_realized)
		return -1;

	html_query_geometry (_w, &x, &y, &wd, &ht);
	w->w.w = wd;
	w->w.h = ht;

	if (w->area_w <= 0)
		w->area_w = w->w.w - w->area_x;

	w->parent_win = parent;
	w_querywinsize (parent, 0, &dummy, &w->parent_h);

	w->w.win = wt_create_window (parent, wd, 3*w->parent_h,
					W_NOBORDER|W_MOVE|EV_MOUSE, _w);
	if (!w->w.win)
		return -1;
	w->w.win->user_val = (long)w;

	html_newcode (w);

	w->is_realized = 1;
	w->is_open = 1;
	w_open (w->w.win, w->w.x, w->w.y);
	return 0;
}

static long
html_query_geometry (widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
	html_widget_t *w = (html_widget_t *)_w;

	*xp = w->w.x;
	*yp = w->w.y;
	if (w->w.w > 0 && w->w.h > 0) {
		*wdp = w->w.w;
		*htp = w->w.h;
	} else {
		(*_w->class->query_minsize) (_w, wdp, htp);
		if (w->w.w > 0)
			*wdp = w->w.w;
	}
	return 0;
}

static long
html_query_minsize (widget_t *_w, long *wdp, long *htp)
{
	html_widget_t *w = (html_widget_t *)_w;

	if (!w->is_realized) {
		*wdp = w->usrwd > 0 ? w->usrwd : 100;
		*htp = w->usrht > 0 ? w->usrht : 100;
	} else {
		*wdp = 10;
		*htp = 10;
	}
	return 0;
}

static long
html_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
	html_widget_t *w = (html_widget_t *)_w;
	int ret = 0;

	if (x != w->w.x || y != w->w.y) {
		if (w->is_realized && x != w->w.x)
			w_move (w->w.win, x, w->w.y + w->yoffs);
		w->w.x = x;
		if (w->is_realized && y != w->w.y)
			html_vmove (w, y);
		else
			w->w.y = y;
		ret = 1;
	}
	if (w->is_realized) {
		long pht;
		wt_geometry (w->w.parent, NULL, NULL, NULL, &pht);
		if (pht > w->parent_h) {
			marker_t *mp;
			w_resize (w->w.win, w->w.w, 3*pht);
			mp = marker_find (w, - w->w.y + 3*w->parent_h);
			if (mp) {
				marker_set (w, mp);
				w->ymin = - w->w.y + 3*w->parent_h;
				w->ymax = - w->w.y + 3*pht;
				w->parent_h = pht;
				parse_html (w);
			}
			w->parent_h = pht;
		}
	}
	if (wd != w->w.w) {
		w->area_w = wd - (w->w.w - w->area_w);
		w->w.w = wd;
		w_resize (w->w.win, wd, 3*w->parent_h);
		if (w->is_realized) {
			/*
			 * tell the user the positions of the inline
			 * images are becoming invalid.
			 */
			if (w->img_inval_cb)
				(*w->img_inval_cb) (_w, 0);
			html_newcode (w);
		}
		ret = 1;
	}
	return ret;
}

static long
html_setopt (widget_t *_w, long key, void *val)
{
	html_widget_t *w = (html_widget_t *)_w;
	short mask = 0;
	char *cp;

	switch (key) {
	case WT_XPOS:
		if (html_reshape (_w, *(long *)val, w->w.y, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_YPOS:
		if (html_reshape (_w, w->w.x, *(long *)val, w->w.w, w->w.h))
			mask |= WT_CHANGED_POS;
		break;

	case WT_WIDTH:
		w->usrwd = MAX (0, *(long *)val);
		if (html_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_HEIGHT:
		w->usrht = MAX (0, *(long *)val);
		if (html_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
			mask |= WT_CHANGED_SIZE;
		break;

	case WT_LABEL:
		if (w->is_locked)
			return -1;
		if (!(cp = strdup (val))) {
			return -1;
		}
		if (w->html_code) {
			free (w->html_code);
		}
		if (w->is_realized && w->img_inval_cb) {
			/*
			 * tell the user the positions of the inline
			 * images are becoming invalid.
			 */
			(*w->img_inval_cb) (_w, 1);
		}
		w->html_code = cp;
		if (w->is_realized) {
			html_newcode (w);
		}
		break;

	case WT_APPEND:
		if (w->is_locked)
			return -1;
		if (!w->html_code) {
			w->html_code = strdup ((char *)val);
			if (!w->html_code)
				return -1;
		} else {
			char *cp;
			int len1, len2;

			len1 = strlen (w->html_code);
			len2 = strlen ((char *)val);
			cp = realloc (w->html_code, len1 + len2 + 1);
			if (!cp)
				return -1;
			memcpy (cp+len1, val, len2);
			cp[len1+len2] = 0;
			w->html_code = cp;
		}
		if (w->is_realized) {
			marker_set (w, &w->m0);
			parse_html (w);
		}
		break;

	case WT_QUERY_CB:
		w->img_query_cb = val;
		break;

	case WT_PLACE_CB:
		w->img_place_cb = val;
		break;

	case WT_INVAL_CB:
		w->img_inval_cb = val;
		break;

	case WT_TITLE_CB:
		w->win_title_cb = val;
		break;

	case WT_ACTION_CB:
		w->anchor_cb = val;
		break;
	
	default:
		return -1;
	}
	if (mask && w->is_realized)
		wt_change_notify (_w, mask);
	return 0;
}

static long
html_getopt (widget_t *_w, long key, void *val)
{
	html_widget_t *w = (html_widget_t *)_w;
	int x, y;

	switch (key) {
	case WT_XPOS:
		*(long *)val = w->w.x;
		break;

	case WT_YPOS:
		*(long *)val = w->w.y;
		break;

	case WT_WIDTH:
		*(long *)val = w->w.w;
		break;

	case WT_HEIGHT:
		*(long *)val = w->w.h;
		break;

	case WT_POSITION:
		if (anchor_position (w, *(char **)val, &x, &y)) {
			*(long *)val = y;
			break;
		}
		return -1;
	
	case WT_QUERY_CB:
		*(void **)val = w->img_query_cb;
		break;

	case WT_PLACE_CB:
		*(void **)val = w->img_place_cb;
		break;

	case WT_INVAL_CB:
		*(void **)val = w->img_inval_cb;
		break;

	case WT_TITLE_CB:
		*(void **)val = w->win_title_cb;
		break;

	case WT_ACTION_CB:
		*(void **)val = w->anchor_cb;
		break;

	case WT_LABEL:
		*(char **)val = w->html_code;
		break;

	default:
		return -1;
	}
	return 0;
}

static WEVENT *
html_event (widget_t *_w, WEVENT *ev)
{
	html_widget_t *w = (html_widget_t *)_w;
	rect_t *rp;

	switch (ev->type) {
	case EVENT_MPRESS:
		if (!(ev->key & BUTTON_LEFT))
			return ev;

		wt_getfocus (_w);
		w->sel_a = anchor_find (w, ev->x, ev->y + w->yoffs, &rp);
		if (w->sel_a) {
			if (rp->ismap) {
				w->sel_x = ev->x - rp->x;
				w->sel_y = ev->y + w->yoffs - rp->y;
			} else {
				w->sel_x = -1;
				w->sel_y = -1;
			}
			anchor_highlight (w, w->sel_a);
			if (w->anchor_cb) {
				(*w->anchor_cb) (_w, w->sel_a->url,
					w->sel_x, w->sel_y, 1);
			}
		}
		break;

	case EVENT_MRELEASE:
		if (!(ev->key & BUTTON_LEFT))
			return ev;

		if (w->sel_a) {
			anchor_t *ap;

			ap = anchor_find (w, ev->x, ev->y + w->yoffs, NULL);
			anchor_highlight (w, w->sel_a);
			if (w->anchor_cb) {
				/*
				 * if the user has left the anchor
				 * when releasing button he will get -1,
				 * and 0 if the mouse is still over the
				 * same anchor.
				 */
				(*w->anchor_cb) (_w, w->sel_a->url,
					w->sel_x, w->sel_y,
					w->sel_a == ap ? 0 : -1);
			}
			w->sel_a = NULL;
		}
		break;

	default:
		return ev;
	}

	return NULL;
}

static long
html_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static long
html_focus (widget_t *w, int enter)
{
	return 0;
}

static widget_class_t _wt_html_class = {
	"html", 0,
	html_init,
	html_create,
	html_delete,
	html_close,
	html_open,
	html_addchild,
	html_delchild,
	html_realize,
	html_query_geometry,
	html_query_minsize,
	html_reshape,
	html_setopt,
	html_getopt,
	html_event,
	html_changes,
	html_changes,
	html_focus
};

widget_class_t *wt_html_class = &_wt_html_class;
