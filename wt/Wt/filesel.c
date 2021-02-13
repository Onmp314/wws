/* 
 * A fileselector widget for W toolkit
 * 
 * (w) 1996 by Eero Tamminen
 *
 * Changes:
 * - folders and files into separate listboxes
 * - different sorting options
 * - show the total size of masked, regular files only, instead of all
 * - resizes properly (oddie 08/2000)
 * - fix path getopt types (eero 07/2008)
 *
 * Possible TODOs:
 * - path and type shortcuts (with popups?)
 * - configuration file
 *
 * $Id: filesel.c,v 1.4 2008-08-29 19:47:09 eero Exp $
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <Wlib.h>
#include "Wt.h"
#include "toolkit.h"

typedef struct
{
  char dummy;
  void *dummy2;
} pointer_align_test;

/* directory name space allocator configuration defines */
#define POINTER_ALIGN		(sizeof(pointer_align_test)-sizeof(void*))
#define BLOCK_SIZE		1024

/* entry in the read directory */
typedef struct ENTRY
{
  struct ENTRY *next;
  long size;
  int flags;
} Entry;
/* followed by filename */

/* Memory handling / usage:
 *
 * pointer aligning is needed for the structures after strings in the
 * blocks.  One block can contain at least (BLOCK_SIZE / (NAME_MAX +
 * POINTER_ALIGN + sizeof(void*) * 3)) directory entries.  Using
 * sort_entries() will take further sizeof(char*) / entry.
 */


/* stuff in the masked & sorted directory */
typedef struct
{
  char **filelist;	/* NULL terminated list of file names */
  char **dirlist;	/* -"- list of directory names */
  int files;		/* number of files */
  int dirs;		/* number of dirs */
  long bytes;
} Directory;


/* directory entry file type flags */
#define FLAG_DIR	1

/* lenght of the pattern for the shown filenames on screen */
#define TYPE_WIDTH	12

/* maximum lenghts (at least 30) for certain strings */
#define NAME_WIDTH	32

/* directory / file listbox def. width / height in chars */
#define LIST_LINES	8
#define LIST_WIDTH	12


/* for (atomic) fileinfo widget updates */
char Buffer[NAME_WIDTH+1];


typedef struct
{
  widget_t w;
  short queried;
  short is_realized;
  widget_t *shell;
	widget_t *vpane,*hpane1;
  widget_t *type;
  widget_t *info;
  widget_t *name;
  widget_t *path;
  widget_t *namelist;
  widget_t *pathlist;
  char *memend;				/* last allocated block */
  Entry *start;				/* first directory entry */
  int dirs, files;			/* number of all entries */
  Directory dirinfo;			/* sorted name lists for listboxes */
  char pathbuf[_POSIX_PATH_MAX+1];	/* path parsing buffer */
  int (*sort_cb)(const void *a, const void *b);		/* sort function */
  void (*action_cb)(widget_t *w, char *filename);	/* selection callback */
} filesel_widget_t;


/**********************************************************************/

/*
 * glob-style filename matching: does *, ? and []
 */
static int
mask_name (register const char *s, register const char *p)
{
  register int scc;
  int ok, lc;
  int c, cc;

  for (;;) {
    scc = *s++ & 0177;
    switch ((c = *p++))
    {
      case '[':
	ok = 0;
	lc = 077777;
	while ((cc = *p++)) {
	  if (cc == ']') {
	    if (ok)
	      break;
	    return 0;
	  }
	  if (cc == '-') {
	    if (lc <= scc && scc <= *p++)
	      ok++;
	  } else
	    if (scc == (lc = cc))
	      ok++;
        }
	if (cc == 0)
	{
	  if (ok)
	    p--;
	  else
	    return 0;
	}
	continue;

      case '*':
	if (!*p)
	  return 1;
	s--;
	do {
	  if (mask_name(s, p))
	    return 1;
	} while (*s++);
	return 0;

      case '?':
	if (scc == 0)
	  return 0;
	continue;

      case 0:
	return (scc == 0);

      default:
	if (c != scc)
	  return 0;
	continue;
    }
  }
}

