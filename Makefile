#
# top level makefile for the W Window System
#
# CHANGES
# ++eero 3/1998:
# - moved configuration variables into configs/ subdirectory.
# ++eero 4/2004:
# - added SDL option
# - wt is now part of basic W packages
# ++eero 7/2008
# - more info at install what else needs to be done
# ++eero 8/2009
# - Moved *DIR variables setting for .config file here from configs/* files
# - Added scripts, icons and documentation installation
# - support for separate installation and run paths

WMAJ = 1
WMIN = 4
WPL  = 5

NAME = w$(WMAJ)r$(WMIN)pl$(WPL)
CONFIG = .config
COPY = cp

ifeq ($(CONFIG),$(wildcard $(CONFIG)))
include $(CONFIG)
else
help:
	@echo "First you have to 'make config'!"
	@echo
	@echo "Makefile recognizes automatically some OSes and tries to"
	@echo "configure the build system accordingly.  If this fails or"
	@echo "you want to configure W window system e.g. for a certain"
	@echo "gfx system, there are some special targets. Just check the"
	@echo "main Makefile or look into configs/ subdirectory for examples."
	@false
endif


basic:
	@date > .started
	@echo ; $(MAKE) -C lib
	@echo ; $(MAKE) -C server
	@echo ; $(MAKE) -C apps
	@echo ; $(MAKE) -C games
	@echo ; $(MAKE) -C benchmark
	@echo ; $(MAKE) -C wt

all: $(CONFIG) basic
	@echo
	@echo "make started/finished at:"
	@cat .started ; rm -f .started ; date
	@echo ""

full: $(CONFIG) basic
	@echo ; $(MAKE) -C fonts
	@echo ; $(MAKE) -C demos
	@echo ; $(MAKE) -C w2xlib
	@echo ; $(MAKE) -C wlua
	@echo
	@echo "make started/finished at:"
	@cat .started ; rm -f .started ; date
	@echo ""


installbasic:
	@echo ; $(MAKE) -C lib install
	@echo ; $(MAKE) -C server install
	@echo ; $(MAKE) -C apps install
	@echo ; $(MAKE) -C games install
	@echo ; $(MAKE) -C benchmark install
	@echo ; $(MAKE) -C wt install
	@echo ; $(MAKE) -C fonts install
	@echo ; $(MAKE) -C man install
	@echo ; $(MAKE) -C scripts install
	@echo ; $(MAKE) -C docs install
ifdef DATADIR
	$(INSTALL) -m 644 wconfig $(DATADIR)
	$(INSTALL) -m 755 wrc $(DATADIR)
endif


install: installbasic
	@echo ""
	@echo "W Window System installed, what next?"
	@echo "-------------------------------------"
	@echo ""
	@echo "You may need to run 'ldconfig $(LIBDIR)' (as root) for W shared"
	@echo "libraries to get loaded when W server and clients are started."
	@echo ""
	@echo "And to get terminal programs working, 'tic wterm.terminfo'."
	@echo ""
	@echo "Then you could copy 'wconfig' and 'wrc' to your home directory"
	@echo "as '.wconfig' and '.wrc' and edit them to customize server root"
	@echo "menu, fonts and colors to your preferences and to automatically"
	@echo "run programs when the server starts up."
	@echo ""
	@echo "W window system can be started with the 'startw' script."
	@echo "Some useful scripts are in scripts/ -subdirectory."
	@echo ""

installfull: installbasic
	@echo ; $(MAKE) -C demos install
	@echo ; $(MAKE) -C w2xlib install
	@echo ; $(MAKE) -C wlua install


# Some systems (SVGALIB) need wserver to be suid root to access graphics.
# WTerm would also need to be suid to be able to write into 'utmp'
# file, but as that's not necessary it won't be done by default. Less
# there are possible security holes, the better.
set-rights:
	chown 0.0 $(BINDIR)/wserver
	chmod u+s $(BINDIR)/wserver
	@echo "wserver is now suid root!"


clean:
	@echo ; $(MAKE) -C lib clean
	@echo ; $(MAKE) -C server clean
	@echo ; $(MAKE) -C apps clean
	@echo ; $(MAKE) -C games clean
	@echo ; $(MAKE) -C benchmark clean
	@echo ; $(MAKE) -C wt clean
	@echo ; $(MAKE) -C fonts clean
	@echo ; $(MAKE) -C man clean
	@$(RM) .started

