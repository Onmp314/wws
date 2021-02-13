# makefile for want and whop (w) 1997 by Eero Tamminen

TARGETS	= want whop

include ../../.config
include ../.wtconfig

CFLAGS	= -I../Wt -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../Wt -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(WTLIBS)

LIBDEP	= ../Wt/$(WTLIB) ../../lib/$(WLIB)

all: $(TARGETS)

install: $(TARGETS)
ifdef GAMEDIR
	$(INSTALLDIRS) $(GAMEDIR)
	$(INSTALLBIN) $(TARGETS) $(GAMEDIR)
endif

want: ant.o
	$(CC) $< $(LDFLAGS) $(LDLIBS) -o $@

whop: hopalong.o
	$(CC) $< $(LDFLAGS) $(LDLIBS) $(MATHLIB) -o $@

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(TARGETS)

package: veryclean
	tar -zcvf ..../.wtmath.tgz -C ../ math/
