#
# Makefile for making ASCII manual pages from the originals
#

include ../../.config


MAN1 = $(wildcard ../man1/*.1)
CAT1 = $(subst ../man1/,,$(MAN1))

all:
	for i in $(CAT1) ; do \
		nroff -man ../man1/$$i > $$i; \
	done

veryclean:
	$(RM) *.1
