/*
 * window.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Kay Roemer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- generalized nested window handling code for W
 *
 * CHANGES
 * ++tesche, 1/96:
 * - adapted and bug-fixed for W1R3
 * - implemented W_CONTAINER windows
 * ++eero, 3/03:
 * - SDL/GGI redraw hacks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "config.h"
#include "types.h"
#include "pakets.h"
#include "proto.h"
#include "hash.h"
#include "rect.h"
#include "window.h"


/* define to enable some currently unused code */
/* #define WINDOW_UNUSED_CODE 1 */

/*
 * some global variables
 */

WINDOW *glob_rootwindow;
WINDOW *glob_activewindow;
WINDOW *glob_activetopwin;
short globOpenWindows, globTotalWindows;
WINDOW *glob_backgroundwin;


/*
 * some private variables
 */

static WINDOW *topWindow = NULL;
static WINDOW *window_find_cache;
static hashtab_t *wintab;
static CLIENT dummyClient;


/*
 * return a yet unused window handle.
 */

static ushort window_newid (void)
{
  static ushort winid = 0;

  while (hash_lookup (wintab, winid))
    ++winid;

  return winid++;
}


/*
 * remove window `w' from the window tree
 */

static void wtree_remove (WINDOW *w)
{
  if (w->prev) {
    w->prev->next = w->next;
  } else if (w->parent) {
    w->parent->childs = w->next;
  }
  if (w->next) {
    w->next->prev = w->prev;
  }
}


/*
 * add the new window `neww' into the childlist of window parent
 * right after window `w'.
 */

static void wtree_add_after (WINDOW *parent, WINDOW *w, WINDOW *neww)
{
  neww->prev = w;
  neww->next = NULL;
  if (w) {
    if (w->next) {
      neww->next = w->next;
      w->next->prev = neww;
    }
    w->next = neww;
  }
  neww->parent = parent;
  if (!neww->prev) {
    parent->childs = neww;
  }
}


/*
 * add the new window `neww' into the childlist of window parent
 * right before window `w'.
 */

static void wtree_add_before (WINDOW *parent, WINDOW *w, WINDOW *neww)
{
  neww->next = w;
  neww->prev = NULL;
  if (w) {
    if (w->prev) {
      neww->prev = w->prev;
      w->prev->next = neww;
    }
    w->prev = neww;
  }
  neww->parent = parent;
  if (!neww->prev) {
    parent->childs = neww;
  }
}


/*
 * preorder-traverse the subtree starting at `wtree' and call function `f'
 * for every window visited. f's first argument is the window just beeing
 * visited and the second argument is the one you pass as the second argument
 * to wtree_traverse.
 *
 * return values for f:
 * <  0: something went wrong while visiting the window, abort traversal.
 * == 0: everything ok.
 * >  0: everything ok, but don't visit the childs of the window just visited.
 *
 * If something goes wrong (ie f returns < 0) then wtree_traverse_pre()
 * returns a pointer to the window for which f failed. Otherwise NULL is
 * returned.
 *
 * Because the windows have a backlink to its parent we can use this rather
 * fast non-recursive algorithm.
 */

WINDOW *wtree_traverse_pre (WINDOW *wtree, void *arg,
			    int (*f) (WINDOW *, void *))
{
  WINDOW *w;
  int visited, r = 0;

  for (visited = 0, w = wtree; w; r = 0) {
    if (!visited && (r = (*f) (w, arg)) < 0)
      /*
       * something went wrong while visiting `w'.
       */
      return w;

    if (!visited && !r && w->childs) {
      /*
       * descend, but only if we haven't visited
       * `w' yet and f returned 0.
       */
      w = w->childs;
      visited = 0;
    } else if (w == wtree) {
      /*
       * we are back at the top. all done.
       */
      w = NULL;
      visited = 0;
    } else if (w->next) {
      /*
       * go to the right brother.
       */
      w = w->next;
      visited = 0;
    } else {
      /*
       * go back to parent but mark it as already
       * visited, so we don't descend into its
       * subtrees again.
       */
      w = w->parent;
      visited = 1;
    }
  }
  return NULL;
}


/*
 * the same as wtree_traverse_pre(), except that the tree is traversed
 * in postorder instead of preorder.
 *
 * the algorithm used is nearly the same as for wtree_traverse_pre().
 * The only difference is when to visit a node (ie window)...
 *
 * We have to play tricks, because f may destroy a window and we cannot
 * access it anymore after f was called on it.
 *
 * return values for f:
 * <  0: something went wrong while visiting the window, abort traversal.
 * >= 0: everything ok.
 */

static WINDOW *wtree_traverse_post (WINDOW *wtree, void *arg,
				    int (*f) (WINDOW *, void *))
{
  WINDOW *w, *next_w;
  int visited, next_visited;

  for (visited = 0, w = wtree; w; visited = next_visited, w = next_w) {
    if (!visited && w->childs) {
      /*
       * descend, but only if we haven't visted
       * `w' yet. otherwise we will get an endless
       * loop.
       */
      next_w = w->childs;
      next_visited = 0;
    } else if (w == wtree) {
      /*
       * we are back at the top. all done.
       */
      next_w = NULL;
      next_visited = 0;
    } else if (w->next) {
      /*
       * go to the right brother.
       */
      next_w = w->next;
      next_visited = 0;
    } else {
      /*
       * go back to parent but mark it as already
       * visited, so we don't descend into its
       * subtrees again.
       */
      next_w = w->parent;
      next_visited = 1;
    }
    if ((visited || !w->childs) && (*f) (w, arg) < 0)
      /*
       * something went wrong while visiting `w'.
       */
      return w;
  }
  return NULL;
}


/*
 * traverses a list of subtrees and has otherwise the same characteristics
 * as wtree_traverse_pre (). Has to be very careful with pointers (hence
 * the `nextw' stuff).
 */

static WINDOW *wtree_list_traverse_pre (WINDOW *wtree, void *arg,
					int (*f) (WINDOW *, void *))
{
  WINDOW *badw, *nextw;

  for (; wtree; wtree = nextw) {
    nextw = wtree->next;
    badw = wtree_traverse_pre (wtree, arg, f);
    if (badw)
      return badw;
  }
  return NULL;
}


/*
 * same as wtree_list_traverse_pre() except that the subtrees in the list
 * are traversed in postoder instead of preorder. Has to be very careful
 * with pointers (hence the `nextw' stuff).
 */

static WINDOW *wtree_list_traverse_post (WINDOW *wtree, void *arg,
					 int (*f) (WINDOW *, void *))
{
  WINDOW *badw, *nextw;

  for (; wtree; wtree = nextw) {
    nextw = wtree->next;
    badw = wtree_traverse_post (wtree, arg, f);
    if (badw)
      return badw;
  }
  return NULL;
}


/*
 * build a list of rectangles that make up the visible area of window
 * `w'. stores a backup of the old rectangle list that can be discarded
 * with window_set_recs() and restored with window_restore_recs().
 *
 * also update the w->is_hidden field. this doesn't cost much, because
 * it is done by the rect_list_subtract() and rect_list_clip() routines.
 */

