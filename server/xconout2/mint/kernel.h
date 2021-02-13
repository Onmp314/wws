/*
 *	kernel.h
 */

#define	NULL		(void *)0L

#define	ALERT		(kernel->alert)
#define	DEBUG		(kernel->debug)
#define	TRACE		(kernel->trace)
#define	WAKESELECT	(kernel->wakeselect)

#define	CCONWS		(kernel->dos_tab[0x009])
#define	TGETDATE	(kernel->dos_tab[0x02a])
#define	TGETTIME	(kernel->dos_tab[0x02c])
#define	SYIELD		(kernel->dos_tab[0x00ff])
#define	DCNTL		(kernel->dos_tab[0x130])
