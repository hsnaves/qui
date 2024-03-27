#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/console.h"

/* Functions */

int console_init(struct console *cns)
{
    cns->argc = 0;
    cns->argv = NULL;
    cns->envp = NULL;

    cns->channel = 0;
    cns->argi = cns->argii = 0;
    cns->envi = cns->envii = 0;
    return 0;
}

void console_destroy(struct console *cns)
{
    cns->argc = 0;
    cns->argv = NULL;
    cns->envp = NULL;
}

uint32_t console_read_callback(struct console *cns,
                               struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_CONSOLE_IN:
        v = fgetc(stdin);
        break;
    case IO_CONSOLE_CHANNEL:
        v = cns->channel;
        break;
    case IO_CONSOLE_ARGIN:
        if (cns->argi < cns->argc) {
            v = (uint32_t) cns->argv[cns->argi][cns->argii++];
            if (v == 0) {
                cns->argi++;
                cns->argii = 0;
            }
        } else {
            /* end of arguments */
            v = -1;
        }
        break;
    case IO_CONSOLE_ENVIN:
        if (cns->envp[cns->envi]) {
            v = (uint32_t) cns->envp[cns->envi][cns->envii];
            if (v == 0) {
                cns->envi++;
                cns->envii = 0;
            } else {
                cns->envii++;
            }
        } else {
            /* end of enviroment variables */
            v = -1;
        }
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

    fp = stderr;
    switch (address) {
    case IO_CONSOLE_OUT:
        fp = (cns->channel) ? stderr : stdout;
        fputc(v, fp);
        fflush(fp);
        break;
    case IO_CONSOLE_CHANNEL:
        cns->channel = v;
        break;
    }
}