static int locate_first(const char *string, char **list, int items)
{
  int index, len = strlen (string);

  if(!(len && list && items))
    return -1;

  for(index = 0; index < items; index++)
  {
    if (!strncmp (string, *list++, len))
      return index;
  }
  return -1;
}

/**********************************************************************/

/*
 * Directory access --- The POSIX way (I hope <g>)
 */

static void free_entries(filesel_widget_t *w);

/* get directory entry information and copy it into memory */
static inline void get_stats(filesel_widget_t *w, const char *name, long *size, int *flags)
{
  struct stat st;
  *flags = 0;

  /* get file information */
  if(stat(name, &st) < 0)
    *size = 0;
  else
  {
    *size = st.st_size;
    if(S_ISDIR(st.st_mode))
      *flags |= FLAG_DIR;
    /* set other flags */
  }
}

/* read all the directory entries + handle memory allocation */
static int read_entries(filesel_widget_t *w, const char *dir)
{
  char *new_block;
  struct dirent *entry;
  Entry *current, *prev;
  int len, length, mem_left, flags;
  long size;
  DIR *dfd;

  if(!(dfd = opendir(dir)))
    return 0;

  if(w->memend)
    free_entries(w);

  if(!(w->memend = malloc(BLOCK_SIZE)))
  {
    closedir(dfd);
    return 0;
  }
  *(char **)w->memend = NULL;			/* this is the first block */
  w->start = (Entry *)(w->memend + sizeof(char *));
  mem_left = BLOCK_SIZE - sizeof(char *);
  current = prev = w->start;
  w->files = w->dirs = 0;

  while((entry = readdir(dfd)))
  {
    /* get directory entry information */
    get_stats(w, entry->d_name, &size, &flags);
    len = strlen(entry->d_name);
    if(flags & FLAG_DIR)
      len++;

    /* takes into account string termination zero */
    length = (sizeof(Entry) + len + POINTER_ALIGN) & ~(POINTER_ALIGN-1);

    /* allocate more memory if needed */
    if(mem_left < length)
    {
      if(!(new_block = malloc(BLOCK_SIZE)))
	break;					/* out of memory */

      *(char **)new_block = w->memend;
      current = (Entry *)(new_block + sizeof(char *));
      mem_left = BLOCK_SIZE - sizeof(char *);
      w->memend = new_block;
    }
    /* set entry information */
    current->size = size;
    current->flags = flags;
    strcpy((char*)(current+1), entry->d_name);
    if(flags & FLAG_DIR)
    {
      new_block = (char*)(current+1) + len;
      *(new_block--) = '\0';
      *new_block = '/';
      w->dirs++;
    }
    else
      w->files++;

    /* set next entry */
    prev->next = current;
    prev = current;
    current = (Entry *)((char *)current + length);
    mem_left -= length;
  }
  prev->next = NULL;
  closedir(dfd);
  return 1;
}

/* filename compare */
static int name_compare(const void *a, const void *b)
{
  return strcmp(*(char * const *)a, *(char * const *)b);
}

/* reverse size compare */
static int size_compare(const void *a, const void *b)
{
  int prev, next;
  prev = ((*(Entry* const*)a)-1)->size;
  next = ((*(Entry* const*)b)-1)->size;
  if(prev > next)
    return -1;
  if(prev < next)
    return 1;
  return 0;
}

/* filetype compare */
static int type_compare(const void *a, const void *b)
{
  const char *pdot, *ndot, *prev = *(char* const*)a, *next = *(char* const*)b;
  pdot = prev-1;
  while(*prev)
  {
    if(*prev++ == '.')
      pdot = prev;
  }
  ndot = next-1;
  while(*next)
  {
    if(*next++ == '.')
      ndot = next;
  }
  if(pdot < *(char* const*)a)		/* no type? */
  {
    if(ndot < *(char* const*)b)		/* neither? */
      return 0;
    return -1;
  }
  if(ndot < *(char* const*)b)
    return 1;

  return strcmp(pdot, ndot);
}

