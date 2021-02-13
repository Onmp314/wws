# Makefile for W Wyrms
#
# If you want 16x16 bitmaps instead of 8x8 ones, use -DBIG_BITMAPS...
# Note that then the help 'level' won't fit into your screen
# unless you have a huge screen and do some changes to guiwyrms.h.
#
# You could use W2Xlib for X version:
#	WLIBS = $(LDFLAGS) -L../../w2xlib -L/usr/X11R6/lib -lW2X -lX11

include ../../.config

CPPFLAGS= -I. -DBIG_BITMAPS
CFLAGS	= -I../../lib $(BASECFLAGS) $(CLIENTLDFLAGS)
LDFLAGS	= -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(CLIENTLIBS)
LIBDEP	= ../../lib/$(WLIB)

CLIBS  = -lncurses #-lcurses
WLIBS  = $(LDFLAGS) $(LDLIBS)


# needed object files for CURSES, W, TOS versions
COBJS   = cwyrms.o chr_sys.o clevel.o
WOBJS   = wwyrms.o w_sys.o wlevel.o
TOBJS   = twyrms.o tos_sys.o tlevel.o joyisr.o

# version flags
CDEFS   = -DCURSES_WYRM
WDEFS   = -DW1_WYRM
TDEFS   = -DTOS_WYRM

TARGETS =  wwyrms cwyrms toswyrms

all:
	echo "Select one of wwyrms, cwyrms or toswyrms"


wwyrms: $(WOBJS) $(LIBDEP)
	$(CC) -o $@ $(WOBJS) $(WLIBS) $(SHARED)
wwyrms.o: wyrms.c guiwyrms.h wyrms.h
	$(CC) $(WDEFS) $(CPPFLAGS) $(CFLAGS) $(SHARED) -o $@ -c $<
w_sys.o: w_sys.c guiwyrms.h 16x16.h 8x8.h wyrms.h
	$(CC) $(WDEFS) $(CPPFLAGS) $(CFLAGS) $(SHARED) -o $@ -c $<
wlevel.o: level.c guiwyrms.h wyrms.h
	$(CC) $(WDEFS) $(CPPFLAGS) $(CFLAGS) $(SHARED) -o $@ -c $<


cwyrms: $(COBJS)
	$(CC) -o $@ $(COBJS) $(CLIBS)
cwyrms.o: wyrms.c chrwyrms.h wyrms.h
	$(CC) $(CDEFS) $(CFLAGS) -o $@ -c $<
chr_sys.o: chr_sys.c chrwyrms.h wyrms.h
	$(CC) $(CDEFS) $(CFLAGS) -o $@ -c $<
clevel.o: level.c chrwyrms.h wyrms.h
	$(CC) $(CDEFS) $(CFLAGS) -o $@ -c $<


toswyrms: wyrms.ttp
wyrms.ttp: $(TOBJS)
	$(CC) -o $@ $(TOBJS)
twyrms.o: wyrms.c toswyrms.h wyrms.h
	$(CC) $(TDEFS) $(CFLAGS) -o $@ -c $<
tos_sys.o: tos_sys.c toswyrms.h 16x16.h 8x8.h wyrms.h
	$(CC) $(TDEFS) $(CFLAGS) -o $@ -c $<
tlevel.o: level.c toswyrms.h wyrms.h
	$(CC) $(TDEFS) $(CFLAGS) -o $@ -c $<
# As the joystick source is absolute, you can't compile this basereltive!
joyisr.o: joyisr.s
	$(CC) -mpcrel -o $@ -c $<


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(TARGETS)

package: veryclean
	tar -zcvf ../wyrms.tgz -C../ wyrms
