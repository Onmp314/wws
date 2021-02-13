/*
 * ringd.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a server / daemon for 'ring'
 *
 * Outputs messages either to stdout or local W window
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#ifdef LOCAL_SERVER
#include <sys/un.h>		/* AF_UNIX */
#else
#include <netinet/in.h>		/* AF_INET */
#endif
#include <sys/time.h>		/* fd_set */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <Wlib.h>

#define RING_SOCK	"/tmp/ringd"
#define RING_PORT	21060		/* "RD" like in Ring Daemon */
#define MAX_MSG		256		/* maximum message lenght */

#define WIN_NAME	" RING! "
#define PROPERTIES	(W_MOVE | EV_MOUSE | EV_KEYS)
#define MAX_WINDOWS	16		/* max messages at the time */
#define MSG_WIDTH	64		/* max. window width in chars */

static int Windows;

static void show_msg(char *msg, int lenght)
{
  static long idx = 0;
  int w, h, i, lines, max;
  static WSERVER *server;
  static WFONT *font;
  WWIN *win;

  /* initialize W stuff if needed / possible */

  idx++;
  if(!server)
  {
    if((server = w_init()))
      font = w_loadfont(NULL, 0, 0);
  }

  if(!(server && font))
  {
    if(server)
      fprintf(stderr, "ringd: unable to load W font.\n");
    msg[lenght] = '\0';
    fprintf(stderr, "message #%ld:\n", idx);
    fprintf(stderr, "%s\n", msg);
    return;
  }

  /* split message to given window width */

  lines = 1;
  if(lenght < MSG_WIDTH)
    max = lenght;
  else
  {
    int start;
    /* word wrap */
    max = start = 0;
    for(;;)
    {
      i = start + MSG_WIDTH;
      if(i >= lenght)
        break;

      while(msg[i] > ' ' && --i > start);
      if(i == start)
        i = start + MSG_WIDTH;
      msg[i] = '\0';

      if(i > max)
        max = i;
      start += MSG_WIDTH+1;
      lines++;
    }
  }
  msg[lenght] = '\0';
  msg[lenght+1] = '\0';
  w = max * font->widths['W'];
  h = lines * font->height;

  /* open window and output message */

  if(!(win = w_create(w, h, PROPERTIES | W_TOP)))
  {
    if(!(win = w_create(w, h, PROPERTIES | W_TITLE | W_CLOSE)))
    {
      fprintf(stdout, "can't create output window");
      return;
    }
    if(max > 12)
      w_settitle(win, WIN_NAME);
  }
  w_setfont(win, font);

  lines = 0;
  while(*msg)
  {
    w_printstring(win, 0, lines, msg);
    msg += strlen(msg) + 1;
    lines += font->height;
  }

  /* center window on screen */
  w = (server->width  - w) / 2;
  h = (server->height - h) / 2;
  if(w_open(win, w, h) < 0)
  {
    fprintf(stdout, "can't open output window");
    return;
  }
  Windows++;
  return;
}

static void process_msg(int sock)
{
  char *buf;
  short lenght;
  struct sockaddr_in caddr;
  static char message[MAX_MSG+2];
  int client, len, left;
  socklen_t addrlen;

  /* wait for either a W event or a client request */

  if(Windows)
  {
    fd_set rfd;
    WEVENT *ev;

    FD_ZERO(&rfd);
    FD_SET(sock, &rfd);
    if((ev = w_queryevent(&rfd, NULL, NULL, -1L)))
    {
      /* close last message window at first event */
      w_delete(ev->win);
      Windows--;

      if(ev->type == EVENT_GADGET && ev->key == GADGET_EXIT)
      {
	w_exit();
	Windows = 0;
      }
      return;
    }
    /* not event, got to be socket... */
  }

  /* wait and create a connection to client */
  if((client = accept(sock, (struct sockaddr *)&caddr, &addrlen)) < 0)
  {
    fprintf(stderr, "client connection accept failed.\n");
    return;
  }

  /* process client message */

  len = read(client, &lenght, sizeof(lenght));
  lenght = ntohs(lenght);
  if(len != sizeof(lenght) || lenght < 1)
  {
    fprintf(stderr, "client communication error.\n");
    close(client);
    return;
  }
  if(lenght > MAX_MSG)
    lenght = MAX_MSG;
  left = lenght;
  buf = message;
  while(left > 0)
  {
    len = read(client, buf, left);
    if(len < 1)
    {
      if(len == left)
        break;
      fprintf(stderr, "client communication broken.\n");
      close(client);
      return;
    }
    left -= len;
    buf += len;
  }
  close(client);
  show_msg(message, lenght);
}

int main(int argc, char *argv[])
{
  int sock;

#ifdef LOCAL_SERVER

  struct sockaddr_un saddr;
  int len;

  if((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_UNIX socket.\n");
    return -1;
  }
  /* destroy possible old socket */
  unlink(RING_SOCK);

  /* where we are listening */
  saddr.sun_family = AF_UNIX;
  strcpy(saddr.sun_path, RING_SOCK);
  len = sizeof(saddr.sun_family) + strlen(saddr.sun_path);
  if(bind(sock, (struct sockaddr *)&saddr, len) < 0)
  {
    fprintf(stderr, "can't bind `%s' socket.\n", RING_SOCK);
    close(sock);
    return -1;
  }

#else

  struct sockaddr_in saddr;

#if 0

  char hostname[64];
  struct hostent *host;

  gethostname(hostname, sizeof(hostname));
  if(!(host = gethostbyname(hostname)))
  {
    fprintf(stderr, "host unknown.\n");
    return -1;
  }
  saddr.sin_addr = *(struct in_addr *)host->h_addr_list[0];

#endif

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_INET socket.\n");
    return -1;
  }

  /* where we are listening */
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(RING_PORT);
  saddr.sin_addr.s_addr = INADDR_ANY;
  if(bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
  {
    fprintf(stderr, "can't bind socket (#%d).\n", RING_PORT);
    close(sock);
    return -1;
  }

#endif

  /* queue at max. 4 clients */
  if(listen(sock, 4) < 0)
  {
    fprintf(stderr, "unable to listen on socket.\n");
    close(sock);
    exit(-1);
  }

  for(;;)
    process_msg(sock);
  return 0;
}
