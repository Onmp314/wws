#
# Makefile for W application (section 1) manual pages
#

include ../../.config

MAN1 = $(wildcard *.1)

install: $(MAN1)
	$(INSTALLDIRS) $(MANDIR)/man1
	for i in $(MAN1) ; do \
		echo $(INSTALL) -m 644 $$i $(MANDIR)/man1 ; \
		$(INSTALL) -m 644 $$i $(MANDIR)/man1 ; \
	done
