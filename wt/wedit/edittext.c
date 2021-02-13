/*
 * this file is part of "The W Toolkit".
 *
 * (W) 1997, Eero Tamminen.
 *
 * Text editing widget.  Line lenght cannot be changed after widget has been
 * realized. Font should be a *fixed* width one. print_line() can deal with
 * proportional fonts, but cursor etc. functions not. Besides, you wouldn't
 * then now how the text looks like when it's viewed normally...
 *
 * buffer handling:
 *
 * two double linked lists.  List members are line structures.  First list
 * has lines in use and second free lines.  Free line list is handled as
 * stack and the used line list as circular list.  So lines can be moved
 * between any place on the used list and the free line list (stack) top.
 * Free line list endpoints are dangling.  `lines' var keeps count about how
 * many lines there are available and `used' var keeps count about how
 * many of them are in the used line list.
 *            `used'
 *      |------------------|   `lines'
 *      |-------------------------------------|
 *	#######################################
 *      \__________________/\
 *       list of used lines  free line stack top, deleted or new
 *                           allocated lines will be added here
 *
 * On suitable texts (most lines full) use of static line lenght doesn't
 * take more space than dynamic one would do and static is simpler to
 * implement (= much less code, speedier) and more predictable.  With less
 * cramped text (eg.  C-code), this scheme can easily take several times
 * more space compared to dynamic line lenght ones though!
 *
 * TODO:
 * - Let font be changeble after widget has been realized...
 *
 * NOTES:
 * - insert/delete/join/wrap/indent spaghetti would need design/rewrite.
 *   Cursor position, tabs, previous/current line indent are an added
 *   complication to these routines.  They also need to keep count of which
 *   lines are changed (for screen update) etc.  Current stuff just
 *   evolved and it shows...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <Wlib.h>
#include "Wt.h"
#include "edittext.h"			/* key macros */
#include "toolkit.h"


/*
 * default buffer size
 */
#define DEF_COLS	72
#define DEF_ROWS	12
#define DEF_TABSIZE	8

#define XBORDER		(WT_DEFAULT_XBORDER + 4)
#define YBORDER		(WT_DEFAULT_YBORDER + 2)

/* how much memory allocated for lines at the time */
#define BLOCK_SIZE	4096

typedef struct LINE_T
{
  struct LINE_T *prev;
  struct LINE_T *next;
  uchar *string;		/* buffer for null terminated string */
} line_t;

/* selection types: none, by mouse, by keyboard or by application */
enum { SEL_NONE, SEL_MOUSE, SEL_KEY, SEL_APP, SEL_DONE };

/*
 * edittext widget
 */
typedef struct
{
  widget_t w;
  uchar is_open;
  uchar is_realized;
  widget_t *scrollbar;
  short scrollwd;
  WFONT *font;
  short fsize;

  uchar *tmp;		/* static, for misc operations (see line_print) */
  uchar *undo;		/* static, one line editing undo buffer */
  uchar *text;		/* `user' text for the buffer */

  line_t *buffer;	/* buffer for text lines */
  line_t *first;	/* first text line in buffer */
  line_t *current;	/* current editing line */
  line_t *free;		/* first free line in buffer */

  int lines;		/* lines allocated in buffer */
  int used;		/* how many of the lines in buffer are used */
  int lineno;		/* index to current line */
  short offset;		/* index to cursor position in line */
  short columns;	/* maximum line lenght */

  short top;		/* first visible line */
  short height;		/* text window height in lines */
  short cursor;		/* cursor on/off flag */
  short position;	/* keep horz. position with vert. movement */
  short deladd;		/* how many lines deleted(-) or added(+) for update */
  short last;		/* last line changed by operation for update */

  short tabsize;	/* how many spaces tab equals (1 = convert to spaces) */
  short indent;		/* text indentation flag */
  short wrap;		/* text wrap column */

  short inselect;	/* selection valid */
  short selx0,sely0;	/* selection start */
  short selx1,sely1;	/* selection end   */
  short nclicks;	/* # mouse clicks since clicktime */
  long  clicktime;	/* time of last mouse click */
  long  seltimer;	/* selection timer */

  /* WT_ACTION_CB, returns null if given string isn't modified. */
  uchar *(*process_cb)(widget_t *w, uchar *string, int maxlen, void *ptr);

  /* WT_OFFSET_CB, returns non-zero when done */
  int (*offset_cb)(widget_t *w, uchar *string, int line, void *ptr);

  /* WT_CHANGE_CB, gets called every time cursor position changes. */
  void (*change_cb)(widget_t *w, int col, int row);

  /* WT_MAP_CB, called to remap input character. */
  int (*map_cb)(widget_t *w, int in);

  short usrwd, usrht;
} edittext_widget_t;


/* some utility function prototypes */
static int	join_lines(edittext_widget_t *w, int space);
static void	scroll_window(edittext_widget_t *w, int offset);
static void	cursor_move(edittext_widget_t *w, int x, int y);
static void	update_lines(edittext_widget_t *w, line_t *line, int offset,
                             int down, int first, int last);
static line_t	*newline_edittext(edittext_widget_t *w, line_t *line);
static line_t	*delline_edittext(edittext_widget_t *w, line_t *line);
static void	parse_edittext (edittext_widget_t *w, int to_end);
static void	newline(edittext_widget_t *w);
static void	delline(edittext_widget_t *w);
/*
 * default graphics mode is M_INVERS
 */

/* 
 * print one text line into window from given line buffer offset.
 * parses tabs.
 */
static void
print_line(edittext_widget_t *w, const uchar *str, int offset, int line)
{
  int tab, limit, count, off, idx, ht, y, len;
  WWIN *win = w->w.win;
  uchar *tmp = w->tmp;

  line -= w->top;
  if(line < 0 || line >= w->height)
    return;

  ht = w->font->height;
  y = YBORDER + ht * line;

  /* tab -> space conversion for W output */
  tab = w->tabsize;
  limit = w->columns;
  off = idx = 0;
  do {
    if(offset-- == 0)
      off = idx;
    if(*str == '\t') {
      count = ((idx + tab) / tab) * tab;
      if(count >= limit)
	count = limit;
      while(idx < count) {
	*tmp++ = ' ';
	idx++;
      }
    } else {
      *tmp++ = *str;
      idx++;
    }
  } while(*str++ && idx < limit);
  *tmp = '\0';

  tmp = &w->tmp[off];
  len = w_strlen(w->font, w->tmp);
  w_printstring(win, XBORDER + len - w_strlen(w->font, tmp), y, tmp);

  w_setmode(win, M_CLEAR);
  w_pbox(win, XBORDER + len, y, win->width - len - XBORDER * 2, ht);
  w_setmode(win, M_INVERS);
}

/*********** cursor functions ******************/

/* output strlen to given offset */
static int
offset2window(edittext_widget_t *w, uchar *str, int count)
{
  int idx = 0, tab = w->tabsize;

  while(count-- > 0 && *str)
  {
    if(*str++ == '\t')
      idx = ((idx + tab) / tab) * tab;
    else
      idx++;
  }
  return idx;
}

static int
window2offset(edittext_widget_t *w, uchar *str, int x)
{
  int idx = 0, tab = w->tabsize;
  uchar *start = str;
  
  while(idx < x && *str)
  {
    if(*str++ == '\t')
      idx = ((idx + tab) / tab) * tab;
    else
      idx++;
  }
  return str - start;
}

static void
show_newpos(edittext_widget_t *w)
{
  if(w->change_cb)
    w->change_cb((widget_t*)w, offset2window(w, w->current->string, w->offset), w->lineno);
}

static int
cursor_invert(edittext_widget_t *w)
{
  int wd, ht, x, y;

  y = w->lineno - w->top;
  if(y < 0 || y >= w->height)
    return 0;
  wd = w->font->maxwidth;
  ht = w->font->height;
  x = XBORDER + offset2window(w, w->current->string, w->offset) * wd;
  y = YBORDER + y * ht;
  w_vline(w->w.win, x, y, y + ht - 1);
  return 1;
}

static void
cursor_on(edittext_widget_t *w)
{
  if(w->cursor)
    return;

  w->cursor = cursor_invert(w);
}

/* has to be called before drawing to window or changing w->lineno, w->top
 * or w->offset
 */
static void
cursor_off(edittext_widget_t *w)
{
  if(!w->cursor)
    return;

  cursor_invert(w);
  w->cursor = 0;
}

