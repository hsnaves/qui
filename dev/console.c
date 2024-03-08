#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/console.h"

/* Functions */

int console_init(struct console *cns)
{
    cns->internal = NULL;
    return 0;
}

void console_destroy(struct console *cns)
{
    (void)(cns); /* UNUSED */
}

uint32_t console_read_callback(const struct console *cns,
                               const struct quivm *qvm, uint32_t address)
{
    uint32_t v;

    (void)(cns); /* UNUSED */
    (void)(qvm); /* UNUSED */

    switch (address - IO_CONSOLE_BASE) {
    case CONSOLE_CELL_IN:
        v = fgetc(stdin);
        break;
    default:
        v = -1;
        break;
    }
    return v;
}

void console_write_callback(struct console *cns,  struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    (void)(cns); /* UNUSED */
    (void)(qvm); /* UNUSED */

    switch (address - IO_CONSOLE_BASE) {
    case CONSOLE_CELL_OUT:
        fputc(v, stdout);
        fflush(stdout);
        break;
    }
}
