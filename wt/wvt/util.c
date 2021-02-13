/*
 * some pty helper functions
 *
 * Changes:
 * 
 * ++eero, 2/09:
 * - Add support for Unix98 PTY handling
 *   (Linux doesn't anymore support old PTY handling)
 * 
 * $Id: util.c,v 1.3 2009-09-02 20:22:47 eero Exp $
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <utmp.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "util.h"

#ifdef __MINT__
#define NSIG __NSIG
#elif linux
#define NSIG _NSIG
#endif

#ifndef __MINT__

void
_write_utmp (const char *line, const char *user, const char *host, u_long time)
{
  int fh, offset, isEmpty, isLine;
  struct utmp ut;

  if ((fh = open("/etc/utmp", O_RDWR)) < 0) {
    return;
  }

  /* first of all try to find an entry with the same line */

  offset = 0;
  isEmpty = -1;
  isLine = -1;

  while ((isLine < 0) && (read(fh, &ut, sizeof(ut)) == sizeof(ut))) {
    if (!ut.ut_line[0]) {
      if (isEmpty < 0) {
	isEmpty = offset;
      }
    } else {
      if (!strncmp(ut.ut_line, line, sizeof(ut.ut_line))) {
	isLine = offset;
      }
    }
    offset += sizeof(ut);
  }

  if (isLine != -1) {
    /* we've found a match */
    lseek(fh, isLine, SEEK_SET);
  } else if (isEmpty != -1) {
    /* no match found, but at least an empty entry */
    lseek(fh, isLine, SEEK_SET);
  } else {
    /* not even an empty entry found, assume we can append to the file */
  }

  if (time) {
    strncpy(ut.ut_line, line, sizeof(ut.ut_line));
    strncpy(ut.ut_name, user, sizeof(ut.ut_name));
    strncpy(ut.ut_host, host, sizeof(ut.ut_host));
    ut.ut_time = time;
#ifdef linux
    ut.ut_type = USER_PROCESS;
#endif
  } else {
    memset(&ut, 0, sizeof(ut));
#ifdef linux
    ut.ut_type = DEAD_PROCESS;
#endif
  }
  write(fh, &ut, sizeof(ut));
  close(fh);
}

int forkpty(int *master, char *pty, struct termios *t, struct winsize *w)
{
  int mh, sh, pid;

/* Unix98 specification conforming PTY handling */
#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
  const char *fname;
  /* get new master pty handle */
  if ((mh = posix_openpt(O_RDWR)) < 0) {
    return -1;
  }
  /* prepare slave pty */
  if (unlockpt(mh) < 0 || grantpt(mh) < 0 || !(fname = ptsname(mh))) {
    close(mh);
    return -1;
  }
#else

  int c, i;
  char fname[64];
  /* get a free pty */
  for (c='p'; c <= 'z'; c++) {
    for (i = 0; i < 16; i++) {
      sprintf(fname, "/dev/pty%c%x", c, i);
      if ((mh = open(fname, O_RDWR)) >= 0) {
        /* opened master pty, set slave name */
        fname[5] = 't';
        /* the infernal nested loop exit... */
        goto got_pty;
      }
    }
  }
  return -1;
got_pty:
#endif

  /* open slave */
  if ((sh = open(fname, O_RDWR)) < 0) {
    close(mh);
    return -1;
  }

  /* set terminal size */
  if (w) {
    ioctl(sh, TIOCSWINSZ, w);
  }

  /* try to fork */
  switch (pid = fork()) {

    case -1: /* parent: error */
      close(sh);
      close(mh);
      return -1;

    case 0: /* child */
#if 0
      close(mh);
#endif
      setsid();
      ioctl(sh, TIOCSCTTY, 0);

      dup2(sh, 0);
      dup2(sh, 1);
      dup2(sh, 2);
      if (sh > 2) {
	close(sh);
      }
      return 0;

    default: /* parent: ok */
#if 0
      close(sh);
#endif
      *master = mh;
      strcpy(pty, fname);
      return pid;
  }

  return -1;
}

#endif