/************** text selection functions *****************/

#define SWAP(x, y) { (x) ^= (y); (y) ^= (x); (x) ^= (y); }

/*
 * NOTE:  functions that use limit_pos() (select_start/drag/word/line
 * functions) expect the co-ordinate pair to be window, not text relative!!!
 */

static void
select_paint (edittext_widget_t *w, int x0, int y0, int x1, int y1)
{
  int ht, wd;

  if (y0 > y1)
  {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }
  else
  {
    if (y0 == y1 && x0 > x1)
      SWAP(x0, x1);
  }
  y0 -= w->top;
  y1 -= w->top;
  if(y0 < 0)
  {
    y0 = 0;
    x0 = 0;
  }
  if(y1 >= w->height)
  {
    y1 = w->height - 1;
    x1 = w->columns;
  }
  if(y0 > y1)
    return;

  ht = w->font->height;
  wd = w->font->maxwidth;

  if (y0 == y1)
    w_pbox (w->w.win, XBORDER + x0*wd, YBORDER + y0*ht, (x1-x0)*wd, ht);
  else
  {
    if (x0 < w->columns)
      w_pbox (w->w.win, XBORDER + x0*wd, YBORDER + y0*ht, (w->columns-x0)*wd, ht);
    if (y0+1 < y1)
      w_pbox (w->w.win, XBORDER, YBORDER + (y0+1)*ht, w->columns*wd, (y1-y0-1)*ht);
    w_pbox (w->w.win, XBORDER, YBORDER + y1*ht, x1*wd, ht);
  }
}

static void
unselect (edittext_widget_t *w)
{
  if (w->inselect)
  {
    select_paint (w, w->selx0, w->sely0, w->selx1, w->sely1);
    if (w->seltimer >= 0)
    {
      wt_deltimeout (w->seltimer);
      w->seltimer = -1;
    }
    w->inselect = 0;
  }
}

static void
select_here(edittext_widget_t *w, int x, int y, int type)
{
  unselect (w);
  w->inselect = type;
  show_newpos(w);

  /* start selection text relative */
  w->selx0 = w->selx1 = x;
  w->sely0 = w->sely1 = y;
  select_paint (w, x, y, x, y);
}

static void
select_there(edittext_widget_t *w, int x, int y)
{
  /* drag selection text relative */
  if (x != w->selx1 || y != w->sely1)
  {
    select_paint (w, w->selx1, w->sely1, x, y);
    w->selx1 = x;
    w->sely1 = y;
  }
}

/* convert screen character coords to text character coords with limits */
static void
limit_pos(edittext_widget_t *w, int *x, int *y)
{
  if (*x < 0)
    *x = 0;
  else if (*x > w->columns)
    *x = w->columns;

  if (*y < 0)
    *y = 0;
  else if (*y >= w->height)
    *y = w->height-1;

  *y += w->top;
}

static void
select_start (edittext_widget_t *w, int x, int y)
{
  /* start selection window relative */
  limit_pos(w, &x, &y);

  /* jump to selection */
  w->position = x;
  cursor_move(w, window2offset(w, w->current->string, w->position), y);
  select_here(w, x, y, SEL_MOUSE);
}

static void
select_drag (edittext_widget_t *w, int x, int y)
{
  /* drag selection window relative */
  if (w->inselect != SEL_MOUSE)
    return;

  if(y < 0)
    scroll_window(w, y);
  if(y >= w->height)
    scroll_window(w, y - w->height + 1);
  limit_pos(w, &x, &y);

  select_there(w, x, y);
}

static void
select_end (edittext_widget_t *w, int remove)
{
  line_t *line = w->current;
  int idx, x0, y0, x1, y1;
  w_clipboard_t clip;
  uchar *str;

  if (!w->inselect)
    return;

  if (w->seltimer >= 0)
  {
    wt_deltimeout (w->seltimer);
    w->seltimer = -1;
  }

  x0 = w->selx0;
  y0 = w->sely0;
  x1 = w->selx1;
  y1 = w->sely1;

  if (y0 > y1)
  {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }
  else
  {
    if (y0 == y1)
    {
      if(x0 > x1)
      {
        SWAP(x0, x1);
      }
      else if (x0 == x1)
      {
	unselect (w);
	return;
      }
    }
  }
  if(y0 >= w->used)
  {
    unselect(w);
    return;
  }

  if(y1 >= w->used)
  {
    y1 = w->used - 1;
    x1 = w->columns;
  }

  idx = w->lineno;
  while(idx > y0)
  {
    line = line->prev;
    idx--;
  }
  while(idx < y0)
  {
    line = line->next;
    idx++;
  }

  if(remove)
  {
    w->inselect = 0;
    remove = (y1 != y0);

    w->lineno = idx;
    w->current = line;
    w->offset = window2offset(w, line->string, x0);
    w->position = x0;
    w->deladd = 0;
    if(remove)
    {
      line->string[w->offset] = '\0';
      line = line->next;
      while(y0 < --y1)
      {
        line = delline_edittext(w, line);
	w->deladd--;
      }
      x0 = 0;
    }
    if(x1)
    {
      str = line->string;
      x0 = window2offset(w, str, x0);
      x1 = window2offset(w, str, x1);
      memmove(&str[x0], &str[x1], strlen(&str[x1])+1);
    }
    if(remove)
    {
      w->last = w->lineno+1;
      join_lines(w, -1);
      update_lines(w, w->current, w->offset, 0, w->lineno, w->last);
    }
    else
      print_line(w, w->current->string, w->offset, w->lineno);

    strcpy(w->undo, w->current->string);
    return;
  }

  if(!(clip = w_selopen(W_SEL_TEXT)))
  {
    w_beep();
    unselect(w);
    return;
  }

  while(y0++ < y1)
  {
    str = line->string;
    str += window2offset(w, str, x0);
    w_selappend(clip, str, strlen(str));
    w_selappend(clip, "\n", 1);
    line = line->next;
    x0 = 0;
  }
  if(x1)
  {
    str = line->string;
    x0 = window2offset(w, str, x0);
    x1 = window2offset(w, str, x1) - x0;
    strncpy(w->tmp, str + x0, x1);
    w_selappend(clip, w->tmp, x1);
  }
  w_selclose(clip);
  w->inselect = SEL_DONE;
}

static inline void
select_remove(edittext_widget_t *w)
{
  select_end(w, 1);
}

static void
select_word (edittext_widget_t *w, int x, int y)
{
  uchar *str = w->current->string;

  limit_pos(w, &x, &y);
  x = window2offset(w, str, x);
  if(!isalnum(str[x]) && str[x] < 128)
    return;

  /* presumes all used 8-bit chars to be letters */
  while(--x >= 0 && (isalnum(str[x]) || str[x] >= 128));
  w->position = offset2window(w, str, ++x);
  cursor_move(w, x, y);

  select_here(w, w->position, y, SEL_MOUSE);

  while(isalnum(str[x]) || str[x] >= 128)
    x++;
  select_there(w, offset2window(w, str, x), y);
  select_end (w, 0);
}

static void
select_line (edittext_widget_t *w, int x, int y)
{
  limit_pos(w, &x, &y);
  w->position = 0;
  cursor_move(w, 0, y);
  select_here(w, 0, y, SEL_MOUSE);
  select_there(w, w->columns, y);
  select_end (w, 0);
}

static void
select_timer_cb (long arg)
{
  edittext_widget_t *w = (edittext_widget_t *)arg;
  short x, y;

  w_querymousepos (w->w.win, &x, &y);
  x = (x - XBORDER) / w->font->maxwidth;
  y = (y - YBORDER) / w->font->height;
  select_drag (w, x, y);

  w->seltimer = wt_addtimeout (100, select_timer_cb, arg);
}

/************** window output update functions ***************/

/*
 * scrolls lines from `index' to window (view) bottom edge by `offset' lines.
 * print lines that scrolled to view from below.
 */
