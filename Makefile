# main makefile

# General definitions

RM := rm -f
CAT := cat

KERNEL_DEPS := forth/kernel/pre1.fth forth/bootstrap/inline.fth \
 forth/kernel/pre2.fth forth/kernel/main.fth forth/kernel/scope.fth \
 forth/kernel/scopeimpl.fth forth/io/storage.fth \
 forth/kernel/module.fth forth/kernel/other.fth forth/kernel/end.fth

BOOTSTRAP_DEPS := forth/bootstrap/build.fth \
 forth/meta/meta.fth forth/meta/basic.fth forth/meta/interp.fth \
 forth/meta/scope.fth forth/meta/init.fth forth/bootstrap/globals.fth \
 forth/bootstrap/comp.fth forth/bootstrap/tib.fth \
 forth/bootstrap/inline.fth forth/bootstrap/wordmain.fth \
 forth/bootstrap/parsing.fth forth/bootstrap/find.fth \
 forth/bootstrap/other.fth forth/bootstrap/main.fth \
 forth/bootstrap/interp.fth forth/bootstrap/boot.fth \
 forth/kernel/main.fth forth/kernel/scopeimpl.fth forth/io/console.fth

all: kernel.rom src/qui src/qui-sdl

src/qui:
	$(MAKE) -C src qui

src/qui-sdl: src/kernel_rom.c
	$(MAKE) -C src qui-sdl

kernel.rom: $(KERNEL_DEPS) src/qui
	$(CAT) $(KERNEL_DEPS) | ./src/qui -r bootstrap.rom

bootstrap.rom: $(BOOTSTRAP_DEPS) src/qui
	./src/qui -r kernel.rom forth/bootstrap/build.fth

src/kernel_rom.c: kernel.rom
	./src/qui -r kernel.rom forth/utils/embed_rom.fth kernel.rom src/kernel_rom.c
	$(RM) src/qui src/main.o
	$(MAKE) -C src qui

install:
	$(MAKE) -C src install

clean:
	$(MAKE) -C src clean
	$(RM) kernel.rom src/kernel_rom.c
	$(RM) src/qui src/qui-sdl

.PHONY: all build install clean
