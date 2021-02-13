/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * proxy stuff
 *
 * $Id: proxy.c,v 1.2 2008-08-29 19:20:48 eero Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "proxy.h"
#include "util.h"


servaddr_t *
servaddr_alloc (servaddr_t *sv, const char *name, short port)
{
  int dynamic = !sv;

  if (!sv)
    sv = malloc (sizeof (servaddr_t));
  if (!sv)
    return NULL;

  memset (sv, 0, sizeof (sv));
  sv->name = strdup (name);
  if (!sv->name) {
    if (dynamic)
      free (sv);
    return NULL;
  }
  sv->sin.sin_family = AF_INET;
  sv->sin.sin_port = htons (port);
  sv->sin.sin_addr.s_addr = INADDR_ANY;
  sv->resolved = 0;
  sv->dynamic = dynamic;
  return sv;
}

void
servaddr_free (servaddr_t *sv)
{
  free (sv->name);
  if (sv->dynamic)
    free (sv);
}

int
servaddr_resolve (servaddr_t *sv)
{
  struct hostent *hent;

  if (sv->resolved)
    return 0;

  hent = gethostbyname (sv->name);
  if (!hent)
    return -1;
  memcpy (&sv->sin.sin_addr.s_addr, hent->h_addr, hent->h_length);
  sv->resolved = 1;
  return 0;
}
