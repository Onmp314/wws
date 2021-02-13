/* global variables and function prototypes */

#ifndef __CHAT_H
#define __CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Wlib.h>
#include <Wt.h>

#define MAX_DATA_LEN	256	/* io buffer size */

/* frontend<->frontend messages */
#define MSG_NICK	0x20		/* client name */
#define MSG_STRING	0x21		/* show string */

extern widget_t *Top;

/* chat.c */
extern void show_string(const char *nick, const char *text);
/* comms.c */
extern void network_cb(widget_t *w, int pressed);
extern void broadcast_msg(short id, const char *data, long data_len);

#endif /* __CHAT_H */
