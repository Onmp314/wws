# Makefile for W screensaver
#
# This probably will get numerous silly 'modules', so I moved it
# into a directory of it's own.

include ../../.config

CFLAGS	= -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS) $(MATHCFLAGS)
LDFLAGS	= -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(CLIENTLIBS)
LIBDEP	= ../../lib/$(WLIB)

OBJS	= wsaver.o ants.o bounce.o cracks.o lines.o pyro.o snails.o stars.o
NAME	= wsaver

all: $(NAME)

$(NAME): $(OBJS) $(LIBDEP)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)


$(OBJS): wsaver.h

install: $(TARGETS)
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	$(INSTALLBIN) $(NAME) $(BINDIR)
endif

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME)
