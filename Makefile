# makefile for qui

# General definitions

INSTALL := install
RM := rm -f
CAT := cat

QUI := ./src/qui


all: build rom.bin

build:
	$(MAKE) -C src

rom.bin: forth/rom/extra.fth
	$(CAT) $^ | $(QUI) -r kernel.bin > rom.bin

kernel.bin: forth/kernel/meta.fth forth/kernel/kernel.fth
	$(CAT) $^ | $(QUI) -r rom.bin > kernel.bin

install:
	$(MAKE) -C src install

clean:
	$(MAKE) -C src clean
	$(RM) rom.bin

.PHONY: all build install clean
