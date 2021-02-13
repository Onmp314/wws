/* W sample editor/player (w) 4/96 by Eero Tamminen
 *
 * This file implements copy/cut/paste buffer, different effects and undo.
 */

#include "sampler.h"


widget_t *Effect;		/* sample effect dialog */

/* Cut & Paste buffer */
static struct
{
  long lenght;
  sample_t *data;		/* zero if nothing to paste */
} Buffer = { 0L, NULL };

/* ------------------------------------
 * Function prototypes.
 */

/* effects */
void effects_cb(widget_t *w, int pressed);
static void undo_cb(widget_t *w, int pressed);
static void copy_cb(widget_t *w, int pressed);
static void cut_cb(widget_t *w, int pressed);
static void paste_cb(widget_t *w, int pressed);
static void sign_cb(widget_t *w, int pressed);
static void reverse_cb(widget_t *w, int pressed);
static void fadein_cb(widget_t *w, int pressed);
static void fadeout_cb(widget_t *w, int pressed);

/* auxciliary functions */
static void save_undo(sample_t *begin, sample_t *end);
static void save_buffer(sample_t *begin, sample_t *end);
static void get_area(sample_t **begin, sample_t **end);

/* 
 * Effects callbacks
 *
 * These can trust there always being sample data, because effects
 * dialog isn't available when there are no samples.
 */

static void undo_cb(widget_t *w, int pressed)
{
  long old_len;

  if(Sample->undo)
  {
    memcpy(&Sample->data[Sample->undo_pos],
      Sample->undo, Sample->undo_len * sizeof(sample_t));

    old_len = Sample->lenght;
    if(Sample->lenght < Sample->undo_pos + Sample->undo_len)
      Sample->lenght = Sample->undo_pos + Sample->undo_len;

    if(Sample->show_len == old_len)		/* whole sample shown? */
      Sample->show_len = Sample->lenght;

    Sample->select = &Sample->data[Sample->undo_pos + Sample->undo_len];
    Sample->win_x1 = Sample->win_x2 = spl2win(Sample->select);
    Sample->sel_len = 0;
    free(Sample->undo);
    Sample->undo = NULL;
    draw_sample();
    show_info();
  }
}

/* save the selected area into the undo buffer */
static void save_undo(sample_t *beg, sample_t *end)
{
  int lenght = end - beg;

  if(beg == end)
    return;

  if(!Sample->undo || lenght > Sample->undo_len)
  {
    if(Sample->undo)
      free(Sample->undo);
    if(!(Sample->undo = malloc(lenght * sizeof(sample_t))))
    {
      message("Undo unavailable (not enough memory)!");
      return;
    }
  }
  memcpy(Sample->undo, beg, lenght * sizeof(sample_t));
  Sample->undo_pos = beg - Sample->data;
  Sample->undo_len = lenght;
}

/* save the selected area to buffer for pasting */
static void save_buffer(sample_t *beg, sample_t *end)
{
  int lenght = end - beg;

  if(beg >= end)
  {
    free(Buffer.data);
    Buffer.data = NULL;
    return;
  }

  if(!Buffer.data || lenght > Buffer.lenght)
  {
    if(Buffer.data)
      free(Buffer.data);
    if(!(Buffer.data = malloc(lenght * sizeof(sample_t))))
    {
      message("Cut & Paste unavailable (not enough memory)!");
      return;
    }
  }
  memcpy(Buffer.data, beg, lenght * sizeof(sample_t));
  Buffer.lenght = lenght;
}

static void get_area(sample_t **beg, sample_t **end)
{
  /* here could also be shmem / semafore locking for samples that
   * are being edited by childs (eg. because operation takes too
   * long). Afterwards the main program would be informed about
   * the editing by pipe etc.
   */

  if(Sample->select)
  {
    *beg = Sample->select;
    *end = Sample->select + Sample->sel_len;
  }
  else
  {
    *beg = Sample->show;
    *end = Sample->show + Sample->show_len;
  }
}

