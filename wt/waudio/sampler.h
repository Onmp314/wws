/* W-audio functions, globals and structures */

#ifndef __SAMPLER_H
#define __SAMPLER_H

#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include <Wt.h>

/* Samples are signed, 8-bit ones.
 * Another alternative would be short, but remeber that this is
 * highly audio device specific (ATM waudio doesn't support
 * 16-bit output, nor stereo samples).
 */
typedef signed char sample_t;
#define MAX_VOLUME	128	/* dynamic range: -127 - 0 - +127 */

typedef struct
{
  int output;			/* flags for audio output */
  long device;			/* audio device handle */
  long rate;			/* sample playing speed */
} AudioPack;

/* Sample list item */
typedef struct sample_item
{
  struct sample_item *prev;
  struct sample_item *next;
  char *name;			/* sample name */
  long rate;			/* playing speed (Hz) */
  long mem_len;			/* allocated space */
  long lenght;			/* current sample lenght */
  sample_t *data;		/* sample data */
  sample_t *undo;		/* undo buffer (zero if nothing to undo) */
  sample_t *show;		/* sample part shown */
  sample_t *select;		/* acts also as selection existance flag */
  long timer;			/* for selection mouse check timeouts */
  long win_x1;			/* selected area start in window */
  long win_x2;			/* selected area start in window */
  long sel_len;			/* selection lenght in bytes */
  long show_len;		/* show lenght (bytes) */
  long undo_len;		/* undo buffer lenght */
  long undo_pos;		/* buffer from position... */
  long undo_mode;		/* to put it back use this mode */
} SampleItem;

/* in sampler.c */

/* most used item on whole program...
 * currently shown / editable sample
 */
extern SampleItem *Sample;
extern AudioPack Audio;
extern widget_t *Top;			/* realizable */
extern void message(const char *msg);	/* show a message dialog */
extern long spl2win(sample_t *addr);	/* convert selection co-ords */
extern void play_sample(widget_t *w, int pressed);
extern void change_rate(long change);
extern void invert_selection(void);	/* mostly deselect */
extern void show_sample(void);		/* show sample, info etc. */
extern void draw_sample(void);		/* draw (changed) sample into window */
extern void show_info(void);		/* show sample / selection info */

/* in list.c */
extern void list_cb(widget_t *w, int pressed);
extern SampleItem *create_sample(size_t size);
extern void load_sample(widget_t *w, const char *file);
extern int  update_list(void);
extern void list_exit(void);		/* call at exit */

/* in synth.c */
extern void synth_cb(widget_t *w, int pressed);
extern void synth_exit(void);

/* in effects.c */
extern widget_t *Effect;		/* close when no samples */
extern void effects_cb(widget_t *w, int pressed);
extern void effect_exit(void);

/* in tune.c */
extern widget_t *Tune;			/* close when no samples */
extern void finetune_cb(widget_t *w, int pressed);
extern void tune_exit(void);

#endif
