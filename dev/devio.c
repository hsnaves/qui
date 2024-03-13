#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/devio.h"
#include "dev/console.h"
#include "dev/display.h"

/* Functions */

int devio_init(struct devio *io)
{
    io->cns = NULL;
    io->stg = NULL;
    io->dpl = NULL;

    io->cns = (struct console *) malloc(sizeof(struct console));
    if (!io->cns) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (console_init(io->cns)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the console device\n");
        devio_destroy(io);
        return 1;
    }

    io->stg = (struct storage *) malloc(sizeof(struct storage));
    if (!io->stg) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (storage_init(io->stg)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the storage device\n");
        devio_destroy(io);
        return 1;
    }

    io->dpl = (struct display *) malloc(sizeof(struct display));
    if (!io->dpl) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (display_init(io->dpl)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the display device\n");
        devio_destroy(io);
        return 1;
    }

    return 0;
}

void devio_destroy(struct devio *io)
{
    if (io->cns) {
        console_destroy(io->cns);
        free(io->cns);
    }
    io->cns = NULL;

    if (io->stg) {
        storage_destroy(io->stg);
        free(io->stg);
    }
    io->stg = NULL;

    if (io->dpl) {
        display_destroy(io->dpl);
        free(io->dpl);
    }
    io->dpl = NULL;
}

void devio_update(struct quivm *qvm)
{
    struct devio *io;
    io = (struct devio *) qvm->arg;

    display_update(io->dpl, qvm);
}

uint32_t devio_read_callback(struct quivm *qvm, uint32_t address)
{
    struct devio *io;
    io = (struct devio *) qvm->arg;

    if ((address >= IO_CONSOLE_BASE) && (address < IO_CONSOLE_END)) {
        return console_read_callback(io->cns, qvm, address);
    }
    if ((address >= IO_STORAGE_BASE) && (address < IO_STORAGE_END)) {
        return storage_read_callback(io->stg, qvm, address);
    }
    if ((address >= IO_DISPLAY_BASE) && (address < IO_DISPLAY_END)) {
        return display_read_callback(io->dpl, qvm, address);
    }

    return -1;
}

void devio_write_callback(struct quivm *qvm, uint32_t address, uint32_t v)
{
    struct devio *io;
    io = (struct devio *) qvm->arg;

    if ((address >= IO_CONSOLE_BASE) && (address < IO_CONSOLE_END)) {
        console_write_callback(io->cns, qvm, address, v);
        return;
    }
    if ((address >= IO_STORAGE_BASE) && (address < IO_STORAGE_END)) {
        storage_write_callback(io->stg, qvm, address, v);
        return;
    }
    if ((address >= IO_DISPLAY_BASE) && (address < IO_DISPLAY_END)) {
        display_write_callback(io->dpl, qvm, address, v);
        return;
    }
}

