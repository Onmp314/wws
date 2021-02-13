/* W sample editor/player (w) 4/96 by Eero Tamminen
 *
 * This file implements selection and playing speed finetuning.
 *
 * For looping the selection finetuning really should use close-ups
 * of the selection endpoints...
 */

#include "sampler.h"

widget_t *Tune;

static void freq_cb(widget_t *w, char *text)
{
  long freq = atoi(text);
  if(freq > 4000 && freq < 50000)
  {
    change_rate(freq - Sample->rate);
  }
}

static void from_cb(widget_t *w, char *text)
{
  sample_t *from = Sample->data + atoi(text);
  if(from <= Sample->show + Sample->show_len)
  {
    Sample->show_len += from - Sample->show;
    Sample->show = from;
    show_info();
  }
}

static void to_cb(widget_t *w, char *text)
{
  sample_t to = atoi(text);
  if(to <= Sample->lenght && to > Sample->show - Sample->data)
  {
    Sample->show_len = Sample->data - Sample->show + to;
    show_info();
  }
}

void finetune_cb(widget_t *w, int pressed)
{
  widget_t *pane, *hpane1, *spl, *freq, *herz,
    *hpane2, *sel, *from, *mid, *to;
  long a;

  if(pressed || !Sample)
    return;

  if(Tune)
  {
    wt_open(Tune);
    return;
  }

  Tune   = wt_create(wt_shell_class, Top);
  pane   = wt_create(wt_pane_class, Tune);

  spl    = wt_create(wt_label_class, pane);
  hpane1 = wt_create(wt_pane_class, pane);
  freq   = wt_create(wt_getstring_class, hpane1);
  herz   = wt_create(wt_label_class, hpane1);

  sel    = wt_create(wt_label_class, pane);
  hpane2 = wt_create(wt_pane_class, pane);
  from   = wt_create(wt_getstring_class, hpane2);
  mid    = wt_create(wt_label_class, hpane2);
  to     = wt_create(wt_getstring_class, hpane2);

  if(!(freq && from && to))
  {
    wt_delete(Tune);
    Tune = NULL;
    return;
  }

  wt_setopt(Tune, WT_LABEL, " Finetuning ", WT_ACTION_CB, wt_close, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane1, WT_ORIENTATION, &a, WT_EOL);
  wt_setopt(hpane2, WT_ORIENTATION, &a, WT_EOL);

  a = 5;
  wt_setopt(spl, WT_LABEL, "Frequency:", WT_EOL);
  wt_setopt(freq,
	WT_STRING_LENGTH, &a,
	WT_STRING_MASK, "0-9",
	WT_ACTION_CB, freq_cb,
	WT_EOL);
  wt_setopt(herz, WT_LABEL, "Hz", WT_EOL);

  a = 6;
  wt_setopt(sel, WT_LABEL, "Selection:", WT_EOL);
  wt_setopt(from,
	WT_STRING_LENGTH, &a,
	WT_STRING_MASK, "0-9",
	WT_ACTION_CB, from_cb,
	WT_EOL);
  wt_setopt(mid, WT_LABEL, "to", WT_EOL);
  wt_setopt(to,
	WT_STRING_LENGTH, &a,
	WT_STRING_MASK, "0-9",
	WT_ACTION_CB, to_cb,
	WT_EOL);

  wt_realize(Top);
}

void tune_exit(void)
{
  if(Tune)
  {
    wt_delete(Tune);
    Tune = NULL;
  }
}
