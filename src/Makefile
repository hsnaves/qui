# makefile for building qui and qui-sdl

# Set the defaults

ifndef OPTIMIZE
    OPTIMIZE := 1
endif

ifndef DEBUG
    DEBUG := 0
endif

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := $(HOME)/.local/
endif

# General definitions

RM := rm -f
INSTALL := install
CC ?= gcc

CFLAGS := -Wall -Wextra -pedantic $(EXTRA_CFLAGS)
LDFLAGS :=
INCLUDES := -I.
LIBS :=

# Modify the FLAGS based on the options

ifneq ($(OPTIMIZE), 0)
    CFLAGS := $(CFLAGS) -O3
else
    CFLAGS := $(CFLAGS) -O0
endif

ifneq ($(DEBUG), 0)
    CFLAGS := $(CFLAGS) -g -ggdb
    LDFLAGS := $(LDFLAGS) -g
endif

ifneq ("$(wildcard default_rom.c)","")
    MAIN_CFLAGS := $(CFLAGS) -DINCLUDE_DEFAULT_ROM
else
    MAIN_CFLAGS := $(CFLAGS)
endif

SDL_CFLAGS := $(CFLAGS) $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(LDFLAGS) $(shell sdl2-config --libs)

# Main targets
TARGETS := qui qui-sdl

all: $(TARGETS)

include module.mk

# Pattern rules

qui: $(OBJS) main.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

qui-sdl: $(OBJS) main-sdl.o
	$(CC) $(SDL_LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

main.o: main.c
	$(CC) $(MAIN_CFLAGS) $(INCLUDES) -c $< -o $@

main-sdl.o: main-sdl.c
	$(CC) $(SDL_CFLAGS) $(INCLUDES) -c $< -o $@

install: $(TARGETS)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 755 $(TARGETS) $(DESTDIR)$(PREFIX)/bin/

clean:
	$(RM) $(OBJS) main.o main-sdl.o
	$(RM) $(TARGETS)

.PHONY: all install clean
