# makefile for qui

# General definitions

INSTALL := install
RM := rm -f
CAT := cat
QUI := ./src/qui

ROM_DEPS := forth/rom/control.fth forth/rom/scope.fth \
 forth/rom/scopeimpl.fth forth/io/storage.fth forth/rom/printing.fth \
 forth/rom/other.fth

KERNEL_COMPILE_FILES := forth/utils/module.fth forth/kernel/build.fth
KERNEL_DEPS := $(KERNEL_COMPILE_FILES) \
 forth/meta/meta.fth forth/meta/basic.fth forth/meta/interp.fth \
 forth/meta/scope.fth forth/kernel/globals.fth forth/kernel/compbasic.fth \
 forth/kernel/herelast.fth forth/kernel/compmain.fth \
 forth/kernel/compextra.fth forth/kernel/wordmain.fth \
 forth/kernel/colon.fth forth/kernel/main.fth forth/kernel/parsing.fth \
 forth/kernel/interp.fth forth/kernel/boot.fth forth/rom/control.fth \
 forth/rom/scopeimpl.fth

all: build rom.bin

build:
	$(MAKE) -C src

rom.bin: $(ROM_DEPS)
	$(CAT) $^ | $(QUI) -r kernel.bin

kernel.bin: $(KERNEL_DEPS)
	$(CAT) $(KERNEL_COMPILE_FILES) | $(QUI) -r rom.bin

install:
	$(MAKE) -C src install

clean:
	$(MAKE) -C src clean
	$(RM) rom.bin

.PHONY: all build install clean
