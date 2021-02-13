# makefile for Reversi
#
# (w) 1996 by Eero Tamminen

include ../../../.config
include ../../.wtconfig

DEBUG	= -g -DDEBUG
FRAME	= ..

CFLAGS	= -I../../Wt -I../../../lib -I$(FRAME) $(BASECFLAGS) $(CLIENTCFLAGS)
WLDFLAGS= -L../../Wt -L../../../lib $(CLIENTLDFLAGS)
#WLIBS	= -lWt -lW2X -L/usr/X11R6/lib -lX11 #$(WTLIBS)
WLIBS	= $(WTLIBS)
CLIBS	= $(TEXTLIBS)


# W window system
WOBJS	= $(FRAME)/comms.o $(FRAME)/w_game.o w_gui.o reversi.o

# curses
COBJS	= $(FRAME)/comms.o $(FRAME)/c_game.o c_gui.o reversi.o

# server
SOBJS	= $(FRAME)/server.o reversid.o
GSOBJS	= $(FRAME)/server.o reversid.og

.SUFFIXES: .og

.c.og:
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@


TARGETS = wreversi creversi reversid

all: $(TARGETS)


creversi: $(COBJS)
	$(CC) $(COBJS) -o $@ $(WLDFLAGS) $(CLIBS)

wreversi: $(WOBJS)
	$(CC) $(WOBJS) -o $@ $(WLDFLAGS) $(WLIBS)

reversid: $(SOBJS)
	$(CC) $(SOBJS) -o $@ $(WLDFLAGS) $(SLIBS)

# debug binary
reversid.sym: $(GSOBJS)
	$(CC) -g $(LDFLAGS) $(GSOBJS) $(SYM_LD) -o $@ $(SLIBS)
ifeq ($(ARCH),MiNT)
	toglclr -fshare wreversi
endif


install: $(TARGETS)
ifdef GAMEDIR
	for i in $(TARGETS) ; do \
		$(INSTALLBIN) $$i $(GAMEDIR) ; \
	done
endif


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(TARGETS) *.sym

package: veryclean
	tar -zcvf ../reversi.tgz -C ../ reversi/


# dependencies
reversid.c: $(FRAME)/server.h common.h
reversi.c: $(FRAME)/game.h common.h
c_gui.c: $(FRAME)/c_game.h common.h
w_gui.c: $(FRAME)/w_game.h common.h
