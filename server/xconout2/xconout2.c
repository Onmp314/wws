/*
 * xconout2.c, a part of the W Window System
 *
 * Copyright (C) 1995 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- MiNT device driver for the xconout2 output handler
 */

#define	ARGS_ON_STACK
#define	P_(X)		X
#define	EXITING		volatile
#define	NORETURN

#include <mint/types.h>
#include <mint/file.h>
#include <mint/atarierr.h>
#include <mint/kernel.h>

#define	BUFSIZE	1024


/*
 *
 */

struct kerinfo	*kernel;
long		(*oldxconout2)();
long		rsel;
short		inbuf, wptr, rptr;
char		xconbuf[BUFSIZE];


static inline short
spl7 (void)
{
	short sr;
	__asm__ volatile ("movew sr, %0; oriw #0x0700, sr;" : "=g"(sr));
	return sr; 
}

static inline void
spl (short sr)
{
	__asm__ volatile ("movew %0, sr;" :: "g"(sr)); 
}

/*
 *	the xconout functions
 */

/* geee, that's ugly, but I can't do it any better :( */

void printc(long c)
{
	long	r;

__asm__ volatile
("
	moveml d0-d7/a0-a6,sp@-
	movl %1,sp@-
	movl _oldxconout2, a0
	jbsr a0@
	addqw #4,sp
	moveml sp@+,d0-d7/a0-a6"
: "=r"(r)			/* outputs */
: "g"(c)			/* inputs  */
/*: ""*/
);
}

void myxconout2(long ch)
{
	if (rsel) {
		WAKESELECT(rsel);
	}

	/* do not worry about buffer overflows here */
	if (inbuf == BUFSIZE) {
		return;
	}

	if (ch == 7) {
		printc(ch);
	} else {
		xconbuf[wptr] = ch;
		if (++wptr == BUFSIZE) wptr = 0;
		inbuf++;
	}
}

/*
 *	the device driver routines
 */

long xopen(FILEPTR *f)
{
	long	l;

	if (oldxconout2) {
		DEBUG("xconio: device already in use");
		return EACCDN;
	}

	/* get xconout2 (console) vector */

	l = *((long *)0x586);
	if ((l < 0x00E00000) || (l > 0x00FFFFFF)) {
		/* probably already somewhere in ram */
		return EINTRN;
	}

	/* set new vector */

	inbuf = wptr = rptr = 0;
	oldxconout2 = (void *)l;
	*((long *)0x586) = (long)myxconout2;

	return 0;
}

long xwrite(FILEPTR *f, const char *buf, long bytes)
{
	long	nwrite = 0;

	while (bytes-- > 0) {
		printc((short)*buf++);
		nwrite++;
	}

	return nwrite;
}

long xread(FILEPTR *f, char *buf, long bytes)
{
	long	ret = 0;
	short	sr;

	/*
	 * we must lock out the xcounout handler
	 * while mucking with inbuf and rptr.
	 *
	 * ++kay
	 */
	sr = spl7 ();
	while (bytes && inbuf) {

		*buf++ = xconbuf[rptr];
		if (++rptr == BUFSIZE) rptr = 0;
		inbuf--;
		spl (sr);
		bytes--;
		ret++;
		sr = spl7 ();
	}
	spl (sr);

	return ret;
}

long xlseek(FILEPTR *f, long where, int whence)
{
	/* (terminal) devices are always at position 0 */
	return 0;
}

long xioctl(FILEPTR *f, int mode, void *buf)
{
	switch (mode) {

		case FIONREAD:
			*((long *)buf) = (long)inbuf;
			break;

		case FIONWRITE:
			*((long *)buf) = 1;
			break;

		case FIOEXCEPT:
			*((long *)buf) = 0;
			break;

		default:
			return EINVFN;
	}

	return 0;
}

long xdatime(FILEPTR *f, short *timeptr, int rwflag)
{
	if (rwflag)
		return EACCDN;

	*timeptr++ = TGETTIME();
	*timeptr = TGETDATE();

	return 0;
}

long xclose(FILEPTR *f, int pid)
{
	if (f->links < 0) {

		ALERT("xconio: close(%d), device remaining open", pid);
		return EINTRN;
	}

	if (f->links > 0) {

		return 0;
	}

	/* reset xconout2 (console) vector */

	if (!oldxconout2) {
		return EINTRN;
	}

	*((long *)0x586) = (long)oldxconout2;
	oldxconout2 = 0;

	return 0;
}

long xselect(FILEPTR *f, long proc, int mode)
{
	long	ret;

	switch (mode) {

		case O_RDONLY:
			if (inbuf) {
				ret = 1;
			} else if (!rsel) {
				ret = 0;
				rsel = proc;
			} else
				ret = 2;
			break;

		case O_WRONLY: /* we're always ready to print */
			ret = 1;
			break;

		case O_RDWR: /* no exceptional consitions */
			ret = 0;
			break;

		default: /* we don't know this mode */
			ret = 0;
	}

	return ret;
}

void xunselect(FILEPTR *f, long proc, int mode)
{
	if (mode == O_RDONLY && proc == rsel) {
		rsel = 0;
	}
}


/*
 *	main
 */

DEVDRV			device = {xopen, xwrite, xread, xlseek, xioctl, xdatime, xclose, xselect, xunselect, xwrite, xread};
struct dev_descr	dinfo = {&device, 0, 0, NULL, sizeof(DEVDRV), S_IFCHR |
							   S_IRUSR | S_IWUSR |
							   S_IRGRP | S_IWGRP |
							   S_IROTH | S_IWOTH, {0, 0}};

DEVDRV *main(struct kerinfo *kptr)
{
	kernel = kptr;

	CCONWS("/dev/xconout2.xdd - xconout2 output catcher V0.2\r\n");
	CCONWS("(C) 01/95 by TeSche <itschere@techfak.uni-bielefeld.de>\r\n");
	CCONWS("USE AT YOUR OWN RISK!!!\r\n");

	if (!DCNTL(DEV_INSTALL, "u:\\dev\\xconout2", &dinfo)) {

		CCONWS("FAILED TO INSTALL DEVICE!!!\r\n\n");
		return 0;
	}

	oldxconout2 = NULL;
	rsel = 0;

	CCONWS("device successfully installed...\r\n\n");

	return (DEVDRV *)1;
}
