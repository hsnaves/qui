#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/console.h"

/* Functions */

int console_init(struct console *cns)
{
    cns->unused = NULL;
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

    switch (address) {
    case IO_CONSOLE_IN:
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
    FILE *fp;
    (void)(cns); /* UNUSED */
    (void)(qvm); /* UNUSED */

    fp = stderr;
    switch (address) {
    case IO_CONSOLE_OUT:
        fp = stdout;
        /* fall through */
    case IO_CONSOLE_ERR:
        fputc(v, fp);
        fflush(fp);
        break;
    }
}
