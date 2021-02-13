/*
 * WarZone game for W Window System and W Toolkit
 *
 * Player input interaction / bunker control dialog:
 * - shows current player stats.
 * - gets shooting information from user.
 * - lets user select shied.
 *
 *	(w) 1996 by Eero Tamminen, puujalka@modeemi.cs.tut.fi
 *
 */

#include <math.h>		/* trig. functions */
#include "hill.h"		/* global prototypes */
#include "sound.h"
#include "dial.h"

#define BUF_SIZE	64
static char
  Buf[BUF_SIZE+1];	/* text output scratch buffer */

static short
  OffsetX,
  OffsetY;

static BUNKER
  *Input;

static widget_t			/* parameter output label widget */
  *Shell,
  *Wind,
  *Dial,
  *Power,
  *Bombs,
  *Score,
  *Energy,
  *Shields;


static void stype_cb(widget_t *w, const char *name, int idx)
{
  Input->s_idx = idx;
}

static void btype_cb(widget_t *w, const char *name, int idx)
{
  Input->b_idx = idx;
}

static void shoot_cb(widget_t *w)
{
  Ammo *shot;
  char *value;
  float valf;

  input_close();

  wt_getopt(Power, WT_VALUE, &value, WT_EOL);
  Input->power = atoi(value);

  wt_getopt(Dial, WT_VALUE, &value, WT_EOL);
  sscanf(value, "%f", &valf);
  Input->angle = valf * M_PI / 180.0;

  if(Input->shield)
  {
    Input->shielder = list_getshield(Input->index, Input->s_idx);
    Input->type = shield_type(Input->shielder);
  }

  shot = shot_alloc(Input);
  do_sound(SND_SHOOT);

  /* start processing the shot on the 'background' */
  player_shoot(shot);
}

static void ok_cb(widget_t *w, int pressed)
{
  if(pressed)
    return;

  shoot_cb(w);
}

void input_init(void)
{
  static const char *tmp_l[] = { "           ", NULL };
  widget_t *pane, *angle, *power, *shield, *bomb, *ok;
  long a, b;

  if(Shell)
  {
    wt_open(Shell);
    return;
  }

  Shell   = wt_create(wt_shell_class, Top);
  pane    = wt_create(wt_pane_class, Shell);
  Wind    = wt_create(wt_label_class, pane);
  angle   = wt_create(wt_label_class, pane);
  Dial    = wt_create(wt_dial_class, pane);
  power   = wt_create(wt_label_class, pane);
  Power   = wt_create(wt_range_class, pane);
  bomb    = wt_create(wt_label_class, pane);
  Bombs   = wt_create(wt_listbox_class, pane);
  shield  = wt_create(wt_label_class, pane);
  Shields = wt_create(wt_listbox_class, pane);
  Energy  = wt_create(wt_label_class, pane);
  Score   = wt_create(wt_label_class, pane);
  ok      = wt_create(wt_button_class, pane);

  wt_setopt(Shell, WT_ACTION_CB, shoot_cb, WT_EOL);

  a = AlignFill;
  wt_setopt(pane, WT_ALIGNMENT, &a, WT_EOL);

  wt_setopt(Wind, WT_LABEL, "Wind: -00.0m/s", WT_EOL);

  wt_setopt(angle, WT_LABEL, "Shot angle:", WT_EOL);
  wt_setopt(Dial,
    WT_VALUE_MIN, "0",
    WT_VALUE_MAX, "180",
    WT_EOL);

  wt_setopt(shield, WT_LABEL, "Shield type:", WT_EOL);
  wt_setopt(Shields,
    WT_LIST_ADDRESS, tmp_l,
    WT_ACTION_CB, stype_cb,
    WT_EOL);

  wt_setopt(bomb, WT_LABEL, "Shot type:", WT_EOL);
  wt_setopt(Bombs,
    WT_LIST_ADDRESS, tmp_l,
    WT_ACTION_CB, btype_cb,
    WT_EOL);

  wt_setopt(Energy, WT_LABEL, "Energy: 000/000", WT_EOL);

  wt_setopt(power, WT_LABEL, "Shot power:", WT_EOL);
  a = OrientHorz;
  b = MAX_VALUE;
  wt_setopt(Power,
    WT_ORIENTATION, &a,
    WT_VALUE_MIN, "0",
    WT_VALUE_MAX, MAX_VALUE_STR,
    WT_VALUE_STEPS, &b,
    WT_EOL);

  wt_setopt(Score, WT_LABEL, "Score: 00000", WT_EOL);

  wt_setopt(ok, WT_LABEL, "OK", WT_ACTION_CB, ok_cb, WT_EOL);
}


void input_close(void)
{
  w_querymousepos(wt_widget2win(Shell), &OffsetX, &OffsetY);
  wt_close(Shell);
}

void input_open(BUNKER *input)
{
  short x, y;
  long a, b;

  w_flush();
  if(!Shell)
  {
    input_init();
    wt_realize(Top);
  }

  wt_setopt(Shell, WT_LABEL, input->name, WT_EOL);

  sprintf(Buf, "Wind: %.1fm/s", input->wind / WIND_UNIT);
  wt_setopt(Wind, WT_LABEL, Buf, WT_EOL);


  sprintf(Buf, "%d", (int)(input->angle * 180.0 / M_PI));
  wt_setopt(Dial, WT_VALUE, Buf, WT_EOL);


  sprintf(Buf, "Energy: %d/%d", input->shield, input->energy);
  wt_setopt(Energy, WT_LABEL, Buf, WT_EOL);

  if(input->power > input->energy)
  {
    input->power = input->energy;
    sprintf(Buf, "%d", input->power);
    wt_setopt(Power, WT_VALUE_MAX, Buf, WT_EOL);
  }
  sprintf(Buf, "%d", input->power);
  wt_setopt(Power, WT_VALUE, Buf, WT_EOL);


  input->shields = list_shields(input->index);
  input->bombs = list_bombs(input->index);

  a = input->s_idx;
  wt_setopt(Shields,
    WT_LIST_ADDRESS, input->shields,
    WT_CURSOR, &a,
    WT_EOL);

  a = input->b_idx;
  wt_setopt(Bombs,
    WT_LIST_ADDRESS, input->bombs,
    WT_CURSOR, &a,
    WT_EOL);


  sprintf(Buf, "Score: %d", input->score);
  wt_setopt(Score, WT_LABEL, Buf, WT_EOL);
  

#define BORDER	8

  /* same mouse-relative position when closed */
  w_querymousepos(WROOT, &x, &y);
  x -= OffsetX;  y -= OffsetY;

  /* but still fully on the screen */
  if(x + Shell->w > WROOT->width - BORDER)
    x = WROOT->width - Shell->w - BORDER;
  if(y + Shell->h > WROOT->height - BORDER)
    y = WROOT->height - Shell->h - BORDER;
  if(x + Shell->w < BORDER)
    x = BORDER;
  if(y + Shell->h < BORDER)
    y = BORDER;

  a = x; b = y;
  wt_setopt(Shell, WT_XPOS, &a, WT_YPOS, &b, WT_EOL);  
  wt_open(Shell);

  Input = input;
}
