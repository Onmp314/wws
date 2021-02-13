/*
 * wvt, terminal program for W based on wterm by TeSche.
 *
 * (W) 1996-1999, Kay Roemer, Torsten Scherer, Eero Tamminen.
 *
 * CHANGES
 *
 * ++eero 5/98:
 * - use (POSIX) sigemptyset() for setting sigaction sa_mask.
 * - support to the new VT widget features.
 * - doesn't strip 8th bit anymore.
 * ++eero 4/99:
 * - generate ESC-sequences for W special keys (arrows etc).
 * ++bsittler, 2000.07.04:
 * - TERMCAP is removed from the environment unless EXPORT_TERMCAP
 *   is defined.
 *
 * $Id: wvt.c,v 1.9 2008-08-29 19:43:08 eero Exp $
 */

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <utmp.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Wlib.h>
#include <Wt.h>
#include "util.h"

#ifdef EXPORT_TERMCAP
#include "termtype.h"
#endif

#define	SMALLBUF 63
#define	LARGEBUF 1024


/*
 * some global variables
 */

static int pid, console, cfh;
static int pipeh;
static char pty[SMALLBUF];
static struct winsize winsz;

static long cblink = 0, reverse = 0, visualbell = 0, debug = 0;
static long rows = 24, cols = 80, histsize = 100, curhistsize = 0;
static widget_t *top, *shellwin, *vt, *sb;
static long inphandler = 0;


/********************************************************************/

static void
sigpipe(int sig)
{
  /* this one musn't close the window */
  _write_utmp(pty, "", "", 0);
  kill(-pid, SIGHUP);
  _exit(sig);
}


static void
sigchld(int sig)
{
  _write_utmp(pty, "", "", 0);
  _exit(sig);
}


static void
sigquit(int sig)
{
  signal(sig, SIG_IGN);
  kill(-pid, sig);
}

static void *
mysignal(int signum, void *handler)
{
  struct sigaction sa, so;

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
#if defined(linux)
  sa.sa_flags = SA_NOMASK | SA_RESTART;
#else
  sa.sa_flags = 0;
#endif
  sigaction(signum, &sa, &so);

  return so.sa_handler;
}

/***********************************************************************/

static void
window_resized (void)
{
  struct winsize winsz;
  pid_t pgrp;

  winsz.ws_col = cols;
  winsz.ws_row = rows;

  if (!ioctl (pipeh, TIOCGPGRP, &pgrp) &&
      !ioctl (pipeh, TIOCSWINSZ, &winsz))
    kill (-pgrp, SIGWINCH);
}

static void
send_chars (const uchar *cp, long len)
{
  long r;
  if (cp && len > 0) {
    r = write (pipeh, cp, len);
  }
}

static void
input_cb (long arg, fd_set *r, fd_set *w, fd_set *e)
{
  uchar buf[LARGEBUF];
  long in;

  /* is there tty data to be printed? */
  if (FD_ISSET (pipeh, r)) {
    if ((in = read (pipeh, buf, LARGEBUF)) > 0) {
      wt_opaque_t str;
      str.cp = buf;
      str.len = in;
      wt_setopt (vt, WT_VT_STRING, &str, WT_EOL);

      if (debug) {
	char *ptr = buf;

	while (in-- > 0) {
	  printf("wterm: received 0x%02x\n", *ptr++);
	}
      }
    }
  }

#ifdef __MINT__
  /* in any case: watch out for console output */
  if (console && FD_ISSET (cfh, r)) {
    if ((in = read (cfh, buf, LARGEBUF)) > 0) {
      wt_opaque_t str;
      str.cp = buf;
      str.len = in;
      wt_setopt (vt, WT_VT_STRING, &str, WT_EOL);
    }
  }
#endif

  if (FD_ISSET (pipeh, w)) {
    send_chars (NULL, 0);
  }
}

