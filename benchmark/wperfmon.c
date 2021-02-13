/*
 * wperfmon.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a performance monitor for W1
 *
 * CHANGES
 * ++eero, 6/98:
 * - Sun and netBSD use now util library getopt.
 * - Uses w_gettime() instead of earlier now() function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include <Wlib.h>

#define FNAME "fixed"
#define FSIZE 8
#define	REPS 5

#define WIDTH 320
#define HEIGHT 200

static int width, height, rows, columns;


/*
 * something...
 */

static WWIN *win;
static short fw, fh, end;


static void sigalrm(int sig)
{
  end = 1;
}


/*
 * some tests
 */

static void test_null(short reps, long sec)
{
  short i;
  long vorher, l, suml;
  float s, sums;

  printf("testing null operation...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    vorher = w_gettime();
    l = 0;
    end = 0;
    alarm(sec);
    while (!end) {
      l++;
      w_null();
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_plot(short reps, long sec)
{
  short x, y, i;
  long vorher, l, suml;
  float s, sums;

  printf("testing plot...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    x = 0;
    y = 0;
    l = 0;
    end = 0;
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    w_flush();
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_plot(win, x, y);
      l++;
      if ((x += 13) >= width) {
	x -= width;
	if (++y == height) {
	  y = 0;
	}
      }
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;

    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_line(short reps, long sec)
{
  short i, j;
  long vorher, l, suml;
  float s, sums;

  printf("testing line...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    l = 0;
    end = 0;
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    w_flush();
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      for (j=0; j<width; j++)
	w_line(win, j, 0, width-1-j, height-1);
      for (j=0; j<height; j++)
	w_line(win, 0, j, width-1, height-1-j);
      l += width + height;
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_vline(short reps, long sec)
{
  short x, i;
  long vorher, l, suml;
  float s, sums;

  printf("testing vline...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    x = 0;
    l = 0;
    end = 0;
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    w_flush();
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_vline(win, x, 0, height-1);
      l++;
      if (++x == width) {
	x = 0;
      }
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_hline(short reps, long sec)
{
  short y, i;
  long vorher, l, suml;
  float s, sums;

  printf("testing hline...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    y = 0;
    l = 0;
    end = 0;
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    w_flush();
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_hline(win, 0, y, width-1);
      l++;
      if (++y == height) {
	y = 0;
      }
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


#define POLYPOINTS 9

static void test_ppoly(short reps, long sec)
{
  short i;
  long vorher, l, suml;
  float s, sums;
  short points[POLYPOINTS<<1] = {0, height>>1, width>>2, 0, width>>1, height>>2, (width>>2)*3, 0, width-1, height>>1, (width>>2)*3, height-1, width>>1, (height>>2)*3, width>>2, height-1, 0, height>>1};

  printf("testing ppoly...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    l = 0;
    end = 0;
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    w_flush();

    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_ppoly(win, POLYPOINTS, points);
      l++;
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_circle(short reps, long sec)
{
  short x, y, i;
  long vorher, l, suml;
  float s, sums;

  printf("testing circle...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    x = 50;
    y = 50;
    l = 0;
    end = 0;
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    w_flush();
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_circle(win, x, y, 49);
      l++;
      if (++x >= width-50) {
	x = 50;
	if (++y >= height-50) {
	  y = 50;
	}
      }
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_pcircle(short reps, long sec)
{
  short x, y, i;
  long vorher, l, suml;
  float s, sums;

  printf("testing pcircle...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    x = 50;
    y = 50;
    l = 0;
    end = 0;
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    w_flush();
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_pcircle(win, x, y, 49);
      l++;
      if (++x >= width-50) {
	x = 50;
	if (++y >= height-50) {
	  y = 50;
	}
      }
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_bitblk(short reps, long sec)
{
  short i, j;
  long vorher, l, suml;
  float s, sums;

  printf("testing bitblk...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    for (j=0; j<width; j++)
      w_line(win, j, 0, width-1-j, height-1);
    for (j=0; j<height; j++)
      w_line(win, 0, j, width-1, height-1-j);
    w_flush();
    l = 0;
    end = 0;
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_bitblk(win, 1, 0, width-1, height, 0, 0);
      l++;
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_scroll(short reps, long sec)
{
  short i, j;
  long vorher, l, suml;
  float s, sums;

  printf("testing vscroll...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    w_setmode(win, M_CLEAR);
    w_pbox(win, 0, 0, width, height);
    w_setmode(win, M_INVERS);
    for (j=0; j<width; j++)
      w_line(win, j, 0, width-1-j, height-1);
    for (j=0; j<height; j++)
      w_line(win, 0, j, width-1, height-1-j);
    w_flush();
    l = 0;
    end = 0;
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_vscroll(win, 0, 1, width, height-1, 0);
      l++;
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_printc(short reps, long sec)
{
  short x, y, i;
  long vorher, l, suml;
  float s, sums;

  printf("testing character output...\n");

  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    x = 0;
    y = 0;
    l = 0;
    end = 0;
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      w_printchar(win, x, y, (l++ % 63) + 32);
      if ((x += fw) == width) {
	x = 0;
	if ((y += fh) == height) {
	  y = 0;
	}
      }
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


static void test_prints(short reps, long sec)
{
  short y, i, j;
  long vorher, l, suml;
  float s, sums;
  char buf[65];

  printf("testing string output...\n");

  buf[64] = 0;
  suml = 0;
  sums = 0;
  for (i=0; i<reps; i++) {
    y = 0;
    l = 0;
    end = 0;
    vorher = w_gettime();
    alarm(sec);
    while (!end) {
      for (j=0; j<64; j++) {
	buf[j] = (l++ % 63) + 32;
      }
      w_printstring(win, 0, y, buf);
      if ((y += fh) == height) {
	y = 0;
      }
    }
    w_test(win, 0, 0);
    s = (w_gettime() - vorher) / 1000.0;
    printf("%li ops in %f seconds -> %f ops/sec\n", l, s, l / s);
    suml += l;
    sums += s;
  }

  printf("average: %f secs/op, %f ops/sec\n", sums / suml, suml / sums);
}


/*
 *
 */

static int init (void)
{
  WSERVER *wserver;
  WFONT *wfont;

  struct sigaction sa;

  sa.sa_handler = sigalrm;
  sigemptyset(&sa.sa_mask);
#if defined(linux)
  sa.sa_flags = SA_RESTART;
#else
  sa.sa_flags = 0;
#endif
  sigaction(SIGALRM, &sa, NULL);

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: can't connect to wserver\n");
    return -1;
  }

  if (!(wfont = w_loadfont(FNAME, FSIZE, 0))) {
    fprintf(stderr, "error: can't load font\n");
    return -1;
  }

  if (wfont->flags & F_PROP) {
    fprintf(stderr, "error: this will not work with a proportional font\n");
    return -1;
  }

  fw = wfont->widths[0];
  fh = wfont->height;
  columns = WIDTH / fw;
  rows = HEIGHT / fh;
  width = columns * fw;
  height = rows * fh;

  if ((wserver->width < width) || (wserver->height < height)) {
    fprintf(stderr, "error: screen size (%ix%i) too small (wanted %ix%i)\n",
	    wserver->width, wserver->height, width, height);
    return -1;
  }

  if (!(win = w_create(width, height, W_TOP | W_NOBORDER))) {
    fprintf(stderr, "error: can't create window\n");
    return -1;
  }

  if (w_open(win, 0, 0) < 0) {
    w_delete(win);
    fprintf(stderr, "error: can't open window\n");
    return -1;
  }

  printf("This is WPERFMON, running on W%iR%i",
	 wserver->vmaj, wserver->vmin);
  if (wserver->pl)
    printf (" PL%i", wserver->pl);
  printf("\nscreen: %ix%i, %i colors\n",
	 wserver->width, wserver->height, 1 << wserver->planes);
  printf("window: %ix%i\n", width, height);
  printf("font: %ix%i (%s/%d)\n", fw, fh, FNAME, FSIZE);

  w_setfont(win, wfont);

  return 0;
}


/*
 * guess what...
 */

typedef struct {
  const char *name;
  int doit;
  void (*func)(short, long);
} TEST;

static TEST tests[] = {
  {"null", 0, test_null},
  {"plot", 0, test_plot},
  {"line", 0, test_line},
  {"vline", 0, test_vline},
  {"hline", 0, test_hline},
  {"ppoly", 0, test_ppoly},
  {"circle", 0, test_circle},
  {"pcircle", 0, test_pcircle},
  {"bitblk", 0, test_bitblk},
  {"scroll", 0, test_scroll},
  {"printc", 0, test_printc},
  {"prints", 0, test_prints},
  {NULL, 0, NULL}
};

static void usage (char *prgname)
{
  const TEST *test;

  fprintf(stderr, "usage: %s [<test>]\n", prgname);
  fprintf(stderr, "       where <test> may be any combination of:\n");

  for (test=tests; test->name; test++) {
    fprintf(stderr, "       --%s\n", test->name);
  }

  exit (-1);
}


static const struct option options[] = {
  {"null", 0, NULL, 0},
  {"plot", 0, NULL, 1},
  {"line", 0, NULL, 2},
  {"vline", 0, NULL, 3},
  {"hline", 0, NULL, 4},
  {"ppoly", 0, NULL, 5},
  {"circle", 0, NULL, 6},
  {"pcircle", 0, NULL, 7},
  {"bitblk", 0, NULL, 8},
  {"scroll", 0, NULL, 9},
  {"printc", 0, NULL, 10},
  {"prints", 0, NULL, 11},
  {"all", 0, NULL, 127},
  {NULL, 0, NULL, 0}
};


int main (int argc, char *argv[])
{
  int ret;
  TEST *test;

  /* parse arguments
   */
  while ((ret = getopt_long(argc, argv, "", options, NULL)) != -1) {
    switch(ret) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
        tests[ret].doit = 1;
	break;

      case 127:
	for (test=tests; test->name; test++)
	  test->doit = 1;
	break;

      case ':':
	printf("missing argument for option '%s'\n", argv[optind]);
	usage(argv[0]);

      default:
	usage(argv[0]);
    }
  }

  /* no additional arguments supported so far
   */
  if ((optind < argc) || (argc < 2)) {
    usage(argv[0]);
  }

  if (init ())
    return -1;

  for (test=tests; test->name; test++) {
    if (test->doit) {
      (*test->func)(REPS, 10);
    }
  }

  return 0;
}
