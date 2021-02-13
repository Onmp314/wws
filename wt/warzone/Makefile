# makefile for Warzone (w) 1997 by Eero Tamminen

NAME = warzone

include ../../.config
include ../.wtconfig

# define -DSOUND if you're on Atari to get XBios sound effects
CFLAGS	= -I../Wt -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS) #-DSOUND
LDFLAGS	= -L../Wt -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(WTLIBS) $(MATHLIB)

LIBDEP	= ../Wt/$(WTLIB) ../../lib/$(WLIB)

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))

all: $(NAME)

install: $(NAME)
ifdef GAMEDIR
	$(INSTALLDIRS) $(GAMEDIR)
	$(INSTALLBIN) $(NAME) $(GAMEDIR)
endif

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $(NAME)

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/
