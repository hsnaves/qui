# makefile for qui

# General definitions

INSTALL := install
LD := ld
RM := rm -f
CAT := cat
QUI := ./src/qui

ROM_DEPS := forth/rom/main.fth forth/rom/scope.fth \
 forth/rom/scopeimpl.fth forth/io/storage.fth forth/io/rtclock.fth \
 forth/rom/module.fth forth/rom/table.fth forth/rom/other.fth \
 forth/rom/disasm.fth forth/rom/end.fth

KERNEL_DEPS := forth/kernel/build.fth \
 forth/meta/meta.fth forth/meta/basic.fth forth/meta/interp.fth \
 forth/meta/scope.fth forth/meta/init.fth forth/kernel/globals.fth \
 forth/kernel/ibasic.fth forth/kernel/herelast.fth forth/kernel/imain.fth \
 forth/kernel/wordmain.fth forth/kernel/colon.fth forth/kernel/main.fth \
 forth/kernel/parsing.fth forth/kernel/interp.fth forth/kernel/boot.fth \
 forth/rom/main.fth forth/rom/scopeimpl.fth forth/io/console.fth

all: build-pre rom.bin build

build-pre:
	$(MAKE) -C src

build: src/default_rom.o
	$(MAKE) -C src clean
	USE_DEFAULT_ROM=1 $(MAKE) -C src

rom.bin: $(ROM_DEPS) $(QUI)
	$(CAT) $(ROM_DEPS) | $(QUI) -r kernel.bin

kernel.bin: $(KERNEL_DEPS) $(QUI)
	$(CAT) forth/kernel/build.fth | $(QUI) -r rom.bin

src/default_rom.o: rom.bin
	$(LD) -r -b binary -o $@ $<

install:
	$(MAKE) -C src install

clean:
	USE_DEFAULT_ROM=1 $(MAKE) -C src clean
	$(RM) rom.bin

.PHONY: all build-pre build install clean
