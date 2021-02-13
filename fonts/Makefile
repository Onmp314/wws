#
# makefile for W fonts and converters
#
# installs just fonts, not programs

include ../.config


CPPFLAGS = -I../lib
CFLAGS = $(BASECFLAGS)
LDFLAGS = -L../lib $(BASELDFLAGS)


TARGETS = bdf2wfnt fontconv

all: $(TARGETS)

bdf2wfnt: bdf2wfnt.o
	$(CC) $(LDFLAGS) $< -o $@

fontconv: fontconv.o
	$(CC) $(LDFLAGS) $< -o $@


FONTS = $(wildcard *.wfnt)

install:
	$(INSTALLDIRS) $(FONTDIR)
	for i in $(FONTS) ; do $(INSTALL) -m 644 $$i $(FONTDIR) ; done

clean:
	$(RM) *.o $(JUNK)

veryclean: clean
	$(RM) $(TARGETS)
