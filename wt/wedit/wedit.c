/*
 * A text editor for testing the edittext widget.
 *
 * Fileselectors, query, help and goto line dialogs are dynamical but
 * options and search/replace dialogs are allocated at first invocation and
 * then just opened / closed.  This is to ensure that that there's only one
 * of the options dialogs open so that user won't be confused (and in case
 * of search/replace repetitions it's much faster to use a static dialog).
 *
 * features done:
 * - find and replace strings
 * - goto line, options and help dialogs
 * - clear, load, insert, save and write text
 * - easy to use functionality for load, save and exit
 * - keyboard shortcuts and keymap (256 byte file) option
 * - multiline abbreviations (separate file)
 * - pairing and group indentation
 *
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <Wlib.h>
#include <Wt.h>
#include "edittext.h"
#include "kurzels.h"

#define MAX_INDENT	8
#define DEF_INDENT	8
#define DEF_TABS	8
#define DEF_COLS	78
#define DEF_ROWS	15
#define DEF_KURZELS	".kurzels"	/* abbr. file name in home dir */
#define WIN_NAME	" Wedit v0.97 "


typedef enum { False, True } bool;

/* editor / text state flags */
static bool
  Exiting    = False,		/* program exit initiated  */
  Edited     = False;		/* changes made to text */

static bool
  GroupAlign = False,		/* should `group' items be aligned */
  PairInsert = True;		/* insert other part of the pair */

static int
  PairIndent = DEF_INDENT;	/* how much indentation with PairInsert */


/* global keymap */
static uchar
  KeyMap[256];

/* global abbreviations */
static kurzel_t
  *Kurzels;


/* widgets being updated / used from several functions / callbacks */
static widget_t
  *Top,				/* top widget */
  *Col,				/* show cursor column */
  *Row,				/* -"- row */
  *Main,			/* editor window (for title setting) */
  *Edit,			/* edittext widget */
  *Info;			/* error etc. messages */

static const char *WinName = WIN_NAME;
#define BUFFER_SIZE	(_POSIX_PATH_MAX + _POSIX_NAME_MAX)
static char
  TmpBuffer[BUFFER_SIZE+1],	/* enough for local filenames */
  *Filename;			/* text title (filename) */


/* ---------------------- info line handling ------------------------------ */

/* info line restore timer */
#define INFO_TIME	6000	/* milliseconds */
static long
  InfoTimer = -1;

static void info_cb(long arg)
{
  wt_setopt(Info, WT_LABEL, WIN_NAME "(w) 1997 by Eero Tamminen ", WT_EOL);
  InfoTimer = -1;
}

/* set information line text with `copyright' restore timeout */
static void set_info(const char *text)
{
  if(InfoTimer >= 0) {
    wt_deltimeout(InfoTimer);
  }
  wt_setopt(Info, WT_LABEL, text, WT_EOL);
  InfoTimer = wt_addtimeout(INFO_TIME, info_cb, 0L);
}

/* -------------- window closer actions ------------------------------- */

static void close_cb(widget_t *w)
{
  wt_close(w);
}

static void delete_cb(widget_t *w)
{
  wt_delete(w);
}

/* ---------------- new file loading utils ------------------------------- */

/* change the text/window name */
static void new_name(const char *name)
{
  if(!name)
  {
    if(Filename)
    {
      free(Filename);
      Filename = NULL;
    }
    wt_setopt(Main, WT_LABEL, WinName, WT_EOL);
    return;
  }
  /* same name? */
  if(Filename)
  {
    if(strcmp(name, Filename))
      return;
    free(Filename);
  }
  Filename = strdup(name);
  wt_setopt(Main, WT_LABEL, Filename, WT_EOL);
}


/* clear away the text */
static int new_cb(widget_t *w, int pressed)
{
  if(pressed)
    return 0;

  if(Edited)
  {
    if(wt_dialog(Main, "Discard current text?", WT_DIAL_QUEST,
       WinName, "Discard", "Cancel", NULL) != 1)
      return 0;
  }
  if(wt_setopt(Edit, WT_TEXT_CLEAR, 0, WT_EOL) < 0)
    wt_break(-1);
  else
    new_name(NULL);
  Edited = 0;
  return 1;
}


/* insert given file to widget */
static uchar *get_file(widget_t *w, const char *file)
{
  uchar *check, *text = NULL;
  struct stat st;
  FILE *fp;
  
  if((fp = fopen(file, "rb")))
  {
    stat(file, &st);
    if((text = malloc(st.st_size+1)))
    {
      fread(text, st.st_size, 1, fp);
      check = text + st.st_size;
      /* mark end with null and convert other nulls to spaces */
      *check = '\0';
      while(check-- > text)
      {
	if(!*check)
	  *check = '\n';
      }
    }
    else
      fprintf(stderr, "not enought memory for the file\n");
    fclose(fp);
  }
  return text;
}

/* ---------------------- load file ----------------------------------- */

