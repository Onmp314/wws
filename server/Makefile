#
# makefile for W server
#

include ../.config

CFLAGS	= $(BASECFLAGS) $(SERVERCFLAGS) -I..$(UTILCPP)
LDFLAGS	= -Lgraph $(SERVERLDFLAGS) -L..$(UTILLIB)
LDLIBS	= -lgraph $(SERVERLIBS)
TARGETS	= subdirs wserver

ifeq ($(WLIB),bW.olb)
# old style MiNT library names
DEPEND	= graph/graph.olb
else
DEPEND	= graph/libgraph.a
endif

#
# some definitions
#

WHDR   = config.h proto.h types.h
WLIB_H = ../lib/Wlib.h
SRCS   = $(wildcard *.c)

OBJS = $(patsubst %.c, %.o, $(SRCS))


#
# rules
#

all: subdirs wserver

subdirs: $(WHDR)
	$(MAKE) -C graph
ifdef UTILLIB
	$(MAKE) -C util
endif
ifeq ($(ARCH),MiNT)
ifdef MAC
	$(MAKE) -C macdevs
else
	$(MAKE) -C xconout2
endif
endif

wserver: $(OBJS) $(DEPEND)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

# symbol file for MiNT gdb
wserver.sym: $(OBJS) $(DEPEND)
	$(CC) -g $(LDFLAGS) $(OBJS) $(LDLIBS) $(SYM_LD) -o $@

#####

$(OBJS): $(WHDR)

types.h: $(WLIB_H)

$(WLIB_H):
	$(MAKE) -C $(WLIB_H)

font.o: font.c $(WHDR)
	$(CC) $(CPPFLAGS) $(CFLAGS)  '-DFONTDIR="$(FONTDIR_RUN)"' -c $< -o $@

main.o: main.c $(WHDR)
	$(CC) $(CPPFLAGS) $(CFLAGS)  '-DDATADIR="$(DATADIR_RUN)"' -c $< -o $@

loop.o: pakets.h $(WHDR)
window.o: window.h $(WHDR)
rect.o: rect.h $(WHDR)


#####

install: wserver
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	$(INSTALLBIN) wserver $(BINDIR)
ifeq ($(ARCH),MiNT)
	$(MAKE) -C xconout2 install
endif
endif

clean:
	$(RM) *.o $(JUNK)
	$(MAKE) -C graph clean
	$(MAKE) -C util clean
	$(MAKE) -C xconout2 clean

veryclean:
	$(RM) wserver *.o $(JUNK)
	$(MAKE) -C graph veryclean
	$(MAKE) -C util veryclean
	$(MAKE) -C xconout2 veryclean