/* sort directories and files separately after each other into same list.
 * files are filtered with the 'type' mask.
 */
static int sort_entries(filesel_widget_t *w, const char *type)
{
  Entry *current = w->start;
  char **dirlist, **filelist;
  long bytes = 0;
  int files;

  if(!(current && type &&  w->dirs + w->files))
    return 0;

  /* not just resort? */
  if(!w->dirinfo.dirlist)
  {
    w->dirinfo.dirlist = malloc((w->dirs + w->files + 2) * sizeof(char*));
    if(!w->dirinfo.dirlist)
      return 0;
  }
  dirlist = w->dirinfo.dirlist;
  filelist = w->dirinfo.filelist = dirlist + w->dirs + 1;
  files = 0;

  do
  {
    if(current->flags & FLAG_DIR)
    {
      /* name is right behind the structure... */
      *(dirlist++) = (char*)(current+1);
    }
    else
    {
      *filelist = (char*)(current+1);

      /* if no type or type matched, accept the name */
      if(!*type || mask_name(*filelist, type))
      {
        bytes += current->size;
        filelist++;
	files++;
      }
    }
    current = current->next;
  } while(current);
  *filelist = NULL;
  *dirlist = NULL;

  if(w->sort_cb)
  {
    /* first sort by name... */
    qsort(w->dirinfo.dirlist, w->dirs, sizeof(char*), name_compare);
    qsort(w->dirinfo.filelist, files, sizeof(char*), name_compare);
    /* ...then by other ways */
    if(w->sort_cb != name_compare)
      qsort(w->dirinfo.filelist, files, sizeof(char*), w->sort_cb);
  }
  w->dirinfo.bytes = bytes;
  w->dirinfo.files = files;
  w->dirinfo.dirs  = w->dirs;
  return 1;	/* alles ok */
}

/* free all the allocated spaces */
static void free_entries(filesel_widget_t *w)
{
  char *previous;

  /* free lists */
  if(w->dirinfo.dirlist)
  {
    free(w->dirinfo.dirlist);
    w->dirinfo.filelist = w->dirinfo.dirlist = NULL;
  }

  /* free blocks */
  while(w->memend)
  {
    previous = *(char **)w->memend;
    free(w->memend);
    w->memend = previous;
  }
}


/*
 * Options dialog handling
 */

static void show_newlist(filesel_widget_t *w);

static void resort(filesel_widget_t *w)
{
  char *type;
  (*wt_getstring_class->getopt)(w->type, WT_STRING_ADDRESS, &type);
  sort_entries(w, type);
  show_newlist(w);
}

static void unsort_cb(widget_t *_w)
{
  /* filesel <- shell <- vpane <- hpane <- widget */
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  if(w->sort_cb)
  {
    w->sort_cb = NULL;
    resort(w);
  }
}

static void namesort_cb(widget_t *_w)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  if(w->sort_cb != name_compare)
  {
    w->sort_cb = name_compare;
    resort(w);
  }
}

static void sizesort_cb(widget_t *_w)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  if(w->sort_cb != size_compare)
  {
    w->sort_cb = size_compare;
    resort(w);
  }
}

static void typesort_cb(widget_t *_w)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  if(w->sort_cb != type_compare)
  {
    w->sort_cb = type_compare;
    resort(w);
  }
}

/*
 * Fileselector functionality
 */

/* show file information.
 * *NOTE*: 'name' is supposed to be part of an Entry struct!!!!
 */
static void show_fileinfo(filesel_widget_t *w, char const *name)
{
  sprintf(Buffer, "%ld bytes", ((const Entry *)name - 1)->size);
  (*wt_label_class->setopt)(w->info, WT_LABEL, Buffer);
}

