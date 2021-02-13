/*
 * wlaunch.c, a part of the W Window System
 *
 * Copyright (C) 1997 by Jan Paul Schmidt
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- launch bar for docked applications
 *
 * BUGS
 *   # of commands not dynamic (currently 10 commands).
 *   Line length of rc file is limited to 255 characters.
 *
 * TODO
 *   Kill bugs. 
 *   Special configuration directive for launchbar exit.
 *   Configuration through pop up menu/dialog.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <getopt.h>
#include <Wlib.h>
#include "../server/config.h"		/* BORDERWIDTH */
#include "basename.c"

/* maximum number of commands */
#define COMMANDS 10

/* default path for wlaunchrc */
#ifndef DATADIR
# error "ERROR: DATADIR undefined"
#endif

/* default path for icons */
#ifndef ICONDIR
# error "ERROR: ICONDIR undefined"
#endif

/* maximal line length */
#define RCLINE_MAX 255

/* macros for parse_rc_file */
#define FIRST_PASS 0
#define SECOND_PASS 1

/* macros for calculate_xywh */
#define WINDOW_TOPLEFT 0
#define WINDOW_TOPRIGHT 1
#define WINDOW_RIGHTTOP 2
#define WINDOW_RIGHTBOTTOM 3
#define WINDOW_BOTTOMRIGHT 4
#define WINDOW_BOTTOMLEFT 5
#define WINDOW_LEFTBOTTOM 6
#define WINDOW_LEFTTOP 7
/* only calculte window_w and window_h, window_vertical is already set */
#define WINDOW_USER 8

/* macros for window_ontop */
#define WINDOW_NORMAL 0
#define WINDOW_ONTOP 1

/* command types */
#define COMMAND_NORMAL 0
#define COMMAND_SWALLOW 1

struct command {
  short type;
  union {
    BITMAP *bitmap;
    WWIN *window;
  } p;
  union {
    char *string;
    size_t pid;
  } a;
};

struct internal_commands {
  const char *name;
  short (*function)(char *parameter);
};



static const char *icon_path = ICONDIR;


/* window stuff */
static WWIN *window;
static short window_x;
static short window_y;
static short window_h;
static short window_w;
static ushort window_flags = EV_MOUSE | W_MOVE | W_RESIZE;
static short window_border = BORDERWIDTH;

/* 3d flag and addtional border to bitmap (two in one without dandruff)*/
static int look_3d;

/* border alignment */
static int window_alignment = WINDOW_RIGHTTOP;

/* if 1, window alignment is vertical */
static short window_vertical = 1;

/* if WINDOW_ONTOP, window stays always on top */
static int window_ontop = WINDOW_NORMAL;



/* buffer for command data */
static struct command command [COMMANDS];

/* number of commands */
static short commands = 0;

/* hm... what's that? */
static char *program_name;

/* name of configuration file */
static char *rc = NULL;

/* size for swallowed windows */
static int swallow_size = 48;

/* options for wlaunch. no const for mbaserel's sake */
static struct option long_options [] = {
  { "file", required_argument, NULL, 'f' },
  { "help", no_argument, NULL, 'h' },
  { "iconpath", required_argument, NULL, 'i' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 }
};

static short command_alignment (char *parameter);
static short command_border (char *parameter);
static short command_look (char *parameter);
static short command_ontop (char *parameter);
static short command_swallowsize (char *parameter);

static struct internal_commands internal_commands [] = {
  {"Alignment", command_alignment },
  {"Border", command_border },
  {"Look", command_look },
  {"OnTop", command_ontop },
  {"SwallowSize", command_swallowsize },
  { NULL, NULL }
};

/*
 * Simple error output function which writes string to console.
 */
static void myerror (const char *string) {
  FILE *file_handle;

  if ((file_handle = fopen ("/dev/console", "w"))) {
    fputs (basename (program_name), file_handle);
    fputs (": ", file_handle);
    fputs (string, file_handle);
    fputc ('\n', file_handle);
    fclose (file_handle);
  }
}

