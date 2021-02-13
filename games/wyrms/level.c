/* reads and parses Wyrms level files */

#include <stdio.h>	/* file stuff */
#include <string.h>
#define __LEVEL_C__
#include "wyrms.h"	/* defines and globals */

#define LEVEL_H	(SCREEN_H - 1)

/* last loaded level map */
static unsigned char LevelMap[SCREEN_W][SCREEN_H];

static const char DirConv[4][2] = {{'U', UP}, {'R', RIGHT}, {'D', DOWN}, {'L', LEFT}};

/* structures containing wyrms' initial values */
static Wyrms Crawler[2];

/* prototypes for local functions */
static void parse_map(const char *string);
static int read_bin(FILE *fp);
static int read_ascii(FILE *fp);


/* copy level map to the current map */
void init_level(unsigned char *map, Wyrms *worm)
{
  int x, y;

  for(x = 0; x < SCREEN_W; x++)
    for(y = 0; y < SCREEN_H; y++)
      *(map++) = LevelMap[x][y];

  worm[0].pos[0].x = Crawler[0].pos[0].x;
  worm[0].pos[0].y = Crawler[0].pos[0].y;
  worm[0].dir      = Crawler[0].dir;
  worm[1].pos[0].x = Crawler[1].pos[0].x;
  worm[1].pos[0].y = Crawler[1].pos[0].y;
  worm[1].dir      = Crawler[1].dir;
}

/* clear map and load a new one from a file */
int read_map(const char *path, const char *level)
{
#define TMP_LEN 14
  char id[TMP_LEN], new_dir[4];
  const char *info_msg = INFO_MSG;
  char tmp[FILENAME_MAX];
  int x, y, ok = FALSE;
  FILE *fp;

  /* open a level file */
  if(!(fp = fopen(level, "rb")))
  {
    /* use the map as a temp for file path+name */
    strcpy(tmp, path);
    strcat(tmp, level);
    fp = fopen(tmp, "rb");
  }
  /* defaults */
  new_dir[0] = new_dir[1] = 'D';

  /*  clear map */
  for(x = 0; x < SCREEN_W; x++)
    for(y = 0; y < SCREEN_H; y++)
      LevelMap[x][y] = I_BG;

  /* read a level file */
  if(fp)
  {
    /* read level format id and initial player directions */
    fgets(id, TMP_LEN, fp);
    fgets(new_dir, 4, fp);

    /* read the map either from ascii or binary format level file */
    if(memcmp(id, "WYRMS-A", (size_t)7) == 0)
      ok = read_ascii(fp);
    else
      if(memcmp(id, "WYRMS-B", (size_t)7) == 0)
        ok = read_bin(fp);

    fclose(fp);
  }
  /* parse directions and search starting places or use defauls */
  parse_map(new_dir);

  /* garantee a wall around the level map and
   * copy the game message to the bottom of the map
   */
  for(y = 1; y < INFOLINE - 1; y++)
  {
    LevelMap[0][y] = I_WALL;
    LevelMap[SCREEN_W - 1][y] = I_WALL;
  }
  for(x = 0; x < SCREEN_W; x++)
  {
    LevelMap[x][0] = I_WALL;
    LevelMap[x][INFOLINE - 1] = I_WALL;
    LevelMap[x][INFOLINE] = info_msg[x];
  }
  return ok;
#undef TMP_LEN
}

/* parse player heading */
static void parse_map(const char *string)
{
  int x, y, found = 0;

  for(y = 0; y < 2; y++)
  {
    Crawler[y].dir = 0;
    for(x = 0; x < 4; x++)
      if(DirConv[x][0] == string[y])
        Crawler[y].dir = DirConv[x][1];
  }
  if(!(Crawler[0].dir || Crawler[1].dir))
  {
    message("Bogus wyrm directions");
    Crawler[0].dir = Crawler[1].dir = DOWN;
  }

  /* search for the starting places */
  for(y = 0; y < SCREEN_H; y++)
    for(x = 0; x < SCREEN_W; x++)
      switch(LevelMap[x][y])
      {
	case I_BLACK:
	  Crawler[0].pos[0].x = x;
	  Crawler[0].pos[0].y = y;
	  found |= 1;
	  break;
	case I_WHITE:
	  Crawler[1].pos[0].x = x;
	  Crawler[1].pos[0].y = y;
	  found |= 2;
	  break;
      }
  if(found != 3)
  {
    /* initialdirections & positions if no level file */
    Crawler[0].pos[0].x = SCREEN_W / 3;
    Crawler[1].pos[0].x = SCREEN_W * 2 / 3;
    Crawler[0].pos[0].y = Crawler[1].pos[0].y = LEVEL_H / 2;
    LevelMap[SCREEN_W / 3][LEVEL_H / 2] = I_BLACK;
    LevelMap[SCREEN_W * 2 / 3][LEVEL_H / 2] = I_WHITE;
  }
}

