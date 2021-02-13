/*
 * WarZone game for W Window System and W Toolkit
 *
 * The game is composed of several files:
 * 1. hill.c has the startup code / dialog.
 * 2. range.c takes care of maintaining and drawing onto WarZone.
 * 3. player.c takes care of the player action delegation and checks.
 * 4. input.c shows bunker status and inputs player values.
 * 5. shot.c implements different shot types.
 * 6. shield.c implements various shield types.
 * 7. lists.c manages player shot and shield lists.
 * 8. optional sound effect stuff.
 *
 * On C++ each file would probably be converted into separate class.
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 */

#include <time.h>
#include <unistd.h>		/* getpid() */
#include <string.h>
#include <stdio.h>
#include "hill.h"		/* global prototypes */


widget_t *Top;

static widget_t
  *Hill,
  *Rounds,
  **Name;

static int
  Players,
  Round;


/* local function prototypes */
static void quit_cb(widget_t *w, int pressed);
static void play_cb(widget_t *w, int pressed);


int main(int argc, char *argv[])
{
  widget_t *vpane, *info, *hpane, *round, *play, *quit;
  char buf[] = "player-0";
  int idx;
  long a;

  /* initialization */

  if(argc < 2)
    Players = 2;
  else
  {
    Players = atoi(argv[1]);
    if(argc != 2 || Players < 2 || Players > MAX_PLAYERS)
    {
      fprintf(stderr, "\nKing of the Hill " HILL_VERSION " by Eero Tamminen\n\n");
      fprintf(stderr, "Usage: hill <number of players (2-%d)>\n", MAX_PLAYERS);
      return -1;
    }
  }

  if(!(Name = (widget_t**)malloc(Players * sizeof(widget_t*))))
  {
    fprintf(stderr, "main: not enough memory\n");
    return -1;
  }

  if(!(range_alloc() && players_alloc(Players)))
    return -1;

  /* seed random generator */
  srand(time(0));


  /* user interface */

  Top = wt_init();

  Hill = wt_create(wt_shell_class, Top);
  wt_setopt(Hill, WT_LABEL, " WarZone " HILL_VERSION " ", WT_EOL);

  vpane = wt_create(wt_pane_class, Hill);
  info = wt_create(wt_label_class, vpane);
  wt_setopt(info, WT_LABEL, "Players:", WT_EOL);

  a = 32;
  for(idx = 0; idx < Players; idx++)
  {
    Name[idx] = wt_create(wt_getstring_class, vpane);
    buf[strlen(buf)-1] = '1' + idx;
    wt_setopt(Name[idx],
      WT_STRING_LENGTH, &a,
      WT_STRING_ADDRESS, buf,
      WT_EOL);
  }

  a = OrientHorz;
  hpane = wt_create(wt_pane_class, vpane);
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);

  round = wt_create(wt_label_class, hpane);
  wt_setopt(round, WT_LABEL, "Rounds:", WT_EOL);

  Rounds = wt_create(wt_getstring_class, hpane);
  a = 1;
  wt_setopt(Rounds,
    WT_STRING_LENGTH, &a,
    WT_STRING_ADDRESS, "3",
    WT_STRING_MASK, "1-9",
    WT_EOL);

  play = wt_create(wt_button_class, hpane);
  quit = wt_create(wt_button_class, hpane);
  wt_setopt(play, WT_LABEL, "Play", WT_ACTION_CB, play_cb, WT_EOL);
  wt_setopt(quit, WT_LABEL, "Quit", WT_ACTION_CB, quit_cb, WT_EOL);


  /* process */
  wt_realize(Top);
  wt_run();

  /* exiting... */
  wt_delete(Top);
  players_free();
  range_free();
  lists_free();
  free(Name);
  return 0;
}

static void quit_cb(widget_t *w, int pressed)
{
  if(!pressed)
    wt_break(1);
}

static void play_cb(widget_t *w, int pressed)
{
  char *name;
  int i;

  if(pressed)
    return;

  players_score(0);

  for(i = 0; i < Players; i++)
  {
    /* set player names */
    wt_getopt(Name[i], WT_STRING_ADDRESS, &name, WT_EOL);
      player_name(i, name);
  }
  wt_getopt(Rounds, WT_STRING_ADDRESS, &name, WT_EOL);
  Round = atoi(name);
  if(!Round)
    return;

  wt_close(Hill);
  market_open();
}


void round_start(void)
{
  range_open();		/* calculate new terrain and how range on screen */
  players_init();	/* initialize / position players to the new range */

  /* select starting player: 1) at random or with 2) best, 3) worst score */
  player_select(RND(Players));
}

void round_over(void)
{
  input_close();
  range_close();

  if(--Round > 0)
    market_open();
  else
    wt_open(Hill);
}