/*
 * Creates argv for childs
 *
 * Returns NULL, if error
 */
static char **create_argv (char *string) {
  char **argv;
  char *p;
  char *argv_strings;
  short i = 1; /* for NULL at end of array */
  short in;
  short quote;

  if (!(argv_strings = malloc (strlen (string) + 1))) {
    myerror ("couldn't create argv array");
    return NULL;
  }

  strcpy (argv_strings, string);

  p = argv_strings;

/* count arguments */
  quote = 0;
  in = 0;
  while (*p) {
    if (isspace (*p) && !quote) {
      in = 0;
    }
    else {
      if (*p == '"') {
        quote = !quote;
      }
      else {
        if (!in) {
          i++;
        }
        else {
          in = 1;
        }
      }
    }
    p++;
  }

/* alloc argv pointer array */
  if (!(argv = malloc (sizeof (char *) * i))) {
    myerror ("couldn't create argv array");
    free (argv_strings);
    return NULL;
  }

/* fill it up */
  i = 0;
  in = 0;
  quote = 0;
  p = argv_strings;
  while (*p) {
    if (isspace (*p) && !quote) {
      *p = '\0';
      in = 0;
    }
    else {
      if (*p == '"') {
        *p = '\0';
        quote = !quote;
      }
      else {
        if (!in) {
          argv [i++] = p;
          in = 1;
        }
      }
    }
    p++;
  }

   argv [i] = NULL;

  return argv;
}


/*
 * Frees allocated memory for argv array.
 * Won't work correctly, if string for create_argv starts with
 * whitespaces (they are removed by parse_rc_file anyway).
 */
static void delete_argv (char **argv) {
  free (argv [0]);

  free (argv);
}



/*
 * Draws icon at given coordinates.
 */
#define ICON_NORMAL 0
#define ICON_SELECTED 1
static void draw_icon (BITMAP *b, short x, short y, short state) {
  short w = b->width + look_3d * 2;
  short h = b->width + look_3d * 2;

  if (look_3d) {
    w_setmode (window, M_DRAW);
    w_box (window, x, y, w, h);

    if (state == ICON_SELECTED) {
      w_putblock (b, window, x + look_3d + 1, y + look_3d + 1);
      w_hline (window, x + 1, y + 1, x + w - 3);
      w_vline (window, x + 1, y + 1, y + h - 3);
      w_setmode (window, M_CLEAR);
      w_hline (window, x + 2, y + w - 2, x + w - 2);
      w_vline (window, x + w - 2, y + 2, y + h - 2);
    }
    else {
      w_putblock (b, window, x + look_3d, y + look_3d);
      w_hline (window, x + 2, y + h - 2, x + h - 2);
      w_vline (window, x + w - 2, y + 2, y + h - 2);
      w_setmode (window, M_CLEAR);
      w_hline (window, x + 1, y + 1, x + w - 3);
      w_vline (window, x + 1, y + 1, y + h - 3);
    }
  }
  else {
    w_putblock (b, window, x, y);
    if (state == ICON_SELECTED) {
      w_setmode (window, M_INVERS);
      w_pbox (window, x, y, w, h);
    }
  }
}

/*
 * Draws icons of commands depending on alignment
 * set in window_vertical.
 */
static void draw_window (void) {
  short i;
  short h;
  short w;
  short x = 0;
  short y = 0;

  for (i = 0; i < commands; i++) {
    if (command [i].type == COMMAND_SWALLOW) {
      w = h = swallow_size + look_3d * 2;
 
      if (look_3d) {
        w_setmode (window, M_DRAW);
        w_box (window, x, y, w, h);
        w_hline (window, x + 2, y + h - 2, x + h - 2);
        w_vline (window, x + w - 2, y + 2, y + h - 2);
      }

      w_move (command [i].p.window, x + look_3d, y + look_3d);
    }
    else {
      draw_icon (command [i].p.bitmap, x, y, ICON_NORMAL);
      w = command [i].p.bitmap->width + look_3d * 2;
      h = command [i].p.bitmap->height + look_3d * 2;
    }

    if (window_vertical) {
      y += h;
    }
    else {
      x += w;
    }
  }
}



