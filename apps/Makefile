#
# makefile for W applications
#
# CHANGES:
# - separate crypt library is required for some platforms

include ../.config


CFLAGS	= -I../lib $(BASECFLAGS) $(CLIENTCFLAGS) -I..$(UTILCPP) \
   '-DDATADIR="$(DATADIR_RUN)"'  '-DICONDIR="$(ICONDIR_RUN)"'
LDFLAGS	= -L../lib $(CLIENTLDFLAGS)
LDLIBS	= $(CLIENTLIBS) -L..$(UTILLIB)
LIBDEP	= ../lib/$(WLIB)


SUBDIRS = wsaver

TARGETS = fontedit ring ringd wbiff pbmbitflip ppm24to8 ppm8to24 wbm wclock \
	 wchars wcpyrgt weyes wload wlaunch wsetbg wstatus wterm wkbd \
	 wgone wsaver/wsaver

#
# rules
#

all: $(TARGETS)

install: $(TARGETS) iconinstall
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	for i in $(TARGETS) ; do \
		$(INSTALLBIN) $$i $(BINDIR) ; \
	done
endif

iconinstall: $(wildcard images/icons/*.p*)
ifdef ICONDIR
	$(INSTALLDIRS) $(ICONDIR)
	for i in $^; do $(INSTALL) -m 644 $$i $(ICONDIR); done
endif
	

clean:
	$(RM) *.o $(JUNK)
	for i in $(SUBDIRS) ; do \
		$(MAKE) -C $$i clean ; \
	done

veryclean: clean
	$(RM) $(TARGETS)

#
# more rules
#

fontedit: fontedit.o ../server/config.h
	$(CC) $< $(LDFLAGS) -o $@

ring: ring.o
	$(CC) $< $(LDFLAGS) $(LDLIBS) -o $@

ringd: ringd.o $(LIBDEP)
	$(CC) ringd.o $(LDFLAGS) $(LDLIBS) -o $@

wbiff: wbiff.o $(LIBDEP)
	$(CC) wbiff.o $(LDFLAGS) $(LDLIBS) -o $@

pbmbitflip: pbmbitflip.o $(LIBDEP)
	$(CC) pbmbitflip.o $(LDFLAGS) $(LDLIBS) -o $@

ppm24to8: ppm24to8.o $(LIBDEP)
	$(CC) ppm24to8.o $(LDFLAGS) $(LDLIBS) -o $@

ppm8to24: ppm8to24.o $(LIBDEP)
	$(CC) ppm8to24.o $(LDFLAGS) $(LDLIBS) -o $@

wbm: wbm.o $(LIBDEP)
	$(CC) wbm.o $(LDFLAGS) $(LDLIBS) -o $@

wchars: wchars.o $(LIBDEP)
	$(CC) wchars.o $(LDFLAGS) $(LDLIBS) -o $@

wclock: wclock.o $(LIBDEP)
	$(CC) wclock.o $(LDFLAGS) $(LDLIBS) -o $@

wcpyrgt: wcpyrgt.o $(LIBDEP)
	$(CC) wcpyrgt.o $(LDFLAGS) $(LDLIBS) -o $@

weyes: weyes.o $(LIBDEP)
	$(CC) weyes.o $(LDFLAGS) $(LDLIBS) -o $@

wgone: wgone.o $(LIBDEP)
	$(CC) wgone.o $(LDFLAGS) $(LDLIBS) $(CRYPTLIB) -o $@

wkbd: wkbd.o $(LIBDEP)
	$(CC) wkbd.o $(LDFLAGS) $(LDLIBS) -o $@

wload: wload.o $(LIBDEP)
	$(CC) wload.o $(LDFLAGS) $(LDLIBS) -o $@

wlaunch.o: basename.c

wlaunch: wlaunch.o $(LIBDEP)
	$(CC) wlaunch.o $(LDFLAGS) $(LDLIBS) -o $@

wsetbg: wsetbg.o $(LIBDEP)
	$(CC) wsetbg.o $(LDFLAGS) $(LDLIBS) -o $@

wstatus: wstatus.o $(LIBDEP)
	$(CC) wstatus.o $(LDFLAGS) $(LDLIBS) -o $@

wterm: wterm.o $(LIBDEP)
	$(CC) wterm.o $(LDFLAGS) $(LDLIBS) $(PORTLIB) -o $@

ifdef EXPORT_TERMCAP
wterm.o: wterm.c termtype.h
	$(CC) wterm.c $(CFLAGS) -DEXPORT_TERMCAP -c -o $@
endif

wsaver/wsaver: $(LIBDEP)
	$(MAKE) -C wsaver
