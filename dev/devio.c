#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/devio.h"
#include "dev/console.h"

/* Functions */

int devio_init(struct devio *io)
{
    io->cns = NULL;
    io->stg = NULL;

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
}

void devio_update(struct devio *io)
{
    (void)(io); /* UNUSED */
}

uint32_t devio_read_callback(const struct quivm *qvm, uint32_t address)
{
    const struct devio *io;
    io = (const struct devio *) qvm->arg;

    if ((address >= IO_CONSOLE_BASE) && (address < IO_CONSOLE_END)) {
        return console_read_callback(io->cns, qvm, address);
    }
    if ((address >= IO_STORAGE_BASE) && (address < IO_STORAGE_END)) {
        return storage_read_callback(io->stg, qvm, address);
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
}

