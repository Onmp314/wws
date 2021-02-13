/* AmiGo Include */

enum bVal { EMPTY = 0, BLACK, WHITE };
typedef enum Color bVal;

enum bMove { MOVE_OK = 0, MOVE_FAIL, MOVE_EVEN, MOVE_WIN, MOVE_LOSE };
typedef enum Move bMove;

#define TRUE 1
#define FALSE 0

#define MAXGROUPS 100

#define PLACED 0
#define REMOVED 1

#define numPoints 19
#define maxPoint numPoints - 1

typedef enum bVal sType;
struct Group
{
   enum bVal color;	/* The color of the group */
   short code,		/* The code used to mark stones in the group */
         count,		/* The number of stones in the group */
         internal,	/* The number of internal liberties */
         external,	/* The number of external liberties */
	 liberties,	/* The total number of liberties */
         eyes,		/* The number of eyes */
         alive,		/* A judgement of how alive this group is */
         territory;	/* The territory this group controls */
};

struct bRec
{
   enum bVal Val;	/* What is at this intersection */
   short xOfs, yOfs;
   short mNum;
   short GroupNum;	/* What group the stone belongs to */
   short marked;	/* TRUE or FALSE */
};


/* from amigo.c */
extern struct bRec goboard[19][19];	/* The main go board */
extern short ko, koX, koY;

/* From goplayer.c */
short genMove (enum bVal, short *, short *);
extern const char *playReason;
extern short playLevel;

/* killable.c */
extern short killable (short, short, short *, short *);
