# This is a makefile for two new MacMiNT devices:
# - a keyboard driver which also mapps cursor keys to something
#   that code in W keyboard handler recognizes.
# - a mouse driver outputting Atari style mouse events.
#
# (w) 1999 by Jonathan Oddie

include ../../.config

# you may add -m68020, -m68030, -m68040, depending on your computer
OPTIMIZE = -O2 

# add -DDEBUG_ON to see tracing information without having to turn up the debug level
DEFS = -mshort -DMAC -fomit-frame-pointer -Wall
INCS = -I$(MACINC) -I../xconout2
LIBS = -L$(MACLIB) -llibmac16 -lpml16 -lgnu16
CFLAGS = $(INCS) $(DEFS) $(OPTIMIZE) 
AS = gcc-as

all: macmouse.xdd mackbd.xdd

macmouse.xdd: macmouse.o init.o
	$(CC) -nostdlib -G $(CFLAGS) -o macmouse.xdd init.o macmouse.o $(LIBS)

mackbd.xdd: mackbd.o init.o
	$(CC) -nostdlib -G $(CFLAGS) -o mackbd.xdd init.o mackbd.o $(LIBS)

macmouse.o: macmouse.c
	$(CC) $(CFLAGS) -o macmouse.o -c macmouse.c

mackbd.o: mackbd.c
	$(CC) $(CFLAGS) -o mackbd.o -c mackbd.c

clean:
	rm -f *.o

veryclean: clean
	rm -f *.xdd

install: all
	mv macmouse.xdd mackbd.xdd /mint