static int window_build_recs (WINDOW *w, void *notneeded)
{
  REC *rp, clipped_child;
  WINDOW *wp;
  int i;

  if (!w->is_open)
    return 0;

  w->is_hidden_backup = w->is_hidden;
  w->is_hidden = 0;

  w->vis_recs_backup = w->vis_recs;
  w->vis_recs = NULL;

  /*
   * if an ancestor of `w' is closed then the window is totally hidden, ie.
   * is_hidden is 1 and the rectangle list is empty.
   */
  for (wp = w->parent; wp; wp = wp->parent) {
    if (!wp->is_open) {
      w->is_hidden = 1;
      return 0;
    }
  }

  rp = rect_create (w->pos.x0, w->pos.y0, w->pos.w, w->pos.h);
  if (!rp)
    return -1;

  /*
   * The basic strategy here is to walk up the tree towards the
   * root starting at `w'.
   *
   * We start with a list of one rectangle (the area of the
   * window `w') and subtract all windows that we visit, except
   * the ancestors of `w'. We clip the rect list against those
   * ancestors of `w'.
   *
   * In a final step we subtract the areas of all the child windows
   * of `w' from the list.
   */
  for (wp = w; wp; wp = wp->parent) {
    /*
     * subtract the areas of all the left siblings of `wp'
     * from `rp'.
     */
    while (wp->prev) {
      wp = wp->prev;
      if (wp->is_open) {
	rp = rect_list_subtract (rp, &wp->pos, 1, &i);
	if (rp == RECT_ERROR)
	  return -1;
	w->is_hidden |= i;
      }
    }
    /*
     * clip `rp' to the interior of the window wp->parent.
     */
    if (wp->parent && wp->parent->is_open) {
      /*
       * The check for wp->parent->is_open
       * is somehow redundant...
       */
      rp = rect_list_clip (rp, &wp->parent->work, 1, &i);
      if (rp == RECT_ERROR)
	return -1;
      w->is_hidden |= i;
    }
  }
  /*
   * subtract the areas of w's childs from `rp'. But clip the child
   * to the working area of the window before subtracting.
   */
  for (wp = w->childs; wp; wp = wp->next) {
    if (!wp->is_open)
      continue;

    if (rect_intersect (&wp->pos, &w->work, &clipped_child)) {
      rp = rect_list_subtract (rp, &clipped_child, 1, &i);
      if (rp == RECT_ERROR)
	return -1;
      w->is_hidden |= i;
    }
  }
  w->vis_recs = rp;
  return 0;
}


/*
 * Rebuild the rectangle list for window `w' only, if the rectangle
 * list passed (`_recs') has a nonzero intersection with the window.
 */

static int window_rebuild_recs (WINDOW *w, void *_recs)
{
  REC *rp;

  if (!w->is_open)
    return 0;

  for (rp = _recs; rp; rp = rp->next) {
    if (rect_intersect (&w->pos, rp, NULL))
      break;
  }
  if (rp) {
    /*
     * found an intersection. rebuild.
     */
    return window_build_recs (w, NULL);
  }
  /*
   * No intersection. Therefore all of w's childs can't have
   * an intersection either. We return a value > 0. That will
   * keep wtree_traverse_pre() from visiting w's childs.
   */
  return 1;
}


/*
 * discards the backed up rect list for window `w'.
 */

static int window_set_recs (WINDOW *w, void *notneeded)
{
  if (w->vis_recs_backup) {
    rect_list_destroy (w->vis_recs_backup);
    w->vis_recs_backup = NULL;
    /*
     * kay: do NOT change w->is_hidden_backup here. We still need it in
     * window_redraw_contents_rect() to decide whether the window was
     * hidden before rebuilding the rectangles.
     */
  }
  return 0;
}


/*
 * restores the backed up rect list and discards the current one for
 * window `w'.
 */

static int window_restore_recs (WINDOW *w, void *notneeded)
{
  /*
   * only restore if there is a backup
   */
  if (w->vis_recs_backup) {
    if (w->vis_recs)
      rect_list_destroy (w->vis_recs);
    w->vis_recs = w->vis_recs_backup;
    w->vis_recs_backup = NULL;
    w->is_hidden = w->is_hidden_backup;
    w->is_hidden_backup = 0;
  }
  return 0;
}


/*
 * destroy both the current and the backed up list of rectangles of window `w'.
 */

static int window_delete_recs (WINDOW *w, void *notneeded)
{
  if (w->vis_recs_backup) {
    rect_list_destroy (w->vis_recs_backup);
    w->vis_recs_backup = NULL;
  }
  if (w->vis_recs) {
    rect_list_destroy (w->vis_recs);
    w->vis_recs = NULL;
  }
  w->is_hidden_backup = 0;
  return 0;
}


/*****************************************************************************/

/*
 * two dummy save/redraw routines
 */

static int dummy_window_save (WINDOW *notused, REC *notneeded)
{
  /* now this one's easy... :)
   */
  return 0;
}

static int dummy_window_redraw (WINDOW *w, REC *rp)
{
  /* this one's a bit more difficult. the idea is to walk through the list
   * of visible rectangles, intersect each of them to `rp', clip to that
   * intersection and completely redraw the window with all its gadgets.
   *
   * fortunately, the crawling through the visible rectangles is already
   * done elsewhere and we can be sure `rp' is already intersected against
   * them. so we simply use `rp' as clipping rectangle and redraw...
   *
   * the only thing we must really take care about is re-setting the clipping
   * rectangle after each operation, because some of them might change it
   * (like pbox).
   */

  gc0 = glob_cleargc;

  /* first: clear window */
  glob_clip0 = rp;
  (*glob_screen->pbox)(&glob_screen->bm, w->pos.x0, w->pos.y0,
		       w->pos.w, w->pos.h);

  if (w->flags & W_NOBORDER) {
    return 0;
  }
  windowDrawFrame (w, w != glob_activewindow && w != glob_activetopwin, 0, rp);

  /* third: the gadgets
   */
  gc0 = glob_drawgc;
  glob_clip0 = rp;

  if (w->flags & W_CLOSE) {
    (*glob_screen->line)(&glob_screen->bm,
			 w->pos.x0 + w->area[AREA_CLOSE].x0,
			 w->pos.y0 + w->area[AREA_CLOSE].y0,
			 w->pos.x0 + w->area[AREA_CLOSE].x1,
			 w->pos.y0 + w->area[AREA_CLOSE].y1);
    (*glob_screen->line)(&glob_screen->bm,
			 w->pos.x0 + w->area[AREA_CLOSE].x0,
			 w->pos.y0 + w->area[AREA_CLOSE].y1,
			 w->pos.x0 + w->area[AREA_CLOSE].x1,
			 w->pos.y0 + w->area[AREA_CLOSE].y0);
  }

  if (w->flags & W_ICON) {
    (*glob_screen->pbox)(&glob_screen->bm,
			 w->pos.x0 + w->area[AREA_ICON].x0 + (w->area[AREA_ICON].w >> 1) - 1,
			 w->pos.y0 + w->area[AREA_ICON].y0 + (w->area[AREA_ICON].h >> 1) - 1,
			 2, 2);
  }

  if (w->flags & W_TITLE) {
    (*glob_screen->dpbox)(&glob_screen->bm,
			  w->pos.x0 + w->area[AREA_TITLE].x0,
			  w->pos.y0 + w->area[AREA_TITLE].y0,
			  w->area[AREA_TITLE].w,
			  w->area[AREA_TITLE].h);
    (*glob_screen->prints)(&glob_screen->bm,
			   w->pos.x0 + w->area[AREA_TITLE].x0,
			   w->pos.y0 + w->area[AREA_TITLE].y0,
			   w->title);
  }
  return 0;
}


