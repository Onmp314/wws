#
# Makefile for W library (section 3) W manual pages
#

include ../../.config

MAN3 = $(wildcard *.3w)

install: $(MAN3)
	$(INSTALLDIRS) $(MANDIR)/man3
	for i in $(MAN3) ; do \
		echo $(INSTALL) -m 644 $$i $(MANDIR)/man3 ; \
		$(INSTALL) -m 644 $$i $(MANDIR)/man3 ; \
	done
