/*
 *	 In fact this is only a cut down version of MiNT's file.h,
 *	it only provides what we really need here. TeSche
 */

/*
 * This file has been modified as part of the MH-MiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
Copyright 1991,1992 Eric R. Smith.
Copyright 1992,1993,1994 Atari Corporation.
All rights reserved.
*/

/* file types */
#define S_IFMT	0170000		/* mask to select file type */
#define S_IFCHR	0020000		/* BIOS special file */
#define S_IFDIR	0040000		/* directory file */
#define S_IFREG 0100000		/* regular file */
#define S_IFIFO	0120000		/* FIFO */
#define S_IFMEM	0140000		/* memory region or process */
#define S_IFLNK	0160000		/* symbolic link */

/* special bits: setuid, setgid, sticky bit */
#define S_ISUID	04000
#define S_ISGID 02000
#define S_ISVTX	01000

/* file access modes for user, group, and other*/
#define S_IRUSR	0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP	0020
#define S_IXGRP	0010
#define S_IROTH	0004
#define S_IWOTH	0002
#define S_IXOTH	0001
#define DEFAULT_DIRMODE (0777)
#define DEFAULT_MODE	(0666)

struct filesys;	/* forward declaration */

struct flock {
	short l_type;			/* type of lock */
#define F_RDLCK		0
#define F_WRLCK		1
#define F_UNLCK		3
	short l_whence;			/* SEEK_SET, SEEK_CUR, SEEK_END */
	long l_start;			/* start of locked region */
	long l_len;			/* length of locked region */
	short l_pid;			/* pid of locking process
						(F_GETLK only) */
};

/* structure for internal kernel locks */
typedef struct ilock {
	struct flock l;		/* the actual lock */
	struct ilock *next;	/* next lock in the list */
	long	reserved[4];	/* reserved for future expansion */
} LOCK;

typedef struct f_cookie {
	struct filesys *fs;	/* filesystem that knows about this cookie */
	ushort	dev;		/* device info (e.g. Rwabs device number) */
	ushort	aux;		/* extra data that the file system may want */
	long	index;		/* this+dev uniquely identifies a file */
} fcookie;

typedef struct fileptr {
	short	links;	    /* number of copies of this descriptor */
	ushort	flags;	    /* file open mode and other file flags */
	long	pos;	    /* position in file */
	long	devinfo;    /* device driver specific info */
	fcookie	fc;	    /* file system cookie for this file */
	struct devdrv *dev; /* device driver that knows how to deal with this */
	struct fileptr *next; /* link to next fileptr for this file */
} FILEPTR;

typedef struct devdrv {
	long ARGS_ON_STACK (*open)	P_((FILEPTR *f));
	long ARGS_ON_STACK (*write)	P_((FILEPTR *f, const char *buf, long bytes));
	long ARGS_ON_STACK (*read)	P_((FILEPTR *f, char *buf, long bytes));
	long ARGS_ON_STACK (*lseek)	P_((FILEPTR *f, long where, int whence));
	long ARGS_ON_STACK (*ioctl)	P_((FILEPTR *f, int mode, void *buf));
	long ARGS_ON_STACK (*datime)	P_((FILEPTR *f, short *timeptr, int rwflag));
	long ARGS_ON_STACK (*close)	P_((FILEPTR *f, int pid));
	long ARGS_ON_STACK (*select)	P_((FILEPTR *f, long proc, int mode));
	void ARGS_ON_STACK (*unselect)	P_((FILEPTR *f, long proc, int mode));
/* extensions, check dev_descr.drvsize (size of DEVDRV struct) before calling:
 * fast RAW tty byte io  */
	long ARGS_ON_STACK (*writeb)	P_((FILEPTR *f, const char *buf, long bytes));
	long ARGS_ON_STACK (*readb)	P_((FILEPTR *f, char *buf, long bytes));
/* what about: scatter/gather io for DMA devices...
 *	long ARGS_ON_STACK (*writev)	P_((FILEPTR *f, const struct iovec *iov, long cnt));
 *	long ARGS_ON_STACK (*readv)	P_((FILEPTR *f, const struct iovec *iov, long cnt));
 */
} DEVDRV;