/* autolocator and filename completion */
static char *complete_name(filesel_widget_t *w, const char *name)
{
  long idx;

  idx = locate_first(name, w->dirinfo.filelist, w->dirinfo.files);
  (*wt_listbox_class->setopt)(w->namelist, WT_CURSOR, &idx);
  if(idx >= 0)
  {
    char *item = w->dirinfo.filelist[idx];
    if(strcmp(name, item))
    {
      /* complete filename */ 
      (*wt_getstring_class->setopt)(w->name, WT_STRING_ADDRESS, item);
      show_fileinfo(w, item);
      return NULL;
    }
    return item;
  }
  return NULL;
}

/* show information for new or re-sorted directory */
static void show_newlist(filesel_widget_t *w)
{
  char *file;

  (*wt_listbox_class->setopt)(w->pathlist, WT_LIST_ADDRESS, w->dirinfo.dirlist);
  (*wt_listbox_class->setopt)(w->namelist, WT_LIST_ADDRESS, w->dirinfo.filelist);

  sprintf(Buffer, "%10ld bytes in %4d files", w->dirinfo.bytes,
	  w->dirinfo.dirs + w->dirinfo.files);
  (*wt_label_class->setopt)(w->info, WT_LABEL, Buffer);

  (*wt_getstring_class->getopt)(w->name, WT_STRING_ADDRESS, &file);
  if (file && *file)
    complete_name(w, file);
}

static int get_directory(filesel_widget_t *w, const char *dir)
{
  char *type, *oldpath;
  int len, ok;

  /* change to the directory and read entries */
  oldpath = getcwd(NULL, _POSIX_PATH_MAX);
  chdir(w->pathbuf);
  chdir(dir);			/* stat needs us to be in the directory */

  if((ok = read_entries(w, ".")))
  {
    /* if reading succeeded, show the path */
    getcwd(w->pathbuf, _POSIX_PATH_MAX);
    if((len = strlen(w->pathbuf)) > 1)
    {
      /* not root, add path mark */
      w->pathbuf[len++] = '/';
      w->pathbuf[len] = '\0';
    }
    (*wt_getstring_class->setopt)(w->path, WT_STRING_ADDRESS, w->pathbuf);
  }
  if(oldpath)
  {
    chdir(oldpath);
    free(oldpath);
  }
  if(ok)
  {
    /* mask and sort entries before showing them */
    (*wt_getstring_class->getopt)(w->type, WT_STRING_ADDRESS, &type);
    if(sort_entries(w, type))
    {
      show_newlist(w);
      return 1;
    }
  }
  (*wt_listbox_class->setopt)(w->pathlist, WT_LIST_ADDRESS, "");
  (*wt_listbox_class->setopt)(w->namelist, WT_LIST_ADDRESS, "");
  return 0;
}

/* just locating, no cd */
static char *path_change_cb(widget_t *_w, const char *path)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent);
  long idx;

  for(idx = 0; path[idx]; idx++);
  while(idx >= 0 && path[idx] != '/')
    idx--;
  idx++;
  idx = locate_first(&path[idx], w->dirinfo.dirlist, w->dirinfo.dirs);
  (*wt_listbox_class->setopt)(w->pathlist, WT_CURSOR, &idx);
  if(idx >= 0)
    return w->dirinfo.dirlist[idx];
  return NULL;
}

/* just locating, no completion */
static char *name_change_cb(widget_t *_w, const char *name)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent);
  long idx;

  idx = locate_first(name, w->dirinfo.filelist, w->dirinfo.files);
  (*wt_listbox_class->setopt)(w->namelist, WT_CURSOR, &idx);
  if(idx >= 0)
  {
    show_fileinfo(w, w->dirinfo.filelist[idx]);
    return w->dirinfo.filelist[idx];
  }
  return NULL;
}

