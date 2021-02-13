#
# Makefile for AmiGo Go server
#

NAME = amigod

include ../../../.config
include ../../.wtconfig

DEBUG	= -g -DEBUG
FRAME	= ..

CFLAGS	= -I$(FRAME) $(BASECFLAGS) $(CLIENTCFLAGS) -Wimplicit -Wreturn-type
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
	toglclr -fshare $@.sym
endif


install: $(NAME)
	$(INSTALLBIN) $(NAME) $(GAMEDIR)


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/
