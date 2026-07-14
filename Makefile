# Copyright (c) 2026 etar125 Admanse
# Licensed under ISC (see LICENSE)

include config.mk

SRC = ssgc.c main.c
OBJ = $(SRC:%.c=%.o)

all: ssgc

%.o: %.c config.mk
	$(CC) $(ECFLAGS) -c $< -o $@

ssgc: $(OBJ)
	$(CC) $(ELDFLAGS) $^ -o $@

clean:
	rm -rf ssgc $(OBJ)

cleanobj:
	rm -rf (OBJ)

install: ssgc
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp ssgc $(DESTDIR)$(PREFIX)/bin/
	chmod +x $(DESTDIR)$(PREFIX)/bin/ssgc

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/ssgc

fasttest: ssgc
	memcheck "./ssgc foo.org foo.org.out"

.PHONY: all clean cleanobj install uninstall fasttest
