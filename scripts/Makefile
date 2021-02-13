#
# makefile for W helper scripts
#
# Note: All READMEs are installed by docs/Makefile

include ../.config

#
# rules
#

install: docs scripts

scripts: startw wview wrootpic won xbm2w
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	for i in $^; do $(INSTALL) -m 755 $$i $(BINDIR); done
endif

docs: colors.txt vt52.txt
ifdef DOCDIR
	$(INSTALLDIRS) $(DOCDIR)
	for i in $^; do	$(INSTALL) -m 644 $$i $(DOCDIR); done
endif