/* load the selected file into edit buffer */
static void select_load(widget_t *w, const char *file)
{
  uchar *text;

  if(file)
  {
    text = get_file(Edit, file);
    if(text)
    {
      if(!new_cb(0, 0))
      {
	wt_delete(w);
	return;
      }
      wt_setopt(Edit, WT_TEXT_APPEND, text, WT_EOL);
      free(text);
    }
    else
      set_info("Text loading failed!");

    new_name(file);
  }
  wt_delete(w);
}

/* open fileselector and select file to load */
static void load_cb(widget_t *w, int pressed)
{
  widget_t *fsel;

  if(pressed)
    return;

 if((fsel = wt_create(wt_filesel_class, Top)))
 {
    wt_setopt(fsel,
	WT_LABEL, "Load text...",
	WT_ACTION_CB, select_load,
	WT_EOL);
    wt_realize(Top);
  }
}

/* ------------------- insert file ------------------------------------- */

/* insert the selected file into edit buffer */
static void select_insert(widget_t *w, const char *file)
{
  uchar *text;

  if(file)
  {
    text = get_file(Edit, file);
    if(text)
    {
      Edited = 1;
      wt_setopt(Edit, WT_TEXT_INSERT, text, WT_EOL);
      free(text);
    }
    else
      set_info("Text inserting failed!");
  }
  wt_delete(w);
}

/* open fileselector and select insert file */
static void insert_cb(widget_t *w, int pressed)
{
  widget_t *fsel;

  if(pressed)
    return;

 if((fsel = wt_create(wt_filesel_class, Top)))
 {
    wt_setopt(fsel,
	WT_LABEL, "Insert text...",
	WT_ACTION_CB, select_insert,
	WT_EOL);
    wt_realize(Top);
  }
}

/* ----------------- save file ---------------------------------------- */

/* output given string to file */
static uchar *do_write_cb(widget_t *w, uchar const *string, int maxlen, FILE *fp)
{
  fprintf(fp, "%s\n", string);
  return NULL;
}

/* save the buffer into selected file through above callback */
static void select_save(widget_t *w, const char *file)
{
  FILE *fp;

  if(file)
  {
    /* selected file failed, try again? */
    if(!(fp = fopen(file, "wb")))
      return;

    wt_setopt(Edit, WT_ACTION_CB, do_write_cb, WT_EOL);
    wt_getopt(Edit, WT_ACTION_CB, fp, WT_EOL);
    fclose(fp);
    Edited = 0;

    /* exit in process? */
    if(Exiting)
    {
      wt_break(1);
      return;
    }

    if(!Filename || (strcmp(file, Filename) &&
       wt_dialog(Main, "Adopt the new file name?", WT_DIAL_QUEST,
       WinName, "Adopt", "Cancel", NULL) == 1))
      new_name(file);
  }
  /* may be called by filesel or other functions */
  if(w)
    wt_delete(w);
}

/* open fileselector and select save file */
static void write_cb(widget_t *w, int pressed)
{
  widget_t *fsel;

  if(pressed)
    return;

 if((fsel = wt_create(wt_filesel_class, Top)))
 {
    wt_setopt(fsel,
	WT_LABEL, "Save as...",
	WT_ACTION_CB, select_save,
	WT_EOL);
    wt_realize(Top);
  }
}

/* ---------------------- search / replace ------------------------------ */

/* search replace variables / flags */
typedef struct
{
   const uchar *search;		/* find string */
   const uchar *replace;	/* replace string */
   bool prompt;			/* prompt every replace flag */
   bool icase;			/* ignore case flag */
} search_t;

static search_t
  Find = { NULL, NULL, True, False };

static widget_t
  *Prompt,			/* `ask each replace' checkbutton */
  *Search,			/* search getstring widget */
  *Replace;			/* replace getstring */


#define BUTTON_REPLACE	1
#define BUTTON_NEXT	2
#define BUTTON_CANCEL	3

/* set the toplevel widget usrval to the index of the widget */
static void index_cb(widget_t *w, int pressed)
{
  widget_t *wp;
  long idx;

  if(!pressed)
  {
    for (wp = w; wp->parent; wp = wp->parent)
       ;
     wt_getopt (w,  WT_USRVAL, &idx, WT_EOL);
     wt_setopt (wp, WT_USRVAL, &idx, WT_EOL);
  }
}

static void windex_cb(widget_t *w)
{
  index_cb(w, 0);
}