/* fileselector <- shell <- vpane <- widget */

static void path_cb(widget_t *_w, char *text)
{
  char *path = path_change_cb(_w, text);
  if(path)
    w_flush();		/* show on screen which dir... */
  else
    path = text;
  get_directory((filesel_widget_t*)(_w->parent->parent->parent), path);
}

static void name_cb(widget_t *_w, char *name)
{
  complete_name((filesel_widget_t*)(_w->parent->parent->parent), name);
}

/* fileselector <- shell <- vpane <- hpane <- widget */

static void pathlist_change_cb(widget_t *_w, char *path)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  (*wt_getstring_class->setopt)(w->path, WT_STRING_ADDRESS, path);
}

static void pathlist_cb(widget_t *_w, char *path)
{
  get_directory((filesel_widget_t*)(_w->parent->parent->parent->parent), path);
}

static void namelist_cb(widget_t *_w, char *name)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  (*wt_getstring_class->setopt)(w->name, WT_STRING_ADDRESS, name);
  show_fileinfo(w, name);
}

static WEVENT * route_pathlist_key(widget_t *_w, WEVENT *ev)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  /* route keys unknown to pathlist into path widget */
  return (*wt_getstring_class->event)(w->path, ev);
}

static WEVENT * route_namelist_key(widget_t *_w, WEVENT *ev)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  /* route keys unknown to namelist into name widget */
  return (*wt_getstring_class->event)(w->name, ev);
}

/* fileselector <- shell <- vpane <- hpane <- widget */

static void type_cb(widget_t *_w, const char *mask, int cursor)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  char *type;

  /* re-sort stuff */
  (*wt_getstring_class->getopt)(w->type, WT_STRING_ADDRESS, &type);
  sort_entries(w, type);
  show_newlist(w);
}

static void ok_cb(widget_t *_w, int down)
{
  filesel_widget_t *w = (filesel_widget_t*)(_w->parent->parent->parent->parent);
  char *name, *file;

  if(down)
    return;

  (*wt_getstring_class->getopt)(w->name, WT_STRING_ADDRESS, &name);
  file = complete_name(w, name);
  if(!file)
    file = name;

  if(w->action_cb)
  {
    int len = strlen(w->pathbuf);
    char *buf;

    if((buf = malloc(len + strlen(file) + 1)))
    {
      strcpy(buf, w->pathbuf);
      strcpy(&buf[len], file);
      /* may remove the fileselector, so buffer has to be separate
       * and fileselector may not be referenced anymore
       */
      w->action_cb((widget_t*)w, buf);
      free(buf);
    }
  }
}

static void cancel_cb(widget_t *_w, int down)
{
  filesel_widget_t *w = (filesel_widget_t *)_w->parent->parent->parent->parent;
  if (!down) {
    if (w->action_cb)
      (*w->action_cb) ((widget_t *)w, NULL);
  }
}

static void closer_cb(widget_t *_w)
{
  filesel_widget_t *w = (filesel_widget_t *)_w->parent;
  if (w->action_cb)
    (*w->action_cb) ((widget_t *)w, NULL);
}

static void resize_cb(widget_t *_w,long owd,long oht)
{
	filesel_widget_t *w = (filesel_widget_t *)_w->parent;
	long wd,ht,lbht;
	
	wt_geometry(_w,NULL,NULL,&wd,&ht);
	wt_geometry(w->hpane1,NULL,NULL,NULL,&lbht);
	lbht=lbht+ht-oht;
	
	wt_setopt(w->hpane1,WT_HEIGHT,&lbht,WT_EOL);
	wt_reshape(w->vpane,WT_UNSPEC,WT_UNSPEC,wd,ht);
}

/*
 * Widget handling
 */

static long filesel_init (void)
{
  /* could read config file from `~/.filesel'... */
  return 0;
}

