/* W sample editor/player (w) 4/96 by Eero Tamminen
 *
 * This file implements sample part selection/zooming/showing, sample
 * playing and invocing of other dialogs.
 *
 * Limitations:
 * - Handles only raw 8-bit, signed, mono samples.  ATM you can use 'sox'
 *   to convert other formats.  There's a 'process_type()' function where
 *   you may add sample type processing.
 * - Sample window drawing and mouse selection produce correct results only
 *   with samples which size is a couple (depends on window width) of MBs at
 *   maximum.
 * - To be on the safe side I'll sync audio after writing a sample, so don't
 *   panic if waudio pauses for a while.
 *
 *	- Eero
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "sampler.h"

#define WIN_NAME	" W Sampler v0.5 "

/* for drawing the sample */
#define DRAW_WIN_WIDTH	256
#define DRAW_WIN_HEIGHT	63
#define OFFSET		2

/* flags for audio output */
#define FLAG_SELECTION	1	/* play only selection (eg. in loop) */
#define FLAG_LOOP	2	/* loop while playing a sample */

AudioPack
  Audio;

SampleItem
  *Sample = NULL;		/* currently shown / editable sample */

widget_t
  *Top;				/* realizable */

static WWIN
  *Draw;			/* window into which sample is drawn */

static widget_t
  *Rate,			/* sample rate label */
  *Name,			/* sample name lable */
  *Info;			/* sample info label */


/* ------------------------------------
 * Function prototypes.
 */
/* auxciliary functions */
void show_info(void);
void draw_sample(void);
void show_sample(void);

/* GUI interface and callbacks */
static int  window_init(void);
static void exit_cb(widget_t *w);
static void up_cb(widget_t *w, int pressed);
static void down_cb(widget_t *w, int pressed);
static void selection_cb(widget_t *w, int pressed);
static void loop_cb(widget_t *w, int pressed);
static void zin_cb(widget_t *w, int pressed);
static void zout_cb(widget_t *w, int pressed);

/* selection with mouse */
static int  fit_x(int x);
static WEVENT * event_cb(widget_t *w, WEVENT *ev);
static void timeout_cb(long val);

/* audio device specific functions */
static int  audio_init(void);
void change_rate(long change);
void play_sample(widget_t *w, int pressed);
static void audio_exit(void);

void message(const char *msg) {
  wt_dialog(Top, msg, WT_DIAL_INFO, WIN_NAME, "Proceed", 0);
}

long spl2win(sample_t *addr) {
  return OFFSET + (addr - Sample->show) * (Draw->width - 2*OFFSET) / Sample->show_len;
}

void invert_selection(void) {
  if(Sample->select)
    w_pbox(Draw, Sample->win_x1, OFFSET, Sample->win_x2 - Sample->win_x1, Draw->height - 2*OFFSET);
}

/* 
 * Main loop
 */
int main(int argc, char *argv[])
{
  if(argc > 1 && argv[1][0] == '-')
  {
    fprintf(stderr, "usage: %s [sample [sample2...]]\n", *argv);
    return -1;
  }

  if(!(audio_init() && window_init()))
    return -1;

  while(--argc > 0)
    load_sample(NULL, argv[argc]);

  wt_realize(Top);
  wt_run();
  return 0;
}

/* 
 * Proram options dialog
 */

static void selection_cb(widget_t *w, int pressed)
{
  Audio.output ^= FLAG_SELECTION;
}

static void loop_cb(widget_t *w, int pressed)
{
  Audio.output ^= FLAG_LOOP;
}

static void options_cb(widget_t *w, int pressed)
{
  static widget_t *shell = 0;
  widget_t *pane1, *pane2, *label, *sel, *loop, *text1, *text2;
  long a;

  if(pressed)
    return;

  if(shell)
  {
    wt_open(shell);
    return;
  }

  shell = wt_create(wt_shell_class, Top);
  pane1 = wt_create(wt_pane_class, shell);
  label = wt_create(wt_label_class, pane1);
  pane2 = wt_create(wt_pane_class, pane1);
  sel   = wt_create(wt_checkbutton_class, pane2);
  loop  = wt_create(wt_checkbutton_class, pane2);
  text1 = wt_create(wt_label_class, pane1);
  text2 = wt_create(wt_label_class, pane1);

  if(!(label && sel && loop))
  {
    wt_delete(shell);
    shell = NULL;
    return;
  }

  wt_setopt(shell, WT_LABEL, " Options ", WT_ACTION_CB, wt_close, WT_EOL);
  wt_setopt(label, WT_LABEL, "Playing:", WT_EOL);

  a = AlignLeft;
  wt_setopt(pane2, WT_ALIGNMENT, &a, WT_EOL);
  wt_setopt(sel,   WT_LABEL, "Selected", WT_ACTION_CB, selection_cb, WT_EOL);
  wt_setopt(loop,  WT_LABEL, "Loop", WT_ACTION_CB, loop_cb, WT_EOL);

  wt_setopt(text1, WT_LABEL, "WSampler w/ 1996", WT_EOL);
  wt_setopt(text2, WT_LABEL, "by Eero Tamminen", WT_EOL);

  wt_realize(Top);
}