static void
scroll_lines(edittext_widget_t *w, line_t *line, int index, int offset)
{
  int off, count;

  if(!offset)
    return;

  count = index - w->top;
  if(offset > 0)
    off = w->height - count - offset;
  else
    off = w->height - count;

  if(off > 0 && -offset < w->height)
  {
    int y, h;
    h = w->font->height;
    y = YBORDER + count * h;
    if(offset < 0)
    {
      if(count < -offset)
      {
        y = YBORDER - offset * h;
        off = w->height + offset;
      }
    }
    w_vscroll(w->w.win, 0, y, w->w.win->width, off * h, y + offset * h);
  }

  if(offset < 0)
  {
    /* redraw lines that come into view from below window */
    count = w->top + w->height + offset;
    while(index < count)
    {
      line = line->next;
      index++;
    }
    while(index > count)
    {
      line = line->prev;
      index--;
    }
    count = w->top + w->height;
    while(index < count)
    {
      if(index >= w->used)
	print_line(w, "", 0, index);
      else
      {
	print_line(w, line->string, 0, index);
	line = line->next;
      }
      index++;
    }
  }
}

/* 
 * scroll window contents so that `newtop' index is first line
 * to be shown
 */
static void
scroll_cb (widget_t *_w, int newtop, int pressed)
{
  edittext_widget_t *w = (edittext_widget_t *) _w->parent;
  int count, offset, idx, state = w->cursor;
  line_t *line;

  offset = w->top - newtop;
  if(!offset || newtop < 0 || (newtop + w->height > w->used && newtop > w->top))
    return;

  cursor_off(w);

  /* assure that cursor is inside the new window position */
  while(w->lineno < newtop)
  {
    w->current = w->current->next;
    w->lineno++;
  }
  idx = newtop + w->height;
  while(w->lineno >= idx)
  {
    w->current = w->current->prev;
    w->lineno--;
  }

  idx = w->lineno;
  line = w->current;
  w->top = newtop;
  if(offset < 0)
    newtop -= offset;

  /* change to scroll position */
  while(idx < newtop)
  {
    line = line->next;
    idx++;
  }
  while(idx > newtop)
  {
    line = line->prev;
    idx--;
  }

  if(offset > 0)
  {
    /* scroll window up, text down */
    if(offset < w->height)
      /* with downward scrolling line strings aren't output */
      scroll_lines(w, line, newtop, offset);

    count = offset;
    if(count > w->height)
      count = w->height;
    while(count--)
    {
      print_line(w, line->string, 0, idx++);
      line = line->next;
    }
  }
  else
  {
    /* in this direction scrolling does line printing, otherwise not... */
    scroll_lines(w, line, newtop, offset);
  }

  /* repaint needed part of the selection */
  if(w->inselect)
  {
    int x0, x1, y0, y1;
    x0 = w->selx0;
    y0 = w->sely0;
    x1 = w->selx1;
    y1 = w->sely1;
    if (y0 > y1)
    {
      SWAP(x0, x1);
      SWAP(y0, y1);
    }
    if(offset > 0)
    {
      if(y0 >= w->top && y0 < w->top + w->height && y0 - offset < w->top)
      {
	if(y1 - offset >= w->top)
	{
	  y1 = w->top + offset - 1;
	  x1 = w->columns;
	}
	select_paint(w, x0, y0, x1, y1);
      }
    }
    else
    {
      if(y1 >= w->top && y1 < w->top + w->height && y1 - offset >= w->top + w->height)
      {
	if(y0 - offset < w->top + w->height)
	{
	  y0 = w->top + w->height + offset;
	  x0 = 0;
	}
	select_paint(w, x0, y0, x1, y1);
      }
    }
  }

  if(state)
    cursor_on(w);
  show_newpos(w);
}

/*
 * scroll whole window contents by given amount & update scrollbar
 */
static void
scroll_window(edittext_widget_t *w, int offset)
{
  long pos;

  offset += w->top;
  if(offset + w->height >= w->used)
    offset = w->used - w->height;
  if(offset < 0)
    offset = 0;

  scroll_cb(w->scrollbar, offset, 1);

  pos = w->top;
  (*wt_scrollbar_class->setopt) (w->scrollbar, WT_POSITION, &pos);
}

static void
set_scrollbar(edittext_widget_t *w)
{
  long i;

  if(w->used < w->height)
    i = w->height;
  else
    i = w->used;

  (*wt_scrollbar_class->setopt) (w->scrollbar, WT_TOTAL_SIZE, &i);

  i = w->top;
  (*wt_scrollbar_class->setopt) (w->scrollbar, WT_POSITION, &i);
}

/*
 * - moves the cursor position on screen preserving cursor state.
 * - when changing lines, w->position too has to be correct.
 * - takes care that tabs are accounted for.
 * - saves undo when changing lines
 * - scrolls if necessary
 */
static void
cursor_move(edittext_widget_t *w, int x, int y)
{
  int idx, line_change, state = w->cursor;

  if (y < 0)
    y = 0;
  if (y >= w->used)
    y = w->used - 1;

  line_change = y - w->lineno;

  cursor_off(w);

  /* speedup for *very* long texts */
  if(y + y < w->lineno)
  {
    w->current = w->first;
    w->lineno = 0;
  }
  else
  {
    if(w->lineno + w->lineno < y)
    {
      w->current = w->first->prev;
      w->lineno = w->used - 1;
    }
  }

  if (y != w->lineno)
  {
    line_t *cur = w->current;

    if (w->lineno > y)
    {
      do {
        cur = cur->prev;
      } while (--w->lineno > y);
    }
    else
    {
      do {
        cur = cur->next;
      } while (++w->lineno < y);
    }
    w->current = cur;
  }

  if(line_change)
  {
    strcpy(w->undo, w->current->string);
    x = window2offset(w, w->current->string, w->position);
  }

  if (x > 0)
  {
    idx = strlen(w->current->string);
    if(x > idx)
      x = idx;
  }
  else
    x = 0;

  w->offset = x;
  w->lineno = y;

  y -= w->top;
  if(y < 0 || y >= w->height)
  {
    line_change += w->lineno;
    /* possible to retain same relative window position for the cursor? */
    if(line_change >= w->top && line_change < w->top + w->height)
      scroll_window(w, line_change - w->lineno);
    else
    {
      if(y < 0)
        scroll_window(w, y);
      else
        scroll_window(w, y - w->height + 1);
    }
  }

  if(state)
    cursor_on(w);
}

/* redraw whole widget window */
static void
redraw_edittext(edittext_widget_t *w)
{
  int state = w->cursor, idx = w->lineno;
  line_t *line = w->current;

  while(idx < w->top)
  {
    line = line->next;
    idx++;
  }
  while(idx > w->top)
  {
    line = line->prev;
    idx--;
  }

  cursor_off(w);
  while(idx < w->used && idx < w->top + w->height)
  {
    print_line(w, line->string, 0, idx);
    line = line->next;
    idx++;
  }
  /* update cursor window offset */
  w->position = offset2window(w, w->current->string, w->offset);
  if(state)
    cursor_on(w);
}

/*
 * updates widget window contents to reflect text according to variables:
 * scroll lines below `first' `line' + `down' up (-) or down (+) by
 * `w->deladd', change window position so that current line shows, draw
 * lines from `first' `line' to `last' and update cursor offset for the
 * window.
 *
 * `line' is first line to draw, `offset' is the line offset to use
 * for the that, `first' is the line number for `line' and `last' is
 * the line number for the last line to be drawn.
 */
static void
update_lines(edittext_widget_t *w, line_t *line, int offset,
             int down, int first, int last)
{
  int count;

  /* make room for the lines if necessary */
  if(w->deladd)
  {
    int idx = first, change = last - w->deladd + down;
    line_t *this = line;
    if(w->deladd < 0)
    {
      /* at least one line down */
      this = this->next;
      idx++;
    }
    while(idx < change)
    {
      this = this->next;
      idx++;
    }
    scroll_lines(w, this, idx, w->deladd);
    set_scrollbar(w);
  }

  /* reposition text on window if necessary */
  count = w->lineno - w->top;
  if(count < 0)
    scroll_window(w, count);
  if(count >= w->height)
    scroll_window(w, count - w->height + 1);

  /* print the changed lines */
  print_line(w, line->string, offset, first);
  while(++first <= last && first < w->used)
  {
    line = line->next;
    print_line(w, line->string, 0, first);
  }
  w->position = offset2window(w, w->current->string, w->offset);
}

/************ text buffer manipulation functions ***************/

/*
 * check whether there's anything else besides spaces on a line
 */
static int
line_content(uchar *str)
{
  while(*str && *str <= ' ')
    str++;
  if(*str)
    return 1;
  return 0;
}