static void copy_cb(widget_t *w, int pressed)
{
  sample_t *beg, *end;

  if(!pressed)
  {
    get_area(&beg, &end);
    save_buffer(beg, end);
    invert_selection();
    Sample->select = end;
    Sample->win_x1 = Sample->win_x2 = spl2win(end);
    Sample->sel_len = 0;
  }
}

static void cut_cb(widget_t *w, int pressed)
{
  sample_t *beg, *end;

  if(!pressed)
  {
    get_area(&beg, &end);
    save_buffer(beg, end);
    if(beg != end)
    {
      save_undo(beg, Sample->data + Sample->lenght);
      memmove(beg, end, (Sample->data + Sample->lenght - end) * sizeof(sample_t));
      Sample->lenght -= end - beg;

      if(Sample->show + Sample->show_len > Sample->data + Sample->lenght)
	Sample->show_len = Sample->data + Sample->lenght - Sample->show;

      Sample->select = beg;
      Sample->win_x1 = Sample->win_x2 = spl2win(beg);
      Sample->sel_len = 0;
      draw_sample();
      show_info();
    }
  }
}

/* paste undo area to cursor */
static void paste_cb(widget_t *w, int pressed)
{
  sample_t *beg, *end, *tmp;
  long dist;

  /* only to a selected place */
  if(Buffer.data && !pressed)
  {
    invert_selection();
    get_area(&beg, &end);

    if(Sample->lenght + Buffer.lenght > Sample->mem_len)
    {
      if(!(tmp = malloc((Sample->lenght + Buffer.lenght) * sizeof(sample_t))))
      {
	message("Not enought memory for paste!");
	return;
      }
      Sample->mem_len = Sample->lenght + Buffer.lenght;
      memcpy(tmp, Sample->data, Sample->lenght * sizeof(sample_t));

      /* new addresses */
      dist = tmp - Sample->data;
      Sample->show += dist;
      beg += dist;
      end += dist;

      free(Sample->data);
      Sample->data = tmp;
    }
    save_undo(beg, Sample->data + Sample->lenght);
    memmove(beg + Buffer.lenght, beg, (Sample->data + Sample->lenght - beg) * sizeof(sample_t));
    memcpy(beg, Buffer.data, Buffer.lenght * sizeof(sample_t));

    if(Sample->show_len == Sample->lenght)	/* whole sample? */
      Sample->show_len += Buffer.lenght;
    Sample->lenght += Buffer.lenght;

    Sample->select = beg + Buffer.lenght;
    Sample->win_x1 = Sample->win_x2 = spl2win(Sample->select);
    Sample->sel_len = 0;

    draw_sample();
    show_info();
  }
}

static void sign_cb(widget_t *w, int pressed)
{
  sample_t *data, *end;

  if(!pressed)
  {
    data = Sample->data;
    end  = Sample->data + Sample->lenght;
    /* signed <-> unsigned conversion */
    while(data < end)
    {
      *data ^= MAX_VOLUME;
      data++;
    }
    draw_sample();
  }
}

static void reverse_cb(widget_t *w, int pressed)
{
  sample_t *beg, *end, tmp;

  if(!pressed)
  {
    get_area(&beg, &end);
    save_undo(beg, end);

    while(beg < end)
    {
      tmp = *beg;
      *beg = *end;
      *end = tmp;
      beg++; end--;
    }
    draw_sample();
  }
}

static void fadein_cb(widget_t *w, int pressed)
{
  sample_t *beg, *end, *tmp;
  long len;

  if(!pressed)
  {
    get_area(&beg, &end);
    save_undo(beg, end);

    tmp = beg;
    len = end - beg;
    while(tmp < end)
    {
      *tmp = *tmp * (tmp - beg) / len;
      tmp++;
    }
    draw_sample();
  }
}

