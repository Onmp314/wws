/*
 * clipboard.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Kay Römer and
 * Copyright (C) 1997-1998 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- clipboard reading and writing functions
 *
 * NOTES
 * - This is based on the selection.c in the W Toolkit and moved into W
 *   library so that they both have the same clipboard format.
 * - Not thread-safe as for simplicity's sake selfile() doesn't malloc
 *   the returned string.
 *
 * CHANGES
 * ++eero, 8/97:
 * - Changed to method where clips of different type reside in a certain
 *   directory as _separate_ files.  Clip type and size are taken from the
 *   clip file name and size instead of reading them from the clip file
 *   itself.
 * - Added clipboard open, append and close functions to ease on the fly
 *   clip translation.
 * - Used unix open/read/write/close functions instead of C streams so that
 *   above functions can refer to clips with an int (file descriptor).
 *   Using non-buffered I/O with locking might work better on some OSes too.
 * - Added TRACE stuff to functions.
 * ++eero, 6/98:
 * - For security reasons, keep the clipboard directory in users' $HOME.
 * - If filesystem doesn't support locking, ignore it.
 * ++eero, 9/2000:
 * - replace W_SELTYPE_MAX with sizeof()-1
 */

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "Wlib.h"
#include "proto.h"

#ifndef ENOLCK
/* MiNTlib PL46 doesn't seem to define ENOLCK, ram-fs uses EINVAL
 * (invalid function)...
 */
#define ENOLCK	EINVAL
#endif


static int
dolock (int fd, int mode)
{
	struct flock fl;
	int ret;

	fl.l_type = mode;
	fl.l_whence = 0;
	fl.l_len = 0;
	fl.l_pid = 0;
  
	ret = fcntl (fd, F_SETLKW, &fl);

	if (ret < 0 && errno == ENOLCK) {
		/* if clipboard filesystem[1] doesn't support locking,
		 * presume it doesn't matter.
		 *
		 * [1] if $HOME is not set, clipboard will be in /tmp and
		 * that could be on MiNT ram-fs, DOS-fs etc, which don't
		 * support locking. You'll never know with home users :-).
		 */
		return 0;
	}
	return ret;
}


/* returns the full selection file name and at first calling makes sure
 * the clipboard directory exists
 */
static const char *
selfile (const char *type)
{
#define SEL_DIR	"/.wclips/"
	w_selection_t *dummy;
	static char *dir = NULL;
	static int len;
	const char *home;

	if (!dir) {

		home = getenv("HOME");
		if (!home) {
			home = "/tmp";
		}
		len = strlen(home) + sizeof(SEL_DIR) + sizeof(dummy->type) + 1;
		if (!(dir = malloc(len))){
			return NULL;
		}
		strcpy(dir, home);
		strcat(dir, SEL_DIR);

		mkdir(dir, 0755);
		len = strlen(dir);
	}

	strncpy(&dir[len], type, sizeof(dummy->type)-1);
	return dir;
#undef SEL_DIR
}


void
w_selclose (w_clipboard_t file)
{
	TRACESTART();
	if(file) {
		if (dolock (file, F_UNLCK) < 0) {
			perror("clip unlock");
			TRACEPRINT(("w_selclose(%d) -> unable to unlock\n", file));
		}
		close (file);
	}
	TRACEPRINT(("w_selclose(%d)\n", file));
	TRACEEND();
}

w_clipboard_t
w_selappend (w_clipboard_t file, const char *data, long len)
{
	TRACESTART();
	if (file >= 0 && len > 0) {

		if (write (file, data, len) == len) {
			TRACEPRINT(("w_selappend(%d,%p,%ld) -> %d\n",\
					file, data, len, file));
			TRACEEND();
			return file;
		}
	}
	TRACEPRINT(("w_selappend(%d,%p,%ld) -> -1\n", file, data, len));
	TRACEEND();
	return -1;
}

w_clipboard_t
w_selopen (const char *type)
{
	w_clipboard_t file;

	TRACESTART();

	if((file = open (selfile(type), O_WRONLY|O_CREAT, 0600)) < 0) {
		perror("clip create");
		TRACEPRINT(("w_selopen(`%s') -> unable to open\n", type));
		TRACEEND();
		return -1;
	}
	if (dolock (file, F_WRLCK) < 0) {
		perror("clip lock");
		TRACEPRINT(("w_selopen(`%s') -> unable to lock\n", type));
		TRACEEND();
		close (file);
		return -1;
	}
	ftruncate (file, 0);
	TRACEPRINT(("w_selopen(`%s') -> %d\n", type, file));
	TRACEEND();
	return file;
}


int
w_putselection (const char *type, const char *data, long len)
{
	w_clipboard_t file;

	TRACESTART();

	if((file = w_selopen(type)) < 0) {
		TRACEPRINT(("w_putselection(`%s',%p,%ld) -> unable to open\n",\
				type, data, len));
		TRACEEND();
		return -1;
	}
	w_selappend(file, data, len);
	w_selclose(file);

	TRACEPRINT(("w_putselection(`%s',%p,%ld) -> 0\n", type, data, len));
	TRACEEND();
	return 0;
}

w_selection_t *
w_getselection (const char *type)
{
	w_selection_t *sel = NULL;
	struct stat st;
	const char *name;
	int file;

	TRACESTART();

	name = selfile(type);
	if((file = open (name, O_RDONLY)) < 0) {
		TRACEPRINT(("w_getselection(`%s') -> unable to open clip type\n", type));
		TRACEEND();
		return NULL;
	}
	if (dolock (file, F_RDLCK) < 0)	{
		TRACEPRINT(("w_getselection(`%s') -> unable to lock clip type\n", type));
		TRACEEND();
		close (file);
		return NULL;
	}

	if((sel = calloc (1, sizeof (w_selection_t)))) {

		strncpy(sel->type, type, sizeof(sel->type)-1);

		stat(name, &st);
		if((sel->data = malloc(st.st_size+1))) {

			sel->len = read(file, sel->data, st.st_size);
			sel->data[sel->len] = 0;
			dolock (file, F_UNLCK);
			close (file);

			TRACEPRINT(("w_getselection(`%s') -> %p\n", type, sel));
			TRACEEND();
			return sel;
		}
		free(sel);
	}
	dolock (file, F_UNLCK);
	close (file);

	TRACEPRINT(("w_getselection(`%s') -> alloc failed\n", type));
	TRACEEND();
	return NULL;
}

void
w_freeselection (w_selection_t *sel)
{
	TRACESTART();
	if(sel) {
		free (sel->data);
		free (sel);
	}
	TRACEPRINT(("w_freeselection(%p)\n", sel));
	TRACEEND();
}
