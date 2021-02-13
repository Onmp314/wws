/*
 * main.c, a part of the W Window System
 *
 * Copyright (C) 1994-1998,2002 by Torsten Scherer and Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- main startup / initialization and signal / terminate / exit code
 *
 * CHANGES
 * ++eero, 10/97:
 * - moved terminate code from loop to here.
 * - rewrote 'wconfig' file reading (there are now also some other variables
 *   beside 'menuitem').
 * ++eero, 5/98:
 * - sun and netBSD can use included GNU getopt like MiNT does.
 * ++oddie, 4/99:
 * - little bit of code so that MacMiNT mouse starts up in ul corner
 * ++bsittler, 2000.07.04:
 * - tcp connections are now allowed by the --net[=ADDR] option
 * ++eero, 04/2003:
 * - moved init / exit code to respective backend files
 * ++eero, 08/2009:
 * - LIBDIR -> DATADIR (used for config files)
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pwd.h>
#include <getopt.h>

#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "window.h"

#ifdef __MINT__
# include <support.h>
# include <mintbind.h>
long _stksize = 32000;
#endif


/* 
 * global variables
 */
uid_t glob_uid;
SCREEN *glob_screen = NULL;
int glob_ineth = 0, glob_unixh = 0;
short is_terminating = 0;
short glob_fontsize = 0;
char *glob_fontfamily = NULL;
char *glob_fontpath = NULL;
int glob_mouse_accel = 0;
int glob_debug = 0;

#ifndef AF_UNIX_ONLY

static char *bindAddr = NULL;

#endif

/* 
 * read the config file and set global variables and menu accordingly.
 * settings are of a form:
 *	setting = value
 *	setting = title, command
 *
 * White space and (other) control characters are stripped off from
 * both ends.
 */
static void read_config(struct passwd *pw, const char *config, char **menufont, char **titlefont)
{
	char *file, *line, *value, *cmd, c;
	int idx, maxlen, eq, have_exit_item = 0;
	FILE *fp = NULL;

	menu.items = 0;
	*menufont = NULL;
	*titlefont = NULL;

	maxlen = strlen(pw->pw_dir);
	if(maxlen < (idx = strlen(DATADIR))) {
		maxlen = idx;
	}
	maxlen += strlen(config) + 3;
	if (maxlen < MAX_LINELEN) {
		maxlen = MAX_LINELEN;
	}
	if ((file = malloc(maxlen))) {
		sprintf(file, "%s/.%s", pw->pw_dir, config);
		if(!(fp = fopen(file, "r"))) {
			sprintf(file, "%s/%s", DATADIR, config);
			if(!(fp = fopen(file, "r"))) {
				/* no configuration file */
				free(file);
				file = 0;
			}
		}
	} else {
		fprintf(stderr, "wserver: out of memory\r\n");
	}

	/* read configuration variables */

	while (file && fgets(file, maxlen, fp)) {

		line = file;
		/* search variable start */
		while(*line && *line <= 32) {
			line++;
		}

		eq = 0;
		for (idx = 0; idx < maxlen && line[idx]; idx++) {
			c = line[idx];
			/* search value end */
			if ((c < 32 && c != '\t') || c == '#') {
				break;
			}
			if (c == '=') {
				/* variable end / value start */
				eq = idx;
			}
		}
		if (!eq) {
			continue;
		}

		/* remove white space at the end and start of the value */
		while(--idx > eq && line[idx] < 32)
			;
		line[++idx] = 0;
		value = line + eq;
		while(*++value && *value <= 32)
			;

		/* remove white space at the end of variable name */
		while(line[--eq] <= 32)
			;
		line[++eq] = 0;

		idx = (line + idx) - value;
		/* 'eq' got now variable and 'idx' value lenght */


		/* add into the root menu */

		if (!strcmp(line, "menuitem")) {
			if ((cmd = strchr(value, ','))) {
				line = cmd;
				while (*line <= 32) {
					*line++ = 0;
				}
				do {
					*cmd++ = 0;
				} while (*cmd && *cmd <= 32);
#ifndef REFRESH
				if (!strcasecmp(cmd, "@refresh")) {
					cmd = NULL;
				}
#endif
			}
			if (cmd && (menu.items < (MAXITEMS-1))) {
				menu.item[menu.items].title = strdup(value);
				menu.item[menu.items].command = strdup(cmd);
				if (!strcasecmp(cmd, "@exit")) {
					have_exit_item = 1;
				}
				menu.items++;
			}
			continue;
		}

		/* check for global defaults */

		/* server menufont */
		if (!strcmp(line, "menufont")) {
			*menufont = strdup(value);
			continue;
		}
		/* server titlefont */
		if (!strcmp(line, "titlefont")) {
			*titlefont = strdup(value);
			continue;
		}
		/* font loading path */
		if (!strcmp(line, "fontpath")) {
			glob_fontpath = strdup(value);
			continue;
		}
		/* default client font family... */
		if (!strcmp(line, "fontfamily")) {
			glob_fontfamily = strdup(value);
			continue;
		}
		/* ...and size */
		if (!strcmp(line, "fontsize")) {
			glob_fontsize = atoi(value);
			continue;
		}
		/* mouse acceleration */
		if (!strcmp(line, "mouse_accel")) {
			glob_mouse_accel = atoi(value);
			continue;
		}
		/* add a shared color */
		if (!strcmp(line, "color")) {
			colorInitShared(value);
			continue;
		}
		/* more to come? */
	}

	if (file) {
		fclose(fp);
		free(file);
	}

	/* no server fonts defined? use defaults... */
	if(!*titlefont) {
		*titlefont = strdup(DEF_WTITLEFONT);
	}
	if(!*menufont) {
		*menufont = strdup(DEF_WMENUFONT);
	}

	/* hmm, no menu defined? use default menu... */
	if (!menu.items) {
		menu.item[0].title   = "wterm tiny";
		menu.item[0].command = "wterm -s 7";
		menu.item[1].title   = "wterm default";
		menu.item[1].command = "wterm";
		menu.item[2].title   = "wterm large";
		menu.item[2].command = "wterm -s 13";
		menu.items = 3;
	}

	/* if there is no exit item in the menu, add one... */
	if (!have_exit_item) {
		menu.item[menu.items].title   = "exit";
		menu.item[menu.items].command = "@exit";
		menu.items++;
	}
}


