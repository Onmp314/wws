# Makefile for W-Jewel
#
# Window.c file (and the bitmaps to some extent) is GUI dependent.
# Other files use the functions provided by window.c and should
# be portable.

include ../../.config

CFLAGS = -I../../lib $(BASECFLAGS) $(CLIENTLDFLAGS)
LDFLAGS = -L../../lib $(CLIENTLDFLAGS)
LDLIBS = $(CLIENTLIBS)
LIBDEP = ../../lib/$(WLIB)


OBJS	= 16x16.o 32x32.o jewel.o game.o window.o
NAME	= wjewel

all: $(NAME)

$(NAME): $(OBJS) $(LIBDEP)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

$(NAME).sym: $(OBJS) $(LIBDEP)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SYM_LD)
	toglclr -fshare $(NAME)


$(OBJS): jewel.h


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/