/*
 * Calculates window_w and window_h depending on alignment
 * set in window_vertical.
 */
static void calculate_wh (void) {
  short i;
  short w;
  short h;

  window_w = 0;
  window_h = 0;

  for (i = 0; i < commands; i++) {
    if (command [i].type == COMMAND_SWALLOW) {
      w = h = swallow_size;
    }
    else {
      w = command [i].p.bitmap->width;
      h = command [i].p.bitmap->height;
    }

    w += look_3d * 2;
    h += look_3d * 2;

    if (window_vertical) {
      window_h += h;
      if (window_w < w) {
        window_w = w;
      }	
    }
    else {
      window_w += w;
      if (window_h < h) {
        window_h = h;
      }
    }
  }
}



/*
 * Calculates new cooridinates and sizes for window.
 */
static void calculate_xywh (int position) {
  switch (position) {

    case WINDOW_TOPLEFT:
      window_vertical = 0;
      calculate_wh ();
      window_x = 0;
      window_y = 0;
      break;

    case WINDOW_TOPRIGHT:
      window_vertical = 0;
      calculate_wh ();
      window_x = WROOT->width - window_w - window_border * 2;
      window_y = 0;
      break;

    case WINDOW_RIGHTTOP:
      window_vertical = 1;
      calculate_wh ();
      window_x = WROOT->width - window_w - window_border * 2;
      window_y = 0;
      break;

    case WINDOW_RIGHTBOTTOM:
      window_vertical = 1;
      calculate_wh ();
      window_x = WROOT->width - window_w - window_border * 2;
      window_y = WROOT->height - window_h - window_border * 2;
      break;

    case WINDOW_BOTTOMRIGHT:
      window_vertical = 0;
      calculate_wh ();
      window_x = WROOT->width - window_w - window_border * 2;
      window_y = WROOT->height - window_h - window_border * 2;
      break;

    case WINDOW_BOTTOMLEFT:
      window_vertical = 0;
      calculate_wh ();
      window_x = 0;
      window_y = WROOT->height - window_h - window_border * 2;
      break;

    case WINDOW_LEFTBOTTOM:
      window_vertical = 1;
      calculate_wh ();
      window_x = 0;
      window_y = WROOT->height - window_h - window_border * 2;
      break;

    case WINDOW_LEFTTOP:
      window_vertical = 1;
      calculate_wh ();
      window_x = 0;
      window_y = 0;
      break;

    case WINDOW_USER:
      calculate_wh ();
      if (window_x > WROOT->width - window_w - window_border * 2) {
        window_x = WROOT->width - window_w - window_border * 2;
      }
      if (window_y > WROOT->height - window_h - window_border * 2) {
        window_y = WROOT->height - window_h - window_border * 2;
      }
      break;

    default:
      myerror ("unknwon mode for calculate_xywh");
  }
}


/*
 * Returns 1 if error.
 */
static short command_swallow (char *parameter) {
  short ret = 0;
  char argument[14 + 2*sizeof(ulong)];
  char **argv;

  command [commands].type = COMMAND_SWALLOW;
/* Maybe the child's window should be configurable? */
  if (!(command [commands].p.window =
          w_createChild (window, swallow_size, swallow_size,
                         W_NOBORDER | W_MOVE | W_CONTAINER))) {
    myerror ("wlaunch: couldn't create child window\n");
    ret = 1;
  }
  else {
/* Child window returns NULL! */
    w_open (command [commands].p.window, 0, 0);

    sprintf (argument, " --parent=0x%lx", w_winID(command [commands].p.window));
    strcat (parameter, argument);
    argv = create_argv (parameter);

    switch ((command [commands].a.pid = fork ())) {

      case -1:
        myerror ("wlaunch: couldn't fork child");
        w_delete (command [commands].p.window);
        ret = 1;
        break;

      case 0:
        execvp (argv [0], argv);
        myerror ("wlaunch: couldn't exec child");
        exit (EXIT_FAILURE);

      default:
        break;
    }

    delete_argv (argv);
  }

  return ret;
}



