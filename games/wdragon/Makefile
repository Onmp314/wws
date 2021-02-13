# Makefile for W dragon

include ../../.config

# To enable larger bitmaps, change the "1" to any number from 2 to 5.
N_BITMAP_SIZES = 1

CPPFLAGS= -I../../lib -DN_BITMAP_SIZES=$(N_BITMAP_SIZES)
CFLAGS	= $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(CLIENTLIBS)
LIBDEP	= ../../lib/$(WLIB)


OBJS = board.o button.o draw.o icon.o main.o tile.o tile_bits.o wfuncs.o

wdragon: $(OBJS) $(LIBDEP)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) wdragon

# Dependencies

board.o : board.c main.h proto.h board.h 
button.o : button.c main.h proto.h board.h 
draw.o : draw.c main.h proto.h board.h 
icon.o : icon.c main.h proto.h
main.o : main.c main.h proto.h 
tile.o : tile.c main.h proto.h 
tile_bits.o : tile_bits.c main.h
wfuncs.o : wfuncs.c main.h proto.h 