cleanfull: clean
	@echo ; $(MAKE) -C demos clean
	@echo ; $(MAKE) -C w2xlib clean
	@echo ; $(MAKE) -C wlua clean

verycleanbasic:
	@echo ; $(MAKE) -C lib veryclean
	@echo ; $(MAKE) -C server veryclean
	@echo ; $(MAKE) -C apps veryclean
	@echo ; $(MAKE) -C games veryclean
	@echo ; $(MAKE) -C benchmark veryclean
	@echo ; $(MAKE) -C wt veryclean
	@echo ; $(MAKE) -C fonts veryclean
	@echo ; $(MAKE) -C man veryclean
	@echo ; $(MAKE) -C man/cat1 veryclean

veryclean: verycleanbasic
	@$(RM) .started $(CONFIG)

verycleanfull: verycleanbasic
	@echo ; $(MAKE) -C demos veryclean
	@echo ; $(MAKE) -C w2xlib veryclean
	@echo ; $(MAKE) -C wlua veryclean
	@$(RM) .started $(CONFIG)


ifeq ($(CONFIG),$(wildcard $(CONFIG)))
package: verycleanfull
	tar -zcvf ../$(NAME).tgz --exclude CVS -C ../ wws/
else
package: config verycleanfull
	tar -zcvf ../$(NAME).tgz --exclude CVS -C ../ wws/
endif


#
# some rules to configurate to your setup. it uses 'uname' to
# determine your OS and copy platform dependent defines from
# configs/ directory to '.config' which will be included by subsequent
# makefiles. if you port W to a new platform you must add a new
# section here and corresponding settings file into configs/.
#

# MiNT's "uname" seems to always print a \n even with -n
ARCH = $(shell echo -n `uname -s`)

# for linux get also the machine type (Motorola, Intel, Sparc, Alpha...)
ifeq ($(ARCH),Linux)
ARCH = $(shell uname -s)-$(shell uname -m)
endif


# default configs should always build all the graphics drivers given platform
# supports.  We want to see whether they (still) compile, won't we?
#
# We'll need to check the whole uname output here as the same OS might work
# on platforms requiring different options (libraries, graphics drivers).

Linux-m68k:
	$(COPY) configs/$@ $(CONFIG)

Linux-arm Linux-x86 Linux-i486 Linux-i586 Linux-i686: linuxsdl-config

Linux-mips-r3912: linuxr3912-config messages

linuxr3912-config:
	$(COPY) configs/Linux-mips-r3912 $(CONFIG)

OSF1:
	$(COPY) configs/$@ $(CONFIG)
	@echo
	@echo "***WARNING***  This entry is /intended/ for testing whether W (clients)"
	@echo "can work with 64-bit ints, but the testing/fixing hasn't been done yet."

MiNT:
	$(COPY) configs/$@ $(CONFIG)

NetBSD:
	$(COPY) configs/$@ $(CONFIG)

#
# Sparc2/SunOS-4: sun4c
# Sparc5/Solaris-2.4: sun4m
# Ultra1/Solaris-2.5.1: sun4u
#
# only works on sun4c so far, so no further discrimination
#

SunOS:
	$(COPY) configs/$@ $(CONFIG)


#
# now here are the real rules
#

$(CONFIG):
	@echo
	@echo "You need to 'make config' first before doing the real compile!"
	@echo
	@echo "Alternative configurations are svgalib-config, linuxfb-config,"
	@echo "linuxggi-config and linuxsdl-config which configure W against"
	@echo "different graphics and input backends."
	@echo
	@false


# If any of these differ on your machine, send us mail and we'll move
# that setting to files in configs/!

# overridable installation directory prefix for Debian
ifdef DESTDIR
PREFIX := /usr
else
# overridable installation directory prefix for direct compilation
PREFIX ?= /usr/local
endif