/*
 * two real save/redraw routines
 */

static int real_window_save (WINDOW *w, REC *rp)
{
  (*glob_screen->bitblk)(&glob_screen->bm,
			 rp->x0, rp->y0, rp->w, rp->h,
			 &w->bitmap,
			 rp->x0 - w->pos.x0, rp->y0 - w->pos.y0);

#ifdef SCREEN_REFRESH
  screen_update(rp);
#endif
  return 0;
}

static int real_window_redraw (WINDOW *w, REC *rp)
{
  (*glob_screen->bitblk)(&w->bitmap,
			 rp->x0 - w->pos.x0, rp->y0 - w->pos.y0, rp->w, rp->h,
			 &glob_screen->bm,
			 rp->x0, rp->y0);
#ifdef SCREEN_REFRESH
  screen_update(rp);
#endif
  return 0;
}

static int real_window_bitblk (REC *r1, REC *r2)
{
  (*glob_screen->bitblk)(&glob_screen->bm,
			 r1->x0, r1->y0, r1->w, r1->h,
			 &glob_screen->bm,
			 r2->x0, r2->y0);

#ifdef SCREEN_REFRESH
  {
    if (rect_intersect(r1, r2, NULL)) {
      REC r;
      /* slight optimization */
      rect_union(r1, r2, &r);
      screen_update(&r);
    } else {
      screen_update(r1);
      screen_update(r2);
    }
  }
#endif
  return 0;
}

/*****************************************************************************/


/*
 * TeSche: really save the complete contents of window `w' if at least a
 * part of it intersects with the rectangle in `_rect'.
 *
 * Kay: keeping the backing store bitmap up to date is a bit of trouble:
 *
 * If the window is partially hidden (w->is_hidden != 0) then we will
 * only draw into the backing bitmap, thus the backing bitmap is always
 * more up to date than the screen. Updating the screen from the bitmap is
 * done by calling windowRedrawAllIfDirty() in the main loop now and then.
 *
 * Now if the window becomes suddenly totally visible (w->is_hidden == 0)
 * we have to flush the bitmap to the screen before we draw anything into
 * the window (which will go directly onto the screen which is out of date
 * unless we do flush the bitmap to the screen)! To do this I have changed
 * window_redraw_contents_rect()!
 *
 * The other way round: When the window is totally visible we draw onto the
 * screen but not into the bitmap, thus the screen is always more up to date
 * than the backing bitmap. Updating the bitmap from the screen is done by
 * calling window_save_contents().
 *
 * Now if the window becomes partially hidden we have to save the screen to
 * the bitmap before we draw anything into the window. This saving is done
 * by the window_* functions.
 */

static int window_save_contents (WINDOW *w, void *_rect)
{
  REC *rp;

  /*
   * do not try to save hidden windows, because the bitmap is always
   * more up to date than the screen for them. for some reason (I do
   * not understand) it is possible to have w->is_dirty set for hidden
   * windows (perhaps due to the w->is_dirty = 1 in client_open()?).
   * therefore the explicit check for w->is_hidden.
   * kay.
   */
  if (!w->is_open || !w->is_dirty || w->is_hidden) {
    w->is_dirty = 0;
    return 0;
  }

  if (rect_intersect (&w->pos, _rect, NULL)) {
    /*
     * kay: save only the visible parts. currently the whole window should
     * be visible (because for partially hidden windows we draw into the
     * backing bitmap), but that my be changed some day...
     */
    for (rp = w->vis_recs; rp; rp = rp->next)
      (*w->save) (w, rp);
    w->is_dirty = 0;
  }

  return 0;
}


/*
 * save the contents of window `w' in the area of the rectangles
 * in list `_rect'.
 *
 * TeSche: this one is only used by external routines, like the menu.
 */

int window_save_contents_rect (WINDOW *w, void *_rect)
{
  REC isect, *rp, *rect;

  if (!w->is_open || !w->is_dirty || w->is_hidden) {
    w->is_dirty = 0;
    return 0;
  }

  for (rect = _rect; rect; rect = rect->next) {
    /*
     * kay: the window is no longer dirty if we save
     * a rectangle that contains the whole window.
     */
    if (rect_cont_rect (rect, &w->pos))
      w->is_dirty = 0;

    for (rp = w->vis_recs; rp; rp = rp->next) {
      if (rect_intersect (rp, rect, &isect)) {
	/*
	 * save contents of rectangle `isect'
	 */
	(*w->save) (w, &isect);
      }
    }
  }
  return 0;
}


/*
 * redraw the contents of window `w' in the area of the rectangles
 * in list `_rect'.
 */

int window_redraw_contents_rect (WINDOW *w, void *_rect)
{
  REC r, isect, *rp, *rect;

  if (!w->is_open)
    return 0;

  if (!w->is_hidden && w->dirty.w > 0 && w->dirty.h > 0) {
    /*
     * the window has just become totally visible and the screen is out
     * of date. flush bitmap to the screen.
     * beware: w->dirty is window relative, but w->redraw() wants absolute
     * coords.
     */
    r = w->dirty;
    r.x0 += w->pos.x0;
    r.x1 += w->pos.x0;
    r.y0 += w->pos.y0;
    r.y1 += w->pos.y0;
    for (rp = w->vis_recs; rp; rp = rp->next) {
      if (rect_intersect (rp, &r, &isect)) {
      	(*w->redraw) (w, &isect);
      }
    }
    w->dirty.w = w->dirty.h = 0;
  }
  for (rect = _rect; rect; rect = rect->next) {
    for (rp = w->vis_recs; rp; rp = rp->next) {
      if (rect_intersect (rp, rect, &isect)) {
	/*
	 * redraw contents of rectangle `isect'
	 */
	(*w->redraw) (w, &isect);
      }
    }
  }
  return 0;
}


#ifdef WINDOW_UNUSED_CODE
/*
 * return nonzero if the given point is inside the visible parts of
 * the window.
 */

static int window_cont_point (WINDOW *w, REC *_point)
{
  REC *rp, *point = _point;

  if (!w->is_open)
    return 0;
  for (rp = w->vis_recs; rp; rp = rp->next) {
    if (rect_cont_point (rp, point->x0, point->y0))
      return -1;
  }
  return 0;
}
#endif


/*
 * determine whether window `w' is hidden (ignoring child windows of
 * `w', therefore we can't use w->is_hidden).
 *
 * the structure is basically the same as for window_build_recs().
 */

static int window_is_hidden (WINDOW *w)
{
  WINDOW *wp;

  for (wp = w; wp; wp = wp->parent) {
    while (wp->prev) {
      wp = wp->prev;
      if (!wp->is_open)
	continue;
      if (rect_intersect (&w->pos, &wp->pos, NULL))
	/*
	 * `w' is covered by another window
	 */
	return 1;
    }
    if (wp->parent) {
      if (!wp->parent->is_open || !rect_cont_rect (&wp->parent->work, &w->pos))
      /*
       * either one of w's parents is closed (so it is not visible at all)
       * or `w' is not totally inside one of its ancestors.
       */
      return 1;
    }
  }
  return 0;
}


/*
 * initialize the root window and other things.
 */

