/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * $Id: main.c,v 1.3 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <Wlib.h>
#include <Wt.h>

#include "wetscape.h"
#include "url.h"
#include "io.h"
#include "mime.h"
#include "image.h"
#include "util.h"
#include "wt_keys.h"

#ifdef __MINT__
long _stksize = 32000;
#endif

/*
 * initial configuration for html widget
 */
#if 1	/* use Wt / widget defaults */
#define INITIAL_SETTINGS "<area x=10 y=10 width=-20>"
#else	/* old */
#define INITIAL_SETTINGS \
  "<font normal=lucidat10 teletype=cour10><area x=10 y=10 width=-20>"
#endif


#define BUT_QUIT	0
#define BUT_BACK	1
#define BUT_FORW	2
#define BUT_RELO	3
#define BUT_HOME	4
#define BUT_BOOK	5
#define BUT_OPEN	6
#define BUT_STOP	7
#define BUT_MAX 	8

static widget_t *top, *viewp, *vpane;
static widget_t *urlbox, *urllabel, *urltext;
static widget_t *butbox, *buttons[BUT_MAX];
static widget_t *html, *statusbar;
widget_t *shell;

static io_t*	glob_downloaders;
static image_t*	glob_images;
static int	glob_canceled;

static url_t *glob_currenturl, *glob_lasturl;
static int glob_save_next = 0;

static char urlbuf[128] = "";


/********* Functions to access the status bar and html widget **********/

void
status_set (const char *format, ...)
{
	static char buf[1000];
	va_list args;

	va_start (args, format);
	vsprintf (buf, format, args);
	va_end (args);
	wt_setopt (statusbar, WT_LABEL, buf, WT_EOL);
	w_flush ();
}

void
html_clear (void)
{
	long y = 0, ht = 300;
	wt_setopt (html,
		WT_LABEL, "",
		WT_YPOS, &y,
		WT_HEIGHT, &ht,
		WT_EOL);
}

void
html_resize (long wd, long ht)
{
	wt_setopt (html,
		WT_HEIGHT, &ht,
		WT_WIDTH, &wd,
		WT_EOL);
}

void
html_set (const char *str)
{
	wt_setopt (html, WT_LABEL, str, WT_EOL);
}

void
html_append (const char *str)
{
	wt_setopt (html, WT_APPEND, str, WT_EOL);
}

void
html_setpic (url_t *url)
{
	char buf1[1024], buf2[1024];
	url_print (url, buf1, 1024);
	sprintf (buf2, "<img src=\"%s\">", buf1);
	html_append (buf2);
}

void
html_goto (const char *frag)
{
	long y;

	if (frag) {
		y = (long)frag;
		if (!wt_getopt (html, WT_POSITION, &y, WT_EOL)) {
			y = -y;
			wt_setopt (html, WT_YPOS, &y, WT_EOL);
		}
	} else {
		/*
		 * no fragment means goto top of document
		 */
		y = 0;
		wt_setopt (html, WT_YPOS, &y, WT_EOL);
	}
}

WWIN *
html_getwin (void)
{
	return wt_widget2win (html);
}

void
urltext_set (const char *str)
{
	if (str != urlbuf)
		strncpy (urlbuf, str, sizeof (urlbuf));
	urlbuf[sizeof (urlbuf) - 1] = '\0';
	wt_setopt (urltext,
		WT_STRING_ADDRESS, urlbuf,
		WT_EOL);
}

/*************** Functions for downloading Documents ***************/

static void
downloader_add (io_t *iop)
{
	iop->next= glob_downloaders;
	glob_downloaders = iop;
}

static int
downloader_del (io_t *iop)
{
	io_t **prev, *this;

	prev = &glob_downloaders;
	for (this = *prev; this; prev = &this->next, this = *prev) {
		if (this == iop) {
			*prev = iop->next;
			return 0;
		}
	}
	return -1;
}

static void
download_cancel (void)
{
	io_t *iop, *next;

	for (iop = glob_downloaders; iop; iop = next) {
		next = iop->next;
		(*iop->done) (iop, IO_CANCEL);
	}
	glob_canceled = 1;
}

static void
download_done (io_t *iop, int err)
{
	/*
	 * remove it from the list
	 */
	downloader_del (iop);

	switch (err) {
	case IO_OK:
		if (iop->main_doc)
			html_goto (iop->url->frag);
		if (!glob_downloaders) {
			/*
			 * the last downloader has just finished its work
			 */
			status_set ("document done.");
		}
		break;

	case IO_CANCEL:
		if (!glob_downloaders) {
			/*
			 * the last downloader has been canceled
			 */
			status_set ("download canceled.");
		}
		break;

	default:
		status_set (io_strerror (iop, err));
	}
	io_deref (iop);
}

