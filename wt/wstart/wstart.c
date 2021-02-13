/* 
 * A `Win-95 style' program lauch bar
 * 
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <wait.h>
#include <time.h>
#include <Wlib.h>
#include <Wt.h>

/* whether to redirect child standard streams to /dev/null */
#undef CHILD_REDIRECT

/* additional lauch bar element flags */
#define ADD_TIME	1
#define ADD_QUIT	2

/* standard file types / names */
#define ICON_TYPE	".icon"
#define OPTION_TYPE	".options"
#define DIR_ICON	"folder.icon"
#define FILE_ICON	"file.icon"

#define WIN_BORDER	4		/* width of a W window border */

/* enough space for all pathnames available on this system */
#define MAX_PATH_LEN		(_POSIX_PATH_MAX + _POSIX_NAME_MAX)

/* default icons set in initialize() */
static BITMAP *FolderIcon;
static BITMAP *FileIcon;

static void
directory_cb(widget_t *w, wt_menu_item *menu);

/* 
 * utility functions
 */

static int ReportErrors;

static void
report_error(const char *error)
{
	if (!ReportErrors) {
		return;
	}
	wt_dialog(NULL, error, WT_DIAL_ERROR, " WStart error ", "Cancel", NULL);
}

/* read submenu directory from menu end marker and appen given filename to
 * that.  Return results in a buffer large enough for longest filename OS
 * can have.
 */
static char *
menu_path(const wt_menu_item *item)
{
	/* Here's a temporary buffer for storing file paths and names.  It
	 * has to be large enough for the longest possible file path + name
	 * string.  Functions using this are ATM initialize(), run_cb(),
	 * directory_cb() and their subfunctions.
	 */
	static char buffer[MAX_PATH_LEN];
	const char *name;

	if (!item) {
		/* just return enough space for filepath+name */
		return buffer;
	}
	name = item->string;

	/* search last */
	while (item->type != MenuEnd) {
		item++;
	}
	/* new file path+name to buffer */
	strcpy(buffer, item->string);
	strcpy(&buffer[strlen(buffer)], name);
	return buffer;
}

static inline void
free_path(char *path)
{
	/* is static one from above, doesn't need to free */
}

/* 
 * program execution
 */

/* count arguments in the string and separate them with zeros */
static int
separate_arguments(char *args)
{
	int count;

	/* zero and skip white space */
	while (*args && *args <= ' ') {
		*(args++) = '\0';
	}
	for (count = 0; *args; count++) {
		switch (*args) {
			case '%':
				/* special directive to end of line */
				while (*args && *args != '\n') {
					args++;
				}
				break;
			case '\'':
				/* skip white space enclosed... */
				*args++ = '\0';
				do {
					/* ...quoted argument */
					while (*args && *args != '\'') {
						args++;
					}
				} while (*args && *(args+1) > ' ');
				if (*args == '\'') {
					*args++ = '\0';
				}
				break;
			default:
				/* skip argument */
				while (*args > ' ') {
					args++;
				}
		}
		/* zero and skip white space */
		while (*args && *args <= ' ') {
			*(args++) = '\0';
		}
	}
	return count;
}

/* allocate and return either NULL or string from user (ATM there's
 * only `%f' directive for getting a filename with a fileselector).
 */
static char *
alloc_directive(char *cmd)
{
#define DIRECTIVE_ARGS 4
	const char *opt[DIRECTIVE_ARGS];
	char *arg, *options;
	int idx, count;

	/* not `%f' directive? */
	if (cmd[0] != '%' || cmd[1] != 'f' || cmd[2] > ' ' ) {
		report_error("Unknown option directive!");
		return NULL;
	}
	options = cmd + 2;

	/* get directive arguments */
	count = separate_arguments(options);
	if (count > DIRECTIVE_ARGS) {
		count = DIRECTIVE_ARGS;
	}
	for (idx = 0; idx < count; idx++) {
		while(!*options) {
			options++;
		}
		opt[idx] = options;
		options += strlen(options);
	}
	for (idx = count; idx < DIRECTIVE_ARGS; idx++) {
		opt[idx] = "";
	}

	/* here could be other directives too... */
	arg = wt_fileselect(NULL, opt[0], opt[2], opt[1], opt[3]);
	return arg;
#undef DIRECTIVE_ARGS
}

