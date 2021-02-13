#
# WetScape - Makefile
#
# $Id: Makefile,v 1.3 2009-08-30 19:10:11 eero Exp $
#

NAME = wetscape

include ../../.config
include ../.wtconfig

# Linux and many others have jpeg library as a system one, MiNT doesn't
ifdef JPEGLIB
LIBJPEG = libjpeg/$(JPEGLIB)
INCJPEG = -Ilibjpeg
endif

CFLAGS	= -I../Wt -I../../lib $(INCJPEG) $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../Wt -L../../lib -Llibjpeg  $(CLIENTLDFLAGS)
LDLIBS	= $(WTLIBS) -ljpeg

# ignore these uchar* vs. char* issues for now
CFLAGS += -Wno-pointer-sign

#LDLIBS	= -lW2X -L/usr/X11R6/lib -lX11 -ljpeg
#LDLIBS	=g -L../../w2xlib -lW2Xg -L/usr/X11R6/lib -lX11 -ljpeg -lefence

LIBDEP	= ../Wt/$(WTLIB) ../../lib/$(WLIB) $(LIBJPEG)


SRCS=main.c url.c proxy.c mime.c decoders.c io.c io_file.c io_http.c \
  dither.c image.c image_xbm.c image_gif.c image_jpeg.c util.c config.c

OBJS=main.o url.o proxy.o mime.o decoders.o io.o io_file.o io_http.o \
  dither.o image.o image_xbm.o image_gif.o image_jpeg.o util.o config.o


TARGETS = $(NAME)

all: $(TARGETS)


$(NAME): $(OBJS) $(LIBJPEG)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(NAME).sym: $(OBJS) $(LIBJPEG)
	$(CC) $(SYM_LD) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)


$(LIBJPEG):
ifeq ($(ARCH),MiNT)
	@echo "libjpeg configure/Makefile needs still a bit fixing for MiNT..."
else
	(cd libjpeg; ./configure)
endif
	$(MAKE) -C libjpeg $(JPEGLIB)


clean:
	$(MAKE) -C libjpeg clean
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(NAME) $(NAME).sym $(LIBJPEG)

package: veryclean
	tar -zcvf ../$(NAME).tgz -C ../ $(NAME)/

install: $(NAME)
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	$(INSTALLBIN) $(NAME) $(BINDIR)
endif
