# makefile for qui

# General definitions

INSTALL := install
RM := rm -f
CAT := cat
QUI := ./src/qui

ROM_DEPS := forth/rom/main.fth forth/rom/scope.fth \
 forth/rom/scopeimpl.fth forth/io/storage.fth forth/io/rtclock.fth \
 forth/utils/module.fth forth/rom/other.fth forth/utils/disasm.fth \
 forth/rom/end.fth

KERNEL_DEPS := forth/kernel/build.fth \
 forth/meta/meta.fth forth/meta/basic.fth forth/meta/interp.fth \
 forth/meta/scope.fth forth/kernel/globals.fth forth/kernel/ibasic.fth \
 forth/kernel/herelast.fth forth/kernel/imain.fth forth/kernel/iextra.fth \
 forth/kernel/wordmain.fth forth/kernel/colon.fth forth/kernel/main.fth \
 forth/kernel/parsing.fth forth/kernel/interp.fth forth/kernel/boot.fth \
 forth/rom/main.fth forth/rom/scopeimpl.fth forth/io/console.fth

all: build rom.bin

build:
	$(MAKE) -C src

rom.bin: $(ROM_DEPS)
	$(CAT) $^ | $(QUI) -r kernel.bin

kernel.bin: $(KERNEL_DEPS)
	$(CAT) forth/kernel/build.fth | $(QUI) -r rom.bin

install:
	$(MAKE) -C src install

clean:
	$(MAKE) -C src clean
	$(RM) rom.bin

.PHONY: all build install clean