/* get program arguments either from file or user and return the string */
static char *
get_arguments(const char *name, const char *file)
{
#define ARG_LEN 128
	struct stat st;
	char *args;
	FILE *fp;

	if ((fp = fopen(file, "r"))) {
		stat(file, &st);
		if (!st.st_size) {
			fclose(fp);
			return NULL;
		}
		if (!(args = malloc(st.st_size+1))) {
			fclose(fp);
			report_error("Options file allocation failed!");
			return NULL;
		}
		if (fread(args, st.st_size, 1, fp) != 1) {
			fclose(fp);
			free(args);
			report_error("Options file read failed!");
			return NULL;
		}
		args[st.st_size] = '\0';
		fclose(fp);
	} else {
		/* ask arguments from the user */
		if (!(args = malloc(ARG_LEN+1))) {
			report_error("Argument allocation failed!");
			return NULL;
		}
		args[0] = 0;
		for(;;) {
			switch(wt_entrybox(NULL, args, ARG_LEN, name, "arguments:",
			"Execute program", "Save arguments", NULL)) {
				case 1:
					return args;
				case 2:
					if ((fp = fopen(file, "w"))) {
						fwrite(args, strlen(args), 1, fp);
						fclose(fp);
					}
					break;
				default:
					free(args);
					return NULL;
			}
		}
	}
	return args;
#undef ARG_LEN
}

static inline void
free_arguments(char *args)
{
	free(args);
}

/* Read the program options file if available and allocate / compose an
 * argument string array.
 */
static char **
alloc_argv(const char *name, const char *file)
{
	char *strings, *args, **argv;
	int count, idx, len;

	if(!(strings = get_arguments(name, file))) {
		return NULL;
	}
	/* program name + separate entries + NULL */
        count = 1 + separate_arguments(strings);
	if (!(argv = malloc(sizeof(char*) * (count+1)))) {
		free_arguments(strings);
		report_error("Argv[] allocation failed!");
		return NULL;
	}
	/* setup argument pointers */
	args = strings;
	argv[0] = strdup(name);
	for (idx = 1; idx < count; idx++) {
		while(!*args) {
			args++;
		}
		len = strlen(args);
		if (*args == '%') {
			/* subdivides the argument string and asks user */
			argv[idx] = alloc_directive(args);
			if (!argv[idx]) {
				/* user canceled input */
				while (--idx >= 0) {
					free(argv[idx]);
				}
				free(argv);
				free_arguments(strings);
				return NULL;
			}
		} else {
			argv[idx] = strdup(args);
		}
		args += len;
	}
	argv[idx] = NULL;
	free_arguments(strings);
	return argv;
}

static void
free_argv(char **argv)
{
	int idx;
	for (idx = 0; argv[idx]; idx++) {
		free(argv[idx]);
	}
	free(argv);
}


static void
run_cb(widget_t *w, wt_menu_item *item)
{
	char *program, **argv;
	struct stat st;
	int len;
#ifdef CHILD_REDIRECT
	int fh;
#endif
	/* ATM static & same on all calls! */
	program = menu_path(item);

	len = strlen(program);
	/* add option type temporarily */
	strcpy(&program[len], OPTION_TYPE);
	/*
	 * This has to be done before fork.  Forked processes share file
	 * handles (W socket) and Wt opens connection only once.
	 */
	argv = alloc_argv(item->string, program);
	program[len] = '\0';
	if (!argv) {
		return;
	}
	if (stat(program, &st) < 0) {
		report_error("Program unaccessible!");
	} else {
		if (!fork()) {
			w_exit();
#ifdef CHILD_REDIRECT
			/* above should close W socket and stuff below
			 * remaps standard streams. Other file descriptors
			 * shouldn't be open.
			 */
			if ((fh = open("/dev/null", O_RDWR)) >= 0) {
				dup2(fh, 0);
				dup2(fh, 1);
				dup2(fh, 2);
				close(fh);
			} else {
				fprintf(stderr, "WStart: can't open /dev/null\n");
			}
#endif
			execvp(program, argv);
			exit(-999);
		}
	}
	free_argv(argv);
	free_path(program);
}


/* 
 * subdirectory handling
 *
 * The directory path of which contents menu represents is stored in the
 * `string' member of the last menu item (end marker).
 *
 * Stuff that menu has pointers into, is allocated in one block except
 * for the possible induvidual program icons (default icons are static).
 */

enum { ACCEPT_FILE = 1, ACCEPT_DIR };

/* 0: file is NOT runnable by given user
 * 1: normal file
 * 2: directory
 */
