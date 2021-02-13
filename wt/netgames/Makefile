# Makefile for networked, turn based, two player (board) game interfaces.
#
# -DDEBUG will print information about every message received/sent.
# -DFORK will enable multiple server clients (if OS has fork() = MiNT
#  version >= 1.12.6 or real unix).
#
# (w) 1996 by Eero Tamminen

#DEBUG	= -g -DDEBUG
FORK	= -DFORK

include ../../.config
include ../.wtconfig

CFLAGS	= -I../Wt -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS) $(DEBUG)
LDFLAGS	= -L../Wt -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(WTLIBS)

LIBDEP	= ../Wt/$(WTLIB) ../../lib/$(WLIB)


TARGETS = comms.o server.o w_game.o c_game.o

all: $(TARGETS)
	@echo ; $(MAKE) -C amigod
	@echo ; $(MAKE) -C chess
	@echo ; $(MAKE) -C goes
	@echo ; $(MAKE) -C pented
	@echo ; $(MAKE) -C reversi


# client communication
comms.o: comms.c comms.h game.h

# clients for different GUIs
w_game.o: w_game.c w_game.h game.h comms.h	# W window system

c_game.o: c_game.c c_game.h game.h comms.h	# curses
	$(CC) $(CFLAGS) -c $< -o $@

# server version
server.o: server.c game.h server.h
	$(CC) $(FORK) $(CFLAGS) -c $< -o $@


install:
ifdef GAMEDIR
	$(INSTALLDIRS) $(GAMEDIR)
	@echo ; $(MAKE) -C amigod install
	@echo ; $(MAKE) -C chess install
	@echo ; $(MAKE) -C goes install
	@echo ; $(MAKE) -C pented install
	@echo ; $(MAKE) -C reversi install
endif


clean:
	$(RM) *.o $(JUNK)
	@echo ; $(MAKE) -C amigod clean
	@echo ; $(MAKE) -C chess clean
	@echo ; $(MAKE) -C goes clean
	@echo ; $(MAKE) -C pented clean
	@echo ; $(MAKE) -C reversi clean

veryclean:
	$(RM) *.o $(JUNK)
	@echo ; $(MAKE) -C amigod veryclean
	@echo ; $(MAKE) -C chess veryclean
	@echo ; $(MAKE) -C goes veryclean
	@echo ; $(MAKE) -C pented veryclean
	@echo ; $(MAKE) -C reversi veryclean


package: veryclean
	tar -zcvf ../netgames.tgz -C ../ netgames/

