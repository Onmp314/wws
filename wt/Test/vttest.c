#include <stdio.h>
#include <Wlib.h>
#include <Wt.h>

static widget_t *top, *sb, *hpane, *shell, *vt;


static WEVENT * key_cb (widget_t *w, WEVENT *ev)
{
  wt_opaque_t str;
  char c;

  if (ev->type == EVENT_KEY) {
    c = ev->key & 0x7f;
    switch (c) {
      default:
	str.cp = &c;
	str.len = 1;
	wt_setopt (vt, WT_VT_STRING, &str, WT_EOL);
	break;
    }
    return NULL;
  }
  return ev;
}

static void vt_cb (widget_t *w, long pos, long size,
	long ncols, long nrows, wt_opaque_t *pasted)
{
  long cols = 80, rows = 25;

  if (ncols > 0)
    cols = ncols;
  if (nrows > 0)
    rows = nrows;

  if (size >= 0) {
    long sbpos, osize;
    wt_getopt (sb, WT_POSITION, &sbpos, WT_TOTAL_SIZE, &osize, WT_EOL);
    size += 10;
    sbpos += size - osize;
    wt_setopt (sb, WT_TOTAL_SIZE, &size, WT_SIZE, &rows,
	WT_POSITION, &sbpos, WT_EOL);
  }
  if (pos >= 0) {
    long sbpos, osize;
    wt_getopt (sb, WT_POSITION, &sbpos, WT_TOTAL_SIZE, &osize, WT_EOL);
    sbpos = osize - 10 - pos;
    wt_setopt (sb, WT_POSITION, &sbpos, WT_EOL);
  }
  if (pasted) {
    wt_setopt (vt, WT_VT_STRING, pasted, WT_EOL);
  }
}

static void sb_cb (widget_t *w, long sbpos)
{
  long size, pos;
  wt_getopt (sb, WT_TOTAL_SIZE, &size, WT_EOL);
  pos = size - 10 - sbpos;
  wt_setopt (vt, WT_VT_HISTPOS, &pos, WT_EOL);
}


int
main ()
{
  long wd, ht, a, b, c;

  top = wt_init ();
  shell = wt_create (wt_shell_class, top);
  hpane = wt_create (wt_pane_class, shell);
  vt = wt_create (wt_vt_class, hpane);
  sb = wt_create (wt_scrollbar_class, hpane);

  a = AlignFill;
  b = OrientHorz;
  wt_setopt (hpane, WT_ALIGNMENT, &a, WT_ORIENTATION, &b, WT_EOL);

  a = 10;
  b = 0;
  c = 1;
  wt_setopt (sb,
	     WT_TOTAL_SIZE, &a,
             WT_SIZE, &a,
	     WT_POSITION, &b,
             WT_LINE_INC, &c,
	     WT_PAGE_INC, &a,
	     WT_ACTION_CB, sb_cb,
	     WT_EOL);

  wd = 20;
  ht = 10;
  wt_setopt (vt,
             WT_VT_WIDTH, &wd,
	     WT_VT_HEIGHT, &ht,
	     WT_VT_HISTSIZE, &ht,
	     WT_ACTION_CB, vt_cb,
	     WT_EOL);

  wt_realize (top);
  wt_bind (NULL, EV_KEYS, key_cb);
  wt_run ();
  return 0;
}