/* present a modal dialog and wait until user selects something from it */
static int search_dialog(void)
{
  static long xx, yy;
  static widget_t *top, *win;
  widget_t *hpane, *replace, *next, *cancel;
  WWIN *dwin;
  short x, y;
  long idx;

  /* because this function is called so often, dialog will be composed only
   * once, and be hidden when not needed
   */
  if(!win)
  {
    top     = wt_create(wt_top_class, NULL);
    win     = wt_create(wt_shell_class, top);
    hpane   = wt_create(wt_pane_class, win);
    replace = wt_create(wt_button_class, hpane);
    next    = wt_create(wt_button_class, hpane);
    cancel  = wt_create(wt_button_class, hpane);

    idx = OrientHorz;
    wt_setopt(hpane, WT_ORIENTATION, &idx, WT_EOL);
    idx = BUTTON_REPLACE;
    wt_setopt(replace,
	WT_USRVAL, &idx,
	WT_LABEL, "Replace",
	WT_ACTION_CB, index_cb,
	WT_EOL);
    idx = BUTTON_NEXT;
    wt_setopt(next,
	WT_USRVAL, &idx,
	WT_LABEL, "Next",
	WT_ACTION_CB, index_cb,
	WT_EOL);
    idx = BUTTON_CANCEL;
    wt_setopt(cancel,
	WT_USRVAL, &idx,
	WT_LABEL, "Cancel",
	WT_ACTION_CB, index_cb,
	WT_EOL);
    wt_setopt(win,
	WT_USRVAL, &idx,
	WT_LABEL, WinName,
	WT_ACTION_CB, windex_cb,
	WT_EOL);
    wt_realize(top);
  }
  else
  {
    wt_setopt(win, WT_XPOS, &xx, WT_YPOS, &yy, WT_EOL);
    wt_open(win);
  }

  idx = 0;
  wt_setopt(top, WT_USRVAL, &idx, WT_EOL);
  while (!wt_do_event ())
  {
    wt_getopt (top, WT_USRVAL, &idx, WT_EOL);
    if (idx != 0)
      break;
  }

  dwin = wt_widget2win(win);
  w_querywindowpos (dwin, 1, &x, &y);
  wt_close(win);
  xx = x;
  yy = y;

  wt_getopt (top, WT_USRVAL, &idx, WT_EOL);
  return idx;
}

/* Search the given string. If it's found, select it and present an
 * action selection dialog (replace, cancel) for the user.
 */
static int do_search_cb(widget_t *w, const uchar *string, int row, search_t *find)
{
  static int pass = 0;
  long line, idx = 0, len;
  const uchar *search, *replace;
  int count;

  /* pass earlier inserted text */
  while(pass)
  {
    if(!string[idx++])
      return 1;
    pass--;
  }

  search = find->search;
  len = strlen(search);
  for(;;)
  {
    for(;;)
    {
      count = 0;
      if(find->icase)
	while(toupper(string[idx+count]) == toupper(search[count]) && ++count < len);
      else
	while(string[idx+count] == search[count] && ++count < len);
      if(count == len)
	break;
      if(!string[idx+count])
	return 1;

      idx++;
    }
    replace = find->replace;

    if(replace)
     count = BUTTON_REPLACE;
    else
    {
      find->prompt = True;
      line = ButtonStatePressed;
      /* find is always interactive */
      wt_setopt(Prompt, WT_STATE, &line, WT_EOL);
    }

    if(find->prompt)
    {
      wt_getopt(Edit, WT_TEXT_LINE, &line, WT_EOL);
      wt_setopt(Edit, WT_TEXT_LINE, &line, WT_TEXT_COLUMN, &idx, WT_TEXT_SELECT, &len, WT_EOL);
      count = search_dialog();
    }
    else
    {
      /* the replace(s) need not to be done visible, so jump (window
       * scrolling) into current line can be left out.
       */
      wt_setopt(Edit, WT_TEXT_COLUMN, &idx, WT_TEXT_SELECT, &len, WT_EOL);
    }

    pass = len;
    switch(count)
    {
      case BUTTON_REPLACE:
	if(replace)
	{
	  wt_setopt(Edit, WT_TEXT_INSERT, replace, WT_EOL);
	  pass = strlen(replace);
	  Edited = 1;
	}
      case BUTTON_NEXT:
        /* pass checked text */
        while(pass)
	{
          if(!string[idx++])
            return 1;
	  pass--;
	}
	break;

      default:
        /* interrupt searching */
	pass = 0;
	return 0;
    }
  }
}

/* initiate search */
static void start_search(widget_t *w, const uchar *string)
{
  long x, y;

  if(w == Replace)
  {
    wt_getopt(Search, WT_STRING_ADDRESS, &Find.search, WT_EOL);
    Find.replace = string;
  }
  else
  {
    Find.search  = string;
    Find.replace = NULL;
  }
  if(!*Find.search)
    return;

  wt_getopt(Edit, WT_TEXT_LINE, &y, WT_TEXT_COLUMN, &x, WT_EOL);
  wt_setopt(Edit, WT_OFFSET_CB, do_search_cb, WT_EOL);

  wt_getopt(Edit, WT_OFFSET_CB, &Find, WT_EOL);

  /* unselect text and re-position cursor */
  wt_setopt(Edit, WT_TEXT_SELECT, NULL, WT_EOL);
  wt_setopt(Edit, WT_TEXT_LINE, &y, WT_TEXT_COLUMN, &x, WT_EOL);
}

static void case_cb(widget_t *w, int pressed)
{
  Find.icase = !Find.icase;
}

static void prompt_cb(widget_t *w, int pressed)
{
  Find.prompt = !Find.prompt;
}

