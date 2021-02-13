/* W sample editor/player (w) 4/96 by Eero Tamminen
 *
 * This file implements sample synthetizing.
 *
 */

#include <stdlib.h>
#include "sampler.h"

typedef enum { sin_t, saw_t, noise_t } wave_t;

#define HARMONS	8
#define ENVELOS	4
#define WAVES	4

static struct
{
  int base;
  wave_t type;
  int harmonies[HARMONS];
  int envelope[ENVELOS]; 
} Wave[WAVES];

static int Widx;		/* which wave */

static widget_t
  *Synth,			/* the dialog */
  *Base,			/* new sample base frequency */
  *Lenght,			/* new sample lenght */
  *Name;			/* new sample name */


/* function prototypes */
static void get_values(int idx);
static void synthetize(sample_t *data, size_t len);
static void create_cb(widget_t *w, int pressed);
static void draw_fn(widget_t *w, int x, int y, int wd, int ht);
void synth_cb(widget_t *w, int pressed);


static void get_values(int idx)
{
  char *string;

  wt_getopt(Base, WT_STRING_ADDRESS, &string, WT_EOL);
  Wave[idx].base = atoi(string);
}

static void synthetize(sample_t *data, size_t len)
{
  message("Not yet implemented.");  
}

static void create_cb(widget_t *w, int pressed)
{
  SampleItem *spl;
  size_t lenght;
  char *string;

  if(!pressed)
  {
    wt_getopt(Lenght, WT_STRING_ADDRESS, &string, WT_EOL);
    lenght = atoi(string);
    if(!lenght)
    {
      message("Give sample a size, please!");
      return;
    }

    wt_getopt(Name, WT_STRING_ADDRESS, &string, WT_EOL);
    if(!string || string[0] == '\0')
    {
      message("Name the new sample, please!");
      return;
    }

    if((spl = create_sample(lenght)))
    {
      if((spl->name = malloc(strlen(string) + 1)))
	strcpy(spl->name, string);

      synthetize(spl->data, lenght);
      Sample = spl;
      update_list();
      show_sample();
    }
    else
      message("Not enough memory to create the sample!");
  }
}

static void draw_fn(widget_t *w, int x, int y, int wd, int ht)
{
  w_box(wt_widget2win(w), x, y, wd, ht);
}

void synth_cb(widget_t *w, int pressed)
{
  widget_t *pane, *hpane[5], *waves, *wave[WAVES], *types, *type[4],
	*name, *lenght, *create, *harmon, *base, *bars;

  char tmp[2] = "0";
  long a, b;
  int i;

  if(pressed)
    return;

  if(Synth)
  {
    /* already created... */
    wt_open(Synth);
    return;
  }

  /* create objects */

  Synth = wt_create(wt_shell_class, Top);
  pane = wt_create(wt_pane_class, Synth);

  hpane[0] = wt_create(wt_pane_class, pane);
  waves = wt_create(wt_label_class, hpane[0]);
  for(i = 0; i < WAVES; i++)
    wave[i] = wt_create(wt_radiobutton_class, hpane[0]);

  harmon = wt_create(wt_label_class, pane);
  bars   = wt_create(wt_drawable_class, pane);
  hpane[1] = wt_create(wt_pane_class, pane);
  base   = wt_create(wt_label_class, hpane[1]);
  Base   = wt_create(wt_getstring_class, hpane[1]);

  types  = wt_create(wt_label_class, pane);
  hpane[2] = wt_create(wt_pane_class, pane);
  for(i = 0; i < 4; i++)
    type[i] = wt_create(wt_radiobutton_class, hpane[2]);

  hpane[3] = wt_create(wt_pane_class, pane);
  name = wt_create(wt_label_class, hpane[3]);
  Name = wt_create(wt_getstring_class, hpane[3]);

  hpane[4] = wt_create(wt_pane_class, pane);
  lenght = wt_create(wt_label_class, hpane[4]);
  Lenght = wt_create(wt_getstring_class, hpane[4]);
  create = wt_create(wt_button_class, hpane[4]);

  if(!(Name && Lenght && Base))
  {
    wt_delete(Synth);
    Synth = NULL;
    return;
  }

  /* container options */

  wt_setopt(Synth,
	WT_LABEL, " Sample Synthetizing ",
	WT_ACTION_CB, wt_close,
	WT_EOL);

  a = AlignLeft;
  wt_setopt(pane, WT_ALIGNMENT, &a, WT_EOL);

  b = 4;
  wt_setopt(pane,
	WT_VDIST, &b,
	WT_EOL);

  a = 6;
  b = OrientHorz;
  for(i = 0; i < 5; i++)
  {
    wt_setopt(hpane[i],
	WT_ORIENTATION, &b,
	WT_HDIST, &a,
	WT_EOL);
  }

  /* which wave component */

  wt_setopt(waves, WT_LABEL, "Component:", WT_EOL);

  for(i = 0; i < WAVES; i++)
  {
    tmp[0] = '1' + i;
    wt_setopt(wave[i], WT_LABEL, tmp, WT_EOL);
  }
  a = ButtonStatePressed;
  wt_setopt(wave[0], WT_STATE, &a, WT_EOL);
  Widx = 0;


  /* wave paramenters */

  wt_setopt(harmon, WT_LABEL, "Harmonies:", WT_EOL);
  wt_setopt(base,   WT_LABEL, "Base frequency:", WT_EOL);

  a = 4;
  wt_setopt(Base,
	WT_STRING_LENGTH, &a,
	WT_STRING_MASK, "0-9",
	WT_EOL);

  a = 200;
  b = 68;
  wt_setopt(bars,
	WT_WIDTH, &a,
	WT_HEIGHT, &b,
	WT_DRAW_FN, draw_fn,
	WT_EOL);


  wt_setopt(types, WT_LABEL, "Wave type:", WT_EOL);

  wt_setopt(type[0], WT_LABEL, "Sin", WT_EOL);
  wt_setopt(type[1], WT_LABEL, "Saw", WT_EOL);
  wt_setopt(type[2], WT_LABEL, "Pulse", WT_EOL);
  wt_setopt(type[3], WT_LABEL, "Noise", WT_EOL);

  a = ButtonStatePressed;
  wt_setopt(type[0], WT_STATE, &a, WT_EOL);


  /* whole sample stuff */

  wt_setopt(name,   WT_LABEL, "Name:", WT_EOL);
  wt_setopt(lenght, WT_LABEL, "Lenght:", WT_EOL);

  a = 26;
  wt_setopt(Name, WT_STRING_LENGTH, &a, WT_EOL);
  a = 6;
  wt_setopt(Lenght,
	WT_STRING_LENGTH, &a,
	WT_STRING_MASK, "0-9",
	WT_EOL);

  wt_setopt(create,
	WT_LABEL, "Synthetize",
	WT_ACTION_CB, create_cb,
	WT_EOL);

  wt_realize(Top);
}

void synth_exit(void)
{
  if(Synth)
  {
    wt_delete(Synth);
    Synth = NULL;
  }
}

