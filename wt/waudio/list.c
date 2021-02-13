/* W sample editor/player (w) 4/96 by Eero Tamminen
 *
 * This file implements sample loading/saving/selecting/deleting.
 *
 * Limitations:
 * - Handles only raw 8-bit, signed, mono samples.  ATM you can use 'sox'
 *   to convert other formats.  There's a 'process_type()' function where
 *   you may add sample type processing, but the types really need an ID
 *   (cookie).
 *
 *	- Eero
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "sampler.h"

static struct
{
  int index;		/* index of the current sample in the shown list */
  int samples;		/* number of loaded samples */
  int list_space;	/* for how many names space allocated */
  const char **sample_list;	/* sample names for listbox */
} Table = { 0, 0, 0, NULL };

static widget_t
  *List,		/* sample list dialog */
  *Menu;		/* sample list */

/* sample list */
void list_cb(widget_t *w, int pressed);
static void load_cb(widget_t *w, int pressed);
static void save_cb(widget_t *w, int pressed);
static void remove_cb(widget_t *w, int pressed);
static void menu_cb(widget_t *w, const char *item, int index);
int  update_list(void);
void list_exit(void);

/* file and de/allocation functions */
SampleItem *create_sample(size_t size);
static SampleItem *sample_delete(SampleItem *spl);
static void save_sample(widget_t *w, const char *file);
static void process_type(long lenght);
void load_sample(widget_t *w, const char *file);

/* 
 * list widget / menu creation stuff
 */

void list_cb(widget_t *w, int pressed)
{
  widget_t *pane, *hpane, *new, *del, *load, *save;
  long a, b;

  if(pressed)
    return;

  if(List)
  {
    /* already created... */
    wt_open(List);
    return;
  }

  List  = wt_create(wt_shell_class, Top);
  pane  = wt_create(wt_pane_class, List);
  Menu  = wt_create(wt_listbox_class, pane);
  hpane = wt_create(wt_pane_class, pane);
  new   = wt_create(wt_button_class, hpane); 
  load  = wt_create(wt_button_class, hpane);
  save  = wt_create(wt_button_class, hpane);
  del   = wt_create(wt_button_class, hpane);

  if(!(Menu && load && save && del))
  {
    wt_delete(List);
    List = NULL;
    return;
  }

  wt_setopt(List, WT_LABEL, " Samples ", WT_ACTION_CB, wt_close, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);

  wt_setopt(new, WT_LABEL, "Synthetize...", WT_ACTION_CB, synth_cb, WT_EOL);
  wt_setopt(load, WT_LABEL, "Load", WT_ACTION_CB, load_cb, WT_EOL);
  wt_setopt(save, WT_LABEL, "Save", WT_ACTION_CB, save_cb, WT_EOL);
  wt_setopt(del,  WT_LABEL, "Delete", WT_ACTION_CB, remove_cb, WT_EOL);

  /* ATM no autolocator */
  a = 8;
  b = 32;
  wt_setopt(Menu,
	WT_LIST_HEIGHT, &a,
	WT_LIST_WIDTH, &b,
	WT_ACTION_CB, menu_cb,
	WT_EOL);

  if(!update_list())
    return;

  wt_realize(Top);
}

void list_exit(void)
{
  if(List)
  {
    wt_delete(List);
    List = NULL;
  }
  if(Table.sample_list)
  {
    free(Table.sample_list);
    Table.sample_list = NULL;
  }
  while(Sample)
    sample_delete(Sample);
}

/* 
 * Sample list callbacks
 */

/* called after loading / removing a sample (if there still is a sample) */
int update_list(void)
{
  SampleItem *current;
  long i;

  if(!Menu)
    return 0;

  /* need more space for name pointers? */
  if(Table.list_space <= Table.samples)
  {
    if(Table.list_space)
      free(Table.sample_list);
    if(!(Table.sample_list = (const char**)malloc(sizeof(char*) * (Table.samples+1))))
    {
      Table.list_space = 0;
      message("Not enough memory for sample list!");
      return 0;
    }
    Table.list_space = Table.samples+1;
  }

  current = Sample;
  /* fill name pointer list */
  for(i = 0; i < Table.samples; i++)
  {
    Table.sample_list[i] = current->name;
    current = current->next;
  }
  Table.sample_list[i] = NULL;

  i = 0;
  wt_setopt(Menu, WT_LIST_ADDRESS, Table.sample_list, WT_EOL);
  if(Table.samples)
    wt_setopt(Menu, WT_CURSOR, &i, WT_EOL);

  /* 'Sample' index in the 'Table' */
  Table.index = 0;
  return 1;
}

static void menu_cb(widget_t *w, const char *item, int index)
{
  index -= Table.index;
  Table.index += index;

  if(index > 0)
  {
    while(index--)
      Sample = Sample->next;
  }
  else
    while(index++)
      Sample = Sample->prev;

  /* show current sample, name, rate, lenght, view, selection */  
  Audio.rate = Sample->rate;
  show_sample();
}