/*
 * below `current' (line pointer), `line' (number), `deladd' (line deletions
 * / additions) and `last' (last updated line) widget members have to be
 * modified correctly (just and only in the right place) for text and screen
 * updates to work correctly.
 *
 * functions below wrap/join/new-/delline current line.
 * lines are wrapped/joined/formatted to `wrap' column.
 */

/* 
 * wrap last word on current line to next one.  Return 0 if key can be
 * discarded and < 0 if line can't be cut.  if `key' argument is set,
 * it tells the function whether we're inserting white space or not.
 */
static int
wrap_line(edittext_widget_t *w, uchar key)
{
  uchar *str2, *str = w->current->string;
  int end, len, offset, index, check;

  index = w->offset;
  offset = len = strlen(str);
  while(--offset >= 0 && str[offset] <= ' ');
  offset++;

  /* inserting and empty space at the line end? */
  if(key && (offset < len || (index >= offset && key <= ' ')))
  {
    str[offset] = '\0';
    if(index >= offset)
      newline(w);
    if(key <= ' ')
      return 0;
    return 1;
  }

  if(offset2window(w, str, offset) > w->wrap)
  {
    do {
      offset--;
    } while(offset2window(w, str, offset) > w->wrap);
    while(--offset >= 0 && str[offset] <= ' ');
    offset++;
  }

  if(key > ' ')
    check = 0;
  else
    check = index;
  while(--offset >= check && str[offset] > ' ');

  /* too long word? */
  if(++offset == 0)
  {
    /* able to cut? */
    if(index > 0)
    {
      newline(w);
      return 1;
    }
    return -1;
  }

  /* wrap at offset */

  /* spaces to delete? */
  end = offset;
  while(--end >= 0 && str[end] <= 32);
  end++;

  if(index > end && (index < offset || (index == offset && key == ' ')))
    end = index;

  /* not a line or paragraph end? */
  str2 = w->current->next->string;
  if(index < len && line_content(str2))
  {
    check = offset2window(w, &str[offset], len-offset);
    check += offset2window(w, str2, w->columns);

    /* enough space on the next line for the last word on current line? */
    if(check < w->wrap && w->current->next != w->first)
    {
      int indent = 0;
      while(str2[indent] && str2[indent] <= 32)
        indent++;
      /* insert word + space into string indented */
      len -= offset;
      memmove(&str2[indent+len+1], &str2[indent], strlen(&str2[indent]) + 1);
      memcpy(&str2[indent], &str[offset], len);
      str2[indent + len] = ' ';
      str[end] = '\0';

      if (w->last <= w->lineno)
        w->last = w->lineno + 1;
      if(index > offset || (index == offset && key > ' '))
      {
	w->offset = indent + index - offset;
	w->current = w->current->next;
	w->lineno++;
      }
      return 1;
    }
  }
  /* create a new line for the last word */
  w->offset = offset;
  newline(w);
  str[end] = '\0';

  /* cursor on the word to be wrapped? */
  if(index > offset || (index == offset && key > ' '))
    w->offset += index - offset;
  else
  {
    /* back to previous line */
    w->offset = index;
    w->current = w->current->prev;
    w->lineno--;
  }
  return 1;
}

/*
 * indents the following line as current one.  expects there to be a
 * following line.
 */
static int
indent_next(edittext_widget_t *w)
{
  int indent, indentScreen, index, ret;
  uchar *str, *str2;

  if(!w->indent)
    return 0;

  indent = 0;
  str = w->current->string;
  while(str[indent] <= ' ' && str[indent])
    indent++;

  if(!indent)
    return 0;

  /* wrap line if indentation necessiates it */
  index = w->offset;
  str2 = w->current->next->string;
  indentScreen = offset2window(w, str, indent);
  while(indentScreen + offset2window(w, str2, w->columns) > w->wrap)
  {
    w->lineno++;
    w->offset = 0;
    w->current = w->current->next;
    ret = wrap_line(w, 0);
    w->current = w->current->prev;
    w->offset = index;
    w->lineno--;
    if(ret < 0)
      return 0;
  }

  memmove(&str2[indent], str2, strlen(str2) + 1);
  for(index = 0; index < indent; index++)
    str2[index] = str[index];
  return indent;
}

/*
 * joins from next line to current line as many `words' as possible.
 * `space' = 1 means paragragh formatting: newline+indent converted to 1 space
 * `space' = 0 means that newline+indentation should be removed
 * `space' < 0 means that indentation will be left as it is
 *
 * return values:
 * < 0: not joinable
 *   0: ok
 * > 0: next line had content but joined completely, so call again!
 */
static int
join_lines(edittext_widget_t *w, int space)
{
  int len, end, len2, indent, offset, tab_off, diff;
  uchar *str, *str2;

  if(w->current->next == w->first)
    return 0;

  str = w->current->string;
  len = strlen(str);
  if(!len)
  {
    delline(w);
    return 0;
  }
  end = offset2window(w, str, len);
  if(end > w->wrap)
  {
    return -1;
  }

  str2 = w->current->next->string;
  if(line_content(str2) || space < 0)
  {
    len2 = strlen(str2);
    indent = 0;
    if(space < 0)
      space = 0;
    else
    {
      while(str2[indent] && str2[indent] <= 32)
        indent++;
      if(indent)
      {
        len2 -= indent;
        str2 += indent;
      }
    }
  }
  else
    len2 = indent = 0;

  diff = 0;
  tab_off = len2;
  offset = offset2window(w, str2, len2);
  /* check tab expansion */
  tab_off = 0;
  while(tab_off < len2 && str2[tab_off] != '\t')
    tab_off++;
  if(tab_off < len2)
  {
    diff = ((end + tab_off + w->tabsize) / w->tabsize) * w->tabsize
	 - ((tab_off + w->tabsize) / w->tabsize) * w->tabsize - end;
  }

  /* whole next line fits to end of current one? */
  if(end + offset + diff + space <= w->wrap)
  {
    if(len2)
    {
      if(space)
        str[len] = ' ';
      memcpy(&str[len+space], str2, len2 + 1);
    }
    w->deladd--;
    delline_edittext(w, w->current->next);
    /* if reformatting? */
    if(space)
      return len2;	/* > 0 = next line had content */
    return 0;
  }

  /* the amount of white space delimited chars that can be moved at maximum
   * from the next line to current one.
   */
  offset = w->wrap - end - space;
  do
  {
    len2--;
    while(len2 > 0 && str2[len2]  > 32)
      len2--;
    while(len2 > 0 && str2[len2]  <= 32)
      len2--;
    if(!len2)
    {
      /* joining failed, return next line indentation */
      return -1;
    }
    len2++;
  } while(offset2window(w, str2, len2) + (len2 > tab_off ? diff : 0) > offset);

  if(space)
    str[len] = ' ';
  memcpy(&str[len + space], str2, len2);
  str[len + space + len2] = '\0';

  /* start of next word */
  while(str2[len2] && str2[len2] <= 32)
    len2++;
  memmove(str2, &str2[len2], strlen(&str2[len2]) + 1);
  if(w->last <= w->lineno)
    w->last = w->lineno+1;
  return 0;
}

/*
 * add new line after current line. if newline was added into middle of
 * string, move rest of string to the start of the new string. uses
 * the current line indentation for the new line (by indent_line()).
 */
static void
newline(edittext_widget_t *w)
{
  line_t *new, *old = w->current;

  if(!(new = newline_edittext(w, old)))
    return;

  /* cursor not at the line end */
  if(old->string[w->offset])
  {
    strcpy(new->string, &old->string[w->offset]);
    old->string[w->offset] = '\0';
  }
  w->last++;
  w->deladd++;
  w->offset = indent_next(w);
  w->current = new;
  w->lineno++;
}

static void
delline(edittext_widget_t *w)
{
  line_t *new;

  if(w->current->next == w->first)
  {
    w->offset = 0;
    w->current->string[0] = '\0';
    print_line(w, "", 0, w->lineno);
    return;
  }

  if((new = delline_edittext(w, w->current)))
  {
    w->offset = 0;
    w->current = new;
    w->deladd--;
  }
}

/*
 * insert a character into current cursor position.
 * call word wrap if necessary.
 */
