/* AmiGo Interface.  Started 4/7/88 by Todd R. Johnson
 * 8/8/89 Cleaned up for first release
 * Version 1.0
 * Public Domain
 *
 * Server version:
 * - Replaced GUI interface with a socket one.
 * - Put textual game action reporting behind DEBUG define.
 *
 * TODO:
 * - Scoring
 * - Undo
 *
 *   1996 by Eero Tamminen
 */

#include "server.h"
#include "messages.h"
#include "ports.h"				/* game ports/IDs */
#include "go.h"

#define VERSION	"v0.5"

#if defined(__MINT__) || defined(atarist)
long _stksize = 32000;
#endif

struct bRec goboard[19][19];	/* The main go board */
short ko, koX, koY;

/* Procedures from this file */
static short Connect (enum bVal, short, short, short[4], short[4], short *, short *);
static short Suicide (enum bVal, short, short);
static short StoneLibs (short, short);
static void EraseMarks (void);
static void GoPlaceStone (enum bVal, short, short);
static void MergeGroups (short, short);
static void ReEvalGroups (enum bVal, short, short, short);
static void GroupCapture (short);
static void FixLibs (enum bVal, short, short, short);
static void RelabelGroups (void);
static short CountAndMarkLibs (short, short);
static void CountLiberties (short);
static void CountEyes (void);
#ifdef DEBUG
static void printGroupReport (short, short);
static void PrintGroupInfo (void);
#endif

static struct Group GroupList[MAXGROUPS];	/* The list of Groups */
static short DeletedGroups[4];		/* Codes of deleted groups */

static short GroupCount = 0;		/* The total number of groups */
static short DeletedGroupCount;	/* The total number of groups deleted
				   on a move */
static short blackPrisoners, whitePrisoners;

/* Arrays for use when checking around a point */
static short xVec[4] = {0, 1, 0, -1};
static short yVec[4] = {-1, 0, 1, 0};


#if 0	/* future additions */
static void CheckForEye (short, short, short[4], short, short *);
static short showMoveReason = False, groupInfo = False;

static void CheckForEye (x, y, groups, cnt, recheck)
     short x, y, groups[4], cnt, *recheck;
{
  short i;
  for (i = 0; i < (cnt - 1); i++)
    if (groups[i] != groups[i + 1])
    {
      /* Mark liberty for False eye check */
      goboard[x][y].marked = True;
      (*recheck)++;
      return;
    }
  /* It is an eye */
  GroupList[groups[i]].eyes += 1;
}
#endif

/* Returns the maximum number of liberties for a given intersection */
static inline short Maxlibs (x, y)
     short x, y;
{
  short cnt = 4;
  if (x == 0 || x == maxPoint)
    cnt = cnt - 1;
  if (y == 0 || y == maxPoint)
    cnt = cnt - 1;
  return cnt;
}

static short member (group, grouplist, cnt)
     short group, grouplist[4], cnt;
{
  short i;
  for (i = 0; i < cnt; i++)
    if (grouplist[i] == group)
      return True;
  return False;
}

/* Does a stone at x, y connect to any groups of color? */
static short Connect (color, x, y, fGroups, fCnt, eGroups, eCnt)
     enum bVal color;
     short x, y;
     short fGroups[4], eGroups[4];
     short *fCnt, *eCnt;
{
  unsigned short point = 0;
  short tx, ty, total = 0;
  enum bVal opcolor = WHITE;
  *fCnt = 0;
  *eCnt = 0;
  if (color == WHITE)
    opcolor = BLACK;
  for (point = 0; point <= 3; point++)
  {
    tx = x + xVec[point];
    ty = y + yVec[point];
    if (tx >= 0 && tx <= maxPoint && ty >= 0 && ty <= maxPoint) {
      if (goboard[tx][ty].Val == color
	  && !member (goboard[tx][ty].GroupNum, fGroups, *fCnt))
      {
	fGroups[(*fCnt)++] = goboard[tx][ty].GroupNum;
	total += 1;
      }
      else if (goboard[tx][ty].Val == opcolor
	       && !member (goboard[tx][ty].GroupNum, eGroups, *eCnt))
      {
	eGroups[(*eCnt)++] = goboard[tx][ty].GroupNum;
	total += 1;
      }
    }
  }
  return total;
}