static WEVENT *
key_cb (widget_t *w, WEVENT *ev)
{
  uchar buf[4];
  int len;

  if (ev->type == EVENT_KEY) {
    if (IS_WKEY(ev->key)) {
      len = 2;
      switch(ev->key) {
	case WKEY_F1:
	case WKEY_F2:
	case WKEY_F3:
	case WKEY_F4:
	case WKEY_F5:
	case WKEY_F6:
	case WKEY_F7:
	case WKEY_F8:
	case WKEY_F9:
	case WKEY_F10:
		buf[len++] = '[';
		buf[len++] = '@' + WKEY_FN(ev->key);
		break;
	case WKEY_UP:
		buf[len++] = 'A';
		break;
	case WKEY_DOWN:
		buf[len++] = 'B';
		break;
	case WKEY_LEFT:
		buf[len++] = 'D';
		break;
	case WKEY_RIGHT:
		buf[len++] = 'C';
		break;
	case WKEY_PGUP:
		buf[len++] = '5';
		buf[len++] = '~';
		break;
	case WKEY_PGDOWN:
		buf[len++] = '6';
		buf[len++] = '~';
		break;
	case WKEY_HOME:
		buf[len++] = '1';
		buf[len++] = '~';
		break;
	case WKEY_END:
		buf[len++] = '4';
		buf[len++] = '~';
		break;
	case WKEY_INS:
		buf[len++] = '2';
		buf[len++] = '~';
		break;
	case WKEY_DEL:
		buf[len++] = '3';
		buf[len++] = '~';
		break;
      }
      if (len > 2) {
	buf[0] = '\e';
	buf[1] = '[';
	write (pipeh, buf, len);
      }	
    } else {
      *buf = ev->key & 0xff;
      send_chars (buf, 1);
    }
  }
  return NULL;
}

static void
vt_cb (widget_t *w, long pos, long size, long ncols, long nrows,
       wt_opaque_t *pasted)
{
  if (ncols > 0) {
    assert (size >= 0 && nrows > 0);
    rows = nrows;
    cols = ncols;
    window_resized ();
  }
  if (size >= 0) {
    long sbpos;
    wt_getopt (sb, WT_POSITION, &sbpos, WT_EOL);

    sbpos += size - curhistsize;
    curhistsize = size;
    size += rows;
    wt_setopt (sb,
	       WT_TOTAL_SIZE, &size,
	       WT_SIZE, &rows,
	       WT_PAGE_INC, &rows,
	       WT_POSITION, &sbpos, WT_EOL);
  }
  if (pos >= 0) {
    long sbpos;
    sbpos = curhistsize - pos;
    wt_setopt (sb, WT_POSITION, &sbpos, WT_EOL);
  }
  if (pasted) {
    send_chars (pasted->cp, pasted->len);
  }
}

static void
sb_cb (widget_t *w, long sbpos)
{
  long pos = curhistsize - sbpos;
  wt_setopt (vt, WT_VT_HISTPOS, &pos, WT_EOL);
}

static void
exit_cb (widget_t *w)
{
  sigquit (SIGHUP);
  wt_break (-1);
}

static int
init (const char *family, long fsize, const char *title, long xpos, long ypos)
{
  long a, b, wd, ht;
  widget_t *hpane;

  top = wt_init ();
  shellwin = wt_create (wt_shell_class, top);
  hpane = wt_create (wt_packer_class, shellwin);
  vt = wt_create (wt_vt_class, hpane);
  sb = wt_create (wt_scrollbar_class, hpane);

  if (!(vt && sb)) {
    fprintf(stderr, "Widget creation failed!\n");
    return -1;
  }

  wt_setopt (shellwin,
  	     WT_LABEL, title,
	     WT_ACTION_CB, exit_cb,
	     WT_EOL);

  a = 0;
  b = 1;
  wt_setopt (sb,
	     WT_TOTAL_SIZE, &rows,
             WT_SIZE, &rows,
	     WT_POSITION, &a,
             WT_LINE_INC, &b,
	     WT_PAGE_INC, &rows,
	     WT_ACTION_CB, sb_cb,
	     WT_EOL);

  wt_setopt (vt,
  	     WT_FONT, family,
  	     WT_FONTSIZE, &fsize,
             WT_VT_WIDTH, &cols,
	     WT_VT_HEIGHT, &rows,
	     WT_VT_HISTSIZE, &histsize,
	     WT_VT_VISBELL, &visualbell,
	     WT_VT_BLINK, &cblink,
	     WT_VT_REVERSE, &reverse,
	     WT_ACTION_CB, vt_cb,
	     WT_EOL);

  wt_setopt (hpane,
  	     WT_PACK_WIDGET, vt,
  	     WT_PACK_INFO,   "-side left -expand 1 -fill both -padx 1",
  	     WT_PACK_WIDGET, sb,
  	     WT_PACK_INFO,   "-side left -fill both -padx 1",
  	     WT_EOL);


  wt_geometry(shellwin, &a, &a, &wd, &ht);

  if (xpos < 0 && xpos != UNDEF) {
    xpos += wt_global.screen_wd - wd - 5;
  }
  if (ypos < 0 && ypos != UNDEF) {
    ypos += wt_global.screen_ht - ht - 19;
  }

  wt_setopt (shellwin,
	     WT_XPOS, &xpos,
	     WT_YPOS, &ypos,
	     WT_EOL);

  wt_bind (NULL, EV_KEYS|EV_DEFAULT, key_cb);

  return 0;
}

