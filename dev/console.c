#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/console.h"

/* Functions */

int console_init(struct console *cns)
{
    cns->use_err = 0;
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
    (void)(qvm); /* UNUSED */

    switch (address - IO_CONSOLE_BASE) {
    case IO_CONSOLE_IN:
        v = fgetc(stdin);
        break;
    case IO_CONSOLE_USE_ERR:
        v = cns->use_err;
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
    (void)(qvm); /* UNUSED */

    switch (address - IO_CONSOLE_BASE) {
    case IO_CONSOLE_OUT:
        fp =(cns->use_err) ? stderr : stdout;
        fputc(v, fp);
        fflush(fp);
        break;
    case IO_CONSOLE_USE_ERR:
        cns->use_err = (int) v;
        break;
    }
}
