/*
 * simple html browser.
 *
 * $Id: w4.c,v 1.3 2008-08-28 20:53:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <Wlib.h>
#include <Wt.h>

#define MAX_LINK_NAME	(PATH_MAX + NAME_MAX)

typedef struct _link_trace {
	struct _link_trace *prev;
	char link[1];
} link_trace;

static link_trace *backtrace = NULL;		/* linked link trace */
static char urlbuf[MAX_LINK_NAME+1] = "";	/* text widget buffer */
static char oldurl[MAX_LINK_NAME+1] = "";	/* next trace top */
static char absolute[MAX_LINK_NAME+1] = "";	/* absolute filename */

static widget_t *viewp, *html;
static widget_t *top, *shell, *vpane, *urlbox, *urllabel, *urltext, *retrace;


/* change to directory and saves the absolute filename
 * (without possible "file://" header, should takes care
 * of the relative file references).
 *
 * returns the plain filename.
 */
static char*
save_absolute(char *fname)
{
	int i = strlen(fname);

	/* if path, change there */
	while (i-- && fname[i] != '/');
	if (i++ >= 0) {
		memcpy (absolute, fname, i);
		absolute[i] = 0;
		chdir (absolute);
		fname = &fname[i];
	}
	/* save absolute filename */
	getcwd (absolute, PATH_MAX);
	i = strlen(absolute);
	absolute[i++] = '/';
	strcpy(&absolute[i], fname);

	return fname;
}

/* allocate memory for the file and read it.
 * takes care of directories (soon) and files with both
 * absolute and relative references.
 */
static char*
read_file (char *name)
{
	struct stat sb;
	char *data, *fname = NULL;
	int i;

	if (stat (name, &sb) < 0) {
		printf ("cannot stat %s\n", name);
		return NULL;
	}
	if (S_ISDIR(sb.st_mode)) {
		/* fname = fileselect(name);
		 * Remember to free fname if needed before returning!!!
		 */
	        if(!fname)
			return NULL;
		name = fname;
		if (stat (name, &sb) < 0) {
			printf ("cannot stat %s\n", name);
			free(fname);
			return NULL;
		}
	}
	name = save_absolute(name);

	i = open (name, O_RDONLY);
	if (i < 0) {
		printf ("cannot open %s\n", name);
		return NULL;
	}
	data = malloc (sb.st_size+1);
	if (!data) {
		close (i);
		printf ("oom\n");
		return NULL;
	}
	if (read (i, data, sb.st_size) != sb.st_size) {
		close (i);
		printf ("error reading %s\n", name);
		free (data);
		return NULL;
	}
	close (i);
	data[sb.st_size] = 0;
	return data;
}

typedef struct {
	BITMAP *bm;
	short wd, ht;
} image_t;

static void *
img_query_cb (widget_t *w, char *url, int *wd, int *ht)
{
	image_t *img = malloc (sizeof (image_t));

	if (!img)
		return (long)NULL;

	img->bm = w_readimg (url, &img->wd, &img->ht);
	if (!img->bm) {
		free (img);
		return NULL;
	}
	*wd = img->wd;
	*ht = img->ht;
	return img;
}

static void
img_place_cb (widget_t *w, long handle, int x, int y)
{
	image_t *img = (image_t *)handle;

	w_putblock (img->bm, wt_widget2win (w), x, y);
	free (img->bm->data);
	free (img->bm);
	free (img);
}

static void
img_invalidate_cb (widget_t *w, int can_discard_img_data)
{
}

/* mask may be composed of letters that are matched excatly
 * and '?' which matches any one letter and '*' which matches
 * any letters upto a one matching the next one on the mask.
 * For masking '?' and '*' excactly use '*?' and '**'.
 * returns: 0 for failure, 1 for success
 *
 * should actually be in W-lib as there are quite a lot of uses for this...
 */
static int w_mask_name(const char *name, const char *mask)
{
  for(;;)
  {
    switch(*mask)
    {
      case '\0':
        return 1;

      case '*':
        if(!*++mask)
	  return 1;
	/* a least fit */
        while(*name && *name != *mask)
	  name++;
	/* fall through */

      case '?':
        if(!*name)
	  return 0;
	break;

      default:
        if(*mask != *name)
	  return 0;
    }
    mask++;
    name++;
  }
}

/* handle HTML file reading according to
 * protocol definitions ("file://" etc.)
 */
