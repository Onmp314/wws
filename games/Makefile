#
# makefile for W games
#

include ../.config


CFLAGS	= -I../lib $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../lib $(CLIENTLDFLAGS)
LDLIBS	= $(CLIENTLIBS)
LIBDEP	= ../lib/$(WLIB)


SUBDIRS = wdragon wjewel wyrms

TARGETS = lsys wapfel wlife wmines wpuzzle wreversi wrobots wsolitaire \
	wdragon/wdragon wjewel/wjewel wyrms/wwyrms

#
# rules
#

all: $(TARGETS)

install: $(TARGETS)
ifdef GAMEDIR
	$(INSTALLDIRS) $(GAMEDIR)
	for i in $(TARGETS) ; do \
		$(INSTALLBIN) $$i $(GAMEDIR) ; \
	done
endif

clean:
	$(RM) *.o $(JUNK)
	for i in $(SUBDIRS) ; do \
		$(MAKE) -C $$i clean ; \
	done

veryclean: clean
	$(RM) *.o $(JUNK)
	for i in $(SUBDIRS) ; do \
		$(MAKE) -C $$i veryclean ; \
	done

#
# more rules & dependencies
#

lsys: lsys.o $(LIBDEP)
	$(CC) lsys.o $(LDFLAGS) $(LDLIBS) -o $@  

wapfel: wapfel.o $(LIBDEP)
	$(CC) wapfel.o $(LDFLAGS) $(LDLIBS) -o $@  

wlife: wlife.o $(LIBDEP)
	$(CC) wlife.o $(LDFLAGS) $(LDLIBS) -o $@  

wmines: wmines.o $(LIBDEP)
	$(CC) wmines.o $(LDFLAGS) $(LDLIBS) -o $@  

wpuzzle: wpuzzle.o $(LIBDEP)
	$(CC) wpuzzle.o $(LDFLAGS) $(LDLIBS) -o $@  

wreversi: wreversi.o $(LIBDEP)
	$(CC) wreversi.o $(LDFLAGS) $(LDLIBS) -o $@  

wrobots: wrobots.o $(LIBDEP)
	$(CC) wrobots.o $(LDFLAGS) $(LDLIBS) -o $@  

wsolitaire: wsolitaire.o $(LIBDEP)
	$(CC) wsolitaire.o $(LDFLAGS) $(LDLIBS) -o $@  

# larger games

wdragon/wdragon: $(LIBDEP)
	$(MAKE) -C wdragon

wjewel/wjewel: $(LIBDEP)
	$(MAKE) -C wjewel

wyrms/wwyrms: $(LIBDEP)
	$(MAKE) -C wyrms wwyrms
