# bardo -- bardo lattice (C edition)
#
#   make                 build ./bardo
#   make check           build, then test it in a throwaway folder
#   make install         install to /usr/local (use sudo), or:
#   make install PREFIX=$(HOME)/.local
#   make uninstall
#   make clean

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
MANDIR  ?= $(PREFIX)/share/man/man1

CC      ?= cc
CFLAGS  ?= -O2 -g
CFLAGS  += -std=c99 -Wall -Wextra -pedantic
LDLIBS  += -lm

all: bardo

bardo: bardo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ bardo.c $(LDLIBS)

check: bardo
	@tmp=$$(mktemp -d) && \
	./bardo --dir "$$tmp" print > "$$tmp/out.txt" && \
	n=$$(ls "$$tmp"/[0-9]*/ | grep -c '\.md$$') && \
	y=$$(date +%Y) && \
	want=$$( [ $$(( y % 4 == 0 && y % 100 != 0 || y % 400 == 0 )) -eq 1 ] && echo 366 || echo 365 ) && \
	test "$$n" -eq "$$want" && \
	test -f "$$tmp/$$y/.bardo" && \
	printf 'hello there world\n' >> "$$tmp/$$y/$$y-01-01.md" && \
	./bardo --dir "$$tmp" print | grep -q ' 1 written · 3 words' && \
	rm -rf "$$tmp" && echo "check passed: $$n day files, word counting works"

install: bardo
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(MANDIR)
	install -m 755 bardo $(DESTDIR)$(BINDIR)/bardo
	install -m 644 bardo.1 $(DESTDIR)$(MANDIR)/bardo.1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/bardo $(DESTDIR)$(MANDIR)/bardo.1

clean:
	rm -f bardo

.PHONY: all check install uninstall clean