/* 
 * Main GUI callbacks
 */

static void exit_cb(widget_t *w)
{
  /* delete allocations */
  tune_exit();
  effect_exit();
  synth_exit();
  list_exit();
  audio_exit();
  wt_break(1);
}

static void down_cb(widget_t *w, int pressed)
{
  if(!pressed)
  {
    change_rate(-1024L);
    show_info();
  }
}

static void up_cb(widget_t *w, int pressed)
{
  if(!pressed)
  {
    change_rate(1024L);
    show_info();
  }
}

static void draw_cb(widget_t *w, int x, int y, int wd, int ht)
{
  WWIN *win = wt_widget2win(w);
  w_setmode(win, M_DRAW);
  w_box(win, x, y, wd, ht);
  Draw = win;
  draw_sample();
}

static void zin_cb(widget_t *w, int pressed)
{
  if(Sample && Sample->select && !pressed)
  {
    /* Zoom in */
    Sample->show = Sample->select;
    Sample->show_len = Sample->sel_len;
    Sample->select = 0;
    draw_sample();
    show_info();
  }  
}

static void zout_cb(widget_t *w, int pressed)
{
  if(Sample && !pressed)
  {
    /* Zoom whole sample */
    Sample->show = Sample->data;
    Sample->show_len = Sample->lenght;
    draw_sample();
    show_info();
  }  
}

/* 
 * GUI
 */
static int window_init(void)
{
  widget_t *shell, *hpane, *vpane1, *vpane2, *vpane3, *rpane,
    *play, *up, *down, *tune, *option, *list, *sample, *zin, *zout, *effect;
  long a, b, c;
  WFONT *font;

  Top = wt_init();
  shell  = wt_create(wt_shell_class, Top);
  hpane  = wt_create(wt_pane_class, shell);
  vpane1 = wt_create(wt_pane_class, hpane);
  vpane2 = wt_create(wt_pane_class, hpane);
  vpane3 = wt_create(wt_pane_class, hpane);

  play  = wt_create(wt_button_class, vpane1);
  rpane = wt_create(wt_pane_class, vpane1);
  down  = wt_create(wt_button_class, rpane);
  Rate  = wt_create(wt_label_class, rpane);
  up    = wt_create(wt_button_class, rpane);
  option= wt_create(wt_button_class, vpane1);
  tune  = wt_create(wt_button_class, vpane1);

  list  = wt_create(wt_button_class, vpane2);
  zin   = wt_create(wt_button_class, vpane2);
  zout  = wt_create(wt_button_class, vpane2);
  effect= wt_create(wt_button_class, vpane2);

  Name  = wt_create(wt_label_class, vpane3);
  sample= wt_create(wt_drawable_class, vpane3);
  Info  = wt_create(wt_label_class, vpane3);

  if(!(play && up && Rate && down && tune && option &&
       Name && sample && Info && list && zin && zout && effect))
    return 0;

  wt_setopt(shell, WT_LABEL, WIN_NAME, WT_ACTION_CB, exit_cb, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);
  wt_setopt(rpane, WT_ORIENTATION, &a, WT_EOL);

  a = AlignFill;
  wt_setopt(vpane1, WT_ALIGNMENT, &a, WT_EOL);
  wt_setopt(vpane2, WT_ALIGNMENT, &a, WT_EOL);
  wt_setopt(vpane3, WT_ALIGNMENT, &a, WT_EOL);

  wt_getopt(Rate, WT_FONT, &font, WT_EOL);
  a = w_strlen(font, "50kHz") + 4;
  wt_setopt(Rate, WT_WIDTH, &a, WT_EOL);

  a = LabelModeWithBorder;
  wt_setopt(Rate, WT_MODE, &a, WT_EOL);

  wt_setopt(play,  WT_LABEL, "Play", WT_ACTION_CB, play_sample, WT_EOL);
  wt_setopt(down,  WT_LABEL, "<", WT_ACTION_CB, down_cb, WT_EOL);
  wt_setopt(up,    WT_LABEL, ">", WT_ACTION_CB, up_cb, WT_EOL);
  wt_setopt(option,WT_LABEL, "Options...", WT_ACTION_CB, options_cb, WT_EOL);
  wt_setopt(tune,  WT_LABEL, "Refine...", WT_ACTION_CB, finetune_cb, WT_EOL);

  wt_setopt(list,  WT_LABEL, "Samples...", WT_ACTION_CB, list_cb, WT_EOL);
  wt_setopt(zin,   WT_LABEL, "Zoom In", WT_ACTION_CB, zin_cb, WT_EOL);
  wt_setopt(zout,  WT_LABEL, "Zoom Out", WT_ACTION_CB, zout_cb, WT_EOL);
  wt_setopt(effect,WT_LABEL, "Effects...", WT_ACTION_CB, effects_cb, WT_EOL);

  a = DRAW_WIN_WIDTH  + 2*OFFSET;
  b = DRAW_WIN_HEIGHT + 2*OFFSET;
  c = EV_MOUSE;
  wt_setopt(sample,
	WT_WIDTH, &a,
	WT_HEIGHT, &b,
	WT_EVENT_MASK, &c,
	WT_EVENT_CB, event_cb,
	WT_DRAW_FN, draw_cb,
	WT_EOL);

  return 1;
}