/*
 * my own way to install signal handlers, always != SA_ONESHOT
 */

void *mysignal(int signum, void *handler)
{
  struct sigaction sa, so;

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(signum, &sa, &so);
  return so.sa_handler;
}

static void sigroutine(int sig)
{
  terminate(sig, "signal");
}

/*
 * a two step terminator...
 */

void terminate(int sig, const char *msg)
{
  if (sig) {
    mysignal(sig, SIG_IGN);
  }

  /*
   * shall we just prepare exiting?
   */

  if (!is_terminating && !sig) {
    CLIENT *cptr;
    EVENTP paket;

    memset(&paket, 0, sizeof(EVENTP));
    paket.len        = htons(sizeof(EVENTP));
    paket.type       = htons(PAK_EVENT);
    paket.event.type = htons(EVENT_GADGET);
    paket.event.key  = htonl(GADGET_EXIT);
    paket.event.time = htonl(glob_evtime);
    
    /* this will tell all clients to terminate */
    cptr = glob_clients;
    while (cptr) {
      write(cptr->sh, &paket, sizeof(EVENTP));
      cptr = cptr->next;
    }

    is_terminating = 1;
    mysignal(SIGALRM, sigroutine);
    alarm(10);

    return;
  }

  /*
   * something more urgent occured (or all clients died)
   */

  wserver_exit(sig, msg);
}

/* this should be called even in case of an error so that text screen
 * will be restored and W server socket removed.
 */
void wserver_exit(int sig, const char *msg)
{
  if (glob_unixh > 0) {
    close(glob_unixh);
    if (unlink(UNIXNAME) < 0) {
      fprintf(stderr, "Unlinking " UNIXNAME " socket failed.\r\n");
    }
  }
  
  screen_exit();

  if (sig > 0) {
    fprintf(stderr, "fatal: W server received signal #%i\r\n", sig);
  } else if (sig < 0) {
    fprintf(stderr, "fatal: W server terminated with error message:\r\n*%s!*\r\n", msg);
  }
  exit(sig);  
}

