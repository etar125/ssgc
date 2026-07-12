VERSION = 0.2.1

PREFIX = $(HOME)/.local

ECFLAGS = -Iinclude -std=c99 -pedantic -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -Wall -Wextra -le1l -lelist -DVERSION=\"$(VERSION)\" $(CFLAGS)

CC = cc
AR = ar