/* 
 * Selection / sample drawing window event functions
 */
static int fit_x(int x)
{
  if(x < OFFSET)
    x = OFFSET;
  else
  {
    int w;

    if(Draw->width - 2*OFFSET < Sample->show_len)
      w = Draw->width - OFFSET;
    else
      w = OFFSET + Sample->show_len;
    if(x >= w)
      x = w - 1;
  }
  return x;
}

static void timeout_cb(long val)
{
  short mx, my;
  w_querymousepos(Draw, &mx, &my);

  mx = fit_x(mx);
  if(mx != Sample->win_x2)
  {
    if(mx < Sample->win_x2)
      w_pbox(Draw, mx, OFFSET, Sample->win_x2 - mx, Draw->height - 2*OFFSET);
    else
      w_pbox(Draw, Sample->win_x2, OFFSET, mx - Sample->win_x2, Draw->height - 2*OFFSET);
    Sample->win_x2 = mx;
  }
  Sample->timer = wt_addtimeout(100, timeout_cb, val);
}

static WEVENT * event_cb(widget_t *w, WEVENT *ev)
{
  if(!Sample)
    return ev;

  switch (ev->type)
    {
    case EVENT_MPRESS:
      invert_selection();
      Sample->win_x2 = Sample->win_x1 = fit_x(ev->x);
      timeout_cb(0);
      break;

    case EVENT_MRELEASE:
      if(Sample->timer >= 0)
      {
        long len;

        /* selection ended */
        wt_deltimeout(Sample->timer);

	/* correct order */
	if(Sample->win_x1 > Sample->win_x2)
	{
	  Sample->timer = Sample->win_x1;
	  Sample->win_x1 = Sample->win_x2;
	  Sample->win_x2 = Sample->timer;
	}

	/* convert to sample co-ordinates */
        len = MIN(Draw->width - 2*OFFSET, Sample->show_len);
        Sample->select = Sample->show + (Sample->win_x1-OFFSET) * Sample->show_len / len;
        Sample->sel_len = (Sample->win_x2 - Sample->win_x1) * Sample->show_len / len;
	show_info();

	Sample->timer = -1;
      }
      break;

    default:
      return ev;
    }

  return NULL;
}

/* 
 * Auxiliary functions
 */

void show_sample(void)
{
  /* can show only if there is something on which to show
   * (ie. not called by command line arg loading)
   */
  if(Sample && Sample->name)
    wt_setopt(Name, WT_LABEL, Sample->name, WT_EOL);
  else
    wt_setopt(Name, WT_LABEL, "", WT_EOL);

  change_rate(0L);
  if(Draw)
    draw_sample();
  show_info();
}

void show_info(void)
{
  char info[48];
  float secs;
  long offset;

  if(!Sample)
  {
    wt_setopt(Info, WT_LABEL, "", WT_EOL);
    return;
  }

  if(Sample->select)
  {
    offset = Sample->select - Sample->data;
    secs = (float)Sample->sel_len / Audio.rate;
    sprintf(info, "%ld --- %ld, %.2gs --- %ld", offset, Sample->sel_len, secs, offset + Sample->sel_len);
  }
  else
  {
    offset = Sample->show - Sample->data;
    secs = (float)Sample->show_len / Audio.rate;
    sprintf(info, "%ld --- %ld, %.2gs --- %ld", offset, Sample->show_len, secs, offset + Sample->show_len);
  }
  wt_setopt(Info, WT_LABEL, info, WT_EOL);
}

