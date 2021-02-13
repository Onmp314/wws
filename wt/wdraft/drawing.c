/* Draw for W
 *
 * draw.c implements argument parsing and window, event, callback
 * and keyboard shortcut handling for toolbar and drawing windows.
 *
 * w/ 1996 by Eero Tamminen
 */

#include <stdio.h>
#include "draft.h"

#ifdef __MINT__
long _stksize = 16384;
#endif

widget_t *Top;		/* for realizing */
WWIN *Current;		/* for saving */

/* 'Tool' contains the structure for toolbar callbacks
 *
 * when you'll want to add a new 'tool' to Draw, code the functions for
 * TOOLBAR structure that your tool needs, add the prototypes into end of
 * draft.h and the function pointers (names) into the structure below, onto
 * an appropiate place. recompile.
 */

static TOOLBAR Tool[] =
{
  { 'l', icon_line,  set_point, size_line,  move_line,  final_line,  line_attrib, 0 },
  { 'b', icon_box,   set_point, size_box,   move_box,   final_box,   line_attrib, 0 },
  { 'c', icon_circle,set_point, size_circle,move_circle,final_circle,line_attrib, 0 },
  { 'p', icon_poly,  set_points,size_poly,  move_poly,  final_poly,  line_attrib, FLAG_MULTI },
  { 'z', icon_zoom,  init_zoom, 0,          move_box,   zoom_block,  0,           FLAG_CALLNOW },
  { 'n', icon_new,   init_new,  0, 0, 0, 0, FLAG_NODRAW },
  /* toolbar end */
  { 0,0,0,0,0,0,0,0 }
};


#define TIME_INTERVAL	100
#define TOOL_SIZE	32	/* toolbar 'icon' sizes */
#define BORDER		5	/* border for 'icon's */


/* a huge load of local global variables */
static int
  Tools,			/* number of toolbar 'objects' */
  Selected,			/* current toolbar object in effect */
  Oldtool,			/* previously selected one */
  Oldx, Oldy,			/* previous co-ordinates */
  Move, Shape,			/* Drawing process state variables */
  OutMove, OutShape,		/* inactivated states */
  BrushMode,			/* Continous flow of graphics */
  Timer = -1,			/* timer 'widget' */
  Timeout = -1;			/* current event timeout value */

static widget_t *Toolbar;


/* local prototypes */
static void handle_timeout(long dummy);
static void toolbar_geometry(int size, long *width, long *height);
static void draw_toolbar(widget_t *w, int x, int y, int wd, int ht);
static WEVENT *handle_toolbar(widget_t *w, WEVENT *ev);
static void select_tool(int idx);
static void set_selection(WWIN *w, int new);
static int  selected(int x, int y);
static void remove_drawing(void);
static void handle_keys(int key);


int main(int argc, char *argv[])
{
  long width, height, mask;
  widget_t *toolbar;

  /* check how many tools there are */
  while(Tool[Tools].icon)
    Tools++;

  if(!(Top = wt_init()))
    return -1;
  if(!(toolbar = wt_create(wt_shell_class, Top)))
    return -1;
  if(!(Toolbar = wt_create(wt_drawable_class, toolbar)))
    return -1;

  wt_setopt (toolbar,
	     WT_ICON_STRING, "Toolbar",
	     WT_EOL);

  toolbar_geometry(TOOL_SIZE, &width, &height);
  mask = TOOL_PROPERTIES;
  wt_setopt (Toolbar,
  	     WT_WIDTH, &width,
	     WT_HEIGHT, &height,
	     WT_EVENT_MASK, &mask,
	     WT_DRAW_FN, draw_toolbar,
	     WT_EVENT_CB, handle_toolbar,
	     WT_EOL);

  if(wt_realize(Top) < 0)
    return -2;

  /* load images from the command line arguments */
  while(--argc)
    load_file(NULL, argv[argc]);

  wt_run();
  return 0;
}


static inline void remove_timeout(void)
{
  if(Timer >= 0)
  {
    wt_deltimeout(Timer);
    Timeout = -1;
    Timer = -1;
  }
}

/* Timeout callback time... */
static void handle_timeout(long dummy)
{
  short x, y;

  Timer = wt_addtimeout(Timeout, handle_timeout, 0L);

  w_querymousepos(Current, &x, &y);
  if(x == Oldx && y == Oldy)
    return;

  if(Move)
  {
    /* brushmode? */
    if(BrushMode)
    {
      Tool[Selected].final(Current, x, y);
      return;
    }
    else
    {
      Tool[Selected].move(Current, Oldx, Oldy);
      Tool[Selected].move(Current, x, y);		/* new position */
    }
  }
  else
  {
    Tool[Selected].shape(Current, Oldx, Oldy);
    Tool[Selected].shape(Current, x, y);		/* new size/shape */
  }
  Oldx = x, Oldy = y;
}


