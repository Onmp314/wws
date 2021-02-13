/*
 * Some nice Atari XBIOS dosound() effects in C.
 * Original ones from GFA expert v.2,
 * conversion to C by Eero Tamminen 1994
 */

#ifdef __SOZOBONX__
#include <types.h>
#else
typedef unsigned char UBYTE;
#endif

#define BEGIN   0x80
#define CHANNEL 0x81
#define PAUSE   0x82     /* followed by time in 1/50 seconds */
#define TERMI   0xFF     /* Sound        */
#define NATE    0x00     /*   terminator */

enum REGS { R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14 };

/* 14 parameters for the registers 0-13 and */
/* tone variations: channel, start, +/-step, end */

static UBYTE snd_off[] = {
  TERMI, NATE
};

static UBYTE snd_explosion[] = {
       R1,0, R2,0, R3,100, R4,0, R5,200, R6,0, R7,31,
       R8,198, R9,16, R10,16, R11,16, R12,207, R13,88, R14,0,
       BEGIN,255, CHANNEL,6,0,0,
       TERMI,NATE
};

static UBYTE snd_ding[] = {
       R1,200, R2,0, R3,201, R4,0, R5,100, R6,0, R7,0,
       R8,248, R9,16, R10,16, R11,16, R12,0, R13,20, R14,0,
       TERMI,NATE
};

static UBYTE snd_jingle[] = {
       R1,100, R2,4, R3,101, R4,4, R5,0, R6,0, R7,0,
       R8,252, R9,15, R10,15, R11,0, R12,0, R13,30, R14,0,
       PAUSE,5,
       R1,100, R2,3, R3,101, R4,3, R5,0, R6,0, R7,0,
       R8,252, R9,15, R10,15, R11,0, R12,0, R13,30, R14,0,
       PAUSE,5,
       R1,100, R2,2, R3,101, R4,2, R5,0, R6,0, R7,0,
       R8,252, R9,15, R10,15, R11,0, R12,0, R13,30, R14,0,
       PAUSE,5,
       R1,100, R2,1, R3,101, R4,1, R5,0, R6,0, R7,0,
       R8,252, R9,15, R10,15, R11,0, R12,0, R13,30, R14,0,
       PAUSE,5,
       R1,100, R2,0, R3,101, R4,0, R5,0, R6,0, R7,0,
       R8,252, R9,16, R10,16, R11,0, R12,0, R13,30, R14,0,
       TERMI,NATE
};

/* EOF */