static widget_t *filesel_create(widget_class_t *cp)
{
  filesel_widget_t *w = calloc (1, sizeof (filesel_widget_t));
  widget_t  *hpane2, *hpane3,
		*unsort, *name, *size, *type, *ok, *cancel;
  long a, b;

  if (!w)
    return NULL;
  w->w.class = wt_filesel_class;

  /* create the widget hierarchy */
  if(!(w->shell = wt_create(wt_shell_class, NULL)))
  {
    free(w);
    return NULL;
  }
#if 0
  w->vpane  = wt_create(wt_packer_class,      w->shell);
#else
  w->vpane  = wt_create(wt_pane_class,        w->shell);
#endif
  w->path    = wt_create(wt_getstring_class,   w->vpane);
  w->name    = wt_create(wt_getstring_class,   w->vpane);
  w->hpane1  = wt_create(wt_pane_class,        w->vpane);
  w->pathlist= wt_create(wt_listbox_class,     w->hpane1);
  w->namelist= wt_create(wt_listbox_class,     w->hpane1);
  w->info    = wt_create(wt_label_class,       w->vpane);
  hpane2     = wt_create(wt_pane_class,        w->vpane);
  unsort     = wt_create(wt_radiobutton_class, hpane2);
  name       = wt_create(wt_radiobutton_class, hpane2);
  size       = wt_create(wt_radiobutton_class, hpane2);
  type       = wt_create(wt_radiobutton_class, hpane2);
  hpane3     = wt_create(wt_pane_class,        w->vpane);
  w->type    = wt_create(wt_getstring_class,   hpane3);
  ok         = wt_create(wt_button_class,      hpane3);
  cancel     = wt_create(wt_button_class,      hpane3);

  if(!(cancel && ok && type && size && name && unsort && w->type && w->info
  && w->pathlist && w->namelist && w->name && w->path))
  {
    /* hmm...  this won't free them all if all the containers (panes)
     * didn't make it, but their intended children did because then they
     * would be on different hierarchies (very unlike I hope)...
     */
    wt_delete(w->shell);
    free(w);
    return NULL;
  }

  wt_add_after((widget_t *)w, NULL, w->shell);

  /* set options for widgets that will not change */
	/* hpane1 is now actually in the widget structure but its option setting
			is here anyway - oddie */
  a = OrientHorz;
  (*wt_pane_class->setopt)(w->hpane1, WT_ORIENTATION, &a); 
  (*wt_pane_class->setopt)(hpane2, WT_ORIENTATION, &a);
  (*wt_pane_class->setopt)(hpane3, WT_ORIENTATION, &a);
  a = AlignFill;
  (*wt_pane_class->setopt)(w->hpane1, WT_ALIGNMENT, &a);
  (*wt_pane_class->setopt)(hpane2, WT_ALIGNMENT, &a);

  wt_setopt(ok,     WT_LABEL, "OK",     WT_ACTION_CB, ok_cb, WT_EOL);
  wt_setopt(cancel, WT_LABEL, "Cancel", WT_ACTION_CB, cancel_cb, WT_EOL);
  wt_setopt(unsort, WT_LABEL, "None",   WT_ACTION_CB, unsort_cb, WT_EOL);
  wt_setopt(size,   WT_LABEL, "Size",   WT_ACTION_CB, sizesort_cb, WT_EOL);
  wt_setopt(type,   WT_LABEL, "Type",   WT_ACTION_CB, typesort_cb, WT_EOL);
  a = ButtonStatePressed;
  wt_setopt(name,
	WT_LABEL, "Name",
	WT_ACTION_CB, namesort_cb,
	WT_STATE, &a,
	WT_EOL);
  w->sort_cb = name_compare;


  /* set options for widgets in filesel_widget_t struct */

  wt_setopt(w->shell, WT_ACTION_CB, closer_cb, 
		WT_RESIZE_CB, resize_cb, WT_EOL);

  a = LabelModeWithBorder;
  (*wt_label_class->setopt)(w->info, WT_MODE, &a);

  a = _POSIX_PATH_MAX;
  b = NAME_WIDTH;
  wt_setopt(w->path,
	WT_STRING_LENGTH, &a,
	WT_STRING_WIDTH, &b,
	WT_CHANGE_CB, path_change_cb,
	WT_ACTION_CB, path_cb,
	WT_EOL);

  a = NAME_WIDTH;
  wt_setopt(w->name,
	WT_STRING_LENGTH, &a,
	WT_STRING_WIDTH, &a,
	WT_CHANGE_CB, name_change_cb,
	WT_ACTION_CB, name_cb,
	WT_EOL);

  a = TYPE_WIDTH;
  wt_setopt(w->type,
	WT_STRING_LENGTH, &a,
	WT_STRING_WIDTH, &a,
	WT_STRING_ADDRESS, "*",
	WT_ACTION_CB, type_cb,
	WT_EOL);

  a = LIST_WIDTH;
  b = LIST_LINES;
  wt_setopt(w->pathlist,
	WT_LIST_WIDTH, &a,
	WT_LIST_HEIGHT, &b,
	WT_INKEY_CB,  route_pathlist_key,
	WT_CHANGE_CB, pathlist_change_cb,
	WT_ACTION_CB, pathlist_cb,
	WT_EOL);
  wt_setopt(w->namelist,
	WT_LIST_WIDTH, &a,
	WT_LIST_HEIGHT, &b,
	WT_INKEY_CB,  route_namelist_key,
	WT_CHANGE_CB, namelist_cb,
	WT_ACTION_CB, namelist_cb,
	WT_EOL);

#if 0
  wt_setopt (w->vpane,
  	WT_PACK_WIDGET, w->path,
  	WT_PACK_WIDGET, w->name,
  	WT_PACK_INFO,   "-pady 1 -fill both -side top",
  	WT_PACK_WIDGET, w->listbox,
  	WT_PACK_INFO,   "-pady 1 -fill both -side top -expand 1",
  	WT_PACK_WIDGET, w->info,
  	WT_PACK_WIDGET, w->hpane,
  	WT_PACK_INFO,   "-pady 1 -fill both -side top",
	WT_EOL);
#else
	a=AlignFill;
	wt_setopt (w->vpane,
		WT_ALIGNMENT,&a,WT_EOL);
#endif
  return (widget_t *)w;
}