/*
 * a SIGCHLD routine, just to remove the zombies
 *
 * Hmm... Would POSIX system work with plain SIG_IGN SIGCHLD? ++eero
 */
#ifdef SIGTSTP		/* non-SysV SIGCHLD needs this handler */
static void sigchild(int arg)
{
  /* uk: ask for information about terminated children, until there is no more,
   * just for the case that several processes died at the same time.
   */
  long res;
  int status;

  while ((res = waitpid(-1, &status, WNOHANG)) > 0) {
    /* do nothing so far */
  }
}
#endif

/*
 *
 */

static void startup(struct passwd *pw)
{
  char fname[MAX(strlen(pw->pw_dir), strlen(DATADIR)) + 6];
  short fh;

#ifndef DATADIR
#error You must define DATADIR!!!
#endif

  sprintf(fname, "%s/.wrc", pw->pw_dir);
  if (access(fname, X_OK)) {
    sprintf(fname, "%s/wrc", DATADIR);
    if (access(fname, X_OK)) {
      return;
    }
  }

  switch (fork()) {

    case -1: /* error */
      fprintf(stderr, "Wserver (warning): Can't fork startup sequence\r\n");
      break;

    case 0: /* child */
      if ((fh = open("/dev/null", O_RDWR)) >= 0) {
	dup2(fh, 0);
	dup2(fh, 1);
	dup2(fh, 2);
	close(fh);
      } else {
	fprintf(stderr, "Wserver (warning): Startup sequence can't open /dev/null\r\n");
      }

      /* this shouldn't return */
      execl(pw->pw_shell, pw->pw_shell, fname, NULL);

      /* just to be sure the main program continues as wanted */
      _exit(-99);
      break;

      /* otherwise it's parent */
  }

  /* if this failed we can still go on */
}


/*
 *  how to init the sockets?
 */

