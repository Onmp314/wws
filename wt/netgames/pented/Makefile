# makefile for pente server
#
# -DDEBUG will print the board situation after every move
# (engine.c, play.c).

NAME	= pented

include ../../../.config
include ../../.wtconfig

DEBUG	= -g -DEBUG
FRAME	= ..

CFLAGS	= -I$(FRAME) $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= $(CLIENTLDFLAGS)
LDLIBS	= $(TEXTLIBS)


SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
GOBS = $(patsubst %.o, %.og, $(SRCS))

.SUFFIXES: .og

.c.og:
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@


$(NAME): $(FRAME)/server.o $(OBJS)
	$(CC) -o $@ $< $(OBJS) $(LDFLAGS) $(LDLIBS)

$(NAME).sym: $(FRAME)/server.o $(GOBJS)
	$(CC) -o $@ -g $< $(GOBJS) $(LDFLAGS) $(LDLIBS)
ifeq ($(ARCH),MiNT)
	toglclr -fshare $@
endif


$(SOURCE): board.h play.h


install: $(NAME)
	$(INSTALLBIN) $(NAME) $(GAMEDIR)


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/