static long filesel_delete(widget_t *_w)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  wt_remove(w->shell);
  wt_delete(w->shell);
  free_entries(w);
  free(w);
  return 0;
}

static long filesel_close(widget_t *_w)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  (*wt_shell_class->close)(w->shell);
  return 0;
}

static long filesel_open(widget_t *_w)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  (*wt_shell_class->open)(w->shell);
  return 0;
}

static long filesel_addchild(widget_t *parent, widget_t *child)
{
  return -1;
}

static long filesel_delchild(widget_t *parent, widget_t *child)
{
  return -1;
}

static long filesel_realize(widget_t *_w, WWIN *parent)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;
	long mwd,mht;
  char *file;

  if(w->is_realized)
    return -1;

  if(w->pathbuf[0] != '/')
    getcwd(w->pathbuf, _POSIX_PATH_MAX);

  if(!get_directory(w, "."))
    return -1;

  if((*wt_shell_class->realize)(w->shell, parent) < 0)
    return -1;
	wt_geometry(w->shell,NULL,NULL,&mwd,&mht);
	wt_setopt(w->shell,WT_MIN_WIDTH,&mwd,WT_MIN_HEIGHT,&mht,WT_EOL);

  (*wt_getstring_class->getopt)(w->name, WT_STRING_ADDRESS, &file);
  if (file)
    complete_name (w, file);

  w->is_realized = 1;
  return 0;
}

static long
filesel_query_geometry(widget_t *_w, long *xp, long *yp, long *wdp, long *htp)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  (*wt_shell_class->query_geometry)(w->shell, xp, yp, wdp, htp);
  return 0;
}