/* Determine whether x, y is suicide for color */
static short Suicide (color, x, y)
     enum bVal color;
     short x, y;
{
  enum bVal opcolor = BLACK;
  short friendlycnt, friendlygroups[4], enemycnt, enemygroups[4], total;
  short maxlibs, i, libcnt = 0;
  if (color == BLACK)
    opcolor = WHITE;
  maxlibs = Maxlibs (x, y);
  total = Connect (color, x, y, friendlygroups, &friendlycnt,
		   enemygroups, &enemycnt);
  if (total < maxlibs)
    return False;
  /* Check for a capture */
  for (i = 0; i < enemycnt; i++)
    if (GroupList[enemygroups[i]].liberties == 1)
      return False;
  for (i = 0; i < friendlycnt; i++)
    libcnt += GroupList[friendlygroups[i]].liberties - 1;
  if (libcnt != 0)
    return False;
  return True;
}

/* Returns the number of liberties for x, y */
static short StoneLibs (x, y)
     short x, y;
{
  short cnt = 0, tx, ty;
  unsigned short point;
  for (point = 0; point <= 3; point++)
  {
    tx = x + xVec[point];
    ty = y + yVec[point];
    if (tx >= 0 && tx <= maxPoint && ty >= 0 && ty <= maxPoint &&
	goboard[tx][ty].Val == EMPTY)
      cnt++;
  }
  return cnt;
}

static void EraseMarks (void)
{
  short x, y;
  for (y = 0; y <= maxPoint; y++)
    for (x = 0; x <= maxPoint; x++)
      goboard[x][y].marked = False;
}

/* Place a stone of color at x, y */
static void GoPlaceStone (color, x, y)
     enum bVal color;
     short x, y;
{
  short fgroups[4], egroups[4];	/* group codes surrounding stone */
  short fcnt, ecnt, i;
  short lowest = GroupCount + 1;

  /* validity checks were moved to the make_move() function */

  ko = False;
  DeletedGroupCount = 0;
  goboard[x][y].Val = color;
  /* Does the new stone connect to any friendly stone(s)? */
  Connect (color, x, y, fgroups, &fcnt, egroups, &ecnt);
  if (fcnt)
  {
    /* Find the connecting friendly group with the lowest code */
    for (i = 0; i < fcnt; i++)
      if (fgroups[i] <= lowest)
	lowest = fgroups[i];

    /* Renumber resulting group
     * Raise the stone count of the lowest by one to account for new stone
     */
    goboard[x][y].GroupNum = lowest;
    GroupList[lowest].count += 1;
    for (i = 0; i < fcnt; i++)
      if (fgroups[i] != lowest)
	MergeGroups (lowest, fgroups[i]);
    /* Fix the liberties of the resulting group */
    CountLiberties (lowest);	/* Fix up liberties for group */
  }
  else
  {
    /* Isolated stone.  Create new group. */
    GroupCount += 1;
    lowest = GroupCount;
    GroupList[lowest].color = color;
    GroupList[lowest].count = 1;
    GroupList[lowest].internal = 0;
    GroupList[lowest].external = StoneLibs (x, y);
    GroupList[lowest].liberties = GroupList[lowest].external;
    GroupList[lowest].eyes = 0;
    GroupList[lowest].alive = 0;
    GroupList[lowest].territory = 0;
    goboard[x][y].GroupNum = lowest;
  }
  /* Now fix the liberties of enemy groups adjacent to played stone */
  FixLibs (color, x, y, PLACED);/* Fix the liberties of opcolor */
  ReEvalGroups (color, x, y, lowest);
  RelabelGroups ();
}

/* Merges two groups -- Renumbers stones and deletes second group from list.
 * Fixes stone count of groups.  This does not fix anything else.  FixLibs
 * must be called to fix liberties, etc.
 */
static void MergeGroups (g1, g2)
     short g1, g2;
{
  short x, y;
  for (y = 0; y <= maxPoint; y++)
    for (x = 0; x <= maxPoint; x++)
      if (goboard[x][y].GroupNum == g2)
	goboard[x][y].GroupNum = g1;
  GroupList[g1].count += GroupList[g2].count;
  DeletedGroups[DeletedGroupCount++] = g2;
}

/* Re-evaluate the groups given the last move.  This assumes that the last
 * move has been merged into adjoining groups and all liberty counts are
 * correct.  Handles capture.  Checks for ko.  Keeps track of captured
 * stones.  code is the group number of the stone just played.
 */
