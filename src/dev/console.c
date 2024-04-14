#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

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
    setvbuf(stdin, NULL, _IONBF, 0);
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
    fd_set rfds;
    struct timeval tv;
    uint32_t v;
    int ret;

    switch (address) {
    case IO_CONSOLE_IN:
        switch ((cns->channel >> 8) & 0xFF) {
        case CONSOLE_ICHANNEL_STDIN:
            FD_ZERO(&rfds);
            FD_SET(0, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            ret = select(1, &rfds, NULL, NULL, &tv);
            if (ret > 0) {
                v = fgetc(stdin);
            } else {
                /* halt the machine until it has data */
                qvm->status |= STS_HALTED;
                v = -1;
            }
            break;
        case CONSOLE_ICHANNEL_ARGS:
            if (cns->argi < cns->argc) {
                v = (uint32_t) cns->argv[cns->argi][cns->argii++];
                if (v == 0) {
                    cns->argi++;
                    cns->argii = 0;
                    v = (cns->argi < cns->argc) ? ' ' : '\n';
                }
            } else {
                /* end of arguments */
                v = '\0';
                cns->channel &= 0xFFFF00FF;
            }
            break;
        case CONSOLE_ICHANNEL_ENV:
            if (cns->envp[cns->envi]) {
                v = (uint32_t) cns->envp[cns->envi][cns->envii++];
                if (v == 0) {
                    cns->envi++;
                    cns->envii = 0;
                    v = '\n';
                }
            } else {
                /* end of enviroment variables */
                v = '\0';
                cns->channel &= 0xFFFF00FF;
            }
            break;
        default:
            v = -1;
            break;
        }
        break;
    case IO_CONSOLE_CHANNEL:
        v = cns->channel;
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

    switch (address) {
    case IO_CONSOLE_OUT:
        switch ((cns->channel & 0xFF)) {
        case CONSOLE_OCHANNEL_STDOUT: fp = stdout; break;
        case CONSOLE_OCHANNEL_STDERR: fp = stderr; break;
        default: fp = NULL; break;
        }

        if (fp) {
            fputc(v, fp);
            fflush(fp);
        }
        break;
    case IO_CONSOLE_CHANNEL:
        cns->channel = v;
        break;
    }
}
