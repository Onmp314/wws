/*
 * ioctl() interface to /dev/audio.
 *
 * 11/03/95, Kay Roemer.
 */

#ifndef _AUDIOS_H
#define _AUDIOS_H

/*
 * LMC configuration. Possible values range from 0 (lowest) to 100 (highest)
 * possible value.
 */
#define AIOCSVOLUME	(('A' << 8) | 0)	/* master volume */
#define AIOCSLVOLUME	(('A' << 8) | 1)	/* left channel volume */
#define AIOCSRVOLUME	(('A' << 8) | 2)	/* right channel volume */
#define AIOCSBASS	(('A' << 8) | 3)	/* bass amplification */
#define AIOCSTREBLE	(('A' << 8) | 4)	/* treble amplification */

#define AIOCRESET	(('A' << 8) | 5)	/* reset audio hardware */
#define AIOCSYNC	(('A' << 8) | 6)	/* wait until playing done */
#define AIOCGBLKSIZE	(('A' << 8) | 7)	/* get dma block size */

#define AIOCSSPEED	(('A' << 8) | 8)	/* set play speed */
#define AIOCGSPEED	(('A' << 8) | 9)	/* get play speed */
#define AIOCSSTEREO	(('A' << 8) | 10)	/* stereo on/off */
#define AIOCGCHAN	(('A' << 8) | 11)	/* get # of channels */

#define AIOCGFMTS	(('A' << 8) | 12)	/* get supp. sample formats */
#define AIOCSFMT	(('A' << 8) | 13)	/* set sample format */
#define  AFMT_U8	0x00000001L		/* 8 bits, unsigned */
#define  AFMT_S8	0x00000002L		/* 8 bits, signed */
#define  AFMT_ULAW	0x00000004L		/* u law encoding */
#define  AFMT_ALAW	0x00000008L		/* a law encoding */

#endif
