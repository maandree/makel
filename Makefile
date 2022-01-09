.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	makel.o\
	makefile.o\
	text.o\
	ui.o\
	util.o

HDR =\
	arg.h\
	common.h

all: makel check
$(OBJ): $(HDR)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

makel: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

check: makel
	./test

install: makel
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1/"
	cp -- makel "$(DESTDIR)$(PREFIX)/bin/"
	cp -- makel.1 "$(DESTDIR)$(MANPREFIX)/man1/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/makel"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/makel.1"

clean:
	-rm -f -- *.o *.a *.su *.gcov *.gcno *.gcda
	-rm -f -- makel

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all check install uninstall clean
