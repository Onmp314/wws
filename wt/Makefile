#
# Makefile for running make on toolkit/ subdirectories
#
# NOTES:
# - Warzone isn't even nearly complete, but it has a dial widget
#   (which needs linking with the math lib)
# - Edittext widget is with wedit because it's buggy

# OS specific etc. configuration options (libs etc)
include ../.config

all:
	@echo ; $(MAKE) -C Wt
	@echo ; $(MAKE) -C Test
	@echo ; $(MAKE) -C wvt
	@echo ; $(MAKE) -C wetscape
	@echo ; $(MAKE) -C math
	@echo ; $(MAKE) -C wstart
	@echo ; $(MAKE) -C wicone
	@echo ; $(MAKE) -C wdraft
	@echo ; $(MAKE) -C wchat
	@echo ; $(MAKE) -C waudio
	@echo ; $(MAKE) -C wedit
	@echo ; $(MAKE) -C netgames
#	@echo ; $(MAKE) -C warzone

install:
ifdef BINDIR
	@echo ; $(MAKE) -C Wt install
	@echo ; $(MAKE) -C Doc install
	@echo ; $(MAKE) -C wvt install
	@echo ; $(MAKE) -C wetscape install
	@echo ; $(MAKE) -C math install
	@echo ; $(MAKE) -C wstart install
	@echo ; $(MAKE) -C wicone install
	@echo ; $(MAKE) -C wdraft install
	@echo ; $(MAKE) -C wchat install
	@echo ; $(MAKE) -C waudio install
	@echo ; $(MAKE) -C wedit install
	@echo ; $(MAKE) -C netgames install
#	@echo ; $(MAKE) -C warzone install
ifdef DATADIR
	$(INSTALL) -m 644 wtrc $(DATADIR)
else
	@echo
	@echo "Now edit and copy 'wtrc' to your home directory as '.wtrc'!"
endif
else
	@echo "BINDIR not set in the ../.config include file!"
endif

clean:
	@echo ; $(MAKE) -C Wt clean
	@echo ; $(MAKE) -C Test clean
	@echo ; $(MAKE) -C wvt clean
	@echo ; $(MAKE) -C wetscape clean
	@echo ; $(MAKE) -C math clean
	@echo ; $(MAKE) -C wstart clean
	@echo ; $(MAKE) -C wicone clean
	@echo ; $(MAKE) -C wdraft clean
	@echo ; $(MAKE) -C wchat clean
	@echo ; $(MAKE) -C waudio clean
	@echo ; $(MAKE) -C wedit clean
	@echo ; $(MAKE) -C netgames clean
	@echo ; $(MAKE) -C warzone clean

veryclean:
	@echo ; $(MAKE) -C Wt veryclean
	@echo ; $(MAKE) -C Test veryclean
	@echo ; $(MAKE) -C wvt veryclean
	@echo ; $(MAKE) -C wetscape veryclean
	@echo ; $(MAKE) -C math veryclean
	@echo ; $(MAKE) -C wstart veryclean
	@echo ; $(MAKE) -C wicone veryclean
	@echo ; $(MAKE) -C wdraft veryclean
	@echo ; $(MAKE) -C wchat veryclean
	@echo ; $(MAKE) -C waudio veryclean
	@echo ; $(MAKE) -C wedit veryclean
	@echo ; $(MAKE) -C netgames veryclean
	@echo ; $(MAKE) -C warzone veryclean

# library and programs will be in separate packages
package:
	@echo ; $(MAKE) -C Wt veryclean
	@echo ; $(MAKE) -C Test veryclean
	tar -C ../ -zcvf Wt.tgz toolkit/{Wt,Test,README,README.Eeros,Doc,Makefile,CHANGES../.wtrc,../.configinclude .wtconfig}
	@echo ; $(MAKE) -C wvt package
	@echo ; $(MAKE) -C wetscape package
	@echo ; $(MAKE) -C math package
	@echo ; $(MAKE) -C wstart package
	@echo ; $(MAKE) -C wicone package
	@echo ; $(MAKE) -C wdraft package
	@echo ; $(MAKE) -C wedit package
	@echo ; $(MAKE) -C wchat package
	@echo ; $(MAKE) -C waudio package
	@echo ; $(MAKE) -C netgames package
	@echo ; $(MAKE) -C warzone package