static void fadeout_cb(widget_t *w, int pressed)
{
  sample_t *beg, *end, *tmp;
  long len;

  if(!pressed)
  {
    get_area(&beg, &end);
    save_undo(beg, end);

    tmp = beg;
    len = end - beg;
    while(tmp < end)
    {
      *tmp = *tmp * (end - tmp) / len;
      tmp++;
    }
    draw_sample();
  }
}

void effects_cb(widget_t *w, int pressed)
{
  widget_t *hpane, *vpane1, *vpane2, *play, *undo,
    *cut, *copy, *paste, *mix, *sign, *reverse,
    *fadein, *fadeout, *special, *filters;
  long a;

  if(pressed || !Sample)
    return;

  if(Effect)
  {
    /* already created... */
    wt_open(Effect);
    return;
  }

  Effect = wt_create(wt_shell_class, Top);
  hpane  = wt_create(wt_pane_class, Effect);
  vpane1 = wt_create(wt_pane_class, hpane);
  vpane2 = wt_create(wt_pane_class, hpane);

  play  = wt_create(wt_button_class, vpane1);
  undo  = wt_create(wt_button_class, vpane1);
  cut   = wt_create(wt_button_class, vpane1);
  copy  = wt_create(wt_button_class, vpane1);
  paste = wt_create(wt_button_class, vpane1);
  mix   = wt_create(wt_button_class, vpane1);

  sign    = wt_create(wt_button_class, vpane2);
  reverse = wt_create(wt_button_class, vpane2);
  fadein  = wt_create(wt_button_class, vpane2);
  fadeout = wt_create(wt_button_class, vpane2);
  special = wt_create(wt_button_class, vpane2);
  filters = wt_create(wt_button_class, vpane2);

  if(!(play && copy && cut && paste &&
     undo &&reverse && fadein && fadeout))
  {
    wt_delete(Effect);
    Effect = NULL;
    return;
  }

  wt_setopt(Effect, WT_LABEL, " Effects ", WT_ACTION_CB, wt_close, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);

  a = AlignFill;
  wt_setopt(vpane1, WT_ALIGNMENT, &a, WT_EOL);
  wt_setopt(vpane2, WT_ALIGNMENT, &a, WT_EOL);

  wt_setopt(play,  WT_LABEL, "Play", WT_ACTION_CB, play_sample, WT_EOL);
  wt_setopt(undo,  WT_LABEL, "Undo", WT_ACTION_CB, undo_cb, WT_EOL);
  wt_setopt(cut,   WT_LABEL, "Swap", WT_ACTION_CB, cut_cb, WT_EOL);
  wt_setopt(copy,  WT_LABEL, "Copy", WT_ACTION_CB, copy_cb, WT_EOL);
  wt_setopt(paste, WT_LABEL, "Paste", WT_ACTION_CB, paste_cb, WT_EOL);
  wt_setopt(mix,   WT_LABEL, "Mix", WT_EOL);

  wt_setopt(sign,    WT_LABEL, "Sign", WT_ACTION_CB, sign_cb, WT_EOL);
  wt_setopt(reverse, WT_LABEL, "Reverse", WT_ACTION_CB, reverse_cb, WT_EOL);
  wt_setopt(fadein,  WT_LABEL, "Fade in", WT_ACTION_CB, fadein_cb, WT_EOL);
  wt_setopt(fadeout, WT_LABEL, "Fade out", WT_ACTION_CB, fadeout_cb, WT_EOL);
  wt_setopt(special, WT_LABEL, "Special...", WT_EOL);
  wt_setopt(filters, WT_LABEL, "filters...", WT_EOL);

  wt_realize(Top);
}

void effect_exit(void)
{
  if(Effect)
  {
    wt_delete(Effect);
    Effect = NULL;
  }
  if(Buffer.data)
  {
    free(Buffer.data);
    Buffer.data = NULL;
  }
}