static void ReEvalGroups (color, x, y, code)
     enum bVal color;
     short x, y, code;
{
  short fgroups[4], egroups[4], fcnt, ecnt, i, killcnt = 0, count = 0;
  enum bVal opcolor = BLACK;
  if (color == BLACK)
    opcolor = WHITE;
  /* Check for capture */
  Connect (color, x, y, fgroups, &fcnt, egroups, &ecnt);
  if (ecnt)
  {
    /* See if any of the groups have no liberties */
    for (i = 0; i < ecnt; i++)
      if (GroupList[egroups[i]].liberties == 0)
      {
	killcnt += 1;
	count = GroupList[egroups[i]].count;
	GroupCapture (egroups[i]);
      }
  }
  /* Check for ko.  koX and koY are set in GroupCapture above. */
  if (killcnt == 1 && count == 1
      && GroupList[code].count == 1
      && GroupList[code].liberties == 1)
    ko = True;

  /* Set eye count for groups */
  CountEyes ();
#ifdef DEBUG
  PrintGroupInfo ();
#endif
}

/* Remove a captured group from the board and fix the liberties of any
 * adjacent groups.  Fixes prisoner count.  Sets koX and koY
 */
static void GroupCapture (code)
     short code;
{
  short x, y;
  if (GroupList[code].color == BLACK)
    blackPrisoners += GroupList[code].count;
  else
    whitePrisoners += GroupList[code].count;
  for (y = 0; y <= maxPoint; y++)
    for (x = 0; x <= maxPoint; x++)
      if (goboard[x][y].GroupNum == code)
      {
	FixLibs (GroupList[code].color, x, y, REMOVED);
	goboard[x][y].Val = EMPTY;
	goboard[x][y].GroupNum = 0;
	koX = x;
	koY = y;
      }
  DeletedGroups[DeletedGroupCount++] = code;
}

/* Fix the liberties of groups adjacent to x, y.  move indicates whether a
 * stone of color was placed or removed at x, y This does not change
 * liberty counts of friendly groups when a stone is placed.  Does not do
 * captures.
 */
static void FixLibs (color, x, y, move)
     enum bVal color;
     short x, y, move;
{
  short fgroups[4], fcnt, egroups[4], ecnt, i;
  enum bVal opcolor = BLACK;
  if (color == BLACK)
    opcolor = WHITE;
  Connect (color, x, y, fgroups, &fcnt, egroups, &ecnt);
  if (move == PLACED)
    for (i = 0; i < ecnt; i++)
      GroupList[egroups[i]].liberties -= 1;
  else				/* Stone removed so increment opcolor */
    for (i = 0; i < ecnt; i++)
      GroupList[egroups[i]].liberties += 1;
}

/* if any groups have been deleted as a result of the last move, this
 * routine will delete the old group numbers from GroupList and reassign
 * group numbers.
 */
static void RelabelGroups (void)
{
  short i, j, x, y;
  for (i = 0; i < DeletedGroupCount; i++)
  {
    /* Relabel all higher groups */
    for (y = 0; y <= maxPoint; y++)
      for (x = 0; x <= maxPoint; x++)
	if (goboard[x][y].GroupNum > DeletedGroups[i])
	  goboard[x][y].GroupNum = goboard[x][y].GroupNum - 1;
    /* Move the groups down */
    for (y = DeletedGroups[i]; y < GroupCount; y++)
      GroupList[y] = GroupList[y + 1];
    /* fix the group numbers stored in the deleted list */
    for (j = i + 1; j < DeletedGroupCount; j++)
      if (DeletedGroups[j] > DeletedGroups[i])
	DeletedGroups[j] -= 1;
    GroupCount -= 1;
  }
}

/* Returns liberty count for x, y intersection.  Sets marked to True for
 * each liberty
 */
static short CountAndMarkLibs (x, y)
     short x, y;
{
  short cnt = 0;
  if ((x > 0) && (goboard[x - 1][y].Val == EMPTY)
      && (goboard[x - 1][y].marked == False))
  {
    cnt++;
    goboard[x - 1][y].marked = True;
  }
  if ((x < maxPoint) && (goboard[x + 1][y].Val == EMPTY)
      && (goboard[x + 1][y].marked == False))
  {
    cnt++;
    goboard[x + 1][y].marked = True;
  }
  if ((y > 0) && (goboard[x][y - 1].Val == EMPTY)
      && (goboard[x][y - 1].marked == False))
  {
    cnt++;
    goboard[x][y - 1].marked = True;
  }
  if ((y < maxPoint) && (goboard[x][y + 1].Val == EMPTY)
      && (goboard[x][y + 1].marked == False))
  {
    cnt++;
    goboard[x][y + 1].marked = True;
  }
  return cnt;
}