static WEVENT * handle_toolbar(widget_t *w, WEVENT *ev)
{
  switch(ev->type)
  {
    /* mouse presses */
    case EVENT_MPRESS:
      select_tool(selected(ev->x, ev->y));
      if(ev->key == BUTTON_RIGHT &&			/* config tool? */
	 Tool[Selected].attrib &&
	 Selected == selected(ev->x, ev->y))
	Tool[Selected].attrib();			/* set tool attribs */
      return NULL;

    case EVENT_KEY:
      handle_keys(ev->key & 0xFF);
      return NULL;
  }
  return ev;
}

static void toolbar_geometry(int size, long *width, long *height)
{
#ifdef HORIZONTAL
  *width  = (Tools+1)/2*TOOL_SIZE+(Tools+1)/2-1;
  *height = 2*TOOL_SIZE+1;
#else  
  *width  = 2*TOOL_SIZE+1;
  *height = (Tools+1)/2*TOOL_SIZE+(Tools+1)/2-1;
#endif
}

/* draw toolbar window */
static void draw_toolbar(widget_t *w, int x, int y, int width, int height)
{
  WWIN *win = wt_widget2win(w);
  int idx;

#ifdef HORIZONTAL
  w_hline(win, 0, TOOL_SIZE, (Tools+1)/2*TOOL_SIZE+(Tools+1)/2-2);
  for(idx = 0; idx < Tools; idx++)
  {
    if(idx&1)
      w_vline(win, (idx+1)/2*TOOL_SIZE + idx/2, 0, 2*TOOL_SIZE);
    Tool[idx].icon(win,
      idx/2*TOOL_SIZE + idx/2 + BORDER,
      (idx&1)*TOOL_SIZE + (idx&1) + BORDER,
      TOOL_SIZE - BORDER*2);
  }
#else
  w_vline(win, TOOL_SIZE, 0, (Tools+1)/2*TOOL_SIZE+(Tools+1)/2-2);
  for(idx = 0; idx < Tools; idx++)
  {
    if(idx&1)
      w_hline(win, 0, (idx+1)/2*TOOL_SIZE + idx/2, 2*TOOL_SIZE);
    Tool[idx].icon(win,
      (idx&1)*TOOL_SIZE + (idx&1) + BORDER,
      idx/2*TOOL_SIZE + idx/2 + BORDER,
      TOOL_SIZE - BORDER*2);
  }
#endif
  /* invert current tool */
  set_selection(win, Selected);
}

static void select_tool(int idx)
{
  WWIN *win = wt_widget2win(Toolbar);

  remove_drawing();				/* remove drawing */

  if(!(Tool[idx].flags & FLAG_RESUME))		/* need to resume current? */
    Oldtool = Selected;

  if(!(Tool[idx].flags & FLAG_NODRAW))
  {
    if(Tool[Selected].flags & FLAG_CALLNOW)	/* disable current? */
      Tool[Selected].init(Current, 0, 0, 1);

    set_selection(win, Selected);		/* invert old */
    set_selection(win, idx);			/* set new */
  }

  if(Tool[idx].flags & (FLAG_NODRAW | FLAG_CALLNOW))
    Tool[idx].init(Current, 0, 0, 0);

  BrushMode = Move = OutMove = Shape = OutShape = 0;
}

/* invert old & new toolbar object images */
static void set_selection(WWIN *win, int new)
{
  /* argument validity check */
  if(new < 0 || new >= Tools || !Tool[new].icon)
    return;

#ifdef HORIZONTAL
  w_pbox(win,
    new/2 * TOOL_SIZE + new/2 + 1,
    (new & 1) * TOOL_SIZE + (new & 1) + 1,
    TOOL_SIZE-2, TOOL_SIZE-2);
#else
  w_pbox(win,
    (new & 1) * TOOL_SIZE + (new & 1) + 1,
    new/2 * TOOL_SIZE + new/2 + 1,
    TOOL_SIZE-2, TOOL_SIZE-2);
#endif
  Selected = new;
}

/* convert toolbar co-ordinates into a toolbar object index */
static int selected(int x, int y)
{
#ifdef HORIZONTAL
  return (y / TOOL_SIZE + (x - (x / TOOL_SIZE)) / TOOL_SIZE * 2);
#else
  return (x / TOOL_SIZE + (y - (y / TOOL_SIZE)) / TOOL_SIZE * 2);
#endif
}