long window_init (void)
{
  /* init dummy client
   */
  dummyClient.sh = -1;

  /* initialize the hash table. 211 is just a reasonable
   * large prime number.
   */
  if (!(wintab = hashtab_create (211))) {
    return -1;
  }

  /* initialize the root window, it's just a pseudo window (representing the
   * whole screen) which will never have any rectangles and therefore will
   * never be 'visible' in our context as it will always be occluded by the
   * background window, but we need it to carry W_TOP childs. the background
   * window is a child of this one too, and all other windows are childs of
   * the background window.
   */
  if (!(glob_rootwindow = malloc(sizeof(WINDOW))))
    return -1;

  memset(glob_rootwindow, 0, sizeof(WINDOW));
  glob_rootwindow->cptr = &dummyClient;

  /* w->pos holds position and size of the whole window
   */
  glob_rootwindow->pos.x1 = (glob_rootwindow->pos.w=glob_screen->bm.width)-1;
  glob_rootwindow->pos.y1 = (glob_rootwindow->pos.h=glob_screen->bm.height)-1;

  /* w->work holds position and size of the working area of the window. no
   * border for the root window.
   */
  glob_rootwindow->area[AREA_WORK] = glob_rootwindow->work =
    glob_rootwindow->pos;
  glob_rootwindow->is_open = 1;
  glob_rootwindow->is_hidden = 1;
  glob_rootwindow->save = NULL;
  glob_rootwindow->redraw = NULL;
  glob_rootwindow->bitblk = NULL;

  /* The root window has always id 1
   */
  glob_rootwindow->id = 1;
  if (hash_insert (wintab, glob_rootwindow->id, glob_rootwindow)) {
    free (glob_rootwindow);
    hashtab_delete (wintab, NULL);
    return -1;
  }

  glob_rootwindow->bitmap = glob_screen->bm;
  glob_rootwindow->gc.pattern = DefaultPattern;
  glob_rootwindow->gc.drawmode = M_INVERS;

  /* the rootwindow will always have the global color table.
   */
  glob_rootwindow->colTab = glob_colortable;

  /*
   * now comes the background window
   */

  if (!(glob_backgroundwin = malloc (sizeof (WINDOW)))) {
    free (glob_rootwindow);
    hashtab_delete (wintab, NULL);
    return -1;
  }
  *glob_backgroundwin = *glob_rootwindow;
  set_defaultgc(glob_backgroundwin);

  /* manually link in into the chain
   */
  glob_rootwindow->childs = glob_backgroundwin;
  glob_backgroundwin->parent = glob_rootwindow;

  /* The background has always id 0
   */
  glob_backgroundwin->id = 0;
  if (hash_insert (wintab, glob_backgroundwin->id, glob_backgroundwin)) {
    free (glob_backgroundwin);
    free (glob_rootwindow);
    hashtab_delete (wintab, NULL);
    return -1;
  }

  /* Initially there are no other windows, so the rect list
   * consists of just one rectangle (the window itself).
   */
  glob_backgroundwin->vis_recs =
    rect_create (glob_backgroundwin->pos.x0, glob_backgroundwin->pos.y0,
		 glob_backgroundwin->pos.w, glob_backgroundwin->pos.h);

  if (!glob_backgroundwin->vis_recs) {
    free (glob_backgroundwin);
    free (glob_rootwindow);
    hashtab_delete (wintab, NULL);
    return -1;
  }
  glob_backgroundwin->is_hidden = 0;
  glob_backgroundwin->is_open = 1;

#if 0
  /* this ensures we get a private copy of the startup colortable
   */
  colorChangeColor(glob_backgroundwin, 0, 255, 255, 255);
#endif

  /* those two functions save and redraw portions of the root
   * window. They have to be replaced with something "real" later
   */
  glob_backgroundwin->save = real_window_save;
  glob_backgroundwin->redraw = real_window_redraw;

  /* allocate memory for background
   */
  if (!(*glob_screen->createbm)(&glob_backgroundwin->bitmap,
	glob_screen->bm.width, glob_screen->bm.height, 1)) {
    fprintf(stderr, "fatal: not enough memory to initialize background window\n");
    free (glob_backgroundwin);
    free (glob_rootwindow);
    hashtab_delete (wintab, NULL);
    return -1;
  }

  /* initialize background window
   */
  glob_clip0 = glob_clip1 = &glob_backgroundwin->pos;
  (*glob_screen->bitblk)(&glob_screen->bm, 0, 0,
			 glob_screen->bm.width, glob_screen->bm.height,
			 &glob_backgroundwin->bitmap, 0, 0);

  glob_activewindow = glob_backgroundwin;
  glob_activetopwin = glob_backgroundwin;
  globOpenWindows = globTotalWindows = 0;

  return 0;
}


/*
 * create a new childwindow of window `parent'.
 */

WINDOW *window_create (WINDOW *parent, int flags)
{
  WINDOW *w;

  if ((flags & W_TOP) && topWindow)
    /* you can have only one W_TOP window */
    return NULL;

  if (!(w = malloc (sizeof (WINDOW))))
    return NULL;

  memset (w, 0, sizeof (WINDOW));

  w->id = window_newid ();
  if (hash_insert (wintab, w->id, w)) {
    free (w);
    return NULL;
  }

  /* insert the new window into the tree on top of the childs of `parent'.
   */
  wtree_add_before (parent, parent->childs, w);

  /* those two functions save and redraw portions of normal windows.
   */
  if (flags & W_CONTAINER) {
    w->save = dummy_window_save;
    w->redraw = dummy_window_redraw;
    w->bitblk = real_window_bitblk;
  } else {
    w->save = real_window_save;
    w->redraw = real_window_redraw;
    w->bitblk = real_window_bitblk;
  }

  if (flags & W_TOP)
    topWindow = w;

  w->flags = flags;

  return w;
}

/*
 * tell the upper level if we (really) open a window (ie it has no closed
 * ancestors).
 */
static int window_open_one (WINDOW *w, void *notneeded)
{
  WINDOW *wp;

  for (wp = w; wp; wp = wp->parent) {
    if (!wp->is_open)
      /*
       * this window and all its childs won't become visible
       */
      return 1;
  }
  client_do_open (w);
  return 0;
}

/*
 * this one moves an open window according to the given offset
 */
static int window_move_one (WINDOW *w, void *_offs);

/*
 * open a window (or map it onto the screen). You can even open windows
 * that have closed ancestors. But they will not become visible until
 * all their ancestors are opened.
 */

