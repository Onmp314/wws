#
# Makefile for planar (packed) bitmap W graphics drivers
#
# COLOR and COLORMONO packed graphics drivers work only with interleaved
# bitmaps like Atari <16 bit color modes.  For example Amiga uses
# consecutive bitmaps.
#
# MONO driver works with everything.
#

include ../../../.config

CFLAGS = $(BASECFLAGS) $(SERVERCFLAGS)

ifdef MONO
MODE = MONO
endif
ifdef COLORMONO
MODE = COLORMONO
endif
ifdef COLOR
MODE = COLOR
endif

OBJS = bfmask16.o bfmask32.o bitblit_$(MODE).o line_$(MODE).o dline_$(MODE).o \
	mouse_$(MODE).o point_$(MODE).o scroll_$(MODE).o text_$(MODE).o \
	bitmap_$(MODE).o

all: $(OBJS)

clean:
	$(RM) *~ *.o

veryclean: clean

#
# rules for the .o files
#

bitblit_$(MODE).o: bitblit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

block_$(MODE).o: block.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

line_$(MODE).o: line.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

dline_$(MODE).o: dline.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

mouse_$(MODE).o: mouse.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

point_$(MODE).o: point.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

scroll_$(MODE).o: scroll.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

text_$(MODE).o: text.c ../generic/generic_prints.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@

bitmap_$(MODE).o: bitmap.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D$(MODE) -c $< -o $@