static void search_cb(widget_t *w, int pressed)
{
  static widget_t *win;
  widget_t *vpane, *label1, *label2, *hpane, *ncase;
  long a, b;

  if(pressed)
    return;

  /* only one search/replace at the time! */
  if(win)
  {
    wt_open(win);
    return;
  }

  win     = wt_create(wt_shell_class, Top);
  vpane   = wt_create(wt_pane_class, win);
  label1  = wt_create(wt_label_class, vpane);
  Search  = wt_create(wt_getstring_class, vpane);
  label2  = wt_create(wt_label_class, vpane);
  Replace = wt_create(wt_getstring_class, vpane);
  hpane   = wt_create(wt_pane_class, vpane);
  Prompt  = wt_create(wt_checkbutton_class, hpane);
  ncase   = wt_create(wt_checkbutton_class, hpane);

  wt_setopt(win, WT_LABEL, WinName, WT_ACTION_CB, close_cb, WT_EOL);
  wt_setopt(label1, WT_LABEL, "Search string:", WT_EOL);
  wt_setopt(label2, WT_LABEL, "Replace string:", WT_EOL);

  a = 32;
  b = DEF_COLS;
  wt_setopt(Search,
	WT_STRING_LENGTH, &b,
	WT_STRING_WIDTH, &a,
	WT_ACTION_CB, start_search,
	WT_EOL);
  wt_setopt(Replace,
	WT_STRING_LENGTH, &b,
	WT_STRING_WIDTH, &a,
	WT_ACTION_CB, start_search,
	WT_EOL);

  a = 4;
  b = OrientHorz;
  wt_setopt(hpane, WT_HDIST, &a, WT_ORIENTATION, &b, WT_EOL);

  if(Find.prompt)
    a = ButtonStatePressed;
  else
    a = ButtonStateReleased;
  wt_setopt(Prompt,
	WT_LABEL, "Prompt each",
	WT_STATE, &a,
	WT_ACTION_CB, prompt_cb,
	WT_EOL);

  if(Find.icase)
    a = ButtonStatePressed;
  else
    a = ButtonStateReleased;
  wt_setopt(ncase,
	WT_LABEL, "Ignore case",
	WT_STATE, &a,
	WT_ACTION_CB, case_cb,
	WT_EOL);

  wt_realize(Top);
}

/* ----------------- goto line ----------------------------------------- */

/* go to the line user specified */
static void do_goto_cb(widget_t *w, const char *line)
{
  long a = atoi(line);
  wt_setopt(Edit, WT_TEXT_LINE, &a, WT_EOL);
}

static void goto_cb(widget_t *w, int pressed)
{
  widget_t *win;
  widget_t *hpane, *label, *toline;
  long a;

  if(pressed)
    return;

  win    = wt_create(wt_shell_class, Top);
  hpane  = wt_create(wt_pane_class, win);
  label  = wt_create(wt_label_class, hpane);
  toline = wt_create(wt_getstring_class, hpane);

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);
  wt_setopt(win, WT_LABEL, WinName, WT_ACTION_CB, delete_cb, WT_EOL);
  wt_setopt(label, WT_LABEL, "Goto text line: ", WT_EOL);

  wt_getopt(Edit, WT_TEXT_LINE, &a, WT_EOL);
  sprintf(TmpBuffer, "%ld", a);
  a = 4;
  wt_setopt(toline,
	WT_STRING_WIDTH, &a,
	WT_STRING_LENGTH, &a,
	WT_STRING_ADDRESS, TmpBuffer,
	WT_STRING_MASK, "0-9",
	WT_ACTION_CB, do_goto_cb,
	WT_EOL);

  wt_realize(Top);
}


static void change_cb(widget_t *w, int x, int y)
{
  sprintf(TmpBuffer, "%d", x);
  wt_setopt(Col, WT_LABEL, TmpBuffer, WT_EOL);
  sprintf(TmpBuffer, "%d", y);
  wt_setopt(Row, WT_LABEL, TmpBuffer, WT_EOL);
}

/* ----------------- misc utils ------------------------------------------ */

static void save_cb(widget_t *w, int pressed)
{
  if(pressed)
    return;

  if(Filename)
    select_save(NULL, Filename);
  else
    write_cb(0, 0);
}

static void exit_cb(widget_t *w)
{
  int count;
  if(Edited)
  {
    count = wt_dialog(Main, "Exiting program...", WT_DIAL_QUEST,
      WinName, "Save+exit", "Exit", "Cancel", NULL);
  }
  else
    count = 2;
  switch(count)
  {
    case 1:
      Exiting = 1;
      save_cb(0, 0);
      break;
    case 2:
      wt_break(1);
      break;
  }
}

