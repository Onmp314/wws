#
# The W Toolkit - Makefile
#
# $Id: Makefile,v 1.8 2009-08-29 23:08:49 eero Exp $
#

WTMAJ = 0
WTMIN = 9
WTPL = 5

include ../../.config

CFLAGS = -I. -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS) -DTRACE

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

all: $(WTLIB)

libWt.a: $(OBJS) Wt.h ../../lib/Wlib.h
	$(RM) $@
	$(AR) $@ $(OBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

libWtg.a: $(GOBJS) Wt.h ../../lib/Wlib.h
	$(RM) $@
	$(AR) $@ $(GOBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

# versioned Linux shared library
libWt.so.$(WTMAJ).$(WTMIN): $(SOBJS) Wt.h ../../lib/Wlib.h
	$(CC) -shared -Wl,-soname,$@ $(SOBJS) -o $@ -L../../lib -lW

libWt.so: libWt.so.$(WTMAJ).$(WTMIN)
	$(LN) -sf $< $@

blibWt.a: $(BOBJS) Wt.h ../../lib/Wlib.h
	$(RM) $@
	$(AR) $@ $(BOBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

# some MiNT linkers use this name format
bWt.olb: $(BOBJS) Wt.h ../../lib/Wlib.h
	$(RM) $@
	$(AR) $@ $(BOBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

.SUFFIXES: .O .ob .og

.c.O: Wt.h ../../lib/Wlib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fpic -c $< -o $@
vt.O: vt.c Wt.h ../../lib/Wlib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-pointer-sign -fpic -c $< -o $@

.c.ob: Wt.h ../../lib/Wlib.h
	$(CC) $(CPPFLAGS) $(BASECFLAGS) -mbaserel -c $< -o $@

.c.og: Wt.h ../../lib/Wlib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -g -c $< -o $@


../../lib/Wlib.h:
	$(MAKE) -C ../../lib Wlib.h
	ln -sf $@ .


clean:
	$(RM) *.o *.O *.ob *.og $(JUNK)

veryclean: clean
	$(RM) *.a *.so *.a.* *.so.*

install: Wt.h $(WTLIB)
ifdef INCDIR
	$(INSTALLDIRS) $(INCDIR)
	$(INSTALL) -m 644 Wt.h $(INCDIR)
endif
ifdef LIBDIR
	$(INSTALLDIRS) $(LIBDIR)
ifeq ($(WTLIB),libWt.so)
	# shared libraries are versioned
	$(INSTALL) -m 644 $(WTLIB).$(WTMAJ).$(WTMIN) $(LIBDIR)
	cd $(LIBDIR); ln -s $(WTLIB).$(WTMAJ).$(WTMIN) $(WTLIB)
else
	$(INSTALL) -m 644 $(WTLIB) $(LIBDIR)
endif
endif
