#ifndef __PROXY_H__
#define __PROXY_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct {
  char *name;
  struct sockaddr_in sin;
  short resolved : 1;
  short dynamic : 1;
} servaddr_t;

extern servaddr_t *servaddr_alloc (servaddr_t *, const char *name, short port);
extern void        servaddr_free (servaddr_t *);
extern int         servaddr_resolve (servaddr_t *);

#endif
