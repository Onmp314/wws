/*
 * A chat program using W toolkit and sockets.
 *
 * Notes:
 * - Nick names starting with '*' are special messages for show_string().
 *
 * TODO:
 * - IRCish commands / buttons like: who, send, receive, private.
 * - Text painting & clipboard support.
 * - saving/loading/transmitting files (needs support on comms.c).
 * - 'friends' list to connect dialog.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "chat.h"

#define WIN_NAME	"W-Chat v0.13"

/*  defaults */
#define BORDER		4			/* text border */
#define TEXT_LINES	12			/* visible */
#define HISTORY_LINES	50			/* buffer size */
#define LINE_LENGHT	65			/* string + '\0' */
#define MIN_WIDTH	41

/* used also in comms.c */
widget_t *Top;

static widget_t *Scroll;
static WFONT *Font;
static WWIN *Texts;
static int Width, Height;		/* text window size */
static int XBorder, YBorder;		/* text window borders */
static int LineIdx, ShowIdx;		/* indeces for write and show lines */
static char *Buffer, *BufferEnd;	/* for history */
static char *Current;			/* buffer write pointer */

static struct
{
  long lines;				/* how many lines visible */
  long width;				/* text window width in chars */
  long history;				/* lines in text buffer */
} Text = { TEXT_LINES, LINE_LENGHT, HISTORY_LINES };


/* function prototypes. returns 0 if ok */
static void help(void);
static int  process_arguments(int argc, const char *argv[]);
static int  initialize_window(void);
static void initialize_texts(widget_t *w, int x, int y, int wd, int ht);
static void scroll_cb(widget_t *w, int idx, int pressed);
static void input_cb(widget_t *w, const char *text);
static void scrollup_text(int lines);
static void draw_text(void);


int main(int argc, const char *argv[])
{
  if(process_arguments(argc, argv))
    return -1;

  if(initialize_window())
  {
    fprintf(stderr, "GUI initialization failed.\n");
    return -1;
  }

  wt_realize(Top);
  wt_run();

  return 0;
}

static void help(void)
{
  fprintf(stderr, "\n%s (w) 1996 by Eero Tamminen\n", WIN_NAME);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  -l <lines>\n");
  fprintf(stderr, "  -c <columns>\n");
  fprintf(stderr, "  -b <buffered lines>\n");
}

static int process_arguments(int argc, const char *argv[])
{
  int idx = 0;

  while(++idx < argc)
  {
    /* one letter option with an argument? */
    if(argv[idx][0] == '-' && argv[idx][1] && !argv[idx][2] && idx+1 < argc)
    {
      switch(argv[idx++][1])
      {
	case 'l':
	  Text.lines = atoi(argv[idx]);
	  break;
	case 'c':
	  Text.width = atoi(argv[idx])+1;
	  break;
	case 'b':
	  Text.history = atoi(argv[idx]);
	  break;
	default:
	  help();
	  return 1;
      }
    }
    else
    {
      help();
      return 1;
    }
  }
  if(Text.lines < 2)
    Text.lines = 2;

  if(Text.width < MIN_WIDTH)
    Text.width = MIN_WIDTH;

  if(Text.history < Text.lines)
    Text.history = Text.lines;

  return 0;
}

static int initialize_window(void)
{
  widget_t *shell, *vpane, *hpane1, *hpane2,
    *connect, *draw, *input;
  long a, b;

  if(!(Top = wt_init()))
    return 1;

  if(!(Font = w_loadfont(NULL, 0, 0)))
    return 1;

  shell   = wt_create(wt_shell_class, Top);
  vpane   = wt_create(wt_pane_class, shell);
  hpane1  = wt_create(wt_pane_class, vpane);
  connect = wt_create(wt_button_class, hpane1);
  hpane2  = wt_create(wt_pane_class, vpane);
  Scroll  = wt_create(wt_scrollbar_class, hpane2);
  draw    = wt_create(wt_drawable_class, hpane2);
  input   = wt_create(wt_getstring_class, vpane);

  if(!input)
    return 1;

  wt_setopt(shell, WT_LABEL, WIN_NAME, WT_EOL);

  a = AlignFill;
  wt_setopt(vpane, WT_ALIGNMENT, &a, WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane1, WT_ORIENTATION, &a, WT_EOL);
  b = AlignFill;
  wt_setopt(hpane2, WT_ORIENTATION, &a, WT_ALIGNMENT, &b, WT_EOL);
  wt_setopt(connect, WT_LABEL, "Connect", WT_ACTION_CB, network_cb, WT_EOL);

  wt_setopt(Scroll,
    WT_TOTAL_SIZE, &Text.history,
    WT_ACTION_CB, scroll_cb,
    WT_EOL);

  a = Font->widths['W'] * (Text.width-1) + 2*BORDER;
  b = Font->height * Text.lines + 2*BORDER;
  wt_setopt(draw,
    WT_WIDTH, &a,
    WT_HEIGHT, &b,
    WT_DRAW_FN, initialize_texts,
    WT_EOL);

  a = MIN_WIDTH;
  b = MAX_DATA_LEN - 1;
  wt_setopt(input,
    WT_STRING_WIDTH, &a,
    WT_STRING_LENGTH, &b,
    WT_ACTION_CB, input_cb,
    WT_EOL);

  return 0;
}

