# Copyright (c) 2026 etar125 Admanse
# Licensed under ISC (see LICENSE)

include config.mk

SRC = ssgc.c main.c
OBJ = $(SRC:%.c=%.o)

all: ssgc

%.o: %.c config.mk
	$(CC) $(ECFLAGS) -c $< -o $@

ssgc: $(OBJ)
	$(CC) $(ECFLAGS) $^ -o $@

clean:
	rm -rf ssgc
	rm -rf $(OBJ)

install: ssgc
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp ssgc $(DESTDIR)$(PREFIX)/bin/
	chmod +x $(DESTDIR)$(PREFIX)/bin/ssgc

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/ssgc

.PHONY: all clean install uninstall
