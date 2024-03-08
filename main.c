#include <stdio.h>
#include <stdlib.h>

#include "vm/quivm.h"
#include "dev/devio.h"

/* Constants */
#define NUM_INSN_PER_FRAME      10000

/* Functions */

/* Auxiliary function to run the VM with a given set of I/O devices. */
static
int run(struct quivm *qvm, struct devio *io, int argc, char **argv)
{
    (void)(argc); /* UNUSED */
    (void)(argv); /* UNUSED */

    while (quivm_run(qvm, NUM_INSN_PER_FRAME)) {
        devio_update(io);
    }

    return qvm->termvalue;
}

/* main function */
int main(int argc, char **argv)
{
    struct quivm qvm;
    struct devio io;
    const char *filename;
    uint32_t length;
    int termvalue;

    argv++; argc--;
    if (argc > 0) {
        filename = argv[0];
        argv++; argc--;
    } else {
        filename = "rom.bin";
    }

    if (devio_init(&io)) {
        fprintf(stderr, "main: could not initialize I/O\n");
        return 1;
    }

    if (quivm_init(&qvm, &devio_read_callback,
                   &devio_write_callback, &io)) {
        devio_destroy(&io);
        fprintf(stderr, "main: could not initialize the VM\n");
        return 1;
    }

    length = 0;
    if (quivm_load(&qvm, filename, 0, &length)) {
        quivm_destroy(&qvm);
        devio_destroy(&io);
        fprintf(stderr, "main: could not load image `%s`\n", filename);
        return 1;
    }

    termvalue = run(&qvm, &io, argc, argv);

    quivm_destroy(&qvm);
    devio_destroy(&io);
    return termvalue;
}