static void
insert_edittext(edittext_widget_t *w, uchar key)
{
  int len, idx, tab, cur;
  uchar *str;

  /* wrap the line if needed */
  tab = w->tabsize;
  do
  {
    idx = w->offset;
    str = w->current->string;
    cur = offset2window(w, str, w->columns);
    if(key == '\t')
    {
      if(str[idx] == '\t')
	cur += tab;
      else
      {
        idx = offset2window(w, str, idx);
	cur += ((idx + tab) / tab) * tab - idx;
      }
    }
    else
    {
      if(str[idx] == '\t')
      {
        if((offset2window(w, str, idx) + 1) % tab == 0)
	  cur += tab;
      }
      else
        cur++;
    }

    if (cur <= w->wrap)
      break;
    if(wrap_line(w, key) <= 0)
      return;
  } while(tab > 1);

  idx = w->offset;
  str = w->current->string;
  len = strlen(&str[idx]);

  /* insert into middle? */
  if(len)
  {
    /* make room for the letter */
    memmove(&str[idx+1], &str[idx], len+1);
    str[idx] = key;
  }
  else
  {
    str[idx] = key;
    str[idx+1] = '\0';
  }
  w->offset++;
}

/*
 * delete one character from current position and modify line in the window
 * accordingly. check for line underflow (index=0) is done in process_key().
 */
static void
delete_edittext(edittext_widget_t *w)
{
  uchar *str = w->current->string;
  int len, idx;

  idx = w->offset;
  len = strlen(&str[idx]) - 1;
  if(len > 0)
    memmove(&str[idx], &str[idx+1], len+1);
  idx += len;
  str[idx] = '\0';
}

/* traverse downwards in paragraph setting line indents to one used on
 * invocation line, converting tabs on rest of line to spaces and joining
 * lines which aren't fully filled.
 */
static void format_edittext(edittext_widget_t *w)
{
  int index, do_line;
  uchar *str;

  /* skip indentation */
  str = w->current->string;
  while(*str && *str <= ' ')
    str++;
  if(!*str)
    return;

  /* convert current line tabs to spaces */
  while(*str)
  {
    if(*str == '\t')
      *str = ' ';
    str++;
  }

  w->offset = 0;
  for(;;)
  {
    do
    {
      do_line = 1;
      /* wrap line if it's too long */
      if(offset2window(w, w->current->string, w->columns) > w->wrap)
      {
        do
	{
	  if(wrap_line(w, 0) < 0)
	    break;
        } while(offset2window(w, w->current->string, w->columns) > w->wrap);
	do_line = 0;
      }
      if(w->current->next == w->first)
        goto break_format;

      str = w->current->next->string;
      /* remove next line indentation... */
      for(index = 0; str[index] && str[index] <= ' '; index++)
        ;
      if(index)
	memmove(str, &str[index], strlen(&str[index])+1);

      if(!*str)
        goto break_format;

      /* ...and convert it's tabs */
      while(*str)
      {
	if(*str == '\t')
	  *str = ' ';
	str++;
      }
      /* join stuff on next line to current line */
    } while(do_line && join_lines(w, 1) > 0);

    if(w->current->next == w->first)
      break;

    if(!line_content(w->current->next->string))
    {
      w->current = w->current->next;
      w->lineno++;
      break;
    }
    /* indent next line as current one */
    indent_next(w);

    w->current = w->current->next;
    w->lineno++;
  }

break_format:
  strcpy(w->undo, w->current->string);
  w->offset = strlen(w->current->string);
}

/*
 * processes all keyboard input.  cases handle cases when there needs to be
 * newline or one line deleted on the place, otherwise insert function will
 * check whether line got too long and acts accordingly.  cursor is off when
 * this is called.
 */
static int
process_key(edittext_widget_t *w, int key)
{
  int first, offset, count;
  line_t *current;

  w->deladd = 0;
  w->last = first = w->lineno;
  current = w->current;
  offset = w->offset;
  switch(key)
  {
    /* cursor movement */

    case KEY_LEFT:
    case WKEY_LEFT:
      if(offset)
      {
        w->offset--;
        w->position = offset2window(w, current->string, w->offset);
      }
      else if(current != w->first)
      {
        w->position = w->columns;
        cursor_move(w, 0, first - 1);
      }
      return 0;

    case KEY_RIGHT:
    case WKEY_RIGHT:
      if(current->string[offset])
      {
        w->offset++;	
        w->position = offset2window(w, current->string, w->offset);
      }
      else if(first < w->used - 1)
      {
        w->position = 0;
        cursor_move(w, 0, first + 1);
      }
      return 0;

    case KEY_HOME:
    case WKEY_HOME:
      w->position = w->offset = 0;
      return 0;

    case KEY_END:
    case WKEY_END:
      w->offset = strlen(current->string);
      w->position = offset2window(w, current->string, w->offset);
      return 0;

    case KEY_UP:
    case WKEY_UP:
      cursor_move(w, offset, first - 1);
      return 0;

    case KEY_DOWN:
    case WKEY_DOWN:
      cursor_move(w, offset, first + 1);
      return 0;

    case KEY_PGUP:
    case WKEY_PGUP:
      cursor_move(w, offset, first - w->height);
      return 0;

    case KEY_PGDOWN:
    case WKEY_PGDOWN:
      cursor_move(w, offset, first + w->height);
      return 0;


    /* clipboard action */

    case KEY_PASTE:
      {
	/* paste buffer to cursor position */
	w_selection_t *sel = w_getselection (W_SEL_TEXT);
	if (sel)
	{
	  w->text = sel->data;
	  /* does window refresh, undo stuff etc. */
	  parse_edittext(w, 1);
	  w_freeselection (sel);
	}
      }
      return 0;

    /* text actions */

    case '\n':
    case '\r':
      newline(w);
      strcpy(w->undo, w->current->string);
      break;

    case '\t':
      if(w->tabsize > 1)
        insert_edittext(w, '\t');
      else
      {
        /* convert tab to spaces */
        count = ((offset + DEF_TABSIZE) / DEF_TABSIZE) * DEF_TABSIZE;
	count -= offset;
	key = ' ';
        while(count--)
          insert_edittext(w, key);
      }
      break;

    /* character removal */

    case KEY_BS:
      if(offset)
      {
        w->offset = --offset;
	delete_edittext(w);
	break;
      }
      if(current == w->first)
        return 0;

      w->lineno--;
      w->current = current->prev;
      w->last = first = w->lineno;
      w->offset = offset = strlen(w->current->string);
      if(join_lines(w, -1) < 0)
        join_lines(w, 0);
      current = w->current;
      strcpy(w->undo, current->string);
      break;

    case KEY_DEL:
    case WKEY_DEL:
      if(current->string[offset])
      {
	delete_edittext(w);
	break;
      }
      if(join_lines(w, -1) < 0)
        join_lines(w, 0);
      current = w->current;
      break;

    case KEY_KILLINE:
      delline(w);
      strcpy(w->undo, w->current->string);
      current = w->current;
      offset = w->columns;	/* nothing to print */
      break;

    case KEY_TRANSPOSE:
      if(offset > 0 && offset < strlen(current->string))
      {
        uchar tmp = current->string[offset];
        current->string[offset] = current->string[offset-1];
	current->string[--offset] = tmp;
	w->offset++;
	break;
      }
      return 0;

    case KEY_FORMAT:
      /* format text until `empty' line or text ends */
      format_edittext(w);
      offset = 0;
      break;

    case KEY_UNDO:
      strcpy(w->tmp, current->string);
      strcpy(current->string, w->undo);
      strcpy(w->undo, w->tmp);
      cursor_move(w, w->offset, first);		/* reposition cursor */
      offset = 0;
      break;

    default:
      if(key < 32 || key > 255)
	return key;
      insert_edittext(w, key);
  }
  update_lines(w, current, offset, 1, first, w->last);
  return 0;
}

static uchar
process_keyevent(edittext_widget_t *w, int key)
{
  int state = w->cursor;
  cursor_off(w);

  /* remove selection if needed */
  if(w->inselect)
  {
    if(DESTRUCTIVE(key))
    {
      select_end(w, 0);
      select_end(w, 1);		/* remove selected */
      cursor_on(w);
      return 0;
    }
    if(MOVEMENT(key) || key == KEY_COPY || key == KEY_PASTE)
    {
      if(w->inselect != SEL_KEY || key == KEY_COPY || key == KEY_PASTE)
      {
	select_end(w, 0);
	unselect(w);		/* clear selected */
	cursor_on(w);
	return 0;
      }
    }
    else
    {
      select_end(w, 0);
      select_end(w, 1);		/* remove selected */
    }
    state = 1;
  }
  else
  {
    if(key == KEY_CUT || key == KEY_COPY)
    {
      select_here(w,
        offset2window(w, w->current->string, w->offset), w->lineno, SEL_KEY);
      return 0;
    }
  }

  key = process_key(w, key);
  if(w->inselect == SEL_KEY)
    select_there(w, offset2window(w, w->current->string, w->offset), w->lineno);
  else
  {
    if(state)
      cursor_on(w);
  }
  show_newpos(w);
  return key;
}