long window_open (WINDOW *w, int x, int y)
{
  REC offs;

  if (w->is_open)
    return 0;

  /* root window cannot be mapped
   */
  if (!w->parent)
    return -1;

  offs.x0 = (x - w->pos.x0);
  offs.y0 = (y - w->pos.y0);

  w->work.x0 += offs.x0;
  w->work.y0 += offs.y0;
  w->work.x1 += offs.x0;
  w->work.y1 += offs.y0;

  w->pos.x0 = x;
  w->pos.y0 = y;
  w->pos.x1 = x + w->pos.w - 1;
  w->pos.y1 = y + w->pos.h - 1;

  w->is_open = 1;

  /* Next we do backing storage for all the windows that may be
   * obscured by mapping the new window onto the screen.
   *
   * TeSche: but if so, then save the complete window, not just the part
   * which may be occluded - that would lead to having part of the window
   * up-to-date on screen and the other part up-to-date in the backing
   * store. one could deal with this in the drawing routines, especially
   * if they do proper clipping, but it would cost them a lot more time.
   */
  wtree_list_traverse_pre (w->next, &w->pos, window_save_contents);
  window_save_contents (w->parent, &w->pos);

  /*
   * move the childs that were "pseudo-closed" so their relative position
   * to the parent stays the same.
   */
  wtree_list_traverse_pre (w->childs, &offs, window_move_one);

  /*
   * rebuild the rectangle list for the window, its childs, right siblings
   * and the parent.
   *
   * argl! window_rebuild_recs() may return something > 0 which is
   * not an error condition. We did erronously check for != 0.
   */
  if (wtree_list_traverse_pre (w, &w->pos, window_rebuild_recs) ||
      window_rebuild_recs (w->parent, &w->pos) < 0) {
    /*
     * something went wrong, restore the old
     * rectangles.
     */
    wtree_list_traverse_pre (w, NULL, window_restore_recs);
    window_restore_recs (w->parent, NULL);
    w->is_open = 0;
    return -1;
  }
  wtree_list_traverse_pre (w, NULL, window_set_recs);
  window_set_recs (w->parent, NULL);

  /*
   * and now tell the upper level which windows will appear on the screen
   */
  wtree_traverse_pre (w, NULL, window_open_one);

  /*
   * and finally draw the contents of the new window and its childs
   * (including the border, hence w->pos, not w->work).
   */
  wtree_traverse_pre (w, &w->pos, window_redraw_contents_rect);

  return 0;
}

/*
 * close one window. All its subwindows are assumed to be unmapped
 * already (the postorder traversal guarantees this).
 */

static int window_close_one (WINDOW *w, void *top_win)
{
  WINDOW *wp;

  /*
   * don't care about windows that are closed already or have a closed
   * ancestor (ie are pseudo-closed).
   */
  for (wp = w; wp; wp = wp->parent) {
    if (!wp->is_open)
      return 0;
  }

  /*
   * first save the contents of the window beeing unmapped
   */
  window_save_contents (w, &w->pos);

  /*
   * delete the rectangle list to save memory
   */
  window_delete_recs (w, NULL);

  /*
   * tell the upper level that this window is beeing closed.
   */
  client_do_close (w);

  if (w == (WINDOW *)top_win) {
    /*
     * this is the window window_close() was called with.
     */
    w->is_open = 0;
  } else {
    /*
     * this is an open child window. it becomes invisible now.
     */
    w->is_hidden = 1;
  }
  return 0;
}


/*
 * Close (or unmap) a window. This implies closing all the subwindows first.
 */

long window_close (WINDOW *w)
{
  WINDOW *wp;

  if (!w->is_open)
    return 0;

  /* cannot unmap the root window
   */
  if (!w->parent)
    return -1;

  /*
   * don't care about windows that are pseudo-closed already
   */
  for (wp = w->parent; wp; wp = wp->parent) {
    if (!wp->is_open)
      return 0;
  }

  /* first unmap all subwindows, this will also delete any rectangle
   * lists for them.
   */
  wtree_traverse_post (w, w, window_close_one);

  /* TeSche: but we mustn't delete the rectangle lists of all (logically)
   * lower windows here, because `window_rebuild_recs' only rebuilds them
   * for windows intersecting the position of the closed window - and that
   * mustn't necessarily be true for all lower windows!
   */

  /* all windows in the subtree rooted at `w' are (virtually)
   * unmapped now. We have to update the rectangle lists and
   * redraw the contents of the right siblings and the parent
   * of `w'.
   *
   * start by rebuilding the rectangle lists for those windows.
   */
  if (wtree_list_traverse_pre (w->next, &w->pos, window_rebuild_recs) ||
      window_rebuild_recs (w->parent, &w->pos) < 0) {
    /* too bad, something went miserably wrong.
     */
    terminate(SIGHUP, "out of memory in window_close()");
  }

  /* still we've got to set the new rectangles.
   */
  wtree_list_traverse_pre (w->next, NULL, window_set_recs);
  window_set_recs (w->parent, NULL);

  /* and now redraw the area exposed by unmapping `w'.
   */
  wtree_list_traverse_pre (w->next, &w->pos, window_redraw_contents_rect);
  window_redraw_contents_rect (w->parent, &w->pos);

  return 0;
}


/*
 * delete one window. we assume that it has no subwindows anymore
 * (the postorder traversal guarantees this).
 */

static int window_delete_one (WINDOW *w, void *notneeded)
{
  if (w->childs) {
    wserver_exit(-1, "window_delete_one(): deleting window with childs");
  }

  hash_delete (wintab, w->id);

  /*
   * danger: wtree_traverse_post() has to be very careful
   * with the pointer handling, because we remove the windows
   * from the tree under its feet.
   */

  /*
   * rearrange the pointers in the tree thus removing `w'
   * from it.
   */
  wtree_remove (w);

  /*
   * and free the memory used by the window. Everything else
   * has been done by window_close() already.
   */
  window_delete_recs (w, NULL);

  /*
   * invalidate the window-find-cache if the cached window is
   * deleted.
   */
  if (window_find_cache == w)
    window_find_cache = NULL;

  /*
   * tell the upper level that this window is beeing closed. Among
   * other things this will delete the backing bitmap if the window
   * has one.
   */
  client_do_delete (w);

  /* if it was the W_TOP window, mark it as released
   */
  if ((w->flags & W_TOP) && (w == topWindow)) {
    topWindow = NULL;
  }

  free (w);

  return 0;
}


/*
 * Delete the window `w'. This implies deleting all the subwindows
 * of it first.
 */

long window_delete (WINDOW *w)
{
  if (!w->parent)
    return -1;

  if (w->is_open)
    /*
     * window_close() either succeeds or panics.
     */
    window_close (w);

  wtree_traverse_post (w, NULL, window_delete_one);

  return 0;
}


/*
 * moves one window according to the given offset.
 */

static int window_move_one (WINDOW *w, void *_offs)
{
  REC *offs = _offs;

  if (w->is_open) {
    w->pos.x0 += offs->x0;
    w->pos.y0 += offs->y0;
    w->pos.x1 += offs->x0;
    w->pos.y1 += offs->y0;

    w->work.x0 += offs->x0;
    w->work.y0 += offs->y0;
    w->work.x1 += offs->x0;
    w->work.y1 += offs->y0;
  }

  return 0;
}


/*
 * moves the rectangles of one window according to the given offset.
 */

static int window_move_recs (WINDOW *w, void *_offs)
{
  REC *rp, *offs = _offs;

  if (w->is_open) {
    for (rp = w->vis_recs; rp; rp = rp->next) {
      rp->x0 += offs->x0;
      rp->y0 += offs->y0;
      rp->x1 += offs->x0;
      rp->y1 += offs->y0;
    }
  }

  return 0;
}


/*
 * move a window to the given absolute position
 */

