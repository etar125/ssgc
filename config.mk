VERSION = 0.1.0

PREFIX = $(HOME)/.local

ECFLAGS = -Iinclude -std=c99 -pedantic -D_XOPEN_SOURCE=700 -Wall -le1l -lelist -DVERSION=\"$(VERSION)\" $(CFLAGS)

CC = cc
AR = ar