/*********** callbacks, text insert, line structure setups **********/

/* 
 * give buffer line strings to the user function for processing
 */
static long
process_edittext (edittext_widget_t * w, void *ptr)
{
  line_t *line;
  uchar *str;
  int idx;

  line = w->first;
  for (idx = 0; idx < w->used; idx++)
  {
    if((str = w->process_cb((widget_t *)w, line->string, w->columns, ptr)))
    {
      if(str != line->string)
	strncpy(line->string, str, w->columns);
      line->string[w->columns] = '\0';
      print_line(w, line->string, 0, idx);
    }
    line = line->next;
  }
  strcpy(w->undo, w->current->string);
  return 0;
}

static long
offset_edittext (edittext_widget_t * w, void *ptr)
{
  int offset;

  while((offset = w->offset_cb((widget_t *)w, w->current->string, w->lineno, ptr)))
  {
    if(offset < 0)
    {
      if(w->current == w->first)
        break;
      w->current = w->current->prev;
      w->lineno--;
    }
    else
    {
      if(w->current->next == w->first)
        break;
      w->current = w->current->next;
      w->lineno++;
    }
    w->offset = 0;
  }
  strcpy(w->undo, w->current->string);
  return 0;
}

/*
 * insert a null terminated text into the buffer. `to_end' should be used
 * whenever there's already something else in the buffer, then cursor will
 * be moved to the end of the insertation and text be justified to what's
 * set for justification instead of widget line lenght.
 */
static void
parse_edittext (edittext_widget_t * w, int to_end)
{
  int indent, changed, last, word, count, idx, end, i;
  uchar chr, *str, *text, *next, *first_str;
  line_t *line, *tmp, *new = 0;

  select_remove(w);
  if (!(w->text && *w->text))
    return;

  text = w->text;
  line = w->current;
  str = line->string;
  idx = w->offset;
  count = offset2window(w, str, idx);
  if(to_end)
    end = w->wrap;
  else
    end = w->columns;
  changed = 0;
  chr = 0;

  if(str[idx] && (new = newline_edittext(w, line)))
  {
    strcpy(new->string, &str[idx]);
    str[idx] = '\0';
  }
  first_str = str;
  indent = 0;
  if(w->indent)
  {
    while(str[indent] && str[indent] <= ' ')
      indent++;  
  }

  /* scan the text for line breaks etc. and copy it to buffer */
  for(;;)
  {
    chr = *text++;
    switch(chr)
    {
      case 0:
	goto text_done;

      case '\r':
	if (*text == '\n')
	  text++;
	break;

      case '\n':
	if (*text == '\r')
	  text++;
	break;

      case '\t':
	if(w->tabsize > 1)
	{
          count = ((count + w->tabsize) / w->tabsize) * w->tabsize;
	  str[idx++] = '\t';
	}
	else
	{
          i = ((count + DEF_TABSIZE) / DEF_TABSIZE) * DEF_TABSIZE;
	  /* convert tab to spaces */
	  while(count < i && count < end)
	  {
	    str[idx++] = ' ';
	    count++;
	  }
	}
	if (count > end)
	  break;
	continue;

      default:
	if(chr < 32)
	  continue;

	str[idx++] = chr;
	if(++count <= end)
	  continue;
    }

    /* check where line breaking is needed */
    last = idx;
    if(chr > 32)
    {
      while(--idx >= indent && str[idx] > 32);
      if(++idx <= indent)
        idx = last;
    }
    word = idx;

    /* string end / space removal */
    while(--idx >= 0 && str[idx] <= 32);
    if(++idx > 0)
      str[idx] = '\0';
    else
    {
      str[last] = '\0';
      word = last;	/* (n)one word on line (->no wrap) */
    }

    if(!(tmp = newline_edittext(w, line)))
      break;

    /* indent current & word wrap previous line */
    idx = 0;
    next = tmp->string;
    while(idx < indent) {
      next[idx] = first_str[idx];
      idx++;
    }
    while(word < last) {
      next[idx++] = str[word++];
    }
    count = idx;

    line = tmp;
    str = next;
    changed++;
  }

text_done:
  str[idx] = '\0';

  i = w->lineno;
  end = w->offset;
  tmp = w->current;

  /* try to join back */
  w->offset = idx;
  w->current = line;
  w->deladd = changed;
  w->lineno += changed;
  if(new)
  {
    w->deladd++;
    join_lines(w, -1);
  }
  w->last = i + w->deladd;

  /* if new lines */
  if(w->last != i)
    strcpy(w->undo, w->current->string);

  /* do not move cursor to insertation end? */
  if(!to_end)
  {
    w->current = tmp;
    w->offset = end;
    w->lineno = i;
  }

  /* show on screen */
  update_lines(w, tmp, end, 0, i, w->last);
  show_newpos(w);

  w->text = NULL;
}

/*
 * allocate more lines (a suitable amount)...
 */
static line_t *
alloc_lines(edittext_widget_t *w)
{
  line_t *line, *buffer;
  int lines, len, i;
  uchar *string;

  len = w->columns + 1;		/* string + null */

  lines = BLOCK_SIZE / (sizeof(line_t) + len);
  if(lines < w->height)
    lines = w->height;
  if(!(buffer = malloc(sizeof(line_t*) + (sizeof(line_t)+len) * lines)))
    return NULL;

  /* setup buffer pointers */
  *(line_t**)buffer = w->buffer;
  w->buffer = buffer++;

  line = buffer;
  string = (uchar*)(line + lines);
  for (i = 0; i < lines; i++)
  {
    line->prev = line - 1;
    line->next = line + 1;
    line->string = string;
    string += len;
    line++;
  }

  w->lines += lines;
  return buffer;
}

/* 
 * the next two function allocate and free a line, they will not
 * change / update screen.
 */
static line_t *
newline_edittext(edittext_widget_t *w, line_t *line)
{
  line_t *tmp;

  /* allocate new block if needed */
  if(w->used >= w->lines && !(w->free = alloc_lines(w)))
    return NULL;

  tmp = w->free->next;
  line->next->prev = w->free;
  w->free->next = line->next;
  line->next = w->free;
  w->free->prev = line;
  line = w->free;
  w->free = tmp;
  w->used++;

  line->string[0] = '\0';
  return line;
}

static line_t *
delline_edittext(edittext_widget_t *w, line_t *line)
{
  line_t *tmp;
  if(line->next == line)
  {
    line->string[0] = '\0';
    return line;
  }

  tmp = line->next;
  if(w->first == line)
    w->first = tmp;

  line->next->prev = line->prev;
  line->prev->next = line->next;
  w->free->prev = line;
  line->next = w->free;
  w->free = line;
  w->used--;

  return tmp;
}

/* 
 * initialize the editing buffer and scrollbar sizes
 */
static line_t *
setup_edittext(edittext_widget_t *w)
{
  line_t *line;
  long par;
  int len;

  unselect(w);

  /* free / zero current text */
  line = w->buffer;
  while(line)
  {
    w->buffer = line;
    line = *(line_t**)line;
    free(w->buffer);
  }
  w->free = w->current = w->buffer = NULL;
  w->lines = 0;

  if(w->tmp)
  {
    free(w->tmp);
    w->undo = w->tmp = NULL;
  }

  len = w->columns + 1;
  if(!(w->tmp = malloc(len * 2)))
    return NULL;

  w->undo = w->tmp + len;
  w->undo[0] = w->tmp[0] = 0;

  if(!(line = alloc_lines(w)))
    return NULL;

  w->free = line->next;
  w->top = w->lineno = w->position = w->offset = 0;
  w->current = w->first = line;
  w->first->prev = line;
  w->first->next = line;
  line->string[0] = 0;
  w->used = 1;

  par = w->height;
  (*wt_scrollbar_class->setopt) (w->scrollbar, WT_SIZE, &par);
  (*wt_scrollbar_class->setopt) (w->scrollbar, WT_PAGE_INC, &par);

  par = 1;
  (*wt_scrollbar_class->setopt) (w->scrollbar, WT_LINE_INC, &par);
  set_scrollbar(w);

  return w->first;
}

