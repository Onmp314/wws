#
# makefile for xconout2, MiNT output redirector device
#

include ../../.config

all: xconout2.xdd

xconout2.xdd: xconinit.o xconout2.o
	$(CC) -nostdlib -mshort xconinit.o xconout2.o -lc -o xconout2.xdd

xconinit.o: xconinit.s
	$(CC) $< -c -o $@

xconout2.o: xconout2.c
	$(CC) -I. -mshort -c $< -o $@

clean:
	$(RM) *~ *.o *.ob

veryclean: clean
	$(RM) *.a

install:
	@echo "Please copy xconout2.xdd to whatever your MiNT directory might be..."
