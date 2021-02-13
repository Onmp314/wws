/*
 * A networked two player game socket interface functions for the clients.
 *
 * (w) 1996 by Eero Tamminen
 */

#include "game.h"

typedef struct
{
  int  timeout;			/* minutes to think a move */
  int  socket;			/* game server socket */
  int  client;			/* socket for the opponent */
  int  toread;			/* bytes of message yet to arrive */
  int sending;			/* sent message not yet responded to */
  int handshake;		/* handshake in process */
  long alignment;
  uchar msg[MSG_SIZE];		/* buffer for the message (long aligned!) */
} SERVER;

/* UI specific */
extern void disconnect_cb(void);		/* called at disconnecting */
extern void show_turn(int turn);		/* show_info() in game.h */
extern GAME Game;

/* from comms.c */
extern SERVER Server;
extern int setup_server(void);			/* returns server handle */
extern int connect_server(const char *addr);	/* returns client handle */
extern void disconnect_client(const char *info);
extern int server_cb(void);			/* returns client handle */
extern void client_cb(void);

extern void mysignal(int sig, void (*handler)(int));


