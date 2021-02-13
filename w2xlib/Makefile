#
# Makefile for W2Xlib, Wlib emulation on top of Xlib
#

NAME = libW2X.so

include ../.config

#
# targets, depending on the machine
#

CFLAGS = $(BASECFLAGS) $(CLIENTCFLAGS) -DTRACE

#
# some definitions
#

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
SOBJS = $(patsubst %.c, %.O, $(SRCS))
BOBJS = $(patsubst %.c, %.ob, $(SRCS))
GOBJS = $(patsubst %.c, %.og, $(SRCS))

#
# rules
#

# have to run Make again so that patterns get right *after* links are generated
all: links
	@echo ; $(MAKE) -C ./ $(NAME)
#	@echo
#	@echo "Btw. if there are suspiciously few object files,"
#	@echo "run '$(MAKE) links' and then try '$(MAKE)' again."


# versioned Linux shared library
libW2X.so.$(WMAJ).$(WMIN): Wlib.h $(SOBJS)
	$(CC) -shared -Wl,-soname,$@ $(SOBJS) -o $@ -lX11

libW2X.so: libW2X.so.$(WMAJ).$(WMIN)
	$(LN) -sf $< $@

libW2X.a: Wlib.h $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

libW2Xg.a: Wlib.h $(GOBJS)
	$(RM) $@
	$(AR) $@ $(GOBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

blibW2X.a: Wlib.h $(BOBJS)
	$(RM) $@
	$(AR) $@ $(BOBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

# some MiNT linkers use this name format
bW2X.olb: Wlib.h $(BOBJS)
	$(RM) $@
	$(AR) $@ $(BOBJS)
ifdef RANLIB
	$(RANLIB) $@
endif


# mouse.c and winutil.c not implemented
# socket.c and vhline.c not needed
DONT_USE = mouse.c socket.c vhline.c winutil.c

# make sure needed links exist
links:
	@cd ../lib ; \
	for i in *.[ch] ; do \
		if [ ! -f ../w2xlib/$$i ] ; then \
			ln -s ../lib/$$i ../w2xlib/$$i ; \
		fi ; \
	done ; \
	cd ../w2xlib ; \
	for i in $(DONT_USE); do \
		if [ -L $$i ] ; then \
			rm $$i ; \
		fi ; \
	done
	@true


.SUFFIXES: .O .ob .og

.c.O: Wlib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fpic -c $< -o $@

.c.ob: Wlib.h
	$(CC) $(CPPFLAGS) $(BASECFLAGS) -mbaserel -c $< -o $@

.c.og: Wlib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -g -c $< -o $@


Wlib.h: ../lib/Wlib.h

../lib/Wlib.h:
	$(MAKE) -C ../lib Wlib.h
	ln -sf $@ .


clean:
	$(RM) *.o *.O *.ob *.og $(JUNK)

veryclean: clean
	$(RM) *.a *.so *.a.* *.so.* *.olb

install: Wlib.h $(NAME)
ifdef INCDIR
	$(INSTALLDIRS) $(INCDIR)
	$(INSTALL) -m 644 Wlib.h $(INCDIR)
endif
ifdef LIBDIR
	$(INSTALLDIRS) $(LIBDIR)
ifeq ($(NAME),libW2X.so)
	# shared libraries are versioned
	$(INSTALL) -m 644 $(NAME).$(WMAJ).$(WMIN) $(LIBDIR)
	cd $(LIBDIR); ln -s $(NAME).$(WMAJ).$(WMIN) $(NAME)
else
	$(INSTALL) -m 644 $(NAME) $(LIBDIR)
endif
endif