static long
filesel_query_minsize(widget_t *_w, long *wdp, long *htp)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  (*wt_shell_class->query_minsize)(w->shell, wdp, htp);
  return 0;
}

static long
filesel_reshape (widget_t *_w, long x, long y, long wd, long ht)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  return (*wt_shell_class->reshape)(w->shell, x, y, wd, ht);
}

static long filesel_setopt (widget_t *_w, long key, void *val)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  switch (key)
  {
    case WT_XPOS:
    case WT_YPOS:
    case WT_WIDTH:
    case WT_HEIGHT:
    case WT_LABEL:
      (*wt_shell_class->setopt)(w->shell, key, val);
      break;

    case WT_LIST_HEIGHT:
      (*wt_listbox_class->setopt)(w->pathlist, key, val);
      (*wt_listbox_class->setopt)(w->namelist, key, val);
      break;

    case WT_FILESEL_PATH:
      if(w->is_realized)
        get_directory(w, (char*)val);
      else
      {
        strncpy(w->pathbuf, (char*)val, _POSIX_PATH_MAX);
	w->pathbuf[_POSIX_PATH_MAX] = '\0';
      }
      break;

    case WT_FILESEL_MASK:
      (*wt_getstring_class->setopt)(w->type, WT_STRING_ADDRESS, val);

      if(w->is_realized)
      {
        /* resort and show again */
        sort_entries(w, (char*)val);
        (*wt_listbox_class->setopt)(w->pathlist, WT_LIST_ADDRESS, w->dirinfo.dirlist);
        (*wt_listbox_class->setopt)(w->namelist, WT_LIST_ADDRESS, w->dirinfo.filelist);
      }
      break;

    case WT_FILESEL_FILE:
      (*wt_getstring_class->setopt)(w->name, WT_STRING_ADDRESS, val);
      if (w->is_realized) {
	complete_name (w, val);
      }
      break;

    case WT_ACTION_CB:
      w->action_cb = val;
      break;

    default:
      return -1;
  }
  return 0;
}

static long filesel_getopt (widget_t *_w, long key, void *val)
{
  filesel_widget_t *w = (filesel_widget_t *)_w;

  switch (key)
  {
    case WT_XPOS:
    case WT_YPOS:
    case WT_WIDTH:
    case WT_HEIGHT:
    case WT_LABEL:
      (*wt_shell_class->getopt)(w->shell, key, val);
      break;

    case WT_LIST_HEIGHT:
     (* wt_listbox_class->getopt)(w->pathlist, key, val);
      break;

    case WT_FILESEL_PATH:
      (*wt_getstring_class->getopt)(w->path, WT_STRING_ADDRESS, val);
      break;

    case WT_FILESEL_MASK:
      (*wt_getstring_class->getopt)(w->type, WT_STRING_ADDRESS, val);
      break;

    case WT_FILESEL_FILE:
      (*wt_getstring_class->getopt)(w->name, WT_STRING_ADDRESS, val);
      break;

    case WT_ACTION_CB:
      *(void **)val = w->action_cb;
      break;

    default:
      return -1;
  }
  return 0;
}

static WEVENT *filesel_event (widget_t *_w, WEVENT *ev)
{
	return ev;
}

static long filesel_changes (widget_t *w, widget_t *w2, short changes)
{
	return 0;
}

static widget_class_t _wt_filesel_class = {
	"filesel", 0,
	filesel_init,
	filesel_create,
	filesel_delete,
	filesel_close,
	filesel_open,
	filesel_addchild,
	filesel_delchild,
	filesel_realize,
	filesel_query_geometry,
	filesel_query_minsize,
	filesel_reshape,
	filesel_setopt,
	filesel_getopt,
	filesel_event,
	filesel_changes,
	filesel_changes
};

widget_class_t *wt_filesel_class = &_wt_filesel_class;
