#
# The W Toolkit testers - Makefile
#
# $Id: Makefile,v 1.3 2009-08-30 15:48:37 eero Exp $
#

include ../../.config
include ../.wtconfig

CFLAGS	= -I../Wt -I../../lib $(BASECFLAGS) $(CLIENTCFLAGS)
LDFLAGS	= -L../Wt -L../../lib $(CLIENTLDFLAGS)
LDLIBS	= $(WTLIBS)

LIBDEP	= ../Wt/$(WTLIB) ../../lib/$(WLIB)

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))

.SUFFIXES: .o
.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


TARGETS = buttons draw drawables fileselect iconeditor labels lister \
	listtest packtest popper ranger textinput timer_io viewports \
	vttest w4


all: $(TARGETS)

buttons: buttons.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

draw: draw.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

drawables: drawables.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

fileselect: fileselect.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

iconeditor: iconeditor.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

labels: labels.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

lister: lister.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

listtest: listtest.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

packtest: packtest.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

popper: popper.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

ranger: ranger.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

textinput: textinput.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

timer_io: timer_io.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

viewports: viewports.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

vttest: vttest.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)

w4: w4.o #$(LIBDEP)
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS)


clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(TARGETS)

