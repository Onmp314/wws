#
# Makefile for (W library) demos
#

include ../.config


CFLAGS	= -I../lib $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../lib $(CLIENTLDFLAGS)
LDLIBS	= $(CLIENTLIBS)
LIBDEP	= ../lib/$(WLIB)

# all possible targets
TARGETS_ALL = 3point boxtest gfxdemo filltest linetest pietest warp \
	 wbuttons wcolor wdim wfont wfontdemo wmouse wresize wscroll

# by default do only these, could be "TARGETS_DEFAULT = $(TARGETS_ALL)"
TARGETS_DEFAULT = warp wfont wfontdemo

#
# rules
#

default: $(TARGETS_DEFAULT)

all: $(TARGETS_ALL)


3point: 3point.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) 3point.o $(LDLIBS) -o $@

boxtest: boxtest.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) boxtest.o $(LDLIBS) -o $@

gfxdemo: gfxdemo.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) gfxdemo.o $(LDLIBS) -o $@

filltest: filltest.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) filltest.o $(LDLIBS) -o $@

linetest: linetest.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) linetest.o $(LDLIBS) -o $@

pietest: pietest.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) pietest.o $(LDLIBS) -o $@

warp: warp.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) warp.o $(LDLIBS) -o $@

wbuttons: wbuttons.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wbuttons.o $(LDLIBS) -o $@

wcolor: wcolor.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wcolor.o $(LDLIBS) -o $@

wdim: wdim.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wdim.o $(LDLIBS) -o $@

wfont: wfont.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wfont.o $(LDLIBS) -o $@

wfontdemo: wfontdemo.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wfontdemo.o $(LDLIBS) -o $@

wmouse: wmouse.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wmouse.o $(LDLIBS) -o $@

wresize: wresize.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wresize.o $(LDLIBS) -o $@

wscroll: wscroll.o $(LIBDEP)
	$(CC) $(CFLAGS) $(LDFLAGS) wscroll.o $(LDLIBS) -o $@


install: $(TARGETS_DEFAULT)
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	for i in $(TARGETS_DEFAULT) ; do \
		$(INSTALLBIN) $$i $(BINDIR) ; \
	done
endif


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(TARGETS)