short command_alignment (char *parameter) {
  if (!strcmp (parameter, "TopLeft")) {
    window_alignment = WINDOW_TOPLEFT;
  }
  else if (!strcmp (parameter, "TopRight")) {
    window_alignment = WINDOW_TOPRIGHT;
  }
  else if (!strcmp (parameter, "RightTop")) {
    window_alignment = WINDOW_RIGHTTOP;
  }
  else if (!strcmp (parameter, "RightBottom")) {
    window_alignment = WINDOW_RIGHTBOTTOM;
  }
  else if (!strcmp (parameter, "BottomRight")) {
    window_alignment = WINDOW_BOTTOMRIGHT;
  }
  else if (!strcmp (parameter, "BottomLeft")) {
    window_alignment = WINDOW_BOTTOMLEFT;
  }
  else if (!strcmp (parameter, "LeftBottom")) {
    window_alignment = WINDOW_LEFTBOTTOM;
  }
  else if (!strcmp (parameter, "LeftTop")) {
    window_alignment = WINDOW_LEFTTOP;
  }
  else {
    myerror ("wlaunch: unknown parameter for Alignment");
  }

  return 1;
};



short command_look (char *parameter) {
  if (!strcmp (parameter, "Flat")) {
    look_3d = 0;
  }
  else if (!strcmp (parameter, "3D")) {
    look_3d = 2;
  }
  else {
    myerror ("wlaunch: unknown parameter for Look");
  }

  return 1;
}



short command_border (char *parameter) {
  if (!strcmp (parameter, "On")) {
    window_border = BORDERWIDTH;
  }
  else if (!strcmp (parameter, "Off")) {
    window_border = 0;
  }
  else {
    myerror ("wlaunch: unknown parameter for Border");
  }

  return 1;
}



short command_ontop (char *parameter) {
  if (!strcmp (parameter, "Yes")) {
    window_ontop = WINDOW_ONTOP;
  }
  else if (!strcmp (parameter, "No")) {
    window_ontop = WINDOW_NORMAL;
  }
  else {
    myerror ("wlaunch: unknown parameter for OnTop");
  }

  return 1;
}



short command_swallowsize (char *parameter) {
  sscanf (parameter, "%i", &swallow_size);

  return 1;
}



/*
 * Parses rc file.
 *
 * First looks for $HOME/.wlaunchrc and then in $DATADIR/wlaunchrc.
 * Fills up command array if line contains a icon name and a command
 * string. Icon name and command string are seperated by whitespaces,
 * comments start with a `#' at the beginning of line, leading
 * whitespaces are ignored in both cases.
 * Returns 1 if no rc file found, no or no valid icon names and
 * command strings in rc file
 */
