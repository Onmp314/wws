/*
 * Client/server, variable lenght packet messaging for multiple connections
 * with client broadcasting (not server broadcast coupled with routing).
 *
 * W dialog for making / braking the connection.
 *
 * Todo:
 * - timeouts.
 * - transferring packets (eg. files parts) at the background.
 *
 * (w) 1996 by Eero Tamminen
 */

#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>		/* fd_set stuff */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "chat.h"		/* includes standard + W stuff */

#define ERROR	"*error*"	/* used on own error messages */

typedef struct
{
  short lenght;			/* packet size */
  short type;			/* message id */
} MESSAGE;

typedef struct client_t
{
  struct client_t *prev;
  struct client_t *next;
  char *nick;		/* nick name for the connection owner */
  int socket;		/* socket for the opponent */
  long handle;		/* toolkit handle for the above */
  long to_read;		/* how much from of the msg data remains to be read */
  long allocated;	/* how long the message data buffer is */
  MESSAGE msg;		/* received message header */
  int msg2read;		/* bytes left of header to read */
  char *data;		/* message buffer */
} CLIENT;

static struct
{
  char *name;
  int timeout;		/* minutes to think a move */
  long local;		/* server toolkit handle */
  int socket;		/* server socket */
  int clients;		/* if server, how many clients */
  CLIENT *client;	/* linked list of connected clients */
} Server;

static short Port = 0x4354;			/* "CT" (ChaT) */
static widget_t *Address, *Nick;


/* function prototypes. returns 0 if ok */

static void nick_cb(widget_t *w, const char *nick);
static void address_cb(widget_t *w, const char *addr);
static void connect_cb(widget_t *w, int pressed);
static void disconnect_cb(widget_t *w, int pressed);
static void setup_server_cb(widget_t *w, int pressed);
static void broadcast_cb(widget_t *w, int pressed);
static void connect_server(const char *addr, const char *nick);
static void disconnect_client(CLIENT *client, const char *info, int notify);
static void server_cb(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex);
static void client_cb(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex);
static int  send_msg(CLIENT *client, short id, const char *data, long data_len);
static const char *process_msg(CLIENT *client);
static int  add_client(int sock);


static void close_win(widget_t *w)
{
  wt_close(w);
}

void network_cb(widget_t *w, int pressed)
{
  static widget_t *network = 0;
  widget_t *vpane, *hpane1, *hpane2, *label1, *label2,
    *cast, *setup, *connect, *discon;
  long a, b;

  if(pressed)
    return;

  if(network)
  {
    wt_open(network);
    return;
  }

  network = wt_create(wt_shell_class, Top);
  vpane   = wt_create(wt_pane_class, network);
  setup   = wt_create(wt_button_class, vpane);
  hpane1  = wt_create(wt_pane_class, vpane);
  label1  = wt_create(wt_label_class, hpane1);
  Nick    = wt_create(wt_getstring_class, hpane1);
  cast    = wt_create(wt_button_class, hpane1);
  label2  = wt_create(wt_label_class, vpane);
  Address = wt_create(wt_getstring_class, vpane);
  hpane2  = wt_create(wt_pane_class, vpane);
  connect = wt_create(wt_button_class, hpane2);
  discon  = wt_create(wt_button_class, hpane2);

  if(!(Address && Nick))
  {
    fprintf(stderr, "unable to create network dialog\n");
    return;
  }

  wt_setopt(network, WT_LABEL, "Server", WT_ACTION_CB, close_win, WT_EOL);

  wt_setopt(setup,
    WT_LABEL, "Setup server",
    WT_ACTION_CB, setup_server_cb,
    WT_EOL);

  a = OrientHorz;
  wt_setopt(hpane1, WT_ORIENTATION, &a, WT_EOL);
  wt_setopt(hpane2, WT_ORIENTATION, &a, WT_EOL);

  wt_setopt(label1, WT_LABEL, "Name: ", WT_EOL);

  a = 12;
  wt_setopt(Nick,
    WT_STRING_LENGTH, &a,
    WT_ACTION_CB, nick_cb,
    WT_EOL);

  wt_setopt(cast,
    WT_LABEL, "Broadcast",
    WT_ACTION_CB, broadcast_cb,
    WT_EOL);

  wt_setopt(label2, WT_LABEL, "Host network address:", WT_EOL);

  a = 32;
  b = 64;
  wt_setopt(Address,
    WT_STRING_WIDTH, &a,
    WT_STRING_LENGTH, &b,
    WT_ACTION_CB, address_cb,
    WT_EOL);

  wt_setopt(connect,
    WT_LABEL, "Connect",
    WT_ACTION_CB, connect_cb,
    WT_EOL);

  wt_setopt(discon,
    WT_LABEL, "Disconnect",
    WT_ACTION_CB, disconnect_cb,
    WT_EOL);

  wt_realize(Top);
}