static void help_cb(widget_t *w, int pressed)
{
  widget_t *win;
  widget_t *text;
  WFONT *font;
  long x, y;
  const char *msg =
    "Cursor keys:\n"
    "   left: ^b   right:  ^f   up:    ^p   down:   ^n\n"
    "   home: ^a   end:    ^e   pgup:  ^<   pgdown: ^>\n"
    "\nOther keys:\n"
    "   Cut:  ^x   Copy:   ^c   Paste: ^v\n"
    "   Format paragraph:  ^z   Transpose chars:   ^t\n"
    "   Kill line:         ^k   Line undo:         ^u\n"
    "\nShortcuts:\n"
    "   New:    ESC-N    Find:  ESC-F    Help: ESC-H\n"
    "   Load:   ESC-L    Save:  ESC-S    Goto: ESC-G\n"
    "   Insert: ESC-I    Write: ESC-W    Quit: ESC-X\n"
    "   Abbreviation expansion: ESC-ESC\n"
    "\nLeft button selects and right button pastes text.\n"
    "Click without dragging just positions the cursor.";

  if(pressed)
    return;

  win  = wt_create(wt_shell_class, Top);
  text = wt_create(wt_text_class, win);
  font = w_loadfont(wt_global.font_fixed, wt_global.font_size, 0);

  wt_setopt(win, WT_LABEL, WinName, WT_ACTION_CB, delete_cb, WT_EOL);

  x = font->maxwidth * 50;
  y = font->height * 17;
  wt_setopt(text,
	WT_FONT, font->family,
	WT_WIDTH, &x,
	WT_HEIGHT, &y,
	WT_LABEL, msg,
	WT_EOL);

  wt_realize(Top);
}

/* ------------------ option handling ------------------------------------ */

static void tabs_cb(widget_t *w, const char *text)
{
  long tabs = atoi(text);
  wt_setopt(Edit, WT_TEXT_TAB, &tabs, WT_EOL);
}

static void wrap_cb(widget_t *w, const char *text)
{
  long wrap = atoi(text);
  wt_setopt(Edit, WT_TEXT_WRAP, &wrap, WT_EOL);
}

static void indent_cb(widget_t *w, int pressed)
{
  long a = pressed;
  wt_setopt(Edit, WT_TEXT_INDENT, &a, WT_EOL);
}

static void parindent_cb(widget_t *w, const char *text)
{
  PairIndent = atoi(text);
  if(PairIndent > MAX_INDENT)
    PairIndent = MAX_INDENT;
}

static void groupalign_cb(widget_t *w, int pressed)
{
  GroupAlign = pressed;
}

static void pairinsert_cb(widget_t *w, int pressed)
{
  PairInsert = pressed;
}

static void option_cb(widget_t *w, int pressed)
{
  static widget_t *win;
  widget_t *vpane1, *hpane1, *hpane2, *hpane3, *label1, *label2, *label3,
    *tabs, *wrap, *parind, *vpane2, *indent, *insert, *group;
  long a;

  if(pressed)
    return;

  if(win)
  {
    wt_open(win);
    return;
  }

  win    = wt_create(wt_shell_class, Top);
  vpane1 = wt_create(wt_pane_class, win);
  hpane1 = wt_create(wt_pane_class, vpane1);
  hpane2 = wt_create(wt_pane_class, vpane1);
  hpane3 = wt_create(wt_pane_class, vpane1);

  label1 = wt_create(wt_label_class, hpane1);
  wrap   = wt_create(wt_getstring_class, hpane1);
  label2 = wt_create(wt_label_class, hpane2);
  tabs   = wt_create(wt_getstring_class, hpane2);
  label3 = wt_create(wt_label_class, hpane3);
  parind = wt_create(wt_getstring_class, hpane3);

  vpane2 = wt_create(wt_pane_class, vpane1);
  indent = wt_create(wt_checkbutton_class, vpane2);
  insert = wt_create(wt_checkbutton_class, vpane2);
  group  = wt_create(wt_checkbutton_class, vpane2);

  wt_setopt(win, WT_LABEL, WinName, WT_ACTION_CB, close_cb, WT_EOL);

  a = AlignRight;
  wt_setopt(vpane1, WT_ALIGNMENT, &a, WT_EOL);
  a = AlignLeft;
  wt_setopt(vpane2, WT_ALIGNMENT, &a, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane1, WT_ORIENTATION, &a, WT_EOL);
  wt_setopt(hpane2, WT_ORIENTATION, &a, WT_EOL);
  wt_setopt(hpane3, WT_ORIENTATION, &a, WT_EOL);


  wt_setopt(label1, WT_LABEL, "Word wrap: ",   WT_EOL);
  wt_setopt(label2, WT_LABEL, "Tab size: ",    WT_EOL);
  wt_setopt(label3, WT_LABEL, "Pair indent: ", WT_EOL);

  wt_getopt(Edit, WT_TEXT_WRAP, &a, WT_EOL);
  sprintf(TmpBuffer, "%ld", a);
  wt_setopt(wrap,
	WT_STRING_ADDRESS, TmpBuffer,
	WT_STRING_MASK, "0-9",
	WT_ACTION_CB, wrap_cb,
	WT_EOL);

  wt_getopt(Edit, WT_TEXT_TAB, &a, WT_EOL);
  sprintf(TmpBuffer, "%ld", a);
  wt_setopt(tabs,
	WT_STRING_ADDRESS, TmpBuffer,
	WT_STRING_MASK, "0-9",
	WT_ACTION_CB, tabs_cb,
	WT_EOL);

  sprintf(TmpBuffer, "%d", PairIndent);
  wt_setopt(parind,
	WT_STRING_ADDRESS, TmpBuffer,
	WT_STRING_MASK, "0-9",
	WT_ACTION_CB, parindent_cb,
	WT_EOL);


  wt_getopt(Edit, WT_TEXT_INDENT, &a, WT_EOL);
  if (a)
    a = ButtonStatePressed;
  else
    a = ButtonStateReleased;
  wt_setopt(indent,
	WT_LABEL, "Auto indent",
	WT_ACTION_CB, indent_cb,
	WT_STATE, &a,
	WT_EOL);

  if (PairInsert)
    a = ButtonStatePressed;
  else
    a = ButtonStateReleased;
  wt_setopt(insert,
	WT_LABEL, "Pair insert",
	WT_ACTION_CB, pairinsert_cb,
	WT_STATE, &a,
	WT_EOL);

  if (GroupAlign)
    a = ButtonStatePressed;
  else
    a = ButtonStateReleased;
  wt_setopt(group,
	WT_LABEL, "Group Aligning",
	WT_ACTION_CB, groupalign_cb,
	WT_STATE, &a,
	WT_EOL);

  wt_realize(Top);
}