/**********************************************************************/

static void
usage (const char *progname)
{
  fprintf(stderr, "usage: %s [-f <font family>] [-s <font size>] [-g <geometry>]\n", progname);
  fprintf(stderr, "       [-h <histsize>] [-b] [-c] [-d] [-r] [-v] [-- program {args}]\n");
}

int
main (int argc, char **argv)
{
  const char *family = NULL, *shell = NULL, *cptr, *geometry = NULL;
  char buf[80], *ptr;
  int fsize = 0, c;
  struct passwd *pw;
  fd_set rfds;
  short uid;
  short xp, yp;
  char thesh[128];
#ifdef __MINT__
  int fd;
#endif
  extern char *optarg;
  extern int optind;

  /* just in case we're started in the background */
  signal(SIGTTOU, SIG_IGN);

  /* who am I? */
  if (!(pw = getpwuid((uid = getuid())))) {
    fprintf(stderr, "error: wterm can't determine determine your login name\n");
    return -1;
  }

  /*
   * scan arguments...
   */

  console = 0;
  while ((c = getopt (argc, argv, "bcdrvf:g:h:s:")) != EOF) {
    switch (c) {
      case 'b':
	cblink = 1;
	break;

      case 'c':
	console = 1;
	break;

      case 'd':
	debug = 1;
	break;

      case 'r':
	reverse = 1;
	break;

      case 'v':
	visualbell = 1;
	break;

      case 'f':
	family = optarg;
	break;

      case 's':
	fsize = atoi(optarg);
	break;

      case 'g':
	geometry = optarg;
	break;

      case 'h':
	histsize = atol (optarg);
	if (histsize < 0)
	  histsize = 0;
	break;

      default:
	usage(argv[0]);
	return 1;
    }
  }
  argv += optind;

  /*
   * now *argv either points to a program to start or is zero
   */

  if (*argv) {
    shell = *argv;
  }
  if (!shell) {
    shell = getenv("SHELL=");
  }
  if (!shell) {
    shell = pw->pw_shell;
  }
  if (!shell) {
    shell = "/bin/sh";
  }

  if (!*argv) {
    /*
     * the '-' makes the shell think it is a login shell,
     * we leave argv[0] alone if it isn`t a shell (ie.
     * the user specified the program to run as an argument
     * to wterm.
     */
    cptr = strrchr(shell, '/');
    sprintf (thesh, "-%s", cptr ? cptr + 1 : shell);
    *--argv = thesh;
  }

  xp = UNDEF;
  yp = UNDEF;
  if (geometry) {
    short col, lin;
    scan_geometry(geometry, &col, &lin, &xp, &yp);
    if (col > 0) {
      cols = col;
    }
    if (lin > 0) {
      rows = lin;
    }
  }

  /* 
   * we'll need to do this here so that we'll know how many colors VT
   * supports.
   */

  sprintf(buf, "wvt: %s", shell);
  if (init (family, fsize, buf, xp, yp))
    return -1;

#ifdef EXPORT_TERMCAP

  /*
   * this one should enable us to get rid of an /etc/termcap entry for
   * both curses and ncurses, hopefully...
   */

  if (termcap_string) {
    long colors;

    sprintf (termcap_string + strlen (termcap_string), "li#%ld:co#%ld:",
	     rows, cols);

    wt_getopt (vt, WT_COLORS, &colors, WT_EOL);
    if (colors) {
      /* 
       * add ANSI style color changing definitions for ncurses programs
       * (they'll convert these into terminfo database entry under your
       * $HOME/.terminfo/ directory)
       */
      /* colors, pairs, set fg, set bg, original pair */
      sprintf(termcap_string + strlen (termcap_string),
              "Co#%ld:pa#%ld:AF#\\Eb%%:AB#\\Ec%%:op#\\Ec0\\Eb%ld:",
      	      colors, colors * colors, colors - 1);
    }

    putenv (termcap_string);
  }

#else

  /*
   * we eliminate any pre-existing (incorrect) TERMCAP string
   */
# ifdef __MINT__
  putenv ("TERMCAP");
# else
  unsetenv ("TERMCAP");
# endif

#endif

  /*
   * what kind of terminal do we want to emulate?
   */
#ifdef __MINT__
  putenv ("TERM=wterm");
#else
  /* be sure to disable X11 use */
  unsetenv ("DISPLAY");
  setenv ("TERM", "wterm", 1);
#endif

  /*
   * create a pty
   */

  winsz.ws_col = cols;
  winsz.ws_row = rows;
  if ((pid = forkpty(&pipeh, pty, NULL, &winsz)) < 0) {
    printf("wterm: can't create pty\r\n");
    sleep(2);
    exit(1);
  }

  if ((ptr = rindex(pty, '/'))) {
    strcpy(pty, ptr + 1);
  }
  
  if (!pid) {
    int i;
    for (i = getdtablesize(); --i >= 3; )
      close (i);
    /*
     * SIG_IGN are not reset on exec()
     */
    for (i = NSIG; --i >= 0; )
      signal (i, SIG_DFL);
 
    /* caution: start shell with correct user id! */
    seteuid(getuid());
    setegid(getgid());

    /* this shall not return */
    execvp(shell, argv);

    /* oops? */
    printf("wterm: can't start shell\r\n");
    sleep(3);
    _exit(1);
  }

#ifdef __MINT__
  /*
   * the slave line opened in openpty() may still be our ctty.
   * so loose it.
   */
  fd = open ("/dev/null", O_RDWR);
  dup2 (fd, -1);
  close (fd);
#endif

  _write_utmp(pty, pw->pw_name, "", time(0));

  /* catch some signals */
  mysignal(SIGTERM, sigquit);
  mysignal(SIGHUP, sigquit);
  mysignal(SIGINT, sigquit);
  mysignal(SIGQUIT, sigquit);
  mysignal(SIGPIPE, sigpipe);
  mysignal(SIGCHLD, sigchld);

  /* prepare to catch console output */
  if (console) {

    /* for any OS chr$(7) might cause endless loops if catched from console */
    visualbell = 1;

#ifdef __MINT__
    if ((cfh = open("/dev/xconout2", O_RDONLY)) < 0) {
      printf("wterm: can't catch console output, device busy or not present\r\n");
      console = 0;
    }
#elif defined(sun) || defined(linux)
    console = 0;                     /* data will come to normal pipe handle */
    ioctl(pipeh, TIOCCONS, 0);
#else
    printf("wterm: can't catch console output on this operating system\r\n");
    console = 0;
#endif
  }

  /* caution: start shell with correct user id! */
  seteuid(getuid());
  setegid(getgid());


  fcntl (pipeh, F_SETFL, fcntl (pipeh, F_GETFL, 0) | O_NDELAY);
  if (console)
    fcntl (cfh, F_SETFL, fcntl (cfh, F_GETFL, 0) | O_NDELAY);

  FD_ZERO (&rfds);
  FD_SET (pipeh, &rfds);
  if (console)
    FD_SET (cfh, &rfds);

  inphandler = wt_addinput (&rfds, NULL, NULL, input_cb, 0);

  if (debug)
    w_trace(1);

  if (wt_realize (top)) {
    fprintf(stderr, "Widget realization failed (wrong font?)!\n");
    exit(-1);
  }

  wt_run ();
  return 0;
}
