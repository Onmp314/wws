#
# makefile for libutil
#

include ../../.config

CPPFLAGS = -I.
CFLAGS = $(BASECFLAGS)

.SUFFIXES: .ob

.c.ob:
	$(CC) $(CFLAGS) -mbaserel -c $< -o $@

OBJS = getopt.o getopt1.o
BOBJS = getopt.ob getopt1.ob


#
# targets, system dependent
#

ifeq ($(ARCH),SunOS)
TARGETS = libutil.a
OBJS += memcpy_sparc.o
endif

ifeq ($(ARCH),NetBSD)
TARGETS = libutil.a
endif

ifeq ($(ARCH),MiNT)
ifeq ($(WLIB),bW.olb)
# old style library names
TARGETS = util.olb butil.olb
else
TARGETS = libutil.a blibutil.a
endif
endif

all: $(TARGETS)

memcpy_sparc.o: memcpy_sparc.S

libutil.a: $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

blibutil.a: $(BOBJS)
	$(RM) $@
	$(AR) $@ $(BOBJS)
ifdef RANLIB
	$(RANLIB) $@	
endif

util.olb: $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
ifdef RANLIB
	$(RANLIB) $@
endif

butil.olb: $(BOBJS)
	$(RM) $@
	$(AR) $@ $(BOBJS)
ifdef RANLIB
	$(RANLIB) $@	
endif

clean:
	$(RM) *~ *.o *.ob

veryclean: clean
	$(RM) *.a
