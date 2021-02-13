# makefile for Wedit (w) 1997 by Eero Tamminen

NAME = wedit

include ../../.config
include ../.wtconfig

CFLAGS	= -I../Wt -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../Wt -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(WTLIBS)

# ignore these uchar* vs. char* issues for now
CFLAGS += -Wno-pointer-sign

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

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/
