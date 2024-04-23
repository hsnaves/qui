# makefile for qui

ifndef BUILD_WASM
    BUILD_WASM := 0
endif

# General definitions

INSTALL := install
LD := ld
RM := rm -f
CAT := cat

ROM_DEPS := forth/rom/main.fth forth/rom/scope.fth \
 forth/rom/scopeimpl.fth forth/io/storage.fth forth/rom/module.fth \
 forth/rom/table.fth forth/rom/other.fth forth/rom/disasm.fth \
 forth/rom/end.fth

KERNEL_DEPS := forth/kernel/build.fth \
 forth/meta/meta.fth forth/meta/basic.fth forth/meta/interp.fth \
 forth/meta/scope.fth forth/meta/init.fth forth/kernel/globals.fth \
 forth/kernel/ibasic.fth forth/kernel/herelast.fth forth/kernel/imain.fth \
 forth/kernel/wordmain.fth forth/kernel/colon.fth forth/kernel/main.fth \
 forth/kernel/parsing.fth forth/kernel/interp.fth forth/kernel/boot.fth \
 forth/rom/main.fth forth/rom/scopeimpl.fth forth/io/console.fth

ifneq ($(BUILD_WASM), 0)
    TARGET := src/qui.js
else
    TARGET := src/qui
endif

all: main.rom $(TARGET)

src/qui-headless:
	INCLUDE_DEFAULT_ROM=0 USE_SDL=0 BUILD_WASM=0 \
	TARGET=qui-headless $(MAKE) -C src
	$(RM) src/main.o

ifneq ($(BUILD_WASM), 0)
$(TARGET): src/default_rom.c
	$(MAKE) -C src clean-objs
	INCLUDE_DEFAULT_ROM=1 BUILD_WASM=1 $(MAKE) -C src
	$(MAKE) -C src clean-objs
else
$(TARGET): src/default_rom.c
	INCLUDE_DEFAULT_ROM=1 BUILD_WASM=0 $(MAKE) -C src
endif

main.rom: $(ROM_DEPS) src/qui-headless
	$(CAT) $(ROM_DEPS) | ./src/qui-headless -r kernel.rom

kernel.rom: $(KERNEL_DEPS) src/qui-headless
	./src/qui-headless -r main.rom < forth/kernel/build.fth

src/default_rom.c: main.rom
	./src/qui-headless -r main.rom < forth/utils/default_rom.fth

ifneq ($(BUILD_WASM), 0)
install:
	$(MAKE) -C src install
	$(MAKE) -C src clean-objs
else
install:
	$(MAKE) -C src install
endif

clean:
	$(MAKE) -C src clean
	$(RM) main.rom src/default_rom.c
	$(RM) src/qui src/qui-headless src/qui.js src/qui.wasm

.PHONY: all build install clean
