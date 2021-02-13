#
# makefile for W Lua interpreter
#

include ../.config

NAME	= wlua

CFLAGS	:= -I../lib -I../wt/Wt $(BASECFLAGS) $(CLIENTCFLAGS) $(shell pkg-config --cflags lua5.1)
LDFLAGS	:= -L../lib -L../wt/Wt $(CLIENTLDFLAGS) $(shell pkg-config --libs lua5.1)
LDLIBS	:= -lWt $(CLIENTLIBS) -ltolua # lua4: -llua -llualib -lm
OBJS	= wlua5.o wt.o wlib.o


all: $(NAME)

$(NAME): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

wlua4.o: wlua4.c
	$(CC) -c -o $@ $(CFLAGS) -D_POSIX_SOURCE $<

wlib.c: fixwlib.sh ../lib/Wlib.h
	./$<

../lib/Wlib.h:
	$(MAKE) -C ../lib Wlib.h

wt.c: fixwt.sh wt.pkg wt-util.h
	./$<


install: $(NAME)
ifdef BINDIR
	$(INSTALLDIRS) $(BINDIR)
	$(INSTALLBIN) $(NAME) $(BINDIR)
endif


# NOTE: wlib.pkg is auto-generated, wt.pkg not!
clean:
	$(RM) wt.c wlib.c wlib.pkg $(OBJS) $(JUNK)

veryclean: clean
	$(RM) wlua