/* ------------------ key mapping / shortcuts ----------------------------- */

/* search and select abbreviation and insert abbreviation expansion */
static void check_kurzels(void)
{
  uchar *string;
  long offset, count;
  kurzel_t *abbr;

  if(!Kurzels)
  {
    set_info("No abbreviations!");
    return;
  }
  wt_getopt(Edit, WT_TEXT_INSERT, &string, WT_TEXT_COLUMN, &offset, WT_EOL);

  /* search the abbreviation */
  if(!(isalnum(string[offset]) || (offset && isalnum(string[offset-1]))))
  {
    set_info("No abbreviation!");
    return;
  }
  while(--offset >= 0 && isalnum(string[offset]));

  offset++;
  count = 0;
  string += offset;
  while(isalnum(string[++count]));
  abbr = search_kurzel(Kurzels, string, count);

  if(abbr)
  {
    /* replace abbreviation... */
    wt_setopt(Edit, WT_TEXT_COLUMN, &offset, WT_TEXT_SELECT, &count, WT_EOL);
    wt_setopt(Edit, WT_TEXT_INSERT, abbr->before, WT_EOL);
    if(*abbr->after)
    {
      /* text coming after cursor */
      wt_getopt(Edit, WT_TEXT_LINE, &count, WT_TEXT_COLUMN, &offset, WT_EOL);
      wt_setopt(Edit, WT_TEXT_APPEND, abbr->after, WT_EOL);
      wt_setopt(Edit, WT_TEXT_LINE, &count, WT_TEXT_COLUMN, &offset, WT_EOL);
    }
  }
  else
    set_info("Not an abbreviation!");
}


/* insert given text and then correct cursor position with given char offsets */
static void text_insert(const uchar *text, int dx, int dy)
{
  long x, y;
  wt_setopt(Edit, WT_TEXT_INSERT, text, WT_EOL);
  if(dx || dy)
  {
    wt_getopt(Edit, WT_TEXT_LINE, &y, WT_TEXT_COLUMN, &x, WT_EOL);
    if(dy)
    {
      y += dy;
      wt_setopt(Edit, WT_TEXT_LINE, &y, WT_EOL);
    }
    if(dx)
    {
      x += dx;
      wt_setopt(Edit, WT_TEXT_COLUMN, &x, WT_EOL);
    }
  }
}

static int get_pair(int key)
{
  switch(key)
  {
    case '{':
      return '}';
      break;
    case '[':
      return ']';
      break;
    case '(':
      return ')';
      break;
  }
  return key;
}

/* pairing onto separate lines with one line inserted between */
static void block_pair(int key)
{
  uchar *pair = TmpBuffer; /* pair[5 + MAX_INDENT]; */
  long value, idx = 0;

  wt_getopt(Edit, WT_TEXT_TAB, &value, WT_EOL);
  pair[idx] = '\n';
  if(value > 1 && value == PairIndent)
  {
    pair[++idx] = '\t';
    idx++;
  }
  else
  {
    while(idx++ < PairIndent)
      pair[idx] = ' ';
  }
  wt_getopt(Edit, WT_TEXT_CHAR, &value, WT_EOL);
  if(value)
  {
    /* there's something at the line end... */
    pair[idx] = '\0';
    text_insert(pair, 0, 0);
    return;
  }
  pair[idx++] = '\n';
  pair[idx++] = get_pair(key);
  pair[idx] = '\0';
  text_insert(pair, PairIndent-1, -1);
}

