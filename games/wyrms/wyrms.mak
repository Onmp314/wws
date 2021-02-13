# A SozobonX C makefile for wyrms (C) 1995 by Eero Tamminen
#
# Use -DBIG_BITMAPS for 16x16 bitmaps (help 'level' won't
# fit into screen then though...)

GEM     = -DGEM_WYRM
TOS     = -DTOS_WYRM
CC      = cc
CFLAGS  = -I. -O -e -v -DBIG_BITMAPS
TOBJS   = twyrms.o tos_sys.o tlevel.o joyisrz.o
GOBJS   = gwyrms.o gfx_sys.o glevel.o
GEMLIBS = gem_gfx.a xaesfast.a xvdifast.a

all: wyrms.gtp wyrms.ttp

gemwyrms: wyrms.gtp
wyrms.gtp: $(GOBJS)
	$(CC) -o $@ $(GOBJS) $(LDFLAGS) $(GEMLIBS)

toswyrms: wyrms.ttp
wyrms.ttp: $(TOBJS)
	$(CC) -o $@ $(TOBJS) $(LDFLAGS)


gwyrms.o: wyrms.c guiwyrms.h
	$(CC) $(GEM) $(CFLAGS) -o $@ $< -c
gfx_sys.o: gfx_sys.c guiwyrms.h 16x16.h 8x8.h
	$(CC) $(GEM)  $(CFLAGS) -o $@ $< -c
glevel.o: level.c guiwyrms.h
	$(CC) $(GEM)  $(CFLAGS) -o $@ $< -c

wrapper.o: wrapper.c wrapper.h
	$(CC) $(CFLAGS) -o $@ $< -c

twyrms.o: wyrms.c toswyrms.h
	$(CC) $(TOS) $(CFLAGS) -o $@ $< -c
tos_sys.o: tos_sys.c toswyrms.h 16x16.h 8x8.h
	$(CC) $(TOS) $(CFLAGS) -o $@ $< -c
tlevel.o: level.c toswyrms.h
	$(CC) $(TOS) $(CFLAGS) -o $@ $< -c
joyisrz.o: joyisrz.s