static int
accept(const char *name, const uid_t uid, const gid_t gid)
{
	struct stat st;
	int mode;

	if (stat(name, &st) < 0)
		return 0;

	if (S_ISDIR(st.st_mode)) {
		mode = ACCEPT_DIR;
	} else {
		mode = ACCEPT_FILE;
	}
	/* executable? */
	if (st.st_mode & S_IXOTH)
		return mode;
	if ((st.st_mode & S_IXGRP) && st.st_gid == gid)
		return mode;
	if ((st.st_mode & S_IXUSR) && st.st_uid == uid)
		return mode;

	return 0;
}

/* Read directory, count accepted menu entries and then compose menu
 * structure of them.
 *
 * String buffer pointed to by `path' is used in this function so it should
 * be long enough for the longest possible file path + name!
 *
 * Returns NULL or an error message.
 */
static wt_menu_item *
read_menu(char *path)
{
	int mode, index, count, pathlen, namelen, len;
	wt_menu_item *menu, *item;
	struct dirent *entry;
	char *names;
	uid_t uid;
	gid_t gid;
	DIR *dfd;

	if(!(dfd = opendir(path))) {
		report_error("Directory open failed!");
		return NULL;
	}
	gid = getgid();
	uid = getuid();
	pathlen = strlen(path);
	count = namelen = 0;
	/* 
	 * files starting with `.'  are links to current and parent
	 * directory or configuration files / directories so they are
	 * ignored too.
	 */
	while((entry = readdir(dfd))) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		strcpy(&path[pathlen], entry->d_name);
		if ((mode = accept(path, uid, gid))) {
			count++;
			namelen += strlen(entry->d_name) + 1;
			if (mode == ACCEPT_DIR)
				namelen++;
		}
	}
	path[pathlen] = '\0';
	if (!count) {
		closedir(dfd);
		report_error("No usable entries!");
		return NULL;		/* no entries */
	}
	count++;
	/* menu items, file names, directory path */
	menu = malloc(sizeof(wt_menu_item)*count + namelen + strlen(path)+2);
	if (!menu) {
		closedir(dfd);
		report_error("Directory allocation failed!");
		return NULL;
	}
	names = (char *)(menu + count);
	memset(menu, 0, names - (char *)menu);
	count--;

	index = 0;
	rewinddir(dfd);
	/* re-read directory entries into the menu */
	while(index < count && (entry = readdir(dfd))) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		strcpy(&path[pathlen], entry->d_name);
		if ((mode = accept(path, uid, gid))) {

			/* check that name fits to allocation in case dir
			 * contents are changed in the meanwhile...
			 */
			len = strlen(entry->d_name) + 1;
			if (namelen < len) {
				break;
			}
			item = &(menu[index++]);
			strcpy(names, entry->d_name);
			item->string = names;
			namelen -= len;
			names += len;

			if (mode == ACCEPT_DIR) {
				namelen--;
				*(names-1) = '/';
				*(names++) = '\0';
				item->select_cb = directory_cb;
				item->type = MenuSub;
			} else {
				item->select_cb = run_cb;
				item->type = MenuItem;
			}
		}
	}
	path[pathlen] = '\0';
	closedir(dfd);

	strcpy(names, path);
	/* store `menu' path to the end mark item */
	menu[index].string = names;
	menu[index].type = MenuEnd;
	return menu;
}

static inline void
free_menu(wt_menu_item *data)
{
	free(data);
}

/* 
 * load icons for all the menu items
 *
 * String buffer pointed to by `path' is used in this function so it should
 * be long enough for the longest possible file path + name!
 */
static void
load_icons(char *path, wt_menu_item *item)
{
	int len = strlen(path);
	while (item->type != MenuEnd) {
		if (item->type == MenuItem) {
			strcpy(&path[len], item->string);
			strcpy(&path[len + strlen(item->string)], ICON_TYPE);
			item->icon = w_readpbm(path);
			if (!item->icon) {
				item->icon = FileIcon;
			}
		} else {
			/* sub-directory */
			item->icon = FolderIcon;
		}
		item++;
	}
	path[len] = '\0';
}

static void
release_icons(wt_menu_item *item)
{
	while (item->type != MenuEnd) {
		if (item->icon != FolderIcon && item->icon != FileIcon) {
			/* icon data should be on same block */
			w_freebm(item->icon);
		}
		item++;
	}
}

/*
 * Release given old submenu and compose a new one from the current
 * contents of the pointed directory or return an error message.
 * Reads also icons for the newly created submenu items.
 */