/************************** Widget Code ******************************/

static long edittext_query_geometry (widget_t *, long *, long *, long *, long *);

static long
edittext_init (void)
{
  return 0;
}

/*
 * load a new font
 */
static int
loadfont (edittext_widget_t *w, char *fname, short fsize)
{
  WFONT *fp;

  if (!(fp = wt_loadfont (fname, fsize, 0, 1)))
    return -1;

  w_unloadfont (w->font);
  w_setfont (w->w.win, fp);
  w->fsize = fsize;
  w->font = fp;
  return 0;
}

static widget_t *
edittext_create (widget_class_t * cp)
{
  edittext_widget_t *wp;
  long tmp;

  wp = calloc (1, sizeof(edittext_widget_t));
  if (!wp)
    return NULL;

  wp->w.class = wt_edittext_class;
  if (loadfont (wp, NULL, 0))
  {
    free (wp);
    return NULL;
  }

  wp->scrollbar = wt_create (wt_scrollbar_class, NULL);
  if (!wp->scrollbar)
  {
    w_unloadfont (wp->font);
    free (wp);
    return NULL;
  }
  wt_add_after((widget_t*)wp, NULL, wp->scrollbar);

  (*wt_scrollbar_class->setopt) (wp->scrollbar, WT_ACTION_CB, scroll_cb);
  (*wt_scrollbar_class->getopt) (wp->scrollbar, WT_WIDTH, &tmp);
  wp->scrollwd = tmp;

  wp->wrap = DEF_COLS;
  wp->columns = DEF_COLS;
  wp->height = DEF_ROWS;
  wp->tabsize = 1;
  wp->indent = 1;

  wp->seltimer = -1;
  wp->clicktime = time (NULL);
  return (widget_t *) wp;
}

static long
edittext_delete (widget_t * _w)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;
  line_t *tmp;

  wt_ungetfocus (_w);
  wt_remove (w->scrollbar);
  wt_delete (w->scrollbar);
  if (w->is_realized)
    w_delete (w->w.win);

  tmp = w->buffer;
  while(tmp)
  {
    w->buffer = tmp;
    tmp = *(line_t**)tmp;
    free(w->buffer);
  }
  if (w->tmp)
    free (w->tmp);

  w_unloadfont (w->font);
  free (w);
  return 0;
}

static long
edittext_close (widget_t * _w)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;

  if (w->is_realized && w->is_open)
  {
    wt_close (w->scrollbar);
    w_close (w->w.win);
    w->is_open = 0;
  }
  return 0;
}

static long
edittext_open (widget_t * _w)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;

  if (w->is_realized && !w->is_open)
  {
    wt_open (w->scrollbar);
    w_open (w->w.win, w->w.x, w->w.y);
    w->is_open = 1;
  }
  return 0;
}

static long
edittext_addchild (widget_t * parent, widget_t * child)
{
  return -1;
}

static long
edittext_delchild (widget_t * parent, widget_t * child)
{
  return -1;
}

static long
edittext_realize (widget_t * _w, WWIN * parent)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;
  long x, y, wd, ht;
  WWIN *win;

  if (w->is_realized)
    return -1;

  edittext_query_geometry (_w, &x, &y, &wd, &ht);
  win = wt_create_window (parent, wd - w->scrollwd - 2, ht,
	W_NOBORDER | W_MOVE | EV_KEYS | EV_MOUSE, _w);
  if (!win)
    return -1;

  w->w.w = wd;
  w->w.h = ht;
  w->w.win = win;
  win->user_val = (long) w;

  w_setmode(win, M_INVERS);
  w_hline(win, 2, 1, win->width - 2);
  w_vline(win, 1, 1, win->height - 2);
  w_setfont (win, w->font);

  if (!setup_edittext(w))
  {
    w_delete (win);
    return -1;
  }
  parse_edittext(w, 0);
  cursor_on(w);

  w->is_open = 1;
  w->is_realized = 1;

  wt_reshape (w->scrollbar, w->w.x + win->width + 2, w->w.y, WT_UNSPEC, w->w.h);
  (*wt_scrollbar_class->realize) (w->scrollbar, parent);
  w_open (win, w->w.x, w->w.y);
  return 0;
}

static long
edittext_query_geometry (widget_t * _w, long *xp, long *yp, long *wdp, long *htp)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;

  *xp = w->w.x;
  *yp = w->w.y;
  if (w->w.w > 0 && w->w.h > 0)
  {
    *wdp = w->w.w;
    *htp = w->w.h;
  }
  else
  {
    (*_w->class->query_minsize) (_w, wdp, htp);
  }
  w->w.w = *wdp;
  w->w.h = *htp;
  return 0;
}

static long min_height(edittext_widget_t *w)
{
  long wd, ht, hh;
  (*wt_scrollbar_class->query_minsize) (w->scrollbar, &wd, &ht);
  hh = YBORDER * 2 + w->font->height * w->height;
  return MAX(ht, hh);
}

static long
edittext_query_minsize (widget_t * _w, long *wdp, long *htp)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;
  int wd, ht;

  *wdp = w->usrwd;
  *htp = w->usrht;

  wd = XBORDER * 2 + w->font->maxwidth * w->columns + 2 + w->scrollwd;
  ht = min_height(w);

  if(*wdp < wd)
    *wdp = wd;
  if(*htp < ht)
    *htp = ht;
  return 0;
}

static long
edittext_reshape (widget_t * _w, long x, long y, long wd, long ht)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;
  long ret = 0;

  if (x != w->w.x || y != w->w.y)
  {
    if (w->is_realized)
      w_move (w->w.win, x, y);
    w->w.x = x;
    w->w.y = y;
    ret = 1;
  }
  if (wd != w->w.w || ht != w->w.h)
  {
    long minwd, minht;
    edittext_query_minsize (_w, &minwd, &minht);
    if (wd < minwd)
      wd = minwd;
    if (ht < minht)
      ht = minht;
    if (wd != w->w.w || ht != w->w.h)
    {
      w->w.w = wd;
      w->w.h = ht;
      if (w->is_realized)
      {
	w_resize (w->w.win, wd, ht);
	redraw_edittext(w);
      }
      ret = 1;
    }
  }
  wt_reshape(w->scrollbar, x + w->w.w - w->scrollwd, y, WT_UNSPEC, ht);
  return ret;
}

