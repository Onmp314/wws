# makefile for chess frontend
#
# (w) 1996 by Eero Tamminen

NAME = wchess

include ../../../.config
include ../../.wtconfig

DEBUG = -g -DDEBUG
FRAME = ..

CFLAGS	= -I../../Wt -I../../../lib -I$(FRAME) $(BASECFLAGS) $(CLIENTCFLAGS)
WLDFLAGS= -L../../Wt -L../../../lib $(CLIENTLDFLAGS)
WLIBS	= $(WTLIBS)


COMMS	= $(FRAME)/comms.o
WCHESS	= ../w_game.o $(COMMS) w_chess.o pieces.o
WGOBJS	= ../w_game.og $(COMMS) w_chess.og pieces.og

.SUFFIXES: .og

.c.og:
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@


all: $(NAME)

wchess: chess.o $(WCHESS)
	$(CC) $(WLDFLAGS) $(WCHESS) $< -o $@ $(WLIBS)

wchess.sym: chess.og $(WGOBJS)
#	$(CC) $(WLDFLAGS) $(WGOBJS) $< -o $@ $(WLIBS)
ifeq ($(ARCH),MiNT)
	toglclr -fshare $@
endif

chess.c: $(FRAME)/game.h chess.h
w_chess.c: $(FRAME)/w_game.h chess.h


install: $(NAME)
	$(INSTALLBIN) $(NAME) $(GAMEDIR)


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/
