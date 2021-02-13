# makefile for go related game (gomoku, pente, renju, go) frontends.
#
# (w) 1996 by Eero Tamminen

include ../../../.config
include ../../.wtconfig

#DEBUG	= -g -DDEBUG
FRAME	= ..

CFLAGS	= -I../../Wt -I../../../lib -I$(FRAME) $(BASECFLAGS) $(CLIENTCFLAGS) $(DEBUG)
WLDFLAGS= -L../../Wt -L../../../lib $(CLIENTLDFLAGS)
WLIBS	= $(WTLIBS)
CLIBS	= $(TEXTLIBS)


COMMS	= $(FRAME)/comms.o

# W window system
WOBJS	= $(COMMS) $(FRAME)/w_game.o w_common.o

# curses
COBJS	= $(COMMS) $(FRAME)/c_game.o c_common.o


TARGETS = cgomoku wgomoku cpente wpente cgo wgo

all: $(TARGETS)


cgomoku: gomoku.o $(COBJS)
	$(CC) $(WLDFLAGS) $(COBJS) $< -o $@ $(CLIBS)

wgomoku: gomoku.o $(WOBJS)
	$(CC) $(WLDFLAGS) $(WOBJS) $< -o $@ $(WLIBS)


cpente: pente.o $(COBJS)
	$(CC) $(WLDFLAGS) $(COBJS) $< -o $@ $(CLIBS)

wpente: pente.o $(WOBJS)
	$(CC) $(WLDFLAGS) $(WOBJS) $< -o $@ $(WLIBS)


cgo: go.o $(COBJS)
	$(CC) $(WLDFLAGS) $(COBJS) $< -o $@ $(CLIBS)

wgo: go.o $(WOBJS)
	$(CC) $(WLDFLAGS) $(WOBJS) $< -o $@ $(WLIBS)


# for debugging the framework under MiNT
wpente.sym: pente.o $(WOBJS)
	$(CC) $(LDFLAGS) $(WOBJS) $< -o wpente.sym $(SYM_LD) $(WLIBS)
	toglclr -fshare wpente


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
	tar -zcvf ../goes.tgz -C ../ goes/


# dependencies
go.c pente.c gomoku.c: $(FRAME)/game.h common.h
c_common.c: $(FRAME)/c_game.h common.h
w_common.c: $(FRAME)/w_game.h common.h