static long
edittext_setopt (widget_t * _w, long key, void *val)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;
  /* cursor state is restored so you can use cursor_off() freely.
   * note that only functions process_keyevent, redraw_edittext and
   * cursor_move manipulate cursor by themselves (showpos just calls
   * user callback)
   */
  int mask = 0, idx, state = w->cursor;

  switch (key)
  {
    case WT_XPOS:
      if (edittext_reshape (_w, *(long *) val, w->w.y, w->w.w, w->w.h))
	mask |= WT_CHANGED_POS;
      break;

    case WT_YPOS:
      if (edittext_reshape (_w, w->w.x, *(long *) val, w->w.w, w->w.h))
	mask |= WT_CHANGED_POS;
      break;

    case WT_WIDTH:
      w->usrwd = MAX (0, *(long *) val);
      if (edittext_reshape (_w, w->w.x, w->w.y, w->usrwd, w->w.h))
	mask |= WT_CHANGED_SIZE;
      break;

    case WT_HEIGHT:
      w->usrht = MAX (0, *(long *) val);
      if (edittext_reshape (_w, w->w.x, w->w.y, w->w.w, w->usrht))
	mask |= WT_CHANGED_SIZE;
      break;

    case WT_VT_WIDTH:
      if (w->is_realized || *(long *) val <= w->tabsize)
        return -1;
      w->columns = *(long *) val;
      if(w->wrap < w->columns)
        w->wrap = w->columns;
      break;

    case WT_VT_HEIGHT:
      if (*(long *) val < 4)
        return -1;
      w->height = *(long *) val;
      if (edittext_reshape (_w, w->w.x, w->w.y, w->w.w, MAX(w->w.h, min_height(w))))
	mask |= WT_CHANGED_SIZE;
      break;

    case WT_TEXT_INDENT:
      if(*(long *)val)
        w->indent = 1;
      else
        w->indent = 0;
      break;

    case WT_TEXT_WRAP:
      if(*(long *) val > w->tabsize && *(long *) val < w->columns)
        w->wrap = *(long *) val;
      else
        w->wrap = w->columns;
      break;

    case WT_TEXT_TAB:
      if (*(long *) val >= w->columns)
        return -1;
      w->tabsize = *(long *) val;
      if(w->tabsize < 1)
        w->tabsize = 1;
      if(w->is_realized)
        redraw_edittext(w);
      break;

    case WT_TEXT_LINE:
      if(w->is_realized)
      {
        cursor_move(w, window2offset(w, w->current->string, w->position), *(long *) val);
        show_newpos(w);
      }
      break;

    case WT_TEXT_COLUMN:
      if(w->is_realized)
      {
	cursor_off(w);
	idx = strlen(w->current->string);
	if(*(long*)val < idx)
	  w->offset = *(long *)val;
	else
	  w->offset = idx;
	w->position = offset2window(w, w->current->string, w->offset);
        show_newpos(w);
      }
      break;

    case WT_TEXT_CHAR:
      if(w->is_realized)
      {
        process_keyevent(w, *(long *)val);
        /* above does this, but returns cursor if there's a selection */
      }
      break;

    case WT_TEXT_CLEAR:
      w->text = NULL;
      if(w->is_realized)
      {
        cursor_off(w);
	if(!setup_edittext(w))
	{
	  fprintf(stderr, "Wt/edittext error: Text struct setup failed\n");
	  return -1;
	}
	for(idx = 0; idx < w->height; idx++)
	  print_line(w, "", 0, idx);
        show_newpos(w);
      }
      break;

    case WT_TEXT_APPEND:
      w->text = (uchar *)val;
      if (w->is_realized)
      {
        cursor_off(w);
 	parse_edittext (w, 0);
      }
      break;

    case WT_TEXT_INSERT:
      w->text = (uchar *)val;
      if (w->is_realized)
      {
        cursor_off(w);
 	parse_edittext (w, 1);
      }
      break;

    case WT_TEXT_SELECT:
      if(w->is_realized)
      {
	cursor_off(w);
        /* cursor has to be in the window area! */
        if(val && *(long*)val)
	{
	  idx = offset2window(w, w->current->string, w->offset);
	  select_here(w, idx, w->lineno, SEL_APP);
	  idx = offset2window(w, w->current->string, w->offset + *(long*)val);
	  select_there(w, idx, w->lineno);
	  select_end(w, 0);
	}
	else
	  unselect(w);
      }
      break;

    case WT_ACTION_CB:
      w->process_cb = (uchar *(*)(widget_t *, uchar*, int, void*)) val;
      break;

    case WT_OFFSET_CB:
      w->offset_cb = (int (*)(widget_t *, uchar*, int, void*)) val;
      break;

    case WT_CHANGE_CB:
      w->change_cb = (void (*)(widget_t *, int, int)) val;
      break;

    case WT_MAP_CB:
      w->map_cb = *(int (*)(widget_t *, int)) val;
      break;

    case WT_FONTSIZE:
      if (w->is_realized || loadfont (w, NULL, *(long *)val))
	return -1;
      break;

    case WT_FONT:
      /* at the moment font can be changed only before realization */
      if (w->is_realized || loadfont (w, (char *)val, w->fsize))
	return -1;
      break;

    default:
      return -1;
  }
  if(state)
    cursor_on(w);
  if (mask && w->is_realized)
    wt_change_notify (_w, mask);
  return 0;
}

static long
edittext_getopt (widget_t * _w, long key, void *val)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;
  int state = w->cursor;

  switch (key)
  {
    case WT_XPOS:
      *(long *) val = w->w.x;
      break;

    case WT_YPOS:
      *(long *) val = w->w.y;
      break;

    case WT_WIDTH:
      *(long *) val = w->w.w;
      break;

    case WT_HEIGHT:
      *(long *) val = w->w.h;
      break;

    case WT_VT_WIDTH:
      *(long *) val = w->columns;
      break;

    case WT_VT_HEIGHT:
      *(long *) val = w->height;
      break;

    case WT_FONT:
      *(WFONT **)val = w->font;
      break;

    case WT_TEXT_INDENT:
      *(long *) val = w->indent;
      break;

    case WT_TEXT_WRAP:
      *(long *) val = w->wrap;
      break;

    case WT_TEXT_TAB:
      *(long *) val = w->tabsize;
      break;

    case WT_TEXT_LINE:
      *(long *) val = w->lineno;
      break;

    case WT_TEXT_COLUMN:
      *(long *) val = w->offset;
      break;

    case WT_TEXT_CHAR:
      *(long *) val = w->current->string[w->offset];
      break;

    case WT_TEXT_APPEND:
    case WT_TEXT_INSERT:
      *(uchar **) val = w->current->string;
      break;

    case WT_TEXT_SELECT:
      *(long *) val = w->inselect;
      break;

    case WT_ACTION_CB:
      if(!w->is_realized)
        return -1;
      cursor_off(w);
      key = process_edittext(w, val);
      show_newpos(w);
      if(state)
        cursor_on(w);
      return key;

    case WT_OFFSET_CB:
      if(!w->is_realized)
        return -1;
      cursor_off(w);
      key = offset_edittext(w, val);
      show_newpos(w);
      if(state)
        cursor_on(w);
      return key;

    case WT_CHANGE_CB:
      *(void (**)(widget_t *, int, int)) val = w->change_cb;
      break;

    case WT_MAP_CB:
      *(int (**)(widget_t *, int)) val = w->map_cb;
      break;

    default:
      return -1;
  }
  return 0;
}

static WEVENT *
edittext_event (widget_t * _w, WEVENT * ev)
{
  edittext_widget_t *w = (edittext_widget_t *) _w;
  int x, y, key;
  long tm;

  switch (ev->type)
  {
    case EVENT_MPRESS:
      /* get keyboard focus */
      wt_getfocus (_w);
      cursor_off(w);

      /* new cursor position */
      x = (ev->x - XBORDER) / w->font->maxwidth;
      y = (ev->y - YBORDER) / w->font->height;

      if(ev->key == BUTTON_RIGHT)
      {
	limit_pos(w, &x, &y);
	w->position = x;
	cursor_move(w, window2offset(w, w->current->string, w->position), y);
	break;
      }
      if(ev->key != BUTTON_LEFT)
      {
        cursor_on(w);
        break;
      }

      tm = time (NULL);
      if (tm != w->clicktime)
      {
	w->clicktime = tm;
	w->nclicks = 0;
      }
      if(++w->nclicks == 1)
      {
        select_start (w, x, y);
        select_timer_cb ((long)w);
      }
      break;


    case EVENT_MRELEASE:
      if(ev->key == BUTTON_RIGHT)
      {
	process_key(w, KEY_PASTE);
	show_newpos(w);
	cursor_on(w);
	break;
      }
      if(ev->key != BUTTON_LEFT)
        break;

      /* new cursor position */
      x = (ev->x - XBORDER) / w->font->maxwidth;
      y = (ev->y - YBORDER) / w->font->height;

      switch(w->nclicks)
      {
	case 1:
	  select_drag(w, x, y);
	  select_end(w, 0);
	  break;
	case 2:
	  select_word (w, x, y);
	  break;
	case 3:
	  select_line (w, x, y);
          w->nclicks = 0;
	  break;
      }
      cursor_on(w);
      break;


    case EVENT_KEY:
      key = ev->key;
      if(w->map_cb)
        if(!(key = w->map_cb(_w, key)))
	  break;

      key = process_keyevent(w, key);
      /* if(key) return ev; */
      break;


    default:
      return ev;
    }

  return NULL;
}

static long
edittext_changes (widget_t * w, widget_t * w2, short changes)
{
  return 0;
}

static long
edittext_focus (widget_t *_w, int enter)
{
  WWIN *win = wt_widget2win(_w);
  w_box(win, 0, 0, win->width, win->height);
  return 0;
}

static widget_class_t _wt_edittext_class =
{
  "edittext", 0,
  edittext_init,
  edittext_create,
  edittext_delete,
  edittext_close,
  edittext_open,
  edittext_addchild,
  edittext_delchild,
  edittext_realize,
  edittext_query_geometry,
  edittext_query_minsize,
  edittext_reshape,
  edittext_setopt,
  edittext_getopt,
  edittext_event,
  edittext_changes,
  edittext_changes,
  edittext_focus
};

widget_class_t *wt_edittext_class = &_wt_edittext_class;