static void
directory_cb(widget_t *w, wt_menu_item *item)
{
	char *buffer;

	/* free old submenu and it's contents */
	if (item->sub) {
		release_icons(item->sub);
		/* rest is allocated in one piece in read_menu() */
		free_menu(item->sub);
		item->sub = NULL;
	}

	/* ATM static & same on all calls! */
	buffer = menu_path(item);

	/* modifies buffer, but returns it back to what it was */
	if (!(item->sub = read_menu(buffer))) {
		free_path(buffer);
		return;
	}

	/* modifies buffer, but returns it back to what it was */
	load_icons(buffer, item->sub);
	free_path(buffer);
}

/* 
 * other functionality
 */

static void
timer_cb(const long arg)
{
	widget_t *wp = (widget_t *)arg;
	char label[6];
	time_t value;
	struct tm *t;

	value = time(&value);
	t = localtime(&value);
	sprintf(label, "%02d:%02d", t->tm_hour, t->tm_min);
	wt_setopt(wp, WT_LABEL, label, WT_EOL);
	/* update once every minute or so */
	wt_addtimeout(59000, timer_cb, arg);
}

static void
draw_icon_cb(widget_t *w, short x, short y, BITMAP *bm)
{
	WWIN *win = wt_widget2win(w);
	w_putblock(bm, win, x, y);
}

static void
quit_cb(widget_t *w, int pressed)
{
	if (!pressed)
		wt_break(1);
}

/* remove zombies:  ask for information about terminated children, until
 * there is no more, just for the case that several processes died at the
 * same time. Posix systems should do this automatically when SIG_CHLD
 * is ignored.
 */
#if !defined(POSIX) && !defined(SysV) && !defined(linux)
static void
sigchild(int signal)
{
  int status;
  while (waitpid(-1, &status, WNOHANG) > 0);
}
#endif

/* 
 * Initialization
 */

/* parse command line argument(s), compose `menu'[2] structures with program
 * root directory set to the last item in the menu, read default icons.
 */
static wt_menu_item *
initialize(int argc, char *argv[],
	   int *elements, int *orient, int *align,
	   const char **font, const char **error)
{
	static wt_menu_item menu[2];	/* static -> initialized to zero */
	int len, idx;
	char *buffer;
	DIR *dfd;

#if defined(POSIX) || defined(SysV) || defined(linux)
	/* Posix */
	signal(SIGCHLD, SIG_IGN);
#else
	/* BSDish OS needs signal handler for getting rid of zombie
	 * children.
	 */
	signal(SIGCHLD, sigchild);
#endif
#ifdef SIGTTOU
	/* Allow background writes (from programs) to control
	 * terminal by ignoring terminal I/O signals.
	 */
	signal(SIGTTOU, SIG_IGN);
#endif

	/* default options */
	*align = AlignTop;
	*orient = OrientHorz;
	*font = *error = NULL;
	*elements = ADD_TIME | ADD_QUIT;

	idx = 0;
	while(++idx < argc && argv[idx][0] == '-') {
		switch(argv[idx][1]) {
			case 'e':
				ReportErrors ^= 1;
				break;
			case 't':
				*elements ^= ~ADD_TIME;
				break;
			case 'q':
				*elements ^= ~ADD_QUIT;
				break;
			case 'h':
				*orient = OrientHorz;
				break;
			case 'v':
				*orient = OrientVert;
				break;
			case 'l':
				*align = AlignTop;
				break;
			case 'c':
				*align = AlignCenter;
				break;
			case 'r':
				*align = AlignBottom;
				break;
			case 'f':
				if (++idx < argc) {
					*font = argv[idx];
				} else {
					*error = "font name missing";
					return NULL;
				}
				break;
			default:
				*error = "unrecognized option";
				return NULL;
		}
	}

	/* ATM static & same on all calls! */
	buffer = menu_path(NULL);

	/* if not absolute path, perpend current work directory */
	if (idx < argc && argv[idx] && argv[idx][0] == '/') {
		strcpy(buffer, argv[idx]);
	} else {
		getcwd(buffer, MAX_PATH_LEN);
		if (idx < argc && argv[idx]) {
			len = strlen(buffer);
			buffer[len++] = '/';
			strcpy(&buffer[len], argv[idx]);
		}
	}
	/* check directory validity */
	if(!(dfd = opendir(buffer))) {
		free_path(buffer);
		*error = "directory access failed";
		return NULL;
	}
	closedir(dfd);

	/* assert '/' at the end */
	len = strlen(buffer);
	if (buffer[len-1] != '/') {
		buffer[len++] = '/';
		buffer[len] = '\0';
	}
	/* read default icons */
	strcpy(&buffer[len], DIR_ICON);
	FolderIcon = w_readpbm(buffer);
	strcpy(&buffer[len], FILE_ICON);
	FileIcon = w_readpbm(buffer);
	buffer[len] = '\0';

	/* setup menu for using directory_cb for getting the dir contents */
	menu[0].string = strdup(buffer);
	menu[0].type = MenuSub;
	menu[1].type = MenuEnd;
	menu[1].string = "";

	/* read directory and return it's menu */
	directory_cb(NULL, &menu[0]);
	return menu[0].sub;
}

