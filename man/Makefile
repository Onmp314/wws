#
# Makefile for W manual pages
#

include ../.config

install:
ifdef MANDIR
	$(MAKE) -C man1 install
	$(MAKE) -C man3 install
	$(MAKE) -C man6 install
endif

clean:
	$(RM) cat1/*.1

veryclean: clean