static short parse_rc_file (short pass) {
  FILE *file_handle = NULL;
  char line_buffer [RCLINE_MAX + 1]; /* plus '\0' */
  char *command_string = NULL;
  char *icon_name;
  char *p;
  char file_name [NAME_MAX + 1];
  const char *env;
  struct internal_commands *p2;
  short ret = 1;

  if (rc) {
    file_handle = fopen (rc, "r");
  }
  else {
    if ((env = getenv ("HOME"))) {
      sprintf (file_name, "%s/.wlaunchrc", env);
      file_handle = fopen (file_name, "r");
    }

    if (!file_handle) {
      file_handle = fopen (DATADIR "/wlaunchrc", "r");
    }
  }

  if (!file_handle) {
    myerror ("wlaunch: couldn't open configuration file");
  }
  else {
    while (!feof (file_handle) && commands < COMMANDS) {
      fgets (line_buffer, RCLINE_MAX, file_handle);
      p = line_buffer;

/* strip newline */
      if (p [strlen (p) -1] == '\n') {
        p [strlen (p) - 1] = '\0';
      }

/* ignore whitespaces at begin of line */
      while (isspace (*p) && *p) {
        p++;
      } 

/* comment? */
      if (*p != '#' && *p) {
        icon_name = p;

        while (!isspace (*p) && *p) {
          p++;
        }

        if (*p) {
          *p++ = '\0';
          while (isspace (*p) && *p) {
            p++;
          }
          command_string = p;
        }

        if (command_string && icon_name) {
          ret = 0;
          p2 = internal_commands;
          while (p2->name) {
            if (!strcmp (icon_name, p2->name)) {
              break;
            }
            p2++;
          }

          if (pass == FIRST_PASS) {
            if (p2->name) {
              p2->function (command_string);
            }
          }
          else if (p2->name) {
          }
          else if (!strcmp (icon_name, "Swallow")) {
            if (!command_swallow (command_string)) {
              commands++;
              ret = 0;
            }
          }
          else {
            p = icon_name;
            if (*icon_name != '/') {
              p = file_name;
              env = getenv ("ICONDIR");
              if (!env) {
                env = icon_path;
              }
              sprintf (p, "%s/%s", env, icon_name);
            }

            if (!(command [commands].p.bitmap = w_readpbm (p))) {
              myerror ("couldn't load icon");
            }
            else {
              command [commands].a.string =
                malloc (strlen (command_string) + 1);
              strcpy (command [commands].a.string, command_string);
              command [commands].type = COMMAND_NORMAL;
              commands++;
              ret = 0;
            }
          }
        }
      }
    }
  }
  fclose (file_handle);

  return ret;
}



/*
 * Catches exit status of childs to avoid zombies.
 */
static void handle_SIGCHLD (int ignored) {
  int status;
  while (waitpid (-1, &status, WNOHANG) > 0);  
}



/*
 * Handles termination signals.
 * Kills every child with swallowed windows.
 */

static void handle_termination_signals (int ignored) {
  short i;

  for (i = 0; i < commands; i++) {
    if (command [i].type == COMMAND_SWALLOW) {
      kill (command [i].a.pid, SIGKILL);
    }
  }

  w_close (window);
  w_delete (window);
  w_exit ();
  exit (EXIT_SUCCESS);
}



/*
 * Displays usage help and exits.
 */
static void usage (void) {
  printf ("\
Usage: %s [OPTION]...\n\
Start launchbar. Default position is the lefttop border.\n\
\n\
  -f, --file=FILE            use FILE as configuration file\n\
  -h, --help                 display this help and exit\n\
  -i PATH, --iconpath=PATH   Overwrite default icon search path\n\
  -v, --version              output version information and exit\n\
\n\
Report bugs to Jan.P.Schmidt@mni.fh-giessen.de\n\
", basename (program_name));
  exit (EXIT_SUCCESS);
}




/*
 * This is, where's it at...
 */