long window_move (WINDOW *w, int x, int y)
{
  REC offs, oldpos, newpos;
  int is_hidden;
  WINDOW *wp;

  if (!w->is_open || !w->parent)
    /*
     * moving closed windows or the root is nonsense.
     */
    return -1;

  if (x == w->pos.x0 && y == w->pos.y0)
    return 0;

  /* determine whether the window is hidden (ignoring child windows,
   * therefore we can't use w->is_hidden).
   */
  is_hidden = window_is_hidden (w);

  oldpos = w->pos;
  offs.x0 = x - w->pos.x0;
  offs.y0 = y - w->pos.y0;

  /* save screen to bitmap for:
   * - window beeing moved (lets call this one W) and its childs
   * - siblings below W if they intersect the new position of W
   * - parent of W if it intersects new position of W
   */
  wtree_traverse_pre (w, &w->pos, window_save_contents);
  newpos = w->pos;
  newpos.x0 += offs.x0;
  newpos.x1 += offs.x0;
  newpos.y0 += offs.y0;
  newpos.y1 += offs.y0;
  wtree_list_traverse_pre (w->next, &newpos, window_save_contents);
  window_save_contents (w->parent, &newpos);

  /* repostion the window and all its subwindows
   */
  wtree_traverse_pre (w, &offs, window_move_one);

  /* determine whether the window is hidden after the move.
   */
  is_hidden += window_is_hidden (w);

  /* and rebuild the rectangle lists for the window,
   * its subwindows and rights siblings.
   */
  if (!is_hidden) {
    /* the window is neither hidden before nor after the move.
     * we can just shift the rectangles for `w' and its childs.
     */
    wtree_traverse_pre (w, &offs, window_move_recs);
    wp = w->next;
  } else {
    /* have to rebuilt
     */
    wp = w;
  }
  oldpos.next = &w->pos;
  if (wtree_list_traverse_pre (wp, &oldpos, window_rebuild_recs) ||
      window_rebuild_recs (w->parent, &oldpos) < 0) {
    /*
     * something went wrong. Reverse everything we have
     * done so far.
     */
    wtree_traverse_pre (wp, NULL, window_restore_recs);
    window_restore_recs (w->parent, NULL);
    offs.x0 = -offs.x0;
    offs.y0 = -offs.y0;
    wtree_traverse_pre (w, &offs, window_move_one);
    if (!is_hidden)
      wtree_traverse_pre (w, &offs, window_move_recs);
    return -1;
  }
  wtree_list_traverse_pre (wp, NULL, window_set_recs);
  window_set_recs (w->parent, NULL);

  /* redraw the interior of the window.
   */
  if (!is_hidden) {
    /*
     * The window was not hidden before and
     * after the move, so we can bitblit()
     * the window to the new position instead of
     * redrawing
     */
    (*w->bitblk) (&oldpos, &w->pos);
  } else {
    /*
     * do a (slow) redraw
     */
    wtree_traverse_pre (w, &w->pos, window_redraw_contents_rect);
  }

  /*
   * and now redraw the area that was exposed by moving the
   * window
   */
  oldpos.next = NULL;
  wtree_list_traverse_pre (w->next, &oldpos, window_redraw_contents_rect);
  window_redraw_contents_rect (w->parent, &oldpos);

  return 0;
}


#ifdef WINDOW_UNUSED_CODE
/*
 * resize a window to the given size.
 */

long window_resize (WINDOW *w, int wd, int ht)
{
  REC oldpos, oldarea, *rp;

  if (!w->parent || wd <= 0 || ht <= 0)
    return -1;

  if (wd == w->area.w && ht == w->area.h)
    return 0;

  /*
   * Kay: save `w' and its childs in case the window becomes hidden
   * after the resize. this is really needed, see comments before
   * window_save_contents().
   */
  wtree_traverse_pre (w, &w->pos, window_save_contents);

  oldpos = w->pos;
  oldarea = w->area;

  w->pos.w += (wd - w->area.w);
  w->pos.h += (ht - w->area.h);

  w->area.w = wd;
  w->area.h = ht;

  if (!w->is_open) {
    /*
     * if the window is closed, rectangle lists
     * and such will be built when opening the
     * window.
     *
     * just change the backing storage bitmap here.
     */
    return 0;
  }

  /*
   * Next we do backing storage for all the windows that may be
   * obscured by resizing the window.
   *
   * TAKE CARE HERE when the backing storage bitmap is more
   * up to date then the window on the screen.
   */
  wtree_list_traverse_pre (w->next, &w->pos, window_save_contents);
  window_save_contents (w->parent, &w->pos);

  /*
   * and rebuild the rectangle lists for the window,
   * its subwindows and rights siblings.
   */
  oldpos.next = &w->pos;
  if (wtree_list_traverse_pre (w, &oldpos, window_rebuild_recs) ||
      window_rebuild_recs (w->parent, &oldpos) < 0) {
    /*
     * something went wrong. Reverse everything we have
     * done so far.
     */
    wtree_traverse_pre (w, NULL, window_restore_recs);
    window_restore_recs (w->parent, NULL);
    w->pos = oldpos;
    w->area = oldarea;
    return -1;
  }
  wtree_list_traverse_pre (w, NULL, window_set_recs);
  window_set_recs (w->parent, NULL);

  /*
   * the backing storage bitmap should be changed to the new size
   * here.
   */

  /*
   * redraw the interior of the window. only the following area
   * has to be redrawn:
   *
   *  w->pos - (oldpos + new_right_border + new_bottom_border)
   *
   * the subtrahend is calculated below and stored in `oldarea'.
   */
  oldarea.w = MIN (XR (&oldarea), XR (&w->area)) - XL (&w->pos);
  oldarea.h = MIN (YU (&oldarea), YU (&w->area)) - YO (&w->pos);
  oldarea.x = XL (&w->pos);
  oldarea.y = YO (&w->pos);

  rp = rect_subtract (&w->pos, &oldarea, NULL, NULL);
  if (rp == RECT_ERROR) {
    /*
     * something went wrong. Redraw the whole window area.
     */
    wtree_traverse_pre (w, &w->pos, window_redraw_contents_rect);
  } else if (rp) {
    wtree_traverse_pre (w, rp, window_redraw_contents_rect);
    rect_list_destroy (rp);
  }

  /*
   * and now redraw the area that was exposed by resizing the
   * window
   */
  oldpos.next = NULL;
  wtree_list_traverse_pre (w->next, &oldpos, window_redraw_contents_rect);
  window_redraw_contents (w->parent, &oldpos);

  return 0;
}
#endif


/*
 * change the stacking position of window `w' to be just on top
 * (if on_top != 0) or just below (if on_top == 0) of `w2'.
 *
 * `w' and `w2` must have the same parent.
 */