void draw_sample(void)
{
  int w, h, idx, half, len, vol;

  /* draw the sample */
  w = Draw->width;
  h = Draw->height;
  w_setmode(Draw, M_CLEAR);
  w_pbox(Draw, OFFSET, OFFSET, w - 2*OFFSET, h - 2*OFFSET);

  half = h / 2;
  w_setmode(Draw, M_DRAW);
  w_hline(Draw, OFFSET, half, w - OFFSET - 1);

  if(!Sample)
    return;

  len = w - 2*OFFSET;
  if(len > Sample->show_len && Sample->lenght > Sample->show + Sample->show_len - Sample->data)
  {
    /* in case window view will not be compressed (1:1),
     * show as much as is available of the Sample->
     */
    Sample->show_len = Sample->data + Sample->lenght - Sample->show;
    if(len > Sample->show_len)
      len = Sample->show_len;
    else
      Sample->show_len = len;
  }

  /* draw sample */
  for(idx = 0; idx < len; idx++)
  {
    /* scale the sample value at the requested (scaled to window) place */
    vol = Sample->show[idx * Sample->show_len / len] * (half - OFFSET) / MAX_VOLUME;
    w_vline(Draw, idx + OFFSET, half - vol, half + vol);
  }

  /* redraw selection for effects */
  w_setmode(Draw, M_INVERS);
  invert_selection();
}

/* 
 * OS specific audio device stuff
 */

#include <fcntl.h>
#include <sys/ioctl.h>

#define DEV_AUDIO	"/dev/audio"

/* ioctl control definitions */
#ifdef __MINT__
#include "audios.h"
#else
#include <sys/soundcard.h>
#endif

static int audio_init(void)
{
  /* try open audiodev for reading & writing */
  if((Audio.device = open(DEV_AUDIO, O_RDWR)) < 0)
  {
    fprintf(stderr, DEV_AUDIO " not available!\n");
    return 0;
  }

  /* set signed & get speed */
#ifdef __MINT__
  {
    long format = AFMT_S8;
    ioctl(Audio.device, AIOCSFMT, &format);
    ioctl(Audio.device, AIOCGSPEED, &Audio.rate);
  }
#else
  Audio.rate = 8000;
  ioctl(Audio.device, SOUND_PCM_READ_RATE, &Audio.rate);
#endif
  return 1;
}

/* change the playing speed and output the value onto widget */
void change_rate(long change)
{
  char info[8];
  long old_rate;

  old_rate = Audio.rate;
  do
  {
    Audio.rate += change;
#ifdef __MINT__
    ioctl(Audio.device, AIOCSSPEED, (void *)Audio.rate);
    ioctl(Audio.device, AIOCGSPEED, &Audio.rate);
#else
  ioctl(Audio.device, SOUND_PCM_WRITE_RATE, &Audio.rate);
  ioctl(Audio.device, SOUND_PCM_READ_RATE, &Audio.rate);
#endif
    change <<= 1;
    /* step size depends on the h/w available */
  } while(Audio.rate == old_rate && change &&
    (change < 0 ? -change : change) < old_rate);

  /* show information (with value rounding) */
  sprintf(info, "%ldkHz", (Audio.rate + 512) / 1024);
  wt_setopt(Rate, WT_LABEL, info, WT_EOL);  

  if(Sample)
    Sample->rate = Audio.rate;
}

/* play the sample through audio device. */
void play_sample(widget_t *w, int pressed)
{
  if(pressed)
    return;

  if(!Sample)
  {
    message("Load a sample first!");
    return;
  }

  /* play sample */
  if(Audio.output & FLAG_LOOP)
  {
    sample_t *data;
    long lenght;

    /* play first part and set new play variables */
    if((Audio.output & FLAG_SELECTION) && Sample->select)
    {
      write(Audio.device, Sample->data, Sample->select - Sample->data);
      lenght = Sample->sel_len;
      data = Sample->select;
    }
    else
    {
      data = Sample->data;
      lenght = Sample->lenght;
    }

    /* loop while no events */    
    while(!w_queryevent(NULL, NULL, NULL, 0))
    {
      /* wait for sample (part) to end and play again... */
#ifdef __MINT__
      ioctl(Audio.device, AIOCSYNC, (void *)0L);
#else
      ioctl(Audio.device, SOUND_PCM_SYNC);
#endif
      write(Audio.device, data, lenght);
    }

    /* play possible end part */
    lenght = (Sample->data + Sample->lenght) - (data + lenght);
    if(lenght > 0)
      write(Audio.device, Sample->select + Sample->sel_len, lenght);
  }
  else
  {
    /* play once without waiting */
    if((Audio.output & FLAG_SELECTION) && Sample->select)
      write(Audio.device, Sample->select, Sample->sel_len);
    else
      write(Audio.device, Sample->data, Sample->lenght);
  }
#ifndef __MINT__
      ioctl(Audio.device, SOUND_PCM_SYNC);
#endif
}

static void audio_exit(void)
{
  if(Audio.device)
  {
    close(Audio.device);
    Audio.device = 0;
  }
}