static void
download_redirect (io_t *iop, url_t *newurl)
{
	url_t *url, *sibling;

	if (!glob_currenturl || !url_same (glob_currenturl, iop->url) ||
	    !(url = url_clone (newurl)))
		return;

	if (glob_currenturl->next) {
		sibling = glob_currenturl->next;
		url_remove (glob_currenturl);
		url_insert_before (sibling, url);
	} else {
		sibling = glob_currenturl->prev;
		url_remove (glob_currenturl);
		url_insert_after (sibling, url);
	}
	if (glob_lasturl == glob_currenturl) {
		glob_lasturl = url;
	}
	url_free (glob_currenturl);
	glob_currenturl = url;

	url_print (url, urlbuf, sizeof (urlbuf));
	urltext_set (urlbuf);
}

static int
download_start (url_t *old, url_t *new, int force_download)
{
	io_t *downloader;

	/*
	 * cancel pending downloads
	 */
	download_cancel ();
	glob_canceled = 0;

	if (force_download || !old || !url_same (old, new)) {
		/*
		 * we have to fetch the document
		 */
		downloader = io_download (new,
				mime_input_handler,
				download_done,
				download_redirect);
		if (!downloader) {
			return -1;
		}
		downloader->show_status = 1;
		downloader->main_doc = 1;
		downloader_add (downloader);
	} else {
		/*
		 * we have this document already, we just have to go to
		 * another fragment id.
		 */
		html_goto (new->frag);
		status_set ("document done.");
	}
	glob_lasturl = new;

	/*
	 * print the new URL in the entry field
	 */
	url_print (new, urlbuf, sizeof (urlbuf));
	urltext_set (urlbuf);
	return 0;
}

void
download_lock (io_t *this_one_not)
{
	io_t *iop;

	for (iop = glob_downloaders; iop; iop = iop->next) {
		if (iop->handler_id >= 0 && iop != this_one_not) {
			wt_chginput (iop->handler_id, NULL, NULL, NULL);
		}
	}
}

void
download_unlock (io_t *this_one_not)
{
	fd_set fds;
	io_t *iop;

	for (iop = glob_downloaders; iop; iop = iop->next) {
		if (iop->handler_id >= 0 && iop != this_one_not) {
			FD_ZERO (&fds);
			FD_SET (iop->fh, &fds);
			wt_chginput (iop->handler_id,
				(iop->flags & IO_RDONLY) ? &fds : NULL,
				(iop->flags & IO_WRONLY) ? &fds : NULL,
				(iop->flags & IO_EXCEPT) ? &fds : NULL);
		}
	}
}

/********************** Support for inline Images *********************/

void
image_add (image_t *img)
{
	img->next= glob_images;
	glob_images = img;
}

int
image_del (image_t *img)
{
	image_t **prev, *this;

	prev = &glob_images;
	for (this = *prev; this; prev = &this->next, this = *prev) {
		if (this == img) {
			*prev = img->next;
			return 0;
		}
	}
	return -1;
}

#define IMAGE_DONE	1
#define IMAGE_BAD	-2000

static void
image_download_done (io_t *iop, int err)
{
	if (!iop->errcode) {
		/*
		 * mark for img_query_cb()
		 */
		iop->errcode = (err == IO_OK) ? IMAGE_DONE : IMAGE_BAD;
	}
}

/********************** Widget Callbacks ****************************/