int main (int argc, char *argv[])
{
	widget_t *top, *shell, *menu, *time, *quit;
	long rwd, rht, wd, ht, mode;
	int elements, orient, align;
	wt_menu_item *menustruct;
	const char *font, *error;

	menustruct = initialize(argc, argv, &elements, &orient, &align, &font, &error);
	if (!menustruct) {
		fprintf(stderr, "\n%s error: %s\n", *argv, error);
		fprintf(stderr, "%s, a W program launcher.\n", *argv);
		fprintf(stderr, "usage: %s [<options)] [<path>]\n", *argv);
		fprintf(stderr, "  -f <name>	set text font\n");
		fprintf(stderr, "  -v|-h	alignment\n");
		fprintf(stderr, "  -l|-c|-r	position\n");
		fprintf(stderr, "  -t|-e	show time, errors\n");
		fprintf(stderr, "  -q		quit button\n");
		return -1;
	}

	top = wt_init();
	mode = ShellModePopup;
	shell = wt_create(wt_shell_class, top);
	wt_setopt(shell, WT_MODE, &mode, WT_EOL);
	menu = wt_create(wt_menu_class, shell);

	wt_setopt(menu,
		WT_LIST_ADDRESS, menustruct,
		WT_DRAW_FN, draw_icon_cb,
		WT_EOL);

	/* if you'll want these after the start button on the wstart line,
	 * you'll have to add them to menu after setting WT_LIST_ADDRESS.
	 */
        if (elements & ADD_TIME) {
		mode = LabelModeWithBorder;
		time = wt_create(wt_label_class, menu);
		wt_setopt(time, WT_MODE, &mode, WT_EOL);
		timer_cb((long)time);
	}
	if (elements & ADD_QUIT) {
		quit = wt_create(wt_pushbutton_class, menu);
		wt_setopt(quit,
			WT_LABEL, "Quit",
			WT_ACTION_CB, quit_cb,
			WT_EOL);
	}
	if (font) {
		/* propagates to _all_ children */
		wt_setopt(menu, WT_FONT, font, WT_EOL);
	}

	if (orient == OrientVert) {
		mode = orient;
		ht = AlignFill;
		wt_setopt(menu, WT_ALIGNMENT, &ht,
			  WT_ORIENTATION, &mode,
			  WT_EOL);
	}
	/* position lauch bar */
	wt_minsize(shell, &wd, &ht);
	rwd = WROOT->width  - WIN_BORDER * 2;
	rht = WROOT->height - WIN_BORDER * 2;
	switch (align) {
	case AlignTop:
		if (orient == OrientVert) {
			wt_reshape(shell, 0, 0, WT_UNSPEC, rht);
			mode = AlignRight;
		} else {
			wt_reshape(shell, 0, 0, rwd, WT_UNSPEC);
			mode = AlignBottom;
		}
		wt_setopt(menu, WT_ALIGNMENT, &mode, WT_EOL);
		break;
	case AlignCenter:
		if (orient == OrientVert) {
			wt_reshape(shell, (rwd - wd) / 2, 0, WT_UNSPEC, rht);
		} else {
			wt_reshape(shell, 0, (rht - ht) / 2, rwd, WT_UNSPEC);
		}
		break;
	case AlignBottom:
		if (orient == OrientVert) {
			wt_reshape(shell, rwd - wd, 0, WT_UNSPEC, rht);
			mode = AlignLeft;
		} else {
			wt_reshape(shell, 0, rht - ht, rwd, WT_UNSPEC);
			mode = AlignTop;
		}
		wt_setopt(menu, WT_ALIGNMENT, &mode, WT_EOL);
		break;
	}
	wt_realize(top);
	wt_run();
	return 0;
}