static long window_restack (WINDOW *w, WINDOW *w2, short on_top)
{
  void (*wtree_restore_fn) (WINDOW *, WINDOW *, WINDOW *);
  void (*wtree_add_fn) (WINDOW *, WINDOW *, WINDOW *);
  WINDOW *oldw2, *wp, *oldnext, *oldprev;
  int moving_up, r;

  if (!w->parent || w->parent != w2->parent || w == w2)
    return -1;

  if (on_top) {
    if (w->next == w2)
      /*
       * we are already at the desired stacking postion
       */
      return 0;
    wtree_add_fn = wtree_add_before;
  } else {
    if (w->prev == w2)
      /*
       * we are already at the desired stacking postion
       */
      return 0;
    wtree_add_fn = wtree_add_after;
  }

  /*
   * now we are sure there are at least two different windows
   * (`w' and `w2') and `w' is not at the requested postition.
   */
  if (w->next) {
    oldw2 = w->next;
    wtree_restore_fn = wtree_add_before;
  } else {
    oldw2 = w->prev;
    wtree_restore_fn = wtree_add_after;
  }

  /*
   * determine whether the window is beeing moved up or down.
   */
  for (wp = w2; wp && wp != w; wp = wp->next)
    ;
  moving_up = !!wp;

  /*
   * remove `w' from the tree and insert it into the new position.
   */
  oldnext = w->next;
  oldprev = w->prev;
  wtree_remove (w);
  (*wtree_add_fn) (w->parent, w2, w);

  if (!w->is_open) {
    /*
     * if the window is closed, rectangle lists
     * and such will be built when opening the
     * window.
     */
    return 0;
  }

  /*
   * do backing storage for the windows that may be covered by other
   * windows after restacking.
   *
   * If `w' is beeing moved up then these are the windows inbetween
   * the old and the new position of `w' including `w'. Otherwise
   * only `w' itself is affected.
   * The parent window of `w' is never affected.
   *
   * TAKE CARE HERE when the backing storage bitmap is more
   * up to date then the window on the screen.
   */
  if (moving_up) {
    for (wp = oldprev; wp != w; wp = wp->prev)
      wtree_traverse_pre (wp, &w->pos, window_save_contents);
  }
  wtree_traverse_pre (w, &w->pos, window_save_contents);

  /*
   * rebuild rectangles for the windows that may be covered or
   * exposed by restacking `w'. These are the windows inbetween
   * the old and new position of `w' and `w' itself.
   * the parent window is not affected.
   */
  r = 0;
  if (moving_up) {
    for (wp = oldprev; wp != w; wp = wp->prev)
      r |= !!wtree_traverse_pre (wp, &w->pos, window_rebuild_recs);
  } else {
    for (wp = oldnext; wp != w; wp = wp->next)
      r |= !!wtree_traverse_pre (wp, &w->pos, window_rebuild_recs);
  }
  r |= !!wtree_traverse_pre (w, &w->pos, window_rebuild_recs);

  if (r) {
    /*
     * something went wrong. reverse the operation.
     */
    if (moving_up) {
      for (wp = oldprev; wp != w; wp = wp->prev)
	wtree_traverse_pre (wp, NULL, window_restore_recs);
    } else {
      for (wp = oldnext; wp != w; wp = wp->next)
	wtree_traverse_pre (wp, NULL, window_restore_recs);
    }
    wtree_traverse_pre (w, NULL, window_restore_recs);
    (*wtree_restore_fn) (w->parent, oldw2, w);
    return -1;
  }
  if (moving_up) {
    for (wp = oldprev; wp != w; wp = wp->prev)
      wtree_traverse_pre (wp, NULL, window_set_recs);
  } else {
    for (wp = oldnext; wp != w; wp = wp->next)
      wtree_traverse_pre (wp, NULL, window_set_recs);
  }
  wtree_traverse_pre (w, NULL, window_set_recs);

  /*
   * when moving up only `w' has to be redrawn, otherwise all
   * windows inbetween the old and new position of `w' have to
   * be redrawn (but not `w').
   */
  if (moving_up) {
    /*
     * kay: do a full redraw instead of trying to be clever and redrawing
     * only the union of the windows above. That did not work if there were
     * closed windows on top of `w'.
     */
    wtree_traverse_pre (w, &w->pos, window_redraw_contents_rect);
  } else {
    for (wp = oldnext; wp != w; wp = wp->next)
      wtree_traverse_pre (wp, &w->pos, window_redraw_contents_rect);
  }
  return 0;
}


/*
 * some useful helpers for restacking windows.
 */

long window_to_top (WINDOW *w)
{
  if (!w->parent)
    return -1;

  if (!w->prev)
    return 0;

  return window_restack (w, w->parent->childs, 1);
}

long window_to_bottom (WINDOW *w)
{
  WINDOW *wp;

  if (!w->next)
    return 0;

  for (wp = w; wp->next; wp = wp->next)
    ;

  return window_restack (w, wp, 0);
}

#ifdef WINDOW_UNUSED_CODE
long window_to_top_one (WINDOW *w)
{
  if (!w->prev)
    return 0;

  return window_restack (w, w->prev, 1);
}

long window_to_bottom_one (WINDOW *w)
{
  if (!w->next)
    return 0;

  return window_restack (w, w->next, 0);
}
#endif


/*
 * find the window whose visible parts contain the point (x, y).
 *
 * This will be called for every mouse-movement to check whether
 * we leave or enter a new window. Therefore it has to be very
 * fast.
 *
 * We cache the last looked up window and rectangle, so window_find()
 * should be reasonably fast for looking up continuous coordinates
 * (like the mouse position).
 *
 * however, if you are going to look up random coords, then you should
 * disable the cache by passing 0 for `usecache'.
 */

WINDOW *window_find (int x, int y, int usecache)
{
  WINDOW *wp, *found;
  REC **prev, *rp;

  /*
   * first try the cache
   */
  if (usecache && window_find_cache && window_find_cache->is_open) {
    prev = &window_find_cache->vis_recs;
    for (rp = *prev; rp; prev = &rp->next, rp = *prev) {
      if (rect_cont_point (rp, x, y))
	break;
    }
    if (rp) {
      /*
       * put the rectangle that contains the point
       * to the front of the list, so we try it first
       * next time.
       */
      if (window_find_cache->vis_recs != rp) {
	*prev = rp->next;
	rp->next = window_find_cache->vis_recs;
	window_find_cache->vis_recs = rp;
      }
      return window_find_cache;
    }
  }

  /*
   * cache miss. do a full search.
   */
  found = NULL;
  for (wp = glob_rootwindow; wp; wp = wp->childs) {
    for ( ; wp; wp = wp->next) {
      if (wp->is_open && rect_cont_point (&wp->pos, x, y))
	break;
    }
    if (!wp) {
      /*
       * (x,y) is inside `found', but outside of
       * all its childs.
       */
      break;
    }
    if (!rect_cont_point (&wp->work, x, y)) {
      /*
       * (x,y) is on the border of `wp'
       */
      found = wp;
      break;
    }

    /*
     * (x,y) is inside the working area of `wp', so
     * check wp's childs.
     */
    found = wp;
  }
  if (usecache)
    window_find_cache = found;

  return found;
}


#ifdef WINDOW_UNUSED_CODE
/*
 * return a rectangle that contains the visible working area of a window
 * (child windows of `w' and windows that aren`t ancastors of it are
 * ignored here).
 */

static long window_visible (WINDOW *w, REC *vis)
{
  WINDOW *wp;

  *vis = w->work;
  for (wp = w->parent; wp; wp = wp->parent) {
    if (!wp->is_open || !rect_intersect (vis, &wp->work, vis))
      /*
       * the window is invisible
       */
      return -1;
  }
  return 0;
}
#endif


/*
 * return a WINDOW pointer given a window id (or handle).
 */

WINDOW *windowLookup (ushort id)
{
  return hash_lookup (wintab, id);
}


/*** TeSche: we need some more functions... ***/


/*
 * windowKillClient(): delete all windows belonging to a particular client
 */

static int killClient(WINDOW *win, void *ptr)
{
  if (win->cptr == (CLIENT *)ptr) {
    client_delete((CLIENT *)ptr, win->id);
  }

  return 0;
}