static long
img_query_cb (widget_t *w, char *urlstr, int *wd, int *ht)
{
	io_t *downloader;
	image_t *img = NULL;
	url_t *url;

	url = url_make (glob_currenturl, urlstr);
	if (!url) {
		return 0;
	}
	/*
	 * first see if we have the image already in memory
	 */
	for (img = glob_images; img; img = img->next) {
		/*
		 * take care here when one image is used several times
		 * on one page! therefore we check img->locked.
		 */
		if (img->locked && img->url && url_same (img->url, url))
			break;
	}
	if (img) {
		/*
		 * found it.
		 */
		*wd = img->wd;
		*ht = img->ht;
		return (long)img;
	}
	if (glob_canceled) {
		/*
		 * downloads for this page have been canceled
		 */
		return 0;
	}
	/*
	 * Have to fetch it from the net...
	 */
	downloader = io_download (url, mime_input_handler,
			image_download_done, NULL);
	if (!downloader) {
		url_free (url);
		return 0;
	}
	downloader_add (downloader);

	/*
	 * lock out all other downloaders so we do not append HTML code
	 * while we are in the html callback...
	 */
	download_lock (downloader);

	/*
	 * wait until we have seen enough of the image data so we can
	 * tell the html widget what size it has.
	 */
	while (42) {
		if (wt_do_event ()) {
			/*
			 * application exit or some such thing.
			 */
			img = NULL;
			break;
		}
		if (downloader->errcode < 0) {
			/*
			 * something went wrong or we have been
			 * canceled.
			 */
			img = NULL;
			break;
		}
		if (downloader->mimetype) {
			if (!downloader->is_image) {
				/*
				 * this is not an image.
				 */
				img = NULL;
				break;
			}
			img = image_get (downloader);
			if (img && img->wd > 0 && img->ht > 0) {
				/*
				 * now we know the size of the image
				 */
				break;
			}
		}
	}
	/*
	 * allow other downloaders to continue
	 */
	download_unlock (downloader);

	if (img) {
		*wd = img->wd;
		*ht = img->ht;
		if (downloader->errcode == IMAGE_DONE) {
			/*
			 * download done. can delete downloader. But the
			 * image will not be deleted, it is in glob_images
			 * now.
			 */
			downloader_del (downloader);
			io_deref (downloader);
		} else {
			/*
			 * download still in progress. switch to
			 * download_done().
			 */
			downloader->errcode = 0;
			downloader->done = download_done;
		}
		return (long)img;
	}
	/*
	 * something went wrong
	 */
	if (downloader->is_image && (img = image_get (downloader))) {
		image_del (img);
		image_free (img);
	}
	downloader_del (downloader);
	io_deref (downloader);
	return 0;
}

static void
img_place_cb (widget_t *w, long handle, int x, int y)
{
	image_t *img = (image_t *)handle;
	image_place (img, x, y);
}

static void
img_invalidate_cb (widget_t *w, int can_discard_img_data)
{
	if (can_discard_img_data) {
		/*
		 * we switched to a new document. can discard all
		 * the images.
		 */
		image_free_list (glob_images);
		glob_images = NULL;
	} else {
		/*
		 * html widget has been reconfigured and images will
		 * be placed at a new position. Just lock the images
		 * so that nothing is drawn to the html widget anymore.
		 */
		image_lock_list (glob_images);
	}
}

static void
anchor_press_cb (widget_t *w, char *urlstr, int x, int y, int pressed)
{
	url_t *url;

	if (pressed == 0) {
		url = url_make (glob_currenturl, urlstr);
		if (!url) {
			status_set ("invalid url.");
			return;
		}
		if (x >= 0 && y >= 0) {
			/*
			 * clicked into an imagemap. add
			 * ?x,y to the url
			 */
			if (url->search)
				free (url->search);
			url->search = malloc (20);
			if (url->search)
				sprintf (url->search, "%d,%d", x, y);
		}
		if (download_start (glob_lasturl, url, glob_save_next)) {
			url_free (url);
		} else if (glob_save_next) {
			glob_save_next = 0;
			glob_downloaders->do_save = 1;
		} else {
			url_insert_after (glob_currenturl, url);
			glob_currenturl = url;
		}
	} else if (pressed == 1)
		status_set (urlstr);
}

/* 
 * add missing part to the first url user gave
 */
static char *
make_starturl(char *url)
{
	if(url_ispart(url)) {
		if (url[0] == '/') {
			sprintf (urlbuf, "file:%s", url);
		} else {
			char pwd[256];
			getcwd (pwd, 256);
			sprintf (urlbuf, "file:%s/%s", pwd, url);
		}
	} else {
		strcpy (urlbuf, url);
	}
	return urlbuf;
}

static void
urltext_cb (widget_t *w, char *input, int cursor)
{
	url_t *url;

	if (!glob_currenturl) {
		input = make_starturl(input);
	}
	url = url_make (glob_currenturl, input);
	if (!url) {
		status_set ("invalid url.");
		return;
	}
	if (download_start (glob_lasturl, url, 0)) {
		url_free (url);
	} else {
		url_insert_after (glob_currenturl, url);
		glob_currenturl = url;
	}
}