/* search index of the character after last _open_ pair */
static int group_indent(void)
{
#define STK_SIZE  BUFFER_SIZE
  uchar key, *line, *stack = TmpBuffer;	/* stack[STK_SIZE] */
  int idx, level;

  wt_getopt(Edit, WT_TEXT_INSERT, &line, WT_EOL);
  
  /* skip indent */
  for(idx = 0; line[idx] && line[idx] <= ' '; idx++)
    ;
  if(!line[idx])
    return 0;

  /* search first unbalanced pair opener */
  level = 0;
  for(line += idx; line[idx]; idx++)
  {
    key = line[idx];
    switch(key)
    {
      case '{':
      case '[':
      case '(':
        if(level < STK_SIZE-1)
	{
          stack[level] = key;
	  stack[level+1] = idx;
	}
        level += 2;
	break;
      case '}':
      case ']':
      case ')':
        if(level > 0 && (level -= 2) < STK_SIZE-1)
	{
	  if(get_pair(stack[level]) != key)
	  {
	    set_info("Unbalanced pair!");
	    while(level > 0 && get_pair(stack[(level -= 2)] != key))
	      ;
	  }
	}
    }
  }
  if(level > 0)
    return stack[level-1] + 1;

  return 0;
#undef STK_SIZE
}


/* special key (ESC) mappings */
static int map_cb(widget_t *w, int key)
{
  uchar pair[4];
  static int prev;
  int ret = 0;

  /* special key */
  if (key > 0xff)
    return key;
	
  /* remap key */
  if(KeyMap[key])
    key = KeyMap[key];

  switch(prev)
  {
    case '\e':
      switch(key)
      {
	case '\e':
          check_kurzels();
          break;

	case 'n':
	case 'N':
	  new_cb(0, 0);
	  break;

	case 'l':
	case 'L':
	  load_cb(0, 0);
	  break;

	case 'i':
	case 'I':
	  insert_cb(0, 0);
	  break;

	case 's':
	case 'S':
	  save_cb(0, 0);
	  break;

	case 'w':
	case 'W':
	  write_cb(0, 0);
	  break;

	case 'f':
	case 'F':
	  search_cb(0, 0);
	  break;

	case 'g':
	case 'G':
	  goto_cb(0, 0);
	  break;

	case 'h':
	case 'H':
	  help_cb(0, 0);
	  break;

	case 'x':
	case 'X':
	  exit_cb(0);
	  break;
      }
      break;

    case '{':
    case '[':
    case '(':
      if(PairInsert)
      {
        switch(key)
	{
          case '\n':
	  case '\r':
	    block_pair(prev);
	    break;
	  case ' ':
	    pair[0] = ' ';
	    pair[1] = ' ';
	    pair[2] = get_pair(prev);
	    pair[3] = '\0';
	    text_insert(pair, -2, 0);
	    break;
	  default:
            ret = key;
	}
      }
      else
        ret = key;
      break;

    default:
      ret = key;
  }

  if((ret == '\n' || ret == '\r') && GroupAlign)
  {
    int indent = group_indent();
    if(!indent && prev == ':')
      indent = PairIndent;
    if(indent && indent < BUFFER_SIZE)
    {
      TmpBuffer[0] = '\n';
      memset(TmpBuffer+1, ' ', indent);
      TmpBuffer[indent+1] = '\0';
      wt_setopt(Edit, WT_TEXT_INSERT, TmpBuffer, WT_EOL);
      ret = 0;
    }
  }
  if(ret && !MOVEMENT(ret))
    Edited = 1;

  prev = ret;
  return ret;
}

/* ------------------ initialize ------------------------------------ */

static int parse_args(int argc, char *argv[], char **text, widget_t *edit)
{
  int idx = 0;
  FILE *fp;
  char arg;
  long a;

  /* check arguments */
  while(++idx < argc && (*++argv)[0] == '-')
  {
    /* not a proper argument */
    if((*argv)[2] != '\0' || ++idx >= argc)
      return -1;

    arg = (*argv)[1];
    a = atoi(*++argv);
    switch(arg)
    {
      case 'a':
	if(Kurzels)
	  free_kurzels(Kurzels);
	if(!(Kurzels = read_kurzels(*argv)))
	{
          fprintf(stderr, "error: unable to load abbreviation file %s\n", *argv);
	  return -1;
	}
	break;
      case 'k':
	if((fp = fopen(*argv, "rb")))
	{
	  if(fread(KeyMap, sizeof(KeyMap), sizeof(*KeyMap), fp) == sizeof(KeyMap))
	    break;
	}
        fprintf(stderr, "error: faulty keymap file %s\n", *argv);
	return -1;
      case 'f':
        wt_setopt(edit, WT_FONT, *argv, WT_EOL);
        break;
      case 'h':
        wt_setopt(edit, WT_VT_HEIGHT, &a, WT_EOL);
        break;
      case 'w':
        wt_setopt(edit, WT_VT_WIDTH, &a, WT_EOL);
        break;
      case 'j':
        wt_setopt(edit, WT_TEXT_WRAP, &a, WT_EOL);
        break;
      case 't':
        wt_setopt(edit, WT_TEXT_TAB, &a, WT_EOL);
        break;
      case 'i':
        PairIndent = MIN(MAX_INDENT, a);
	break;
      default:
        return -1;
    }
  }

  if(idx >= argc)
    return 0;

  *text = get_file(Edit, *argv);
  if(text)
  {
    wt_setopt(Edit, WT_TEXT_APPEND, *text, WT_EOL);
    wt_setopt(Main, WT_LABEL, *argv, WT_EOL);
    Filename = strdup(*argv);
    Edited = 0;
    return 0;
  }
  fprintf(stderr, "error: unable to load text %s\n", *argv);
  return -1;
}

