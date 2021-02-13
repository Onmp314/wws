/*
 * Some nice XBIOS dosound() effects in C.
 * Original ones from GFA expert v.2,
 * conversion by Eero Tamminen 1994
 */

typedef unsigned char UBYTE;

#define BEGIN   0x80
#define CHANNEL 0x81
#define PAUSE   0x82     /* followed by time in 1/50 seconds */
#define TERMI   0xFF     /* Sound        */
#define NATE    0x00     /*   terminator */

enum REGS { R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14 };

/* 14 parameters for the registers 0-13 and */
/* tone variations: channel, start, +/-step, end */

/* 'absorb' shield */
static UBYTE snd_pieuw[] = {
       R1,1, R2,0, R3,0, R4,0, R5,0, R6,0, R7,0,
       R8,254, R9,16, R10,0, R11,0, R12,0, R13,35, R14,1,
       BEGIN,50, CHANNEL,0,1,100,
       TERMI,NATE };

/* 'bounce' shield */
static UBYTE snd_gong[] = {
       R1,1, R2,5, R3,0, R4,5, R5,2, R6,5, R7,0,
       R8,248, R9,16, R10,16, R11,16, R12,0, R13,20, R14,1,
       TERMI,NATE };

/* shoot */
static UBYTE snd_laser[] = {
       R1,100, R2,0, R3,200, R4,0, R5,50, R6,0, R7,31,
       R8,220, R9,16, R10,0, R11,16, R12,127, R13,37, R14,0,
       BEGIN,0, CHANNEL,0,137,200,
       PAUSE,128,
       TERMI,NATE };

/* bunker explosion (air escapes :)) */
static UBYTE snd_steam[] = {
       R1,0, R2,0, R3,0, R4,0, R5,0, R6,0, R7,10,
       R8,199, R9,16, R10,16, R11,16, R12,0, R13,80, R14,0,
       PAUSE,20,
       R1,0, R2,0, R3,0, R4,0, R5,0, R6,0, R7,10,
       R8,255, R9,0, R10,0, R11,0, R12,0, R13,80, R14,100,
       TERMI,NATE };

/* shot explosion */
static UBYTE snd_explosion1[] = {
       R1,0, R2,0, R3,0, R4,0, R5,0, R6,0, R7,31,
       R8,199, R9,16, R10,16, R11,16, R12,0, R13,50, R14,9,
       TERMI,NATE };

/* 'bounce' shot */
static UBYTE snd_bounce5[] = {
       R1,82, R2,2, R3,251, R4,13, R5,34, R6,0, R7,0,
       R8,248, R9,16, R10,0, R11,0, R12,0, R13,86, R14,0,
       BEGIN,0, CHANNEL,0,11,0,
       TERMI,NATE };

/* 'laser' shot */
static UBYTE snd_tingeling2[] = {
       R1,0, R2,0, R3,0, R4,0, R5,0, R6,0, R7,0,
       R8,254, R9,16, R10,0, R11,0, R12,207, R13,88, R14,0,
       BEGIN,0, CHANNEL,0,41,0,
       TERMI,NATE };

/* 'groundhog' shot */
static UBYTE snd_bell[] = {
       R1,64, R2,0, R3,120, R4,0, R5,0, R6,0, R7,0,
       R8,252, R9,16, R10,16, R11,0, R12,20, R13,20, R14,0,
       TERMI,NATE };

/* open range */
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
       TERMI,NATE };

/* EOF */