static void initialize_texts(widget_t *w, int x, int y, int wd, int ht)
{
  int i;
  long size;

  Width = wd;
  Height = ht;
  Texts = wt_widget2win(w);
  w_box(Texts, x, y, wd, ht);
  w_setmode(Texts, M_CLEAR);
  w_setfont(Texts, Font);

  /* in case text area is larger than asked for, (re)calculate...
   * (filled to scrollbar height)
   */
  Text.lines = (ht - BORDER*2) / Font->height;
  if(Text.lines > Text.history)
    Text.lines = Text.history;

  YBorder = (ht - Text.lines * Font->height) / 2;
  Text.width = (wd - BORDER*2) / Font->widths['W'];
  XBorder = (wd - Text.width * Font->widths['W']) / 2;
  Text.width++;			/* '\0' */

  /* allocate and intialize the buffer
   */
  size = Text.width * Text.history;
  if(!(Buffer = malloc(size)))
  {
    fprintf(stderr, "Not enough memory for the history buffer.\n");
    wt_break(1);
  }
  Current = Buffer;
  for(i = 0; i < Text.history; i++)
  {
    *Current = '\0';
    Current += Text.width;
  }
  LineIdx = 0;
  Current = Buffer;
  BufferEnd = Buffer + size;
  ShowIdx = Text.history - Text.lines;

  /* set scrollbar
   */
  size = Text.history - Text.lines;
  wt_setopt(Scroll,
    WT_PAGE_INC, &Text.lines,
    WT_SIZE, &Text.lines,
    WT_POSITION, &size,
    WT_EOL);
}

static void scroll_cb(widget_t *w, int idx, int pressed)
{
  if(pressed)
    return;

  ShowIdx = LineIdx - (Text.history - idx);
  if(ShowIdx < 0)
    ShowIdx += Text.history;

  draw_text();
}

/* fast enough if short lines and not very high window */
static void draw_text(void)
{
  int y, lines;
  char *line;

  /* clear the window */
  w_pbox(Texts, XBorder, YBorder, Width - 2*XBorder, Height - 2*YBorder);

  line = Buffer + ShowIdx * Text.width;
  lines = Text.lines;
  y = YBorder;
  while(lines--)
  {
    if(line >= BufferEnd)
      line = Buffer + (line - BufferEnd);

    w_printstring(Texts, XBorder, y, line);
    line += Text.width;
    y += Font->height;
  }
}

/* scroll up 'lines', line at the time */
static void scrollup_text(int lines)
{
  int w, h;
  char *line;

  if(lines > 4 || lines > Text.lines)
  {
    ShowIdx += lines;
    if(ShowIdx >= Text.history)
      ShowIdx = ShowIdx % Text.history;
    draw_text();
    return;
  }

  w = Width - 2*XBorder;
  h = Font->height;

  line = Buffer + (ShowIdx + Text.lines) * Text.width;
  while(lines--)
  {
    if(line >= BufferEnd)
      line = Buffer + (line - BufferEnd);

    /* make room for a new line */
    w_vscroll(Texts, XBorder, YBorder + h,  w, Height - h - 2*YBorder, YBorder);
    w_pbox(Texts, XBorder, Height - h - YBorder, w, h);
    w_printstring(Texts, XBorder, Height - h - YBorder, line);
    line += Text.width;
    ShowIdx++;
  }
  if(ShowIdx >= Text.history)
    ShowIdx -= Text.history;
}

/* actually word wraps it into the history buffer... */
void show_string(const char *nick, const char *text)
{
  int idx = 0, len, lenght, lines = 0;
  const char *check, *end;

  if(*nick)
  {
    idx = strlen(nick);
    strcpy(Current, nick);			/* start with a nick name */
    if(*nick != '*')				/* special name */
      Current[idx++] = ':';
    Current[idx++] = ' ';
  }

  lenght = strlen(text);
  for(;;)
  {
    lines++;

    len = Text.width - idx - 1;
    if(len >= lenght)
    {
      strcpy(&Current[idx], text);
      break;
    }
    check = text;
    end = &text[len];
    while(*check != '\n' && check++ < end)
    if(check == end)
      while(*check > ' ' && --check > text);

    /* no newline nor break place */
    if(check == text)
    {
      len = Text.width - idx - 2;
      strncpy(&Current[idx], text, len);
      Current[idx + len] = '\0';
    }
    else
    {
      len = check - text;			/* cut between words */
      strncpy(&Current[idx], text, len);
      Current[idx + len] = '\0';
      while(text[len] && text[len] <= ' ')	/* ignore whitespaces */
        len++;
    }
    text += len;				/* next string line */
    lenght -= len;
    LineIdx++;
    Current += Text.width;			/* next buffer line */
    if(Current >= BufferEnd)			/* wrap buffer */
    {
      Current = Buffer;
      LineIdx = 0;
    }
    /* after nick output indent with two spaces */
    Current[0] = ' ';
    Current[1] = ' ';
    idx = 2;
  }
  LineIdx++;
  Current += Text.width;
  if(Current >= BufferEnd)			/* wrap buffer */
  {
    Current = Buffer;
    LineIdx = 0;
  }
  scrollup_text(lines);
}

static void input_cb(widget_t *w, const char *text)
{
  broadcast_msg(MSG_STRING, text, strlen(text)+1);
  show_string("*I*", text);
  wt_setopt(w, WT_STRING_ADDRESS, "", WT_EOL);
}
