.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	makelint.o\
	ui.o

HDR =\
	common.h

all: makelint
$(OBJ): $(HDR)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

makelint: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

install: makelint
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1/"
	cp -- makelint "$(DESTDIR)$(PREFIX)/bin/"
	cp -- makelint.1 "$(DESTDIR)$(MANPREFIX)/man1/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/makelint"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/makelint.1"

clean:
	-rm -f -- *.o *.a *.lo *.su *.so *.so.* *.gch *.gcov *.gcno *.gcda
	-rm -f -- makelint

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