static void
button_cb (widget_t *w, int pressed)
{
	url_t *url;
	char *file;
	int i;

	if (pressed != 0)
		return;

	for (i=0; i < BUT_MAX && w != buttons[i]; ++i)
		;

	switch (i) {
	case BUT_QUIT:
		wt_break (1);
		break;

	case BUT_OPEN:
		file = wt_fileselect (w, "Load HTML file:", ".", "*.html", "");
		if (file) {
			urltext_cb(w, file, 0);
		}
		break;

	case BUT_BACK:
		if (!glob_currenturl || !glob_currenturl->prev) {
			w_beep ();
			break;
		}
		download_start (glob_lasturl, glob_currenturl->prev, 0);
		glob_currenturl = glob_currenturl->prev;
		break;

	case BUT_FORW:
		if (!glob_currenturl || !glob_currenturl->next) {
			w_beep ();
			break;
		}
		download_start (glob_lasturl, glob_currenturl->next, 0);
		glob_currenturl = glob_currenturl->next;
		break;

	case BUT_RELO:
		if (glob_currenturl) {
			download_start (glob_lasturl, glob_currenturl, 1);
		}
		break;

	case BUT_HOME:
		if (glob_homeurl && (url = url_clone (glob_homeurl))) {
			if (download_start (glob_lasturl, url, 0)) {
				url_free (url);
			} else {
				url_insert_after (glob_currenturl, url);
				glob_currenturl = url;
			}
		}
		break;

	case BUT_BOOK:
		if (glob_bookmarkurl) {
			download_start (glob_lasturl, glob_bookmarkurl, 0);
			glob_lasturl = NULL;
		}
		break;

	case BUT_STOP:
		download_cancel ();
		break;

	default:
		printf ("bad button.\n");
		break;
	}
}

static void
save_link(widget_t *w, wt_menu_item *item)
{
	glob_save_next = 1;
	status_set ("select link to download...");
}

static void
save_doc(widget_t *w, wt_menu_item *item)
{
	if (!glob_currenturl) {
		return;
	}
	if (!download_start (glob_lasturl, glob_currenturl, 1))
		glob_downloaders->do_save = 1;
}

static wt_menu_item popup_menu[] = {
	{ "Save link",		0, "", 0, save_link, MenuItem },
	{ "Save document",	0, "", 0, save_doc, MenuItem },
	{ 0,0,0,0,0, MenuItem },	/* separator */
	{ "Add bookmark",	0, "", 0, 0, MenuItem },
	{ 0,0,0,0,0, MenuItem },	/* separator */
	{ "Show source",	0, "", 0, 0, MenuItem },
	{ "Open local...",	0, "", 0, 0, MenuItem },
	{ 0,0,0,0,0, MenuEnd }
};

static WEVENT *
html_key_cb (widget_t *w, WEVENT *ev)
{
	long height, ypos, error;

	switch (ev->type) {
	case EVENT_KEY:
		error = wt_getopt (html, WT_YPOS, &ypos, WT_EOL);
		error |=  wt_getopt (viewp, WT_HEIGHT, &height, WT_EOL);

		switch (ev->key) {
		case KEY_UP:
		case WKEY_UP:
			ypos += 12;
			break;
		case KEY_DOWN:
		case WKEY_DOWN:
		case '\n':
			ypos -= 12;
			break;
		case KEY_LEFT:			/* page up */
		case WKEY_LEFT:
		case KEY_BS:
			ypos += height/2;
			break;
		case KEY_RIGHT:			/* page down */
		case WKEY_RIGHT:
		case ' ':
			ypos -= height/2;
			break;
		default:
			return ev;
		}
		if(!error)
			wt_setopt (html, WT_YPOS, &ypos, WT_EOL);
		break;

	case EVENT_MPRESS:
		if (!(ev->key & BUTTON_RIGHT))
			return ev;
		wt_popup_cb (top, popup_menu, 1);
		break;

	/* shouldn't get any other events though... */
	default:
		return ev;
	}

	return NULL;
}

static void
win_title_cb (widget_t *w, char *title)
{
	wt_setopt (shell, WT_LABEL, title, WT_EOL);
}

