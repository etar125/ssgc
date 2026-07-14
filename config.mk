VERSION = 0.2.1

PREFIX = $(HOME)/.local

ELDFLAGS = -le1l -lelist $(LDFLAGS)
ECFLAGS = -Iinclude -std=c99 -pedantic -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -Wall -Wextra -DVERSION=\"$(VERSION)\" $(CFLAGS)

CC = cc
AR = ar
