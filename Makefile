.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	mklint.o\
	text.o\
	ui.o

HDR =\
	common.h

all: mklint
$(OBJ): $(HDR)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

mklint: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

install: mklint
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1/"
	cp -- mklint "$(DESTDIR)$(PREFIX)/bin/"
	cp -- mklint.1 "$(DESTDIR)$(MANPREFIX)/man1/"

uninstall:
	-rm -f -- "$(DESTDIR)$(PREFIX)/bin/mklint"
	-rm -f -- "$(DESTDIR)$(MANPREFIX)/man1/mklint.1"

clean:
	-rm -f -- *.o *.a *.lo *.su *.so *.so.* *.gch *.gcov *.gcno *.gcda
	-rm -f -- mklint

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