/* Determine the number of liberties for a group given the group code num
 */
static void CountLiberties (code)
     short code;
{
  short x, y, libcnt = 0;
  for (y = 0; y <= maxPoint; y++)
    for (x = 0; x <= maxPoint; x++)
      if (goboard[x][y].GroupNum == code)
	libcnt += CountAndMarkLibs (x, y);
  EraseMarks ();
  GroupList[code].liberties = libcnt;
}

/* Set the eye count for the groups */
static void CountEyes (void)
{
  short i, x, y, wgroups[4], bgroups[4], wcnt, bcnt, max, cnt, recheck = 0,
    eye;
  for (i = 1; i <= GroupCount; i++)
    GroupList[i].eyes = 0;
  for (y = 0; y <= maxPoint; y++)
    for (x = 0; x <= maxPoint; x++)
    {
      if (goboard[x][y].Val != EMPTY)
	continue;
      cnt = Connect (WHITE, x, y, wgroups, &wcnt, bgroups, &bcnt);
      max = Maxlibs (x, y);
      if (cnt == max && wcnt == 1 && bcnt == 0)
	GroupList[wgroups[0]].eyes += 1;
      else if (cnt == max && bcnt == 1 && wcnt == 0)
	GroupList[bgroups[0]].eyes += 1;
      else if (cnt == max && (bcnt == 0 || wcnt == 0))
      {
	goboard[x][y].marked = True;
	recheck++;
      }
    }
  /* Now recheck marked liberties to see if two or more one eye groups
   * contribute to a False eye
   */
  if (recheck == 0)
    return;
  for (y = 0; y <= maxPoint; y++)
    for (x = 0; x <= maxPoint; x++)
      if (goboard[x][y].marked)
      {
	recheck--;
	goboard[x][y].marked = False;
	Connect (WHITE, x, y, wgroups, &wcnt, bgroups, &bcnt);
	/* If all the groups have at least one eye then all the groups are
	 * safe from capture because of the common liberty at x, y
	 */
	eye = True;
	for (i = 0; i < wcnt; i++)
	  if (GroupList[wgroups[i]].eyes == 0)
	    eye = False;
	if (eye)
	  for (i = 0; i < wcnt; i++)
	    GroupList[wgroups[i]].eyes += 1;
	for (i = 0; i < bcnt; i++)
	  if (GroupList[bgroups[i]].eyes == 0)
	    eye = False;
	if (eye)
	  for (i = 0; i < bcnt; i++)
	    GroupList[bgroups[i]].eyes += 1;
	if (recheck == 0)
	  return;
      }
}

#ifdef DEBUG

/* Print group information */
static void PrintGroupInfo (void)
{
  short i;
  for (i = 1; i <= GroupCount; i++)
    fprintf (stderr, "Group number %d\n\tEyes: %d\nLibs: %d\n",
	    i, GroupList[i].eyes, GroupList[i].liberties);
}

static void printGroupReport (int x, int y)
     short x, y;
{
  short g;
  if (!groupInfo)
    return;
  g = goboard[x][y].GroupNum;

  fprintf (stderr, "Group %d\n", g);
  fprintf (stderr, "Stones: %d\n", GroupList[g].count);
  fprintf (stderr, "Libs: %d\n", GroupList[g].liberties);
  fprintf (stderr, "Eyes: \n", GroupList[g].eyes);
}
#endif


/* ------------------------ */
static short CompX, CompY;
static int ComputerColor, HumanColor;
int
  Moving,		/* computer is (now/still) sending it's move */
  Passed,		/* previous player passed */
  Scoring;		/* game ended, start group removal */

static void restart_game (void)
{
  short x, y;

  ko = False;
  blackPrisoners = 0;
  whitePrisoners = 0;
  Moving = Scoring = Passed = False;

  for (y = 0; y <= maxPoint; y++)
    for (x = 0; x <= maxPoint; x++)
      if (goboard[x][y].Val != EMPTY)
	goboard[x][y].Val = EMPTY;
}

static int set_level(int level)
{
  if(level > 0 && level < 8)
  {
    playLevel = level;
    return True;
  }
  return False;
}