static void broadcast_cb(widget_t *w, int pressed)
{
  char *nick;

  if(!pressed)
  {
    wt_getopt(Nick, WT_STRING_ADDRESS, &nick, WT_EOL);
    if(!nick || !nick[0])
    {
      show_string(ERROR, "No name.");
      return;
    }
    broadcast_msg(MSG_NICK, nick, strlen(nick)+1);
  }
}

static void connect_cb(widget_t *w, int pressed)
{
  char *addr, *nick;

  if(!pressed)
  {
    wt_getopt(Address, WT_STRING_ADDRESS, &addr, WT_EOL);
    wt_getopt(Nick, WT_STRING_ADDRESS, &nick, WT_EOL);
    connect_server(addr, nick);
  }
}

static void address_cb(widget_t *w, const char *addr)
{
  char *nick;

  wt_getopt(Nick, WT_STRING_ADDRESS, &nick, WT_EOL);
  connect_server(addr, nick);
}

static void nick_cb(widget_t *w, const char *nick)
{
  setup_server_cb(NULL, 0);
}

static void connect_server(const char *address, const char *nick)
{
  int sock;
  struct hostent *host;
  struct sockaddr_in addr;

  if(!address || !address[0])
  {
    show_string(ERROR, "No host address.");
    return;
  }

  if(!nick || !nick[0])
  {
    show_string(ERROR, "No name.");
    return;
  }

  /* resolv host address */
  if(!(host = gethostbyname(address)))
  {
    show_string(ERROR, "Unresolvable address.");
    return;
  }

  /* create non-local stream socket */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_INET socket.\n");
    return;
  }

  /* connect specified address */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(Port);
  addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
  if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)))
  {
    show_string(ERROR, "Connection failed.");
    close(sock);
    return;
  }

  if(add_client(sock))
  {
    close(sock);
    return;
  }

  send_msg(Server.client, MSG_NICK, nick, strlen(nick)+1);
}

static int add_client(int sock)
{
  CLIENT *old = Server.client;
  fd_set rfd;

  if(Server.clients >= FD_SETSIZE-1)
  {
    show_string(ERROR, "Out of sockets.");
    return 1;
  }

  if(!(Server.client = malloc(sizeof(CLIENT))))
  {
    fprintf(stderr, "Unable to create client struct.\n");
    Server.client = old;
    return 1;
  }
  memset(Server.client, 0, sizeof(CLIENT));

  if(old)
  {
    Server.client->prev = old;
    Server.client->next = old->next;
    old->next->prev = Server.client;
    old->next = Server.client;
  }
  else
    Server.client->prev = Server.client->next = Server.client;

  FD_ZERO(&rfd);
  FD_SET(sock, &rfd);
  Server.client->handle = wt_addinput(&rfd, NULL, NULL, client_cb, (long)Server.client);
  Server.client->socket = sock;

  Server.clients++;
  return 0;
}

static void disconnect_cb(widget_t *w, int pressed)
{
  if(!pressed)
  {
    while(Server.clients)
      disconnect_client(Server.client, "* disconnected *", 1);
    show_string("", "Not connected.");
  }
}

/* info tells the reason for the disconnection and notify flag
 * indicates that the remote client, not the local user, should
 * be notified about it.
 */
static void disconnect_client(CLIENT *client, const char *info, int notify)
{
  if(client)
  {
    if(notify)
      send_msg(client, MSG_STRING, info, strlen(info)+1);
    else
      show_string(ERROR, info);

    if(client->data)
      free(client->data);
    if(client->nick)
      free(client->nick);

    /* relink list */
    client->prev->next = client->next;
    client->next->prev = client->prev;

    wt_delinput(client->handle);
    close(client->socket);

    if(Server.client == client)
    {
      if(client == client->next)
        Server.client = 0;
      else
        Server.client = client->next;
    }
    free(client);
    Server.clients--;
  }
}

static void setup_server_cb(widget_t *w, int pressed)
{
  int sock;
  struct sockaddr_in addr;
  fd_set rfd;
  char *nick;

  if(pressed)
    return;

  if(Server.socket)
  {
    show_string(ERROR, "Server already setup.");
    return;
  }

  if(Server.clients >= FD_SETSIZE-1)
  {
    /* need some descriptor for connection too... */
    show_string(ERROR, "Out of sockets.");
    return;
  }

  wt_getopt(Nick, WT_STRING_ADDRESS, &nick, WT_EOL);
  if(!nick || !nick[0])
  {
    show_string(ERROR, "No name.");
    return;
  }
  if(Server.name)
    free(Server.name);
  if(!(Server.name = malloc(strlen(nick)+1)))
  {
    fprintf(stderr, "Out of memory.\n");
    return;
  }
  strcpy(Server.name, nick);

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "can't create AF_INET socket.\n");
    return;
  }

  /* where we are listening */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(Port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    /* socket already in use, client stuff should still work though */
    show_string(ERROR, "Server port unavailable.");
    close(sock);
    return;
  }

  /* no queueing */
  if(listen(sock, 4) < 0)
  {
    fprintf(stderr, "unable to listen on server socket.\n");
    close(sock);
    return;
  }

  FD_ZERO(&rfd);
  FD_SET(sock, &rfd);
  Server.local = wt_addinput(&rfd, NULL, NULL, server_cb, 0);
  Server.socket = sock;

  show_string("", "Waiting connections...");
  return;
}

