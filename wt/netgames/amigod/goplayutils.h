#define iNil 32767 /* a distinguished value like nil */
#define  maxGroup 512
#define  maxSPoint 16
#define tryLimit 300

typedef short intBoard[19][19];  /* these were -2 to maxPoint + 2 */

typedef short boolBoard[19][19];

typedef struct
{
   short px, py;
} point;

typedef struct
{
   point p[401];
   short indx;
} pointList;

typedef struct
{
  point p[maxSPoint+1];
  short indx;
} sPointList;

typedef struct
{
   short indx,
   v[401];
} intList;
   
typedef struct { short w, s, sm; } sgRec;

typedef struct
{
   short groupMark,
         atLevel,
	 isLive,
	 isDead,
	 libC,
	 numEyes,
	 size,
	 lx, ly;
} groupRec;

typedef enum {rem, add, chLib, reMap} playType;

typedef struct { short who, xl, yl, nextGID, sNumber; } remAddRec;
typedef struct { short oldLC, oldLevel; } chLibRec;
typedef struct { short oldGID; } reMapRec;
typedef struct
{
   short gID;
   playType kind;
   union {
      remAddRec rem, add;
      chLibRec chLib;
      reMapRec reMap;
   } uval;
} playRec;


/* From goplayutils.c */
extern intBoard bord;
extern intBoard ndbord;
extern intBoard claim;
extern intBoard connectMap;
extern intBoard threatBord;
extern intBoard groupIDs;
extern intBoard protPoints;
extern boolBoard legal;
extern groupRec gList[maxGroup];
extern pointList pList;
extern pointList pList1;
extern pointList plist2;
extern pointList plist3;
extern sType mySType;
extern short maxGroupID;
extern short treeLibLim;
extern short killFlag;
extern short depthLimit;
extern short utilPlayLevel;
extern short sGlist[maxGroup + 1];
extern short gMap[maxGroup];
extern short adjInAtari;
extern short adj2Libs;
extern short playMark;
extern short dbStop;

extern short saveable (short, short, short *, short *);
extern void intersectPlist (pointList *, pointList *, pointList *);
extern void initArray (intBoard);
extern void restoreState (void);
extern void sortLibs (void);
extern void sSpanGroup (short, short, sPointList *);
extern void spanGroup (short, short, pointList *);
extern void genState (void);
extern void initGPUtils (void);
extern void tryPlay (short, short, short);
extern void pause (void);
extern void undoTo (short);
extern void listDiags (short, short, sPointList *);
extern void printBoard (intBoard brd, char *name);
