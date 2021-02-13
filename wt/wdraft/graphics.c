/* Draw for W
 * 
 * graphics.c implements the basic graphics primitives and
 * their attribute setting.
 * 
 * w/ 1996 by Eero Tamminen
 */

#include "draft.h"

#define MAX_POINTS	32	/* maximum points in polygon etc. */

#define FLAG_SOLID	1	/* graphics flags */
#define FLAG_PATTERN	2
#define FLAG_BORDER	4

static short
  Mode = M_DRAW,
  Flags;

static widget_t
  *Attrib;


static void set_draw(widget_t *w, int p)
{
  Mode = M_DRAW;
}
static void set_transparent(widget_t *w, int p)
{
  Mode = M_TRANSP;
}
static void set_clear(widget_t *w, int p)
{
  Mode = M_CLEAR;
}
static void set_inverse(widget_t *w, int p)
{
  Mode = M_INVERS;
}

static void set_pattern(widget_t *w, int p)
{
  Flags ^= FLAG_PATTERN;
}
static void set_solid(widget_t *w, int p)
{
  Flags ^= FLAG_SOLID;
}
static void set_border(widget_t *w, int p)
{
  Flags ^= FLAG_BORDER;
}

static void close_attrib(widget_t *w)
{
  Attrib = 0;
  wt_delete(w);
}

void line_attrib(void)
{
  long
    state;
  widget_t
    *hpane,
    *pane1, *draw, *transp, *clear, *inverse,
    *pane2, *pattern, *solid, *border;

  /* only one attribute window at the time */
  if(Attrib)
    return;

  Attrib  = wt_create(wt_shell_class, Top);
  hpane   = wt_create(wt_pane_class, Attrib);
  pane1   = wt_create(wt_pane_class, hpane);
  draw    = wt_create(wt_radiobutton_class, pane1);
  transp  = wt_create(wt_radiobutton_class, pane1);
  clear   = wt_create(wt_radiobutton_class, pane1);
  inverse = wt_create(wt_radiobutton_class, pane1);
  pane2   = wt_create(wt_pane_class, hpane);
  solid   = wt_create(wt_checkbutton_class, pane2);
  pattern = wt_create(wt_checkbutton_class, pane2);
  border  = wt_create(wt_checkbutton_class, pane2);

  wt_setopt(Attrib,
        WT_LABEL, "Attributes",
	WT_ACTION_CB, close_attrib,
	WT_EOL);
/* 
  state = OrientHorz;
  wt_setopt(hpane,
	WT_ORIENTATION, &state,
	WT_EOL);
*/
  state = AlignLeft;
  wt_setopt(pane1,
	WT_ALIGNMENT, &state,
	WT_EOL);

  wt_setopt(pane2,
	WT_ALIGNMENT, &state,
	WT_EOL);


  state = (Mode == M_DRAW ? ButtonStatePressed : ButtonStateReleased);
  wt_setopt(draw,
	WT_STATE, &state,
  	WT_LABEL, "Draw",
	WT_ACTION_CB, set_draw,
	WT_EOL);

  state = (Mode == M_TRANSP ? ButtonStatePressed : ButtonStateReleased);
  wt_setopt(transp,
	WT_STATE, &state,
  	WT_LABEL, "Transparent",
	WT_ACTION_CB, set_transparent,
	WT_EOL);

  state = (Mode == M_CLEAR ? ButtonStatePressed : ButtonStateReleased);
  wt_setopt(clear,
	WT_STATE, &state,
  	WT_LABEL, "Clear",
	WT_ACTION_CB, set_clear,
	WT_EOL);

  state = (Mode == M_INVERS ? ButtonStatePressed : ButtonStateReleased);
  wt_setopt(inverse,
	WT_STATE, &state,
  	WT_LABEL, "Invert",
	WT_ACTION_CB, set_inverse,
	WT_EOL);


  state = (Flags & FLAG_SOLID ? ButtonStatePressed : ButtonStateReleased);
  wt_setopt(solid,
	WT_STATE, &state,
  	WT_LABEL, "Filled",
	WT_ACTION_CB, set_solid,
	WT_EOL);

  state = (Flags & FLAG_PATTERN ? ButtonStatePressed : ButtonStateReleased);
  wt_setopt(pattern,
	WT_STATE, &state,
  	WT_LABEL, "Pattern",
	WT_ACTION_CB, set_pattern,
	WT_EOL);

  state = (Flags & FLAG_BORDER ? ButtonStatePressed : ButtonStateReleased);
  wt_setopt(border,
	WT_STATE, &state,
  	WT_LABEL, "Border",
	WT_ACTION_CB, set_border,
	WT_EOL);

  wt_realize(Top);
}


static void set_mode(WWIN *win)
{
  /* set mode etc. according to object attributes */
  w_setmode(win, Mode);
}


/* ------------------- */

static short x1, y1, x2, y2;

/* two point functions */
int set_point(WWIN *win, int x, int y, int next)
{
  x1 = x;
  y1 = y;
  return 0;
}


/* ------------------- */

void icon_line(WWIN *win, int x, int y, int size)
{
  w_line(win, x, y, x + size - 1, y + size - 1);
}

void size_line(WWIN *win, int x, int y)
{ 
  x2 = x;
  y2 = y;
  w_line(win, x1, y1, x, y);
}

