/*
 * ring.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a 'ring' client (actually a minimal socket example)
 *
 * sends a msg over network to given ring server which then
 * pops up a window with the message.
 *
 * Ref: "Using C on the UNIX system", O'Reilly & Assiciates, Inc.
 */

#include <stdio.h>
#include <unistd.h>
#ifdef LOCAL_SERVER
#include <sys/un.h>		/* AF_UNIX */
#else
#include <netinet/in.h>		/* AF_INET */
#endif
#include <sys/time.h>		/* fd_set */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>

#define RING_SOCK	"/tmp/ringd"
#define RING_PORT	21060		/* "RD" like in Ring Daemon */

int main(int argc, char *argv[])
{
  int sock;
  short lenght;
  const char *string;

#ifdef LOCAL_SERVER

  struct sockaddr_un addr;
  int len;

  if(argc != 2)
  {
    fprintf(stderr, "\nusage: %s <message>\n", *argv);
    fprintf(stderr, "sends message to the 'ring' daemon\n");
    return -1;
  }

  if((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_UNIX socket.\n");
    return -1;
  }

  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, RING_SOCK);
  len = sizeof(addr.sun_family) + strlen(addr.sun_path);
  if(connect(sock, (struct sockaddr *)&addr, len))
  {
    fprintf(stderr, "can't connect to local " RING_SOCK " socket.\n");
    close(sock);
    return -1;
  }
  string = argv[1];

#else

  struct hostent *host;
  struct sockaddr_in addr;

  if(argc < 2 || argc > 3 || argv[1][0] == '-')
  {
    fprintf(stderr, "\nusage: %s [address] <message>\n", *argv);
    fprintf(stderr, "sends message to the 'ring' daemon at given address\n");
    return -1;
  }

  if(argc == 3)
    string = argv[1];
  else
    string = "localhost";

  /* resolv host address */
  if(!(host = gethostbyname(string)))
  {
    fprintf(stderr, "can't resolve adress `%s'.\n", string);
    return -1;
  }

  /* create non-local stream socket */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_INET socket.\n");
    return -1;
  }

  /* connect specified address */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(RING_PORT);
  addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
  if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)))
  {
    fprintf(stderr, "can't connect to server `%s' (%s).\n",
      string, inet_ntoa(addr.sin_addr));
    close(sock);
    return -1;
  }

  if(argc == 3)
    string = argv[2];
  else
    string = argv[1];

#endif

  /* send message with it's lenght */
  lenght = htons(strlen(string));
  write(sock, &lenght, sizeof(lenght));
  write(sock, string, strlen(string));
  close(sock);

  return 0;
}