/* read a binary format level file and center it onto play area */
static int read_bin(FILE *fp)
{
  int x = 0, width, height, map_offset = 0;
  long level_offset = 0;

  /* read the level map size */
  fscanf(fp, "%dx%d\n", &width, &height);
  fflush(fp);

  /* skip from start and set width to read */
  if(width < SCREEN_W)
  {
    x = (SCREEN_W - width) / 2;
    width += x;
  }
  if(width > SCREEN_W)
  {
    fseek(fp, (long)(width - SCREEN_W) / 2 * height, SEEK_CUR);
    width = SCREEN_W;
  }
  /* set reading offsets */
  if(height < LEVEL_H)
    map_offset = (LEVEL_H - height) / 2;
  if(height > LEVEL_H)
  {
    level_offset = (height - LEVEL_H);
    fseek(fp, level_offset / 2, SEEK_CUR);
    height = LEVEL_H;
  }

  for(; x < width; x++)
  {
    /* read a line of the level map */
    fread(&LevelMap[x][map_offset], (size_t)1, (size_t)height, fp);
    if(level_offset)
      fseek(fp, level_offset, SEEK_CUR);
  }
  return TRUE;
}

/* read an ascii format level file, centered horizontally */
static int read_ascii(FILE *fp)
{
#define CONVERT	16
/* level info: characters converted to:
 * bg, wall, exit, door, key, leaf, shit, flash, bang,
 * up. down. left, right, head, black. white
 */
  char convert[CONVERT + 2];
  char line[SCREEN_W * 2 + 2];
  int offset, len, x, y, i;

  /* read level map configuration */
  fgets(convert, CONVERT + 2, fp);
  if(strlen(convert) < CONVERT)
    strcpy(convert, ".#^%=$?|&UDRL@<>");

  y = 0;
  /* convert map line by line */
  while(y < LEVEL_H && fgets(line, SCREEN_W * 2 + 2, fp))
  {
    /* discount newline (LF or CR) */
    len = strlen(line) - 1;
    offset = x = 0;
    if(len < SCREEN_W)
    {
      x = (SCREEN_W - len) / 2;
      len += x;
    }
    else if(len > SCREEN_W)
    {
      offset = (len - SCREEN_W) / 2;
      len = SCREEN_W;
    }

    for(; x < len; x++, offset++)
    {
      int found = FALSE;
      /* check if value has to be converted */
      for(i = 0; i < CONVERT; i++)
	if(line[offset] == convert[i])
	{
	  found = TRUE;
	  LevelMap[x][y] = i;
	  i = CONVERT;
	}
      /* no conversion -> ascii */
      if(!found)
	LevelMap[x][y] = line[offset];
    }
    y++;
  }
  return TRUE;
#undef CONVERT
}

int write_map(const char *level_file, unsigned const char *map, Wyrms wyrm[])
{
  FILE *fp;
  char startdir[3];
  int i;

  if((fp = fopen(level_file, "wb")))
  {
    fputs("WYRMS-B\n", fp);		/* save format id */
    /* initial wyrm headings */
    startdir[0] = startdir[1] = 'D';
    for(i = 0; i < 4; i++)
    {
      if(wyrm[0].dir == DirConv[i][1])
        startdir[0] = DirConv[i][0];
      if(wyrm[1].dir == DirConv[i][1])
        startdir[1] = DirConv[i][0];
    }
    startdir[2] = '\n';
    fputs(startdir, fp);
    fprintf(fp, "%dx%d\n", SCREEN_W, LEVEL_H);
    /* score line isn't needed */
    for(i = 0; i < SCREEN_W; i++)
    {
      fwrite(map, 1, (size_t)LEVEL_H, fp);
      map += SCREEN_H;
    }
    fclose(fp);
    return TRUE;
  }
  return FALSE;
}