static char*
read_url (char *url)
{
	char *data = NULL;
	int idx = 0;

	while (url[idx]) {
		if (url[idx++] == ':') {
			/* first check stuff without '//', eg. 'mailto:' */

			/* then check stuff with '//'... */
			if (url[idx++] == '/' && url[idx++] == '/') {
				if (strncpy(url, "file", 4)) {
					/* local file, just skip protocol definition */
					url = &url[idx];
					break;
				}
				if (strncpy(url, "http", 4)) {
					fprintf(stderr, "http:// URLs not yet...\n");
					idx = 0; /* flag for NOT read_file() */
					return NULL;
				}
			}
			fprintf(stderr, "unknown protocol!\n");
			return NULL;
		}
	}
	/* local file? */
	if (idx) {
		if (w_mask_name(url, "*.html"))
			data = read_file (url);
		/* Else check whether there's a viewer installed for
		 * that type in the mailcap. If isn't, try to read
		 * the file as text...
		 */
	}
	return data;
}

/* show the current URL and the HTML file itself
 * to the user. save the *previous* URL for back tracing
 * (local files saved as absolute referencies).
 *
 * these should actually check that url isn't longer
 * than the buffer it's copied to (in case the url
 * is from a link, not user typed).
 */
static void
change_view(char *data, char *url, int backward)
{
	link_trace *new;
	int urllen;

	/* show on screen */
	strcpy(urlbuf, url);
	wt_setopt (urltext, WT_STRING_ADDRESS, urlbuf, WT_EOL);
	wt_setopt (html, WT_LABEL, data, WT_EOL);
	free (data);

	if(backward)
		return;

	/* save trace */
	if(*oldurl) {
		if(!(new = malloc(sizeof(link_trace) + strlen(oldurl))))
			return;
		strcpy(new->link, oldurl);
		new->prev = backtrace;
		backtrace = new;
	}
	if(*absolute) {
		urllen = strlen(absolute);
		url = absolute;
	} else
		urllen = strlen(url);
	strncpy(oldurl, url, urllen);
	oldurl[urllen] = 0;
	*absolute = 0;
}

/* get up in the link hierarchy */
static void
retrace_cb (widget_t *w, int down)
{
	link_trace *tmp;
	char *url, *data;

	if (down || !backtrace)
		return;

	url = backtrace->link;
	tmp = backtrace->prev;
	free(backtrace);
	backtrace = tmp;

	if((data = read_url (url)))
		change_view(data, url, 1);
}

static void
urltext_cb (widget_t *w, char *url, int cursor)
{
	char *data;

	if((data = read_url(url)))
		change_view(data, url, 0);
}

static void
anchor_press_cb (widget_t *w, char *url, int x, int y, int down)
{
	char *data;

	if(!down && (data = read_url(url)))
		change_view(data, url, 0);
}

int
main (int argc, char *argv[])
{
	char *oldpath;
	long i, j, k;

	if(argc > 1) {
		if(argv[1][0] == '-') {
			fprintf(stderr, "usage: %s <filename>\n", argv[0]);
			return -1;
		}
		else
			strcpy(urlbuf, argv[1]);
	}

	top = wt_init ();
	shell = wt_create (wt_shell_class, top);
	vpane = wt_create (wt_pane_class, shell);

	retrace = wt_create (wt_button_class, urlbox);
	urlbox = wt_create (wt_pane_class, vpane);
	urllabel = wt_create (wt_label_class, urlbox);
	urltext = wt_create (wt_getstring_class, urlbox);

	viewp = wt_create (wt_viewport_class, vpane);
	html = wt_create (wt_html_class, viewp);

	wt_setopt (shell,
		WT_LABEL, " Wetscape ",
		WT_EOL);

	i = AlignFill;
	wt_setopt (vpane,
		WT_ALIGNMENT, &i,
		WT_EOL);

	j = OrientHorz;
	wt_setopt (urlbox,
		WT_ORIENTATION, &j,
		WT_EOL);

	wt_setopt (retrace,
		WT_LABEL, "Retrace",
		WT_ACTION_CB, retrace_cb,
		WT_EOL);

	wt_setopt (urllabel, WT_LABEL, "Goto URL", WT_EOL);

	i = 36;
	j = sizeof (urlbuf) - 1;
	wt_setopt (urltext,
		WT_STRING_ADDRESS, urlbuf,
		WT_STRING_LENGTH, &j,
		WT_STRING_WIDTH, &i,
		WT_ACTION_CB, urltext_cb,
		WT_EOL);

	j = ViewpModeLifeScroll;
	k = AlignFillHorz;
	wt_setopt (viewp,
		WT_MODE, &j,
		WT_ALIGNMENT, &k,
		WT_EOL);

	wt_setopt (html,
		WT_LABEL, "<font normal=lucidat10 teletype=cour10>",
		WT_QUERY_CB, img_query_cb,
		WT_PLACE_CB, img_place_cb,
		WT_INVAL_CB, img_invalidate_cb,
		WT_ACTION_CB, anchor_press_cb,
		WT_EOL);

	oldpath = getcwd(0, PATH_MAX);
	wt_realize (top);
	wt_run ();

	chdir(oldpath);
	free(oldpath);
	return 0;
}