WEVENT * handle_drawing(widget_t *w, WEVENT *ev)
{
  switch(ev->type)
  {
    /* mouse presses */
    case EVENT_MPRESS:

      /* drawing... */
      Current = ev->win;
      if(ev->key == BUTTON_RIGHT)			/* cancel? */
      {
	/* two point or multipoint moving operation? */
	if(!((Tool[Selected].flags & FLAG_MULTI) && Shape))
	{
	  if(Tool[Selected].flags & FLAG_RESUME)		/* resume old tool? */
	    select_tool(Oldtool);
	  remove_drawing();
	  BrushMode = Move = Shape = 0;
	  remove_timeout();
	  return NULL;
	}
	/* multiple points, shaping done, fall through */
      }

      if(!(Shape || Move))				/* start shaping? */
      {
	Timeout = TIME_INTERVAL;
	Timer = wt_addtimeout(Timeout, handle_timeout, 0L);
	w_setmode(Current, M_INVERS);
	Oldx = ev->x, Oldy = ev->y;

	if(Tool[Selected].init(Current, Oldx, Oldy, 0))	/* enough? */
	  return NULL;

	if(Tool[Selected].shape)			/* shape it? */
	{
	  Tool[Selected].shape(Current, Oldx, Oldy);
	  Shape = 1;
	  return NULL;
	}
	/* no callback, fall through */
      }

      if(!Move)						/* shape ok */
      {
	if(Shape && Tool[Selected].shape)		/* remove? */
	  Tool[Selected].shape(Current, Oldx, Oldy);

	/* next multipoint? */
	if((Tool[Selected].flags & FLAG_MULTI) && ev->key == BUTTON_LEFT)
	{
	  Oldx = ev->x, Oldy = ev->y;
	  Tool[Selected].init(Current, Oldx, Oldy, 1);
	  Tool[Selected].shape(Current, Oldx, Oldy);
	  return NULL;
	}
	Tool[Selected].move(Current, Oldx, Oldy);
	Shape = 0;
	Move = 1;
	return NULL;
      }

      /* permanent place */
      if(Move)						/* remove? */
	Tool[Selected].move(Current, Oldx, Oldy);
      Oldx = ev->x, Oldy = ev->y;
      Tool[Selected].final(Current, Oldx, Oldy);
      if(Tool[Selected].flags & FLAG_RESUME)		/* resume old tool? */
      {
	select_tool(Oldtool);
	return NULL;
      }
      w_setmode(Current, M_INVERS);
      Tool[Selected].move(Current, Oldx, Oldy);	    
      return NULL;


    /* out of the window */
    case EVENT_INACTIVE:
      if(BrushMode)
	BrushMode = 0;
      else
      {
        remove_drawing();
	OutShape = Shape;
	OutMove = Move;
      }
      Move = Shape = 0;
      remove_timeout();					/* no timeouts... */
      return NULL;

    case EVENT_ACTIVE:
      Oldx = ev->x; Oldy = ev->y;			/* bogus values? */
      Shape = OutShape;
      Move = OutMove;
      if(Shape)
      {
	if(ev->win == Current)
	{
	  Tool[Selected].shape(Current, Oldx, Oldy);
	  Timeout = TIME_INTERVAL;
	  Timer = wt_addtimeout(Timeout, handle_timeout, 0L);
	}
	else
	  Shape = 0;
      }
      Current = ev->win;
      if(Move)					/* draw to win */
      {
	Tool[Selected].move(Current, Oldx, Oldy);
	Timeout = TIME_INTERVAL;
	Timer = wt_addtimeout(Timeout, handle_timeout, 0L);
      }
      return NULL;


    case EVENT_KEY:
      handle_keys(ev->key & 0xFF);
      return NULL;
  }
  return ev;
}

/* remove inverted drawing in process (if there such) from the window */
static void remove_drawing(void)
{
  if(Move)
    Tool[Selected].move(Current, Oldx, Oldy);
  else if(Shape)
    Tool[Selected].shape(Current, Oldx, Oldy);
}


static void handle_keys(int key)
{
  int idx;

  if(key == '\033')			/* ESC changes brush mode */
  {
    remove_timeout();
    if(BrushMode)
      BrushMode = Move = Shape = 0;
    else
    {
      BrushMode = 1;
      Timeout = 0;
      Timer = wt_addtimeout(Timeout, handle_timeout, 0L);
    }
    return;
  }
  for(idx = 0; idx < Tools; idx++)
  {
    if(Tool[idx].shortcut == key)
      select_tool(idx);
  }
}