static int
make_widgets (widget_t *top)
{
	long i, j, k;

	/*
	 * shell and vertical pane.
	 */
	shell = wt_create (wt_shell_class, top);
	vpane = wt_create (wt_packer_class, shell);

	if (!(shell && vpane)) {
		return -1;
	}
	wt_setopt (shell,
		WT_LABEL, " WetScape ",
		WT_EOL);

	/*
	 * buttons
	 */
	butbox = wt_create (wt_packer_class, vpane);
	for (i = 0; i < BUT_MAX; ++i) {
		buttons[i] = wt_create (wt_pushbutton_class, butbox);
		if (!buttons[i])
			return -1;
	}

	wt_setopt (buttons[BUT_QUIT],
		WT_LABEL, "Quit",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_setopt (buttons[BUT_OPEN],
		WT_LABEL, "Open",
		WT_ACTION_CB, button_cb,
		WT_EOL);
		
	wt_setopt (buttons[BUT_BACK],
		WT_LABEL, "Back",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_setopt (buttons[BUT_FORW],
		WT_LABEL, "Forward",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_setopt (buttons[BUT_HOME],
		WT_LABEL, "Home",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_setopt (buttons[BUT_STOP],
		WT_LABEL, "Stop",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_setopt (buttons[BUT_RELO],
		WT_LABEL, "Reload",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	wt_setopt (buttons[BUT_BOOK],
		WT_LABEL, "Bookmarks",
		WT_ACTION_CB, button_cb,
		WT_EOL);

	/*
	 * url entry field
	 */
	urlbox = wt_create (wt_packer_class, vpane);
	urllabel = wt_create (wt_label_class, urlbox);
	urltext = wt_create (wt_getstring_class, urlbox);

	if (!(urlbox && urllabel && urltext)) {
		return -1;
	}
	i = LabelModeNoBorder;
	wt_setopt (urllabel,
		WT_LABEL, "Goto URL",
		WT_MODE, &i,
		WT_EOL);

	i = 48;
	j = sizeof (urlbuf) - 1;
	k = GetstrModeWithBorder;
	wt_setopt (urltext,
		WT_STRING_ADDRESS, urlbuf,
		WT_STRING_LENGTH, &j,
		WT_STRING_WIDTH, &i,
		WT_MODE, &k,
		WT_ACTION_CB, urltext_cb,
		WT_EOL);

	/*
	 * html widget
	 */
	viewp = wt_create (wt_viewport_class, vpane);
	html = wt_create (wt_html_class, viewp);

	if (!(viewp && html)) {
		return -1;
	}
	j = ViewpModeLifeScroll;
	i = AlignFillHorz;
	k = 200;
	wt_setopt (viewp,
		WT_MODE, &j,
		WT_ALIGNMENT, &i,
		WT_HEIGHT, &k,
		WT_EOL);

	wt_setopt (html,
		WT_LABEL, INITIAL_SETTINGS,
		WT_TITLE_CB, win_title_cb,
		WT_QUERY_CB, img_query_cb,
		WT_PLACE_CB, img_place_cb,
		WT_INVAL_CB, img_invalidate_cb,
		WT_ACTION_CB, anchor_press_cb,
		WT_EOL);

	wt_bind (html, EV_KEYS|EV_MOUSE, html_key_cb);

	/*
	 * status bar
	 */
	statusbar = wt_create (wt_label_class, vpane);

	if (!statusbar) {
		return -1;
	}
	j = AlignLeft;
	i = LabelModeNoBorder;
	wt_setopt (statusbar,
		WT_ALIGNMENT, &j,
		WT_MODE, &i,
		WT_EOL);

	status_set ("WetScape " WETSCAPE_VERSION
		    ", Copyright \251 '96 Kay R\366mer");

	wt_setopt (vpane,
		WT_PACK_WIDGET, butbox,
		WT_PACK_WIDGET, urlbox,
		WT_PACK_INFO, "-side top -fill both",
		WT_PACK_WIDGET, viewp,
		WT_PACK_INFO, "-side top -fill both -padx 3 -pady 4 -expand 1",
		WT_PACK_WIDGET, statusbar,
		WT_PACK_INFO, "-side top -fill both -padx 3 -pady 4",
		WT_EOL);

	for (i = 0; i < BUT_MAX; ++i) {
		wt_setopt (butbox,
			WT_PACK_WIDGET, buttons[i],
			WT_PACK_INFO, "-side left -expand 1 -fill x -padx 3 -pady 4",
			WT_EOL);
	}

	wt_setopt (urlbox,
		WT_PACK_WIDGET, urllabel,
		WT_PACK_INFO, "-side left -padx 3 -pady 4",
		WT_PACK_WIDGET, urltext,
		WT_PACK_INFO, "-side left -expand 1 -fill x -padx 3 -pady 4",
		WT_EOL);

	return 0;
}

int
main (int argc, char *argv[])
{
	char *buffer;
	url_t *url;

	top = wt_init ();

	config ();

	if (make_widgets (top)) {
		fprintf(stderr, "wetscape: widget creation failed!\n");
		return 1;
	}
	wt_realize (top);

	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'd' && !argv[1][2]) {
		/* output tracing information */
		w_trace(1);
		argv++;
		argc--;
	}

	if (argc > 1) {
		/* buffer should now point to a modifiable string */
		buffer = make_starturl(argv[1]);
		urltext_set (buffer);

		if ((url = url_parse (buffer))) {
			if (download_start (NULL, url, 1) < 0) {
				url_free (url);
			} else {
				glob_currenturl = url;
			}
		} else {
			status_set ("invalid url.");
		}
	}
	wt_run ();
	return 0;
}
