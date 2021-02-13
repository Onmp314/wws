# server/graph/monochrome/Makefile, part of W
#
# Makefile for byte order neutral monochrome bitmap graphics driver
#
# This is converted from packed-monochrome driver by removing COLOR
# and COLORMONO source and changing screen access from long based to
# byte based or using byte order change functions from <netinet/in.h>.
# 'register' directives are also removed as too processor specific.
#
# I wonder whether the bitblk stuff TeSche optimized for longs will now
# that it deals with bytes still be noticably faster than something
# simpler...
#
#	++eero

include ../../../.config

CFLAGS = $(BASECFLAGS) $(SERVERCFLAGS)


OBJS = bfmask8.o bitblit.o line.o dline.o mouse.o point.o scroll.o text.o \
	bitmap.o

all: $(OBJS)

clean:
	$(RM) *~ *.o

veryclean: clean

#
# rules for the .o files
#

bitblit.o: bitblit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

block.o: block.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

line.o: line.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

dline.o: dline.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

mouse.o: mouse.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

point.o: point.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

scroll.o: scroll.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

text.o: text.c ../generic/generic_prints.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

bitmap.o: bitmap.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