int main (int argc, char *argv []) {
  WEVENT *event;
  short i;
  short y;
  short x;
  short show_version = 0;
  short show_usage = 0;
  int c;

  program_name = argv [0];

  while ((c = getopt_long (argc, argv, "f:hi:v", long_options, NULL))
          != -1) {
    switch (c) {

      case 'f':
        rc = optarg;
        break;

      case 'h':
        show_usage = 1;
        break;

      case 'i':
        icon_path = optarg;
        break;

      case 's':
        sscanf (optarg, "%i", &swallow_size);
        break;

      case 'v':
        show_version = 1;
        break;

      case '?':
        exit (EXIT_FAILURE);

      default:
        break;
    }
  }

  if (show_version) {
    printf ("%s as of %s\n", basename (program_name), __DATE__);
    exit (EXIT_SUCCESS);
  }

  if (show_usage) {
    usage ();
  }

  switch (fork ()) {
    case -1:
      myerror ("wlaunch: couldn't detach");
      exit (EXIT_FAILURE);

    case 0:
      break;

    default:
      exit (EXIT_SUCCESS);      
  }

  signal (SIGCHLD, handle_SIGCHLD);
  signal (SIGTTOU, SIG_IGN);

  signal(SIGHUP, handle_termination_signals);
  signal(SIGINT, handle_termination_signals);
  signal(SIGQUIT, handle_termination_signals);
  signal(SIGABRT, handle_termination_signals);
  signal(SIGTERM, handle_termination_signals);

  if (parse_rc_file (FIRST_PASS)) {
    exit (EXIT_SUCCESS);
  }

  if (!w_init ()) {
    myerror ("wlaunch: couldn't connect W server");
    exit (EXIT_FAILURE);
  }

  if (window_ontop) {
    window_flags |= W_TOP;
  }

  if (!window_border) {
    window_flags |= W_NOBORDER;
  }

  if (!(window = w_create (42, 42, window_flags))) {
    myerror ("wlaunch: couldn't create window");
    w_exit ();
    exit (EXIT_FAILURE);
  }

  if (parse_rc_file (SECOND_PASS)) {
    w_delete (window);
    w_exit ();
    exit (EXIT_FAILURE);
  }

  calculate_xywh (window_alignment);
  w_resize (window, window_w, window_h);
  draw_window ();
  if (w_open (window, window_x, window_y)) {
    w_delete (window);
    w_exit ();
    myerror ("wlaunch: couldn't open window");
    exit (EXIT_FAILURE);
  }

/* ... and don't come back now, hear? */
  while (42) {
    if ((event = w_queryevent (NULL, NULL, NULL, -1))) {
      switch (event->type) {
 
        case EVENT_GADGET:

	  switch (event->key) {

	    case GADGET_CLOSE:
              w_close (window);
	      w_delete (window);
              w_exit ();
	      exit (EXIT_SUCCESS);

	    case GADGET_EXIT:
	      exit (EXIT_SUCCESS);
	  }
	  break;

        case EVENT_RESIZE:
          if (window_vertical) {
            if (event->w > window_w) {
              window_vertical = 0;
            }
            else {
              break;
            }
          }
          else {
            if (event->h > window_h) {
              window_vertical = 1;
            }
            else {
              break;
            }
          }
          window_x = event->x;
          window_y = event->y;
          calculate_xywh (WINDOW_USER);
          w_close (window);
          w_resize (window, window_w, window_h);
          draw_window ();
          w_open (window, window_x, window_y);
          break;

        case EVENT_MPRESS:
          if (event->key != BUTTON_LEFT) {
            break;
          }

          x = 0;
          y = 0;
          for (i = 0; i < commands; i++) {
            if (command [i].type == COMMAND_SWALLOW) {
              if (window_vertical) {
                y += swallow_size + look_3d * 2;
              }
              else {
                x += swallow_size + look_3d * 2;
              }
            }
            else {
              BITMAP *b = command [i].p.bitmap;
              short w = b->width + look_3d * 2;
              short h = b->height + look_3d * 2;

              if (event->x >= x && event->x < x + w) {
                if (event->y >= y && event->y < y + h) {
                  char **argv;

/* Give the user some feedback. Peeeeeep ... */
                  draw_icon (b, x, y, ICON_SELECTED);
                  while (w_queryevent (NULL, NULL, NULL, 250));
                  draw_icon (b, x, y, ICON_NORMAL);

/* And now, go for it ... */
                  if (!(argv = create_argv (command [i].a.string))) {
                    break;
                  }

                  switch (fork ()) {

                    case -1:
                       myerror ("couldn't fork child");
                       break;

                    case 0:
                       w_exit ();
                       execvp (argv [0], argv);
                       myerror ("couldn't exec program");
                       exit (EXIT_FAILURE);

                    default:
		      /* othwise it's child, don't wait for them */
		      break;
                  }
                  delete_argv (argv);
                  break;
                }
              }
              if (window_vertical) {
                y += h;
              }
              else {
                x += w;
              }
            }
          }
      }
    }
  }
}
