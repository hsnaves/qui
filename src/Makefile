# makefile for building qui

# Set the defaults

ifndef OPTIMIZE
    OPTIMIZE := 1
endif

ifndef DEBUG
    DEBUG := 0
endif

ifndef USE_SDL
    USE_SDL := 1
endif

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

# General definitions

CC := gcc
INSTALL := install
RM := rm -f

CFLAGS := -Wall -Wextra -pedantic $(EXTRA_CFLAGS)
LDFLAGS :=
INCLUDES := -I.
LIBS :=

# Modify the FLAGS based on the options

ifneq ($(USE_SDL), 0)
    CFLAGS := $(CFLAGS) -DUSE_SDL $(shell sdl2-config --cflags)
    LDFLAGS := $(LDFLAGS) $(shell sdl2-config --libs)
endif

ifneq ($(OPTIMIZE), 0)
    # Maybe add -fno-math-errno
    CFLAGS := $(CFLAGS) -O3
else
    CFLAGS := $(CFLAGS) -O0
endif

ifneq ($(DEBUG), 0)
    CFLAGS := $(CFLAGS) -g -ggdb
    LDFLAGS := $(LDFLAGS) -g
endif

# Main targets

all: qui

include module.mk

# Pattern rules

qui: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: qui
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 755 qui $(DESTDIR)$(PREFIX)/bin/

clean:
	$(RM) qui $(OBJS)

.PHONY: all install clean