pathversion:
	@echo "" >> $(CONFIG)
	@echo "# version" >> $(CONFIG)
	@echo "WMAJ = " $(WMAJ) >> $(CONFIG)
	@echo "WMIN = " $(WMIN) >> $(CONFIG)
	@echo "WPL  = " $(WPL)  >> $(CONFIG)
	@echo "" >> $(CONFIG)
	@echo "# installation directories" >> $(CONFIG)
	@echo "DOCDIR	= " $(DESTDIR)$(PREFIX)/share/doc/wws >> $(CONFIG)
	@echo "MANDIR	= " $(DESTDIR)$(PREFIX)/share/man >> $(CONFIG)
	@echo "INCDIR	= " $(DESTDIR)$(PREFIX)/include >> $(CONFIG)
	@echo "BINDIR	= " $(DESTDIR)$(PREFIX)/bin >> $(CONFIG)
	@echo "LIBDIR	= " $(DESTDIR)$(PREFIX)/lib >> $(CONFIG)
	@echo "GAMEDIR	= " $(DESTDIR)$(PREFIX)/games >> $(CONFIG)
	@echo "DATADIR	= " $(DESTDIR)$(PREFIX)/share/wws >> $(CONFIG)
	@echo "ICONDIR	= " $(DESTDIR)$(PREFIX)/share/wws/icons >> $(CONFIG)
	@echo "FONTDIR	= " $(DESTDIR)$(PREFIX)/share/wws/fonts >> $(CONFIG)
	@echo "# run-time directories for server" >> $(CONFIG)
	@echo "DATADIR_RUN	= " $(PREFIX)/share/wws >> $(CONFIG)
	@echo "ICONDIR_RUN	= " $(PREFIX)/share/wws/icons >> $(CONFIG)
	@echo "FONTDIR_RUN	= " $(PREFIX)/share/wws/fonts >> $(CONFIG)

messages:
	@echo
	@echo "You're running a '"$(ARCH)"' system which is 'supported' by W, fine."
	@echo
	@echo "A file '.config' has been created for you in the main directory. This one will"
	@echo "be included by all the other Makefiles to customize to your setup. You should"
	@echo "check that compiling options, shell utilities and installation paths there are"
	@echo "suitable for your setup (normally they should, but better be sure than sorry)."
	@echo
	@echo "Makefile offers the standard 'install', 'clean' and 'veryclean' targets."
	@echo "There are also 'installfull', 'cleanfull' and 'verycleanfull' targets."
	@echo
	@echo "Now you can continue to build the default binaries by typing '$(MAKE)' or"
	@echo "(in Bourne shell) '$(MAKE) > errors 2>&1' if you want to check out"
	@echo "the compiler messages later."
	@echo

config: $(ARCH) pathversion messages

# --------- special configs -----------

linuxfb:
	$(COPY) configs/Linux-x86 $(CONFIG)
	@echo
	@echo "NOTE: for-x86 linux, you'll need also GPM library for accessing"
	@echo "the mouse.  Because of a GPM feature, W screen contents won't be"
	@echo "restored after VT-switch.  Either compile W with the REFRESH"
	@echo "option (in server/config.h, adds redraw option to W root menu)"
	@echo "or 'repaint' the screen with some window. ;-)"

# linux framebuffer conf
linuxfb-config: linuxfb messages

svgalib:
	$(COPY) configs/Linux-SVGAlib $(CONFIG)

# linux-svgalib config
svgalib-config: svgalib messages

linuxggi:
	$(COPY) configs/Linux-GGI $(CONFIG)
	@echo
	@echo "NOTE: for linux-GGI, you'll need also libGGI/libGII libraries"
	@echo "with an API compatible with this W version.  LibGGI will"
	@echo "allow you to run W (server) in addition to linux-fb also"
	@echo "with X, SVGAlib etc..."

# linux framebuffer conf
linuxggi-config: linuxggi messages

linuxsdl:
	$(COPY) configs/Linux-SDL $(CONFIG)
	@echo
	@echo "NOTE: LibSDL will allow you to run W (server) in X,"
	@echo "Linux framebuffer etc..."

# SDL configuration
linuxsdl-config: linuxsdl messages


# Default handler for unknown systems to provide a proper error message,
# in case any of the following system types don't overrule this.
.DEFAULT:
	@echo "If '$@' target is a platform, it doesn't seem to be supported."
	@echo
	@echo "In that case you could write up the configs/$@ configuration"
	@echo "file and Makefile entry for it and retry 'make config'.  When"
	@echo "you'll succeed, mail your changes to 'oak@welho.com'"
	@echo "(current maintainer)."
	@false