/*
 * this is the structure passed to loaded file systems to tell them
 * about the kernel
 */

struct kerinfo {
	short	maj_version;	/* kernel version number */
	short	min_version;	/* minor kernel version number */
	ushort	default_perm;	/* default file permissions */
	short	reserved1;	/* room for expansion */

/* OS functions */
	Func	*bios_tab;	/* pointer to the BIOS entry points */
	Func	*dos_tab;	/* pointer to the GEMDOS entry points */

/* media change vector */
	void	ARGS_ON_STACK (*drvchng) P_((unsigned));

/* Debugging stuff */
	void	ARGS_ON_STACK (*trace) P_((const char *, ...));
	void	ARGS_ON_STACK (*debug) P_((const char *, ...));
	void	ARGS_ON_STACK (*alert) P_((const char *, ...));
	EXITING void ARGS_ON_STACK (*fatal) P_((const char *, ...)) NORETURN;

/* memory allocation functions */
	void *	ARGS_ON_STACK (*kmalloc) P_((long));
	void	ARGS_ON_STACK (*kfree) P_((void *));
	void *	ARGS_ON_STACK (*umalloc) P_((long));
	void	ARGS_ON_STACK (*ufree) P_((void *));

/* utility functions for string manipulation */
	int	ARGS_ON_STACK (*strnicmp) P_((const char *, const char *, int));
	int	ARGS_ON_STACK (*stricmp) P_((const char *, const char *));
	char *	ARGS_ON_STACK (*strlwr) P_((char *));
	char *	ARGS_ON_STACK (*strupr) P_((char *));
	int	ARGS_ON_STACK (*sprintf) P_((char *, const char *, ...));

/* utility functions for manipulating time */
	void	ARGS_ON_STACK (*millis_time) P_((unsigned long, short *));
	long	ARGS_ON_STACK (*unixtim) P_((unsigned, unsigned));
	long	ARGS_ON_STACK (*dostim) P_((long));

/* utility functions for dealing with pauses, or for putting processes
 * to sleep
 */
	void	ARGS_ON_STACK (*nap) P_((unsigned));
	int	ARGS_ON_STACK (*sleep) P_((int que, long cond));
	void	ARGS_ON_STACK (*wake) P_((int que, long cond));
	void	ARGS_ON_STACK (*wakeselect) P_((long param));

/* file system utility functions */
	int	ARGS_ON_STACK (*denyshare) P_((FILEPTR *, FILEPTR *));
	LOCK *	ARGS_ON_STACK (*denylock) P_((LOCK *, LOCK *));

/* functions for adding/cancelling timeouts */
	struct timeout * ARGS_ON_STACK (*addtimeout) P_((long, void (*)()));
	void	ARGS_ON_STACK (*canceltimeout) P_((struct timeout *));
	struct timeout * ARGS_ON_STACK (*addroottimeout) P_((long, void (*)(), short));
	void	ARGS_ON_STACK (*cancelroottimeout) P_((struct timeout *));

/* miscellaneous other things */
	long	ARGS_ON_STACK (*ikill) P_((int, int));
	void	ARGS_ON_STACK (*iwake) P_((int que, long cond, short pid));

/* reserved for future use */
	long	res2[3];
};

/* flags for open() modes */
#define O_RWMODE  	0x03	/* isolates file read/write mode */
#	define O_RDONLY	0x00
#	define O_WRONLY	0x01
#	define O_RDWR	0x02

/* more constants for various Fcntl's */
#define FIONREAD	(('F'<< 8) | 1)
#define FIONWRITE	(('F'<< 8) | 2)
#define FIOEXCEPT	(('F'<< 8) | 5)

/* Dcntl constants and types */
#define DEV_NEWTTY	0xde00
#define DEV_NEWBIOS	0xde01
#define DEV_INSTALL	0xde02

struct dev_descr {
	DEVDRV	*driver;
	short	dinfo;
	short	flags;
	struct tty *tty;
	long	drvsize;		/* size of DEVDRV struct */
	long	fmode;
	long	reserved[2];
};
