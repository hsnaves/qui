# main makefile

# General definitions

RM := rm -f
CAT := cat

ROM_DEPS := forth/rom/main.fth forth/rom/scope.fth \
 forth/rom/scopeimpl.fth forth/io/storage.fth forth/rom/module.fth \
 forth/rom/other.fth forth/rom/end.fth

KERNEL_DEPS := forth/kernel/build.fth \
 forth/meta/meta.fth forth/meta/basic.fth forth/meta/interp.fth \
 forth/meta/scope.fth forth/meta/init.fth forth/kernel/globals.fth \
 forth/kernel/ibasic.fth forth/kernel/herelast.fth forth/kernel/imain.fth \
 forth/kernel/tib.fth forth/kernel/flags.fth forth/kernel/wordmain.fth \
 forth/kernel/parsing.fth forth/kernel/find.fth forth/kernel/other.fth \
 forth/kernel/main.fth forth/kernel/interp.fth forth/kernel/boot.fth \
 forth/rom/main.fth forth/rom/scopeimpl.fth forth/io/console.fth

all: main.rom src/qui src/qui-sdl

src/qui:
	$(MAKE) -C src qui

src/qui-sdl: src/default_rom.c
	$(MAKE) -C src qui-sdl

main.rom: $(ROM_DEPS) src/qui
	$(CAT) $(ROM_DEPS) | ./src/qui -r kernel.rom

kernel.rom: $(KERNEL_DEPS) src/qui
	./src/qui -r main.rom forth/kernel/build.fth

src/default_rom.c: main.rom
	./src/qui -r main.rom forth/utils/default_rom.fth main.rom src/default_rom.c
	$(RM) src/qui src/main.o
	$(MAKE) -C src qui

install:
	$(MAKE) -C src install

clean:
	$(MAKE) -C src clean
	$(RM) main.rom src/default_rom.c
	$(RM) src/qui src/qui-sdl

.PHONY: all build install clean