void windowKillClient(CLIENT *cptr)
{
  /*
   * you must must must always use *postorder* traversal when
   * "recursivly" destroying windows, else you will get dangling
   * pointers in wtree_traverse_pre(). I think that caused the
   * "window-topping-does-not-redraw-bug".
   * kay.
   */
  wtree_list_traverse_post (glob_rootwindow->childs, cptr, killClient);
}


/*
 * draw the window frame, either dashed or solid. You tell it whether to
 * draw to the screen or the bitmap and pass the clipping rectangle.
 */

void windowDrawFrame (WINDOW *win, int dashed, int tobitmap, REC *clip_rect)
{
  void (*hline_fn) (BITMAP *, long, long, long);
  void (*vline_fn) (BITMAP *, long, long, long);
  void (*box_fn) (BITMAP *, long, long, long, long);
  int xpos, ypos;

  if (win->flags & W_NOBORDER) {
    return;
  }

  if (dashed) {
    hline_fn = glob_screen->dhline;
    vline_fn = glob_screen->dvline;
    box_fn   = glob_screen->dbox;
  } else {
    hline_fn = glob_screen->hline;
    vline_fn = glob_screen->vline;
    box_fn   = glob_screen->box;
  }

  gc0 = glob_drawgc;
  glob_clip0 = clip_rect;

  if (tobitmap) {
    BITMAP *bm = &win->bitmap;

    if (win->flags & W_TITLE) {
      (*hline_fn)(bm, win->area[AREA_WORK].x0 - 1,
		      win->area[AREA_WORK].y0 - 3,
		      win->area[AREA_WORK].x1 + 1);

      (*hline_fn)(bm, win->area[AREA_WORK].x0 - 1,
		      win->area[AREA_WORK].y0 - 2,
		      win->area[AREA_WORK].x1 + 1);
    }
    if (win->flags & W_CLOSE) {
      (*vline_fn)(bm, win->area[AREA_CLOSE].x1 + 2,
		      win->area[AREA_CLOSE].y0 - 1,
		      win->area[AREA_CLOSE].y1 + 1);

      (*vline_fn)(bm, win->area[AREA_CLOSE].x1 + 3,
		      win->area[AREA_CLOSE].y0 - 1,
		      win->area[AREA_CLOSE].y1 + 1);
    }
    if (win->flags & W_ICON) {
      (*vline_fn)(bm, win->area[AREA_ICON].x1 + 2,
		      win->area[AREA_ICON].y0 - 1,
		      win->area[AREA_ICON].y1 + 1);

      (*vline_fn)(bm, win->area[AREA_ICON].x1 + 3,
		      win->area[AREA_ICON].y0 - 1,
		      win->area[AREA_ICON].y1 + 1);
    }
    (*box_fn)(bm, 1, 1, win->pos.w - 2, win->pos.h - 2);
    (*box_fn)(bm, 2, 2, win->pos.w - 4, win->pos.h - 4);
  } else {
    BITMAP *bm = &glob_screen->bm;
    xpos = win->pos.x0;
    ypos = win->pos.y0;

    if (win->flags & W_TITLE) {
      (*hline_fn)(bm, win->work.x0 - 1, win->work.y0 - 3, win->work.x1 + 1);
      (*hline_fn)(bm, win->work.x0 - 1, win->work.y0 - 2, win->work.x1 + 1);
    }
    if (win->flags & W_CLOSE) {
      (*vline_fn)(bm, win->area[AREA_CLOSE].x1 + 2 + xpos,
		      win->area[AREA_CLOSE].y0 - 1 + ypos,
		      win->area[AREA_CLOSE].y1 + 1 + ypos);

      (*vline_fn)(bm, win->area[AREA_CLOSE].x1 + 3 + xpos,
		      win->area[AREA_CLOSE].y0 - 1 + ypos,
		      win->area[AREA_CLOSE].y1 + 1 + ypos);
    }
    if (win->flags & W_ICON) {
      (*vline_fn)(bm, win->area[AREA_ICON].x1 + 2 + xpos,
		      win->area[AREA_ICON].y0 - 1 + ypos,
		      win->area[AREA_ICON].y1 + 1 + ypos);

      (*vline_fn)(bm, win->area[AREA_ICON].x1 + 3 + xpos,
		      win->area[AREA_ICON].y0 - 1 + ypos,
		      win->area[AREA_ICON].y1 + 1 + ypos);
    }
    (*box_fn)(bm, xpos + 1, ypos + 1, win->pos.w - 2, win->pos.h - 2);
    (*box_fn)(bm, xpos + 2, ypos + 2, win->pos.w - 4, win->pos.h - 4);
  }
#ifdef SCREEN_REFRESH
  screen_update(&(win->pos));
#endif
}

/*
 * these two activate and deactivate a window, which merely means drawing a
 * frame around it
 */

void windowActivate(WINDOW *win)
{
  static REC clip;

  if ((win == glob_backgroundwin) || !win->is_open ||
      (win->flags & W_NOBORDER)) {
    return;
  }
  if (win->flags & W_CONTAINER) {
    rectUpdateDirty(&win->dirty, 0, 0, win->pos.w, win->pos.h);
    return;
  }
  if (win->is_hidden) {
    clip.x1 = (clip.w = win->pos.w) - 1;
    clip.y1 = (clip.h = win->pos.h) - 1;
    windowDrawFrame (win, 0, 1, &clip);
    rectUpdateDirty(&win->dirty, 1, 1, win->pos.w - 2, win->pos.h - 2);
  } else {
    windowDrawFrame (win, 0, 0, &win->pos);
    win->is_dirty = 1;
  }
}

void windowDeactivate(WINDOW *win)
{
  static REC clip;

  if ((win == glob_backgroundwin) || !win->is_open ||
      (win->flags & W_NOBORDER)) {
    return;
  }
  if (win->flags & W_CONTAINER) {
    rectUpdateDirty(&win->dirty, 0, 0, win->pos.w, win->pos.h);
    return;
  }
  if (win->is_hidden) {
    clip.x1 = (clip.w = win->pos.w) - 1;
    clip.y1 = (clip.h = win->pos.h) - 1;
    windowDrawFrame (win, 1, 1, &clip);
    rectUpdateDirty(&win->dirty, 1, 1, win->pos.w - 2, win->pos.h - 2);
  } else {
    windowDrawFrame (win, 1, 0, &win->pos);
    win->is_dirty = 1;
  }
}


/*
 * window_redrawAllIfDirty(): redraw only the dirty parts of all windows
 */

static int redrawIfDirty(WINDOW *win, void *notused)
{
  REC r = win->dirty;

  if (r.w) {
    r.x0 += win->pos.x0;
    r.y0 += win->pos.y0;
    r.x1 += win->pos.x0;
    r.y1 += win->pos.y0;
    r.next = NULL;
    /*
     * switch off the mouse if we would overwrite it
     */
    if (glob_mouse.visible && mouse_rcintersect (r.x0, r.y0, r.w, r.h)) {
      mouse_hide ();
    }
    window_redraw_contents_rect(win, &r);
    win->dirty.w = 0;
  }

  return 0;
}


/*
 * Note: the mouse has possibly to be switched on again. But I think
 * this is done in the main loop.
 *
 * Yes, it is.
 */

void windowRedrawAllIfDirty(void)
{
  glob_clip0 = NULL;
  glob_clip1 = NULL;
  wtree_traverse_pre(glob_rootwindow, 0, redrawIfDirty);
}