static void do_fsel(const char *title, void (*cb)(widget_t *w, const char *file))
{
  widget_t *fsel;

  if(!(fsel = wt_create(wt_filesel_class, Top)))
    return;

  wt_setopt(fsel,
    WT_LABEL, title,
    WT_ACTION_CB, cb,
    WT_EOL);

  wt_realize(Top);
}

static void load_cb(widget_t *w, int pressed)
{
  if(!pressed)
    do_fsel(" Load a Sample... ", load_sample);
}

static void save_cb(widget_t *w, int pressed)
{
  if(!pressed)
    do_fsel(" Save Sample as... ", save_sample);
}

static void remove_cb(widget_t *w, int pressed)
{
  if(pressed || !Sample)
    return;

  sample_delete(Sample);
  update_list();

  if(!Sample)
  {
    if(Effect)
      wt_close(Effect);
    if(Tune)
      wt_close(Tune);
  }
  show_sample();
}

/* 
 * De/allocation / file functions
 */

static SampleItem *sample_delete(SampleItem *spl)
{
  SampleItem *prev;

  if(!spl)
    return NULL;

  /* free memory allocated for sample */
  if(spl->undo)
    free(spl->undo);
  if(spl->name)
    free(spl->name);
  if(spl->data)
  free(spl->data);

  /* last sample? */
  prev = spl->prev;
  if(prev == spl)
  {
    free(spl);
    Sample = spl = NULL;
  }
  else
  {
    /* relink and set variables */
    prev->next = spl->next;
    spl->next->prev = prev;
    Audio.rate = prev->rate;
    free(spl);
    if(Sample == spl)
      Sample = prev;
    spl = prev;
  }
  Table.samples--;

  return spl;
}

/* allocates a new sample structure and and re-links the structure pointers
 * return new sample at success and NULL at failure
 */
SampleItem *create_sample(size_t size)
{
  SampleItem *spl;

  if(!(spl = malloc(sizeof(SampleItem))))
  {
    message("Not enought memory for the sample!");
    return NULL;
  }
  memset(spl, 0, sizeof(SampleItem));
  if(Sample)
  {
    spl->prev = Sample;
    spl->next = Sample->next;
    Sample->next->prev = spl;
    Sample->next = spl;
  }
  else
    Sample = spl->prev = spl->next = spl;

  size += (size + sizeof(sample_t) - 1) % sizeof(sample_t);
  if(size > 0)
  {
    spl->data = malloc(size);
  }
  else
  {
    /* got to be a valid address */
    spl->data = malloc(sizeof(sample_t));
    size = sizeof(sample_t);
    spl->data[0] = 0;
  }

  Table.samples++;
  if(!spl->data)
  {
    message("Not enought memory for the sample!");
    sample_delete(spl);
    return NULL;
  }

  /* sample as raw */
  size /= sizeof(sample_t);
  spl->mem_len  = size;
  spl->lenght   = size;
  spl->show_len = size;
  spl->show     = spl->data;
  spl->rate     = Audio.rate;
  spl->timer    = -1;

  return spl;
}

static void save_sample(widget_t *w, const char *file)
{
  FILE *fp;

  if(!file)
    goto fsel_off;

  if(!Sample)
  {
    message("No sample to save!");
    goto fsel_off;
  }

  if((fp = fopen(file, "wb")))
  {
    if(fwrite(Sample->data, 1, Sample->lenght, fp) == Sample->lenght)
    {
      fclose(fp);
      goto fsel_off;
    }
    message("File write failed.");
    fclose(fp);
  }
  else
    message("Unable to open file.");
  return;

fsel_off:
  wt_delete(w);
}

/* process different sample formats (default raw) and set variables */
static void process_type(long size)
{
  /* check for sample IDs...
   * and convert 16-bit/8-bit, decompress etc
   */
}

/* select, allocate load and display sample */
void load_sample(widget_t *w, const char *file)
{
  SampleItem *spl;
  struct stat fst;
  FILE *fp;

  if(!file)
  {
    wt_delete(w);
    return;
  }

  if(stat(file, &fst) < 0)
  {
    message("Unable to stat sample!");
    return;
  }

  if(!(spl = create_sample(fst.st_size)))
  {
    message("Not enough memory to load the sample!");
    return;
  }

  /* get sample */
  if((fp = fopen(file, "rb")))
  {
    if(fread(spl->data, 1, fst.st_size, fp) == fst.st_size)
    {
      int idx;

      /* here's sample type processing and some variable settings */
      process_type(fst.st_size);

      /* strip path of the filename */
      idx = strlen(file);
      while(idx-- > 0 && file[idx] != '/');
      file = &file[++idx];
      if((spl->name = malloc(strlen(file) + 1)))
	strcpy(spl->name, file);

      update_list();
      if(spl == Sample)		/* first one? */
        show_sample();
    }
    else
    {
      sample_delete(spl);
      message("Sample loading failed!");
    }
    fclose(fp);
  }
  else
  {
    spl->name = NULL;
    sample_delete(spl);
    message("unable to open sample!");
  }
}
