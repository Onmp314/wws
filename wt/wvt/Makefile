# makefile for Wvt (w) 1997 by Kay Roemer

NAME = wvt

include ../../.config
include ../.wtconfig

CFLAGS	= -I../Wt -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../Wt -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(WTLIBS) $(PORTLIB)

LIBDEP	= ../Wt/$(WTLIB) ../../lib/$(WLIB)

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))

all: $(NAME)

install: $(NAME)
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	$(INSTALLBIN) $(NAME) $(BINDIR)
endif

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $(NAME)

ifdef EXPORT_TERMCAP
wvt.o: wvt.c termtype.h util.h
	$(CC) wvt.c $(CFLAGS) -DEXPORT_TERMCAP -c -o $@
endif

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/