int main (int argc, char *argv[])
{
  widget_t *hpane, *vpane, *new, *insert, *load, *write, *save,
    *find, *toline, *option, *help;
  char *text = NULL;
  long a, b, c;

  Top = wt_init();

  Main  = wt_create(wt_shell_class, Top);
  vpane = wt_create(wt_pane_class, Main);
  hpane = wt_create(wt_pane_class, vpane);
  Edit  = wt_create(wt_edittext_class, vpane);
  Info  = wt_create(wt_label_class, vpane);

  new    = wt_create(wt_button_class, hpane);
  load   = wt_create(wt_button_class, hpane);
  insert = wt_create(wt_button_class, hpane);
  write  = wt_create(wt_button_class, hpane);
  save   = wt_create(wt_button_class, hpane);
  find   = wt_create(wt_button_class, hpane);
  toline = wt_create(wt_button_class, hpane);
  option = wt_create(wt_button_class, hpane);
  help   = wt_create(wt_button_class, hpane);

  Col = wt_create(wt_label_class, hpane);
  Row = wt_create(wt_label_class, hpane);

  if(!(Edit && Info && help && Row))
  {
    fprintf(stderr, "%s: Widget creation failed!\n", *argv);
    return -1;
  }

  a = OrientHorz;
  wt_setopt(hpane, WT_ORIENTATION, &a, WT_EOL);
  wt_setopt(Main, WT_LABEL, WinName, WT_ACTION_CB, exit_cb, WT_EOL);

  wt_setopt(new,    WT_LABEL, "New",       WT_ACTION_CB, new_cb,    WT_EOL);
  wt_setopt(load,   WT_LABEL, "Load",      WT_ACTION_CB, load_cb,   WT_EOL);
  wt_setopt(insert, WT_LABEL, "Insert",    WT_ACTION_CB, insert_cb, WT_EOL);
  wt_setopt(write,  WT_LABEL, "Write as",  WT_ACTION_CB, write_cb,  WT_EOL);
  wt_setopt(save,   WT_LABEL, "Save",      WT_ACTION_CB, save_cb,   WT_EOL);
  wt_setopt(find,   WT_LABEL, "Find",      WT_ACTION_CB, search_cb, WT_EOL);
  wt_setopt(toline, WT_LABEL, "Goto",      WT_ACTION_CB, goto_cb,   WT_EOL);
  wt_setopt(option, WT_LABEL, "Options",   WT_ACTION_CB, option_cb, WT_EOL);
  wt_setopt(help,   WT_LABEL, "Help",      WT_ACTION_CB, help_cb,   WT_EOL);

  a = LabelModeWithBorder;
  wt_setopt(Col, WT_LABEL, " 0 ", WT_MODE, &a, WT_EOL);
  wt_setopt(Row, WT_LABEL, " 0 ", WT_MODE, &a, WT_EOL);

  a = DEF_COLS;
  b = DEF_ROWS;
  c = DEF_TABS;
  wt_setopt(Edit,
	WT_VT_WIDTH, &a,
	WT_VT_HEIGHT, &b,
	WT_TEXT_TAB, &c,
	WT_CHANGE_CB, change_cb,
	WT_MAP_CB, map_cb,
	WT_EOL);

  if(parse_args(argc, argv, &text, Edit))
  {
    fprintf(stderr, "\nusage: %s [options] [text file]\n", *argv);
    fprintf(stderr, "  -h <height in chars>\n");
    fprintf(stderr, "  -w <width in chars>\n");
    fprintf(stderr, "  -t <tab size>\n");
    fprintf(stderr, "  -j <word wrap column>\n");
    fprintf(stderr, "  -i <pair indent amount>\n");
    fprintf(stderr, "  -a <abbreviation file>\n");
    fprintf(stderr, "  -k <keymap file>\n");
    fprintf(stderr, "  -f <text font>\n");
    fprintf(stderr, "Default abbreviation file is ~/" DEF_KURZELS "\n");
    return -1;
  }
  if(!Kurzels)
  {
    struct passwd *pw;
    if((pw = getpwuid(getuid())))
    {
      /* try to read the default abbreviations file from home directory */
      sprintf(TmpBuffer, "%s/" DEF_KURZELS, pw->pw_dir);
      Kurzels = read_kurzels(TmpBuffer);
    }
  }
  info_cb(0L);

  wt_realize(Top);
  /* edittext gets / parses text only when it's realized, so text couldn't
   * be freed earlier.
   */
  if(text)
    free(text);

  wt_run();
  return 0;
}

