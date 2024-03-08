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

CFLAGS := -Wall -Wextra -ansi -pedantic $(EXTRA_CFLAGS)
LDFLAGS := 

INCLUDES := -I.
LIBS :=

TARGET := qui

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

all: $(TARGET)

include module.mk

# Pattern rules

qui: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: $(TARGET)
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 755 qui $(DESTDIR)$(PREFIX)/bin/

clean:
	$(RM) $(TARGET) $(OBJS)

.PHONY: all install clean
