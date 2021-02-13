/*
 * wstatus.c, a part of the W Window System
 *
 * Copyright (C) 1994-1997 by Torsten Scherer
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- report some statistics about the W server
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <Wlib.h>


int main(void)
{
  short i;
  STATUS st;
  struct in_addr iadr;
  WSERVER *wserver;

  if (!(wserver = w_init())) {
    fprintf(stderr, "error: can't connect to wserver\n");
    return -1;
  }

  printf("This is W%iR%i", wserver->vmaj, wserver->vmin);
  if (wserver->pl) {
    printf("PL%i", wserver->pl);
  }

  printf(", running on %i*%i %s screen with %i planes:\n",
	 wserver->width, wserver->height,
	 wserver->type == BM_DIRECT8 ? "DIRECT8" : "PACKED",
	 wserver->planes);

  printf(" - default server font family and size are: %s, %d pixels\n",
         wserver->fname, wserver->fsize);

  printf(" - server palette has %d colors of which %d are shared\n",
	 1 << wserver->planes, wserver->sharedcolors);

  if (wserver->flags) {
    int flags = wserver->flags;
    printf ("\nServer supports:\n");
    if (flags & WSERVER_KEY_MAPPING) {
      printf (" + special (arrow etc) key mapping to W symbols\n");
    }
    if (wserver->flags & WSERVER_SHM) {
      printf (" + shared memory (for images)\n");
    }
  }

  if ((i = w_querystatus(&st, -1)) < 0) {
    fprintf(stderr, "error: w_querystatus failed\n");
    return -1;
  }

  printf("\nTotal server statistics:\n");
  printf(" - %i of all the %i windows are open\n", st.openWin, st.totalWin);
  printf(" - %li packets have been processed from the\n", st.pakets);
  printf(" - %li received bytes\n", st.bytes);

  printf("\nThere are currently %i clients running including myself.\n", i);

  printf("\nClient statistics:\n");

  i = 0;
  while (42) {

    if (w_querystatus(&st, i++) < 0) {
      return 0;
    }
    if (!st.ip_addr) {
      printf("%2i (local): %i/%i windows, %li packets / %li bytes received\n",
	     i, st.openWin, st.totalWin, st.pakets, st.bytes);
    } else {
      iadr.s_addr = st.ip_addr;
      printf("%2i (%s): %i/%i windows, %li pakets / %li bytes received\n",
	     i, inet_ntoa(iadr), st.openWin, st.totalWin, st.pakets, st.bytes);
    }
  }
}
