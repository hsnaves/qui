# makefile for building qui

# Set the defaults

ifndef OPTIMIZE
    OPTIMIZE := 1
endif

ifndef DEBUG
    DEBUG := 0
endif

ifndef INCLUDE_DEFAULT_ROM
    INCLUDE_DEFAULT_ROM := 0
endif

ifndef USE_SDL
    USE_SDL := 1
endif

ifndef BUILD_WASM
    BUILD_WASM := 0
endif

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

# General definitions

RM := rm -f
INSTALL := install
ifneq ($(BUILD_WASM), 0)
    CC := emcc
else
    CC := gcc
endif

CFLAGS := -Wall -Wextra -pedantic $(EXTRA_CFLAGS)
LDFLAGS :=
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

MAIN_CFLAGS := $(CFLAGS)

ifneq ($(INCLUDE_DEFAULT_ROM), 0)
    MAIN_CFLAGS := $(MAIN_CFLAGS) -DINCLUDE_DEFAULT_ROM
endif

ifneq ($(BUILD_WASM), 0)
    ifneq ($(USE_SDL), 0)
        MAIN_CFLAGS := $(MAIN_CFLAGS) -DUSE_SDL -sUSE_SDL=2
        LDFLAGS := $(LDFLAGS) -sUSE_SDL=2
    endif
else
    ifneq ($(USE_SDL), 0)
        MAIN_CFLAGS := $(MAIN_CFLAGS) -DUSE_SDL \
                       $(shell sdl2-config --cflags)
        LDFLAGS := $(LDFLAGS) $(shell sdl2-config --libs)
    endif
endif

# Main targets

ifneq ($(BUILD_WASM), 0)
    TARGET := qui.js
else
    TARGET := qui
endif

all: $(TARGET)

include module.mk

# Pattern rules

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

main.o: main.c
	$(CC) $(MAIN_CFLAGS) $(INCLUDES) -c $< -o $@

install: qui
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 755 qui $(DESTDIR)$(PREFIX)/bin/

clean-main:
	$(RM) main.o

clean:
	$(RM) $(TARGET) $(OBJS)

.PHONY: all install clean-main clean