static void help(void)
{
  fprintf (stderr, "\nAmiGo, server " VERSION "\n");
  fprintf (stderr, "Player by Stoney Ballard\n");
  fprintf (stderr, "C port and Amiga interface by Todd R. Johnson\n");
  fprintf (stderr, "Server version by Eero Tamminen\n");
  fprintf (stderr, "\nOptions:\n");
  fprintf (stderr, " -l <level (1-7)>\n");
  fprintf (stderr, " -h help\n");
}

static int game_info (char *name, int argc, char *argv[])
{
  int idx = -1;

  while(++idx < argc)
  {
    if(argv[idx][0] == '-' && !argv[idx][2])
      switch(argv[idx][1])
      {
	case 'l':
         if(++idx < argc && set_level(atoi(argv[idx])))
	   break;
	 help();
	 return False;

       case 'h':
         help();
	 break;

       default:
         help();
	 return False;
      }
    else
    {
      help();
      return False;
    }
  }
  return True;
}

static int pass_move(player_t color)
{
  /* opponent done with scoring (removed dead groups & passed)? */
  if(Scoring)
  {
    game_over();
    DEBUG_PRINT("Game over");
    Passed = Scoring = False;
    /* return pass */
    return True;
  }

  /* Game done (both passed)? -> scoring */
  if(Passed)
  {
    DEBUG_PRINT("marking dead groups.");
    Scoring = True;
    Passed = False;
    return True;
  }

  Passed = True;
  return True;
}

static int move_ok(int color, int x, int y)
{
  if(Scoring)
    return False;

  /* illegal moves during game play... */

  if (goboard[x][y].Val != EMPTY)
    return False;

  if (Suicide (color, x, y))
    return False;

  if (ko && koX == x && koY == y)
    return False;

  return True;
}

/* Message names relative to the original message sender.
 * If one trusts opponent to send only legal moves,
 * it's faster to return to the message before acting it.
 */
static int messages(short type, uchar x, uchar y)
{
  if(Scoring)
  {
    if(type == PIECE_REMOVE)
    {
      send_msg(RETURN_REMOVE, x, y);
      return True;
    }
    return False;
  }

  switch(type)
  {
    case PIECE_MINE:
      if(move_ok(HumanColor, x, y))
      {
        send_msg(RETURN_MINE, x, y);
        /* make the move and re-evaluate board */
        GoPlaceStone (HumanColor, x, y);
#ifdef DEBUG
        fprintf(stderr, "Human to (%d,%d).\n", x, y);
#endif
      }
      else
      {
        /* send in case opponent got an inferior game engine */
        send_msg(RETURN_FAIL, 0, 0);
#ifdef DEBUG
        void printGroupReport (x, y);
#endif
      }
      break;

    case RETURN_MINE:
      send_msg(MOVE_NEXT, 0, 0);
      GoPlaceStone(ComputerColor, x, y);
      break;

    default:
      return False;
  }
  /* no error, move done */
  Passed = False;
  return True;
}

/* calculate computer move */
static void comp_move(void)
{
  /* computer move */
  if (genMove (ComputerColor, &CompX, &CompY))
  {
#ifdef DEBUG
    fprintf(stderr, "Computer (%s) to (%d,%d).\n", playReason, CompX, CompY);
#endif
  }
  else
    CompX = -1;

  Moving = True;
}

/* send computer's move to the opponent */
static int send_move(void)
{
  if(Moving)
  {
    Moving = False;
    if(CompX >= 0 && CompY >= 0)
    {
      send_msg(PIECE_MINE, CompX, CompY);
      return True;
    }
    else
      send_msg(MOVE_PASS, 0, 0);
  }
  else
    send_msg(MOVE_NEXT, 0, 0);

  /* turn over, next player */
  return False;
}

static void set_side(player_t player)
{
  if(player == Player0)
  {
    ComputerColor = BLACK;
    HumanColor = WHITE;
  }
  else
  {
    ComputerColor = WHITE;
    HumanColor = BLACK;
  }
}

void get_configuration(GAME *game)
{
  /* variables */
  game->game_id = GO_ID;

  /* functions */
  game->message = messages;
  game->args    = game_info;
  game->start   = restart_game;
  game->level   = set_level;
  game->comp    = comp_move;
  game->move    = send_move;
  game->pass    = pass_move;
  game->side    = set_side;

  playLevel = 7;
}

