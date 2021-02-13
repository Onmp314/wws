#
# makefile for W documentation installation
#

include ../.config

#
# rules
#

ifdef DOCDIR
install: textdocs htmldocs readmes
else
install:
	@echo "WARNING: DOCDIR isn't set, cannot install documentation"
endif

DOCS = BUGS CHANGES CHANGES.Eeros COPYRIGHTS FAQ FILES.doc GPL LGPL MIT \
  PORTING README.MacMiNT README.old TODO ../README ../RELEASE.notes

textdocs: $(DOCS)
	$(INSTALLDIRS) $(DOCDIR)
	for i in $^; do	$(INSTALL) -m 644 $$i $(DOCDIR); done


htmldocs: $(wildcard html/*.html)  $(wildcard html/*.png)
	$(INSTALLDIRS) $(DOCDIR)/html
	for i in $^; do	$(INSTALL) -m 644 $$i $(DOCDIR)/html; done


READMES = $(shell find .. -mindepth 2 -name 'README*'|grep -v /docs/)

readmes: $(READMES)
	$(INSTALLDIRS) $(DOCDIR)
	for i in $^; do	\
	basename=$${i##*/}; \
	dirname=$${i%/*}; \
	dirname=$${dirname##*/}; \
	$(INSTALL) -m 644 $$i $(DOCDIR)/$$basename.$$dirname; \
	done