void move_line(WWIN *win, int x, int y)
{
  w_line(win, x1 + x - x2, y1 + y - y2, x, y);
}

void final_line(WWIN *win, int x, int y)
{
  set_mode(win);
  if(Flags & FLAG_PATTERN)
    w_dline(win, x1 + x - x2, y1 + y - y2, x, y);
  else
    w_line(win, x1 + x - x2, y1 + y - y2, x, y);
}


/* ------------------- */

void icon_box(WWIN *win, int x, int y, int size)
{
  w_box(win, x, y, size, size);
}

void size_box(WWIN *win, int x, int y)
{
  x2 = (x1 > x ? x1 - x : x - x1 );	/* size */
  y2 = (y1 > y ? y1 - y : y - y1 );
  w_box(win, (x1 < x ? x1 : x), (y1 < y ? y1 : y), x2, y2);
}

void move_box(WWIN *win, int x, int y)
{
  w_box(win, x, y, x2, y2);
}

void final_box(WWIN *win, int x, int y)
{
  set_mode(win);

  if(Flags & FLAG_PATTERN)
  {
    if(Flags & FLAG_SOLID)
    {
      w_dpbox(win, x, y, x2, y2);
      if(Flags & FLAG_BORDER)
        w_box(win, x, y, x2, y2);
    }
    else
      w_dbox(win, x, y, x2, y2);
  }
  else
  {
    if(Flags & FLAG_SOLID)
      w_pbox(win, x, y, x2, y2);
    else
      w_box(win, x, y, x2, y2);
  }
}


/* ------------------- */

void icon_circle(WWIN *win, int x, int y, int size)
{
  w_circle(win, x + size / 2, y + size / 2, size / 2);
}

void size_circle(WWIN *win, int x, int y)
{
  x2 = (x1 > x ? x1 - x : x - x1 );	/* distance */
  y2 = (y1 > y ? y1 - y : y - y1 );
  if(x2 < y2)
    x2 = y2;
  w_circle(win, x1, y1, x2);
}

void move_circle(WWIN *win, int x, int y)
{
  w_circle(win, x, y, x2);
}

void final_circle(WWIN *win, int x, int y)
{
  set_mode(win);

  if(Flags & FLAG_PATTERN)
  {
    if(Flags & FLAG_SOLID)
    {
      w_dpcircle(win, x, y, x2);
      if(Flags & FLAG_BORDER)
        w_circle(win, x, y, x2);
    }
    else
      w_dcircle(win, x, y, x2);
  }
  else
  {
    if(Flags & FLAG_SOLID)
      w_pcircle(win, x, y, x2);
    else
      w_circle(win, x, y, x2);
  }
}


/* ------------------- */

static short Points, Coord[MAX_POINTS*2];

/* multiple point functions (polyline, polygon, bezier...) */
int set_points(WWIN *win, int x, int y, int next)
{
  if(next == 0)
    Points = 0;
  Coord[Points<<1] = x;
  Coord[(Points<<1)+1] = y;
  Points++;
  return 0;
}

/* ------------------- */

void icon_poly(WWIN *win, int x, int y, int size)
{
  w_line(win, x+6, y, x+size-1, y+size-1);
  w_line(win, x+size-1, y+size-1, x, y+size-7);
  w_line(win, x, y+size-7, x+6, y);
}

void size_poly(WWIN *win, int x, int y)
{
  Coord[Points<<1] = x;
  Coord[(Points<<1)+1] = y;
  if(Points < 2)
    w_line(win, Coord[0], Coord[1], x, y);	/* so that inverse shows... */
  else
    w_poly(win, Points+1, Coord);
}

static void absolute(int x, int y)
{
  int idx;

  x -= Coord[0];
  y -= Coord[1];
  for(idx = 0; idx < Points<<1;)
  {
    Coord[idx++] += x;
    Coord[idx++] += y;
  }
}

static void relative(int x, int y)
{
  int idx;

  for(idx = 0; idx < Points<<1;)
  {
    Coord[idx++] -= x;
    Coord[idx++] -= y;
  }
}

void move_poly(WWIN *win, int x, int y)
{
  absolute(x, y);

  if(Points < 2)
    w_line(win, Coord[0], Coord[1], Coord[2], Coord[3]);
  else
    w_poly(win, Points, Coord);

  relative(x, y);
}

void final_poly(WWIN *win, int x, int y)
{
  absolute(x, y);
  set_mode(win);

  if(Points < 2)
  {
    if(Flags & FLAG_PATTERN)
      w_dline(win, Coord[0], Coord[1], Coord[2], Coord[3]);
    else
      w_line(win, Coord[0], Coord[1], Coord[2], Coord[3]);
  }
  else
  {
    if(Flags & FLAG_PATTERN)
    {
      if(Flags & FLAG_SOLID)
      {
	w_dppoly(win, Points, Coord);
        if(Flags & FLAG_BORDER)
          w_poly(win, Points, Coord);
      }
      else
	w_dpoly(win, Points, Coord);
    }
    else
    {
      if(Flags & FLAG_SOLID)
	w_ppoly(win, Points, Coord);
      else
	w_poly(win, Points, Coord);
    }
  }

  relative(x, y);
}


/* ------------------- */
