#
# Makefile for the W server graphics library
#

TYPES = ../types.h backend.h gproto.h
CONFIG_FILE = ../../.config

include $(CONFIG_FILE)


CFLAGS = $(BASECFLAGS) $(SERVERCFLAGS)


ifeq ($(WLIB),bW.olb)
# for old MiNT linkers
LIBRARY = graph.olb
else
LIBRARY = libgraph.a
endif


all: subdirs library


OBJS = init.o $(wildcard generic/*.o) $(wildcard packed/*.o) \
	$(wildcard direct8/*.o) $(wildcard monochrome/*.o)


# architecture dependent initialization / exit / # event handling object
ifdef SDL
OBJS += sdl.o
else
ifdef GGI
OBJS += ggi.o
else
ifdef SVGALIB
OBJS += svgalib.o
else
ifeq ($(ARCH),Linux)
OBJS += linux.o
endif
ifeq ($(ARCH),MiNT)
ifdef MAC
OBJS += macmint.o macmint_cookie.o
else
OBJS += mint.o
endif
endif
ifeq ($(ARCH),SunOS)
OBJS += sun.o
endif
ifeq ($(ARCH),NetBSD)
OBJS += netbsd_amiga.o
endif
# not SDL, GGI or SVGALIB input system
OBJS += unix_input.o
endif
endif
endif


# 'flags' for requested graphics drivers
INITFLAGS =

ifdef PACKEDMONO
INITFLAGS += -DPMONO
endif

ifdef PACKEDCOLORMONO
INITFLAGS += -DPCOLORMONO
endif

ifdef PACKEDCOLOR
INITFLAGS += -DPCOLOR
endif

ifdef DIRECT8
INITFLAGS += -DDIRECT8
endif

ifdef BMONO
INITFLAGS += -DBMONO
endif


# these (may) need to be regenerated when options change

init.o: init.c $(TYPES)
unix_input.o: unix_input.c $(TYPES)

sdl.o: sdl.c  $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -c $< -o $@

ggi.o: ggi.c  $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -c $< -o $@

mint.o: mint.c $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -c $< -o $@

macmint.o: macmint.c $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -I$(MACINC) -c $< -o $@

macmint_cookie.o: macmint_cookie.c $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -I../xconout2/mint -c $< -o $@

linux.o: linux.c $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -c $< -o $@

svgalib.o: svgalib.c  $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -c $< -o $@

sun.o: sun.c  $(TYPES) $(CONFIG_FILE)

netbsd_amiga.o: netbsd_amiga.c $(TYPES) $(CONFIG_FILE)
	$(CC) $(CFLAGS) $(INITFLAGS) -c $< -o $@


# compile graphics drivers
subdirs:
	$(MAKE) -C generic
ifdef DIRECT8
	$(MAKE) -C direct8
endif
ifdef BMONO
	$(MAKE) -C monochrome
endif
ifdef PACKEDMONO
	$(MAKE) -C packed MONO=1
endif
ifdef PACKEDCOLORMONO
	$(MAKE) -C packed COLORMONO=1
endif
ifdef PACKEDCOLOR
	$(MAKE) -C packed COLOR=1
endif


# needed to get the object files with wildcards
library:
	$(MAKE) $(LIBRARY)

$(LIBRARY): $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
ifdef RANLIB
	$(RANLIB) $@
endif


clean:
	$(RM) *.o **/*.o $(JUNK)

veryclean: clean
	$(RM) $(LIBRARY)
