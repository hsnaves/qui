# makefile for building qui

# Set the defaults

ifndef OPTIMIZE
    OPTIMIZE := 1
endif

ifndef DEBUG
    DEBUG := 0
endif

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

# General definitions

CC := gcc
INSTALL := install
RM := rm -f

CFLAGS := -Wall -Wextra -ansi -pedantic \
          $(shell sdl2-config --cflags) $(EXTRA_CFLAGS)
LDFLAGS := $(shell sdl2-config --libs)

INCLUDES := -I.
LIBS :=

# Modify the FLAGS based on the options

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

all: qui rom.bin

include module.mk

# Pattern rules

qui: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

rom.bin: rom/extra.fth kernel.bin
	cat rom/extra.fth | ./qui kernel.bin > rom.bin

kernel.bin: rom/meta.fth rom/kernel.fth
	cat rom/meta.fth rom/kernel.fth | ./qui rom.bin > kernel.bin

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: qui rom.bin
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 755 qui $(DESTDIR)$(PREFIX)/bin/

clean:
	$(RM) qui rom.bin $(OBJS)

.PHONY: all install clean