static int initunixsocket(void)
{
  int sh;
  struct sockaddr unixladdr;

  if ((sh = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("server: socket(AF_UNIX)");
    return -1;
  }

  unixladdr.sa_family = AF_UNIX;
  strcpy(unixladdr.sa_data, UNIXNAME);
  if (bind(sh, &unixladdr, sizeof(struct sockaddr))) {
    close(sh);
    perror("server: bind(AF_UNIX)");
    return -1;
  }

  if (listen(sh, MAXLISTENQUEUE)) {
    close(sh);
    perror("server: listen(AF_UNIX)");
    return -1;
  }

  return sh;
}


#ifndef AF_UNIX_ONLY

static int initinetsocket(void)
{
  int sh;
  struct sockaddr_in inetladdr;

  if ((sh = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("server: socket(AF_INET)");
    return -1;
  }

  /* bind to anything which is local */

  inetladdr.sin_family = AF_INET;
  if (bindAddr[0]) {
    struct hostent *he;

    if (!(he = gethostbyname(bindAddr))) {
      perror(bindAddr);
      fprintf(stderr, "server: can't resolve address `%s'.\n", bindAddr);
      return -1;
    }
    
    inetladdr.sin_addr = *(struct in_addr *)he->h_addr_list[0];
  } else {
    inetladdr.sin_addr.s_addr = INADDR_ANY;
  }
  inetladdr.sin_port = htons(SERVERPORT);
  if (bind(sh, (struct sockaddr *)&inetladdr, sizeof(struct sockaddr))) {
    close(sh);
    perror("server: bind(AF_INET)");
    return -1;
  }

  if (listen(sh, MAXLISTENQUEUE)) {
    close(sh);
    perror("server: listen(AF_INET)");
    return -1;
  }

  return sh;
}

#endif


/*
 *  major init code
 *
 ************************************************************************
 * SVGALIB programs have to be suid root for screen initialization, after
 * which the library will give up the root priviledges.  Therefore it's
 * advisable to do screen intialization as soon as possible (so that e.g.
 * wserver socket on /tmp/wserver won't be owned by root)!!!
 ************************************************************************
 *
 * SVGALIB also adds it's own signal handlers to most signals so that it
 * can restore text screen in case of an error!
 */

static const char *initialize (int forcemono)
{
  char *titlefont, *menufont, *ttyn, *ptr = NULL;
  struct passwd *pw;

  if (!(ttyn = ttyname(0))) {
    return "Can't resolve tty name";
  }

  while (*ttyn) {
    if (*ttyn++ == '/') {
      ptr = ttyn;
    }
  }

  if (!ptr) {
    return "Illegal tty name";
  }

#ifdef __MINT__
  /* Check terminal name for systems where only console provides graphics.
   */
  if (strcmp(ptr, "console")) {
    return "W server can be started only from 'console'";
  }
#endif

  mysignal(SIGHUP, sigroutine);
  mysignal(SIGQUIT, sigroutine);
  mysignal(SIGILL, sigroutine);
  mysignal(SIGTERM, sigroutine);
  mysignal(SIGINT, sigroutine);
#ifdef SIGTSTP				/* BSD */
  mysignal(SIGTSTP, SIG_IGN);
  mysignal(SIGCHLD, sigchild);
#else
  /* I wonder whether this would work for POSIX too? */
  mysignal(SIGCLD, SIG_IGN);		/* SYSV */
#endif

  /* the sequence must be:
   *
   * 1) init screen / mouse / keyboard
   * 2) init colors (uses a screen specific function)
   * 3) read configuration file (might set some allocated colors)
   * 4) init color masks (calls variables initialized by color init)
   * 5) init windows (uses color information)
   *
   * With SVGAlib, this should release suid root priviledge
   */
  if (!(glob_screen = screen_init(forcemono))) {
    return "Unable to initialize screen / keyboard / mouse";
  }

  /* can only happen if a client terminated, and we're going to deal with it
   * somewhere else.  As SVGAlib puts up it's own signal handlers', we need
   * to ignore this after SVGAlib is initialized (so that killing clients
   * won't kill W server too...).
   */
  mysignal(SIGPIPE, SIG_IGN);

  /*
   *  check for sockets
   */

  if ((glob_unixh = initunixsocket()) < 0) {
    fprintf(stderr, "\r\nW server is already running or this is an error\r\n");
    fprintf(stderr, "and you should retry after removing %s.\r\n\r\n", UNIXNAME);
    return "Can't create local communication socket";
  }

  /* from here on we assume that no other Wserver is running */

#ifndef AF_UNIX_ONLY

  if (bindAddr) {
    if ((glob_ineth = initinetsocket()) < 0) {
      /* this may fail due to missing network */
      fprintf(stderr, "warning: can't create any network communication socket, but\r\n");
      fprintf(stderr, "         at least the loopback interface should be enabled.\r\n");
    }
  } else {
    glob_ineth = -1;
  }

#endif

  /*
   *  some more initializations
   */

  /*
   *  check for login name
   */
  if (!(pw = getpwuid(glob_uid = getuid()))) {
    return "Couldn't resolve your login name";
  }

  if (colorInit()) {
    return "Color initialization failed";
  }

  /* reads menu definition, font names, colors etc. */
  read_config(pw, "wconfig", &menufont, &titlefont);

  if (font_init(titlefont, menufont)) {
    return "Menu/title font loading failed, check wconfig file";
  }
  if(titlefont) {
    free(titlefont);
  }
  if(menufont) {
    free(menufont);
  }

  init_defmasks();

  if (window_init() < 0) {
    return "Window initialization failed";
  }

  /* hack for monochrome emulation driver(s) */
  if (glob_sharedcolors > (1 << glob_backgroundwin->bitmap.planes)) {
    glob_sharedcolors = 1 << glob_backgroundwin->bitmap.planes;
  }

  if (colorSetColorTable(glob_colortable)) {
    return "Color setting failed";
  }

  /* decode mouse shapes */
  wmouse_init();

#if defined(SVGALIB) || (defined(__MINT__) && defined(MAC))
  /* can not set the mouse to middle of screen */
  glob_mouse.real.x0 = 0;
  glob_mouse.real.y0 = 0;
  mouse_setposition(0,0);
#else
  glob_mouse.real.x0 = glob_screen->bm.width >> 1;
  glob_mouse.real.y0 = glob_screen->bm.height >> 1;
#endif
  glob_mouse.real.x1 = glob_mouse.real.x0 + 15;
  glob_mouse.real.y1 = glob_mouse.real.y0 + 15;

  mouse_show();

  FD_ZERO(&glob_crfd);

  /*
   *  other stuff...
   */

  switch (fork()) {
    case -1:	/* error */
      fprintf (stderr, "wserver: cannot execute copyright\r\n");
      break;
    case 0:	/* child */
      execlp ("wcpyrgt", "wcpyrgt", NULL);
      fprintf (stderr, "wserver: cannot execute copyright\r\n");
      _exit (-99);
  }

  startup(pw);

  return NULL;
}


/*
 * guess what... :)
 */

static void usage (const char *prgname)
{
  fprintf(stderr, "Usage: %s [--help] [OPTION]...\r\n", prgname);
}


static void version (void)
{
  printf("W %d Release %d.%d\n", _WMAJ, _WMIN, _WPL);
  printf("\n");
  printf("Copyright " _W_COPYRIGHT_MARK_ " 1994-2000 by Torsten Scherer, Kay Römer, Eero Tamminen et al.\n");
  printf("\n");
  printf("This package is free software; you can redistribute it and/or modify it\n");
  printf("under the terms specified in the docs/COPYRIGHTS file coming with this\n");
  printf("package.\n");
}

static void help (char *prgname)
{
  printf("Usage: %s [OPTION]...\r\n", prgname);
  printf("Start the W Window System server.\n");
  printf("\n");
  printf("      --debug          don't catch virtual terminal-switches\n");
  printf("      --forcemono      force monochrome display (faster, less memory)\n");
  printf("  -h, --help           display this help and exit\n");
#ifndef AF_UNIX_ONLY
  printf("      --net=127.0.0.1  allow tcp loopback connections\n");
  printf("      --net[=ADDR]     allow tcp network connections [but only on ADDR]\n");
  printf("                       USE OF THIS OPTION ON PUBLIC NETWORKS IS INSECURE!!!\n");
  printf("                       (you must set WDISPLAY to ADDR for tcp clients)\n");
#endif
  printf("      --version        output version information and exit\n");
  printf("\n");
  printf("See <URL:http://koti.welho.com/kmattil4/> for new releases and updates.\n");
}


static struct option options[] = {
  {"debug", 0, NULL, 1},
  {"forcemono", 0, NULL, 2},
#ifndef AF_UNIX_ONLY
  {"net", 2, NULL, 3},
#endif
  {"version", 0, NULL, 4},
  {"help", 0, NULL, 'h'},
  {NULL, 0, NULL, 0}
};


int main (int argc, char *argv[])
{
  int ret, forcemono = 0;
  const char *error;

#ifdef SIGTTOU
  /* (BSD) allow background writes (ignore terminal I/O). */
  mysignal(SIGTTOU, SIG_IGN);
#endif

  /* parse arguments
   */
  while ((ret = getopt_long(argc, argv, "h", options, NULL)) != -1) {
    switch(ret) {
      case 1:
        glob_debug = 1;
	break;
      case 2:
        forcemono = 1;
	break;
#ifndef AF_UNIX_ONLY
      case 3:
        bindAddr = optarg ? optarg : "";
	break;
#endif
      case 4:
        version();
	return 0;
	break;
      case 'h':
        help(argv[0]);
	return 0;
	break;
      case ':':
	fprintf(stderr, "missing argument for option '%s'\r\n", argv[optind]);
	break;
      default:
	usage(argv[0]);
	return -1;
    }
  }

  /* no additional arguments supported so far
   */
  if (optind < argc) {
    fprintf(stderr, "%s: too many arguments\n", argv[0]);
    usage(argv[0]);
    return -1;
  }

  if ((error = initialize(forcemono))) {
    wserver_exit(-1, error);
  }

#ifdef __MINT__
  /* we want the server to be as quick as possible */
  (void)Prenice(Pgetpid(), -50);
#endif

  /* aaaaaand...
   */
  loop();

  /* notreached
   */
  return -99999;
}