static void server_cb(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex)
{
  int client;
  socklen_t addrlen;
  struct sockaddr_in addr;

  if(Server.clients >= FD_SETSIZE-1)
    return;

  /* create a connection to client */
  if((client = accept(Server.socket, (struct sockaddr *)&addr, &addrlen)) < 0)
  {
    fprintf(stderr, "client connection accept failed.\n");
    return;
  }

  if(add_client(client))
  {
    close(client);
    return;
  }

  send_msg(Server.client, MSG_NICK, Server.name, strlen(Server.name)+1);
}

/* read packets from the socket */
static void client_cb(long arg, fd_set *rfd, fd_set *wfd, fd_set *ex)
{
  CLIENT *client = (CLIENT *)arg;
  MESSAGE *msg = &client->msg;
  const char *err;
  int len;

  if(!client->to_read)
  {
    /* new message */
    if(!client->msg2read)
      client->msg2read = sizeof(MESSAGE);

    /* try to read (rest of) the header */
    len = read(client->socket, &msg[sizeof(MESSAGE)-client->msg2read], client->msg2read);
    if(len <= 0)
    {
      disconnect_client(client, "Message receive failed!", 0);
      return;
    }
    if((client->msg2read -= len))
      return;

    /* parse the header */
    msg->lenght = ntohs(msg->lenght);		/* data size */
    if(msg->lenght < 0 || msg->lenght > MAX_DATA_LEN)
    {
      disconnect_client(client, "Illegal message lenght!", 0);
      return;
    }
    msg->type = ntohs(msg->type);		/* message type */
    if(!client->nick && msg->type != MSG_NICK)
    {
      disconnect_client(client, "Anonymous client -> kick out.", 0);
      return;
    }

    client->to_read = msg->lenght;
    if(client->to_read > client->allocated)
    {
      if(client->data)
        free(client->data);
      if(!(client->data = malloc(client->to_read)))
      {
        fprintf(stderr, "Out of memory.\n");
        disconnect_client(client, "* out of memory *", 1);
	return;
      }
      client->allocated = client->to_read;
    }
  }

  if(client->to_read)
  {
    /* read rest of the message */
    len = read(client->socket, client->data, client->to_read);
    if(len <= 0)
    {
      disconnect_client(client, "Message receive failed!", 0);
      return;
    }
    /* still stuff to read? */
    if((client->to_read -= len) > 0)
      return;
  }

  /* parser the message */
  if((err = process_msg(client)))
    disconnect_client(client, err, 1);
}

static const char *process_msg(CLIENT *client)
{
  CLIENT *check;

  switch(client->msg.type)
  {
    case MSG_NICK:
      if(*client->data == '*' || !*client->data)
        return "* illegal name *";

      check = client->next;
      while(check != client)
      {
	if(check->nick && !strcmp(check->nick, client->data))
	{
	  /* new already used, use previous nick? */
	  if(client->nick)
	    return 0;
	  else
	    return "* client name already in use *";
	}
	check = check->next;
      }
      if(client->nick)
        free(client->nick);
      if((client->nick = malloc(client->msg.lenght)))
      {
        strcpy(client->nick, client->data);
	show_string(client->nick, "Connected.");
      }
      else
      {
        fprintf(stderr, "Out of memory.\n");
        return "* out of memory *";
      }
      break;

    case MSG_STRING:
      show_string(client->nick, client->data);
      break;

    default:
      return "* incompatible client *";
  }
  return NULL;
}

void broadcast_msg(short type, const char *data, long data_len)
{
  CLIENT *client;

  if(!Server.client)
  {
    show_string(ERROR, "Not connected.");
    return;
  }

  client = Server.client;
  do
  {
    if(send_msg(client, type, data, data_len))
      return;
    client = client->next;
  } while(client != Server.client);
}

/* For larger than MAX_DATA_LEN stuff (eg. files) there needs to be
 * some kind of spooler function which would work on the background
 * (using timeouts?) and would queue the data on client bases. If
 * files are are used, they could be spooled straight from/to disk.
 */
static int send_msg(CLIENT *client, short id, const char *data, long data_len)
{
  MESSAGE msg;

  if(data_len > MAX_DATA_LEN)
  {
    show_string(ERROR, "Too large data packet.");
    return 1;
  }

  msg.lenght = htons(data_len);
  msg.type = htons(id);

  if(write(client->socket, &msg, sizeof(msg)) != sizeof(msg))
  {
    disconnect_client(client, "Message send failed!", 0);
    return 1;
  }
  if(data_len)
  {
    if(write(client->socket, data, data_len) != data_len)
    {
      disconnect_client(client, "Data send failed!", 0);
      return 1;
    }
  }
  return 0;
}
