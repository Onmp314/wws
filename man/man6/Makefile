#
# Makefile for W games (section 6) manual pages
#

include ../../.config

MAN6 = $(wildcard *.6)

install: $(MAN6)
	$(INSTALLDIRS) $(MANDIR)/man6
	for i in $(MAN6) ; do \
		echo $(INSTALL) -m 644 $$i $(MANDIR)/man6 ; \
		$(INSTALL) -m 644 $$i $(MANDIR)/man6 ; \
	done
