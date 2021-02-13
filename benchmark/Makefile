#
# makefile for W benchmarks
#

include ../.config

CFLAGS	= -I../lib $(BASECFLAGS) $(CLIENTCFLAGS) $(MATHCFLAGS) -I..$(UTILCPP)
LDFLAGS	= -L../lib $(CLIENTLDFLAGS)
LDLIBS	= $(CLIENTLIBS)
LIBDEP	= ../lib/$(WLIB)


TARGETS = wperfmon wengine

#
# rules
#

all: $(TARGETS)

install: $(TARGETS)
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	for i in $(TARGETS) ; do \
		$(INSTALLBIN) $$i $(BINDIR) ; \
	done
endif

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(TARGETS)

#
# more rules
#

wengine: wengine.o $(LIBDEP)
	$(CC) $(LDFLAGS) wengine.o $(LDLIBS) $(MATHLIB) -o $@

wperfmon: wperfmon.o $(LIBDEP)
	$(CC) $(LDFLAGS) wperfmon.o $(LDLIBS) -L..$(UTILLIB) -o $@
