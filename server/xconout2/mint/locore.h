#ifndef MAC

#define GEMDOS 0x84L
#define BIOS 0xb4L
#define XBIOS 0xb8L

#define SYSBASE 0x4f2L

#define RWABS 0x476L
#define MEDIACH 0x47eL
#define GETBPB 0x472L

#define DRVMAP 0x4c2L

#define TIMER 0x400L
#define FIVE 0x114L
#define VBL 4*0x1cL

#define HZ200 0x4baL
#define TICKS 0x442L

#define TERM 0x408L
#define CRIT 0x404L
#define RESVAL 0x426L
#define RESVEC 0x42aL

#define LOGBASE 0x44eL

#define PROCINFO 0x380L

#else

#ifdef ALT_TRAP
#define GEMDOS 0x84L
#define BIOS 0x88L
#define XBIOS 0x8cL
#else
#define GEMDOS 0x84L
#define BIOS 0xb4L
#define XBIOS 0xb8L
#endif

#define SVAR_VERS 3

struct sysvars {
	long version;
	void (*vbl)(void);
	long vbls;
	long vblq;
	void (*timer)(void);
	long hz200;
	long ticks;
	void (*five)(void);
	long sysbase;
	long logbase;
	void *saveda5;
	void *savedsp;
	void (*mac_env)(void);
	void (*mint_env)(void);
	short vm;
	void (*eventloop)(void);
	long (*rwabs)(short, void *, short, short, short, long);
	long (*mediach)(short);
	long (*getbpb)(short);
	long drvmap;
	long term;
	long crit;
	long resvec;
	long resval;
	struct { long buf[20]; } procinfo;
	short alias; /* indicates whether Alias Manager is present */
	short powerpc; /* non-zero indicates a power-pc emulating a 68LC040 */
	void (*jet_dispatch)(long selector);
	long reserved[4]; /* reserved for future expansion for YOUR use! */
};
extern struct sysvars *sysvar;

#define SYSBASE ((long)(&sysvar->sysbase))

#define RWABS ((long)(&sysvar->rwabs))
#define MEDIACH ((long)(&sysvar->mediach))
#define GETBPB ((long)(&sysvar->getbpb))

#define DRVMAP ((long)(&sysvar->drvmap))

#define TIMER ((long)(&sysvar->timer))
#define FIVE ((long)(&sysvar->five))
#define VBL ((long)(&sysvar->vbl))

#define HZ200 ((long)(&sysvar->hz200))
#define TICKS ((long)(&sysvar->ticks))

#define PROCINFO ((long)(&sysvar->procinfo))

#define MAC_ENV (*sysvar->mac_env)()
#define MINT_ENV (*sysvar->mint_env)()
#define HAS_VM ((short)(sysvar->vm))
#define POWERPC ((short)(sysvar->powerpc))

#define TERM ((long)(&sysvar->term))
#define CRIT ((long)(&sysvar->crit))
#define RESVEC ((long)(&sysvar->resvec))
#define RESVAL ((long)(&sysvar->resval))

#define LOGBASE ((long)(&sysvar->logbase))

#endif
