#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "vm/quivm.h"
#include "dev/console.h"

/* Data structures and types */
/* Internal data structure for the console device */
struct cns_internal {
    struct termios old_state;   /* old terminal state */
};

/* Functions */

int console_init(struct console *cns)
{
    struct cns_internal *ci;

    cns->internal = NULL;
    ci = (struct cns_internal *) malloc(sizeof(*ci));
    if (!ci) {
        fprintf(stderr, "console: init: memory exhausted\n");
        return 1;
    }
    tcgetattr(STDIN_FILENO, &ci->old_state);
    cns->internal = (void *) ci;

    cns->channel = (CONSOLE_FLAGS_ECHO | CONSOLE_FLAGS_CANONICAL);
    console_configure(cns, 0, NULL, NULL);
    return 0;
}

void console_destroy(struct console *cns)
{
    struct cns_internal *ci;
    if (cns->internal) {
        ci = (struct cns_internal *) cns->internal;
        tcsetattr(STDIN_FILENO, TCSANOW, &ci->old_state);
        free(cns->internal);
    }
    cns->internal = NULL;
}

void console_configure(struct console *cns, int argc,
                       const char * const *argv,
                       const char * const *envp)
{
    cns->argc = argc;
    cns->argv = argv;
    cns->envp = envp;

    cns->argi = cns->argii = 0;
    cns->envi = cns->envii = 0;
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
        switch (cns->channel & CONSOLE_ICHANNEL_MASK) {
        case CONSOLE_ICHANNEL_STDIN:
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
            ret = select(1, &rfds, NULL, NULL, &tv);

            if (ret > 0) {
                v = 0;
                ret = read(STDIN_FILENO, &v, 1);
                if (ret <= 0) v = -1;
            } else {
                /* halt the machine until it has data */
                qvm->status &= ~(STS_RUNNING);
                qvm->status |= (STS_WAITING);
                if (qvm->status & STS_JMPBUF) {
                    qvm->pc--; /* rewind the PC */
                    quivm_raise(qvm);
                }
                v = -1;
            }
            break;
        case CONSOLE_ICHANNEL_ARGS:
            if (cns->argi < cns->argc) {
                v = (uint32_t) cns->argv[cns->argi][cns->argii++];
                if (v == 0) {
                    cns->argi++;
                    cns->argii = 0;
                    v = '\n';
                }
            } else {
                /* end of arguments, switch to stdin */
                v = '\n';
                cns->channel &= ~CONSOLE_ICHANNEL_MASK;
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
                /* end of enviroment variables, switch to stdin */
                v = '\n';
                cns->channel &= ~CONSOLE_ICHANNEL_MASK;
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

/* Updates the value of the channel variable to `v` */
static
void update_channel(struct console *cns, uint32_t v)
{
    struct cns_internal *ci;
    struct termios new_state;
    if ((cns->channel & CONSOLE_FLAGS_MASK) != (v & CONSOLE_FLAGS_MASK)) {
        /* change the terminal state according to the flags */
        ci = (struct cns_internal *) cns->internal;
        new_state = ci->old_state;
        new_state.c_lflag &= ~(ICANON | ECHO);
        if (v & CONSOLE_FLAGS_ECHO) new_state.c_lflag |= ECHO;
        if (v & CONSOLE_FLAGS_CANONICAL) new_state.c_lflag |= ICANON;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_state);
    }
    cns->channel = v;
}

void console_write_callback(struct console *cns, struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    int fd;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_CONSOLE_OUT:
        switch ((cns->channel & CONSOLE_OCHANNEL_MASK)) {
        case CONSOLE_OCHANNEL_STDOUT: fd = STDOUT_FILENO; break;
        case CONSOLE_OCHANNEL_STDERR: fd = STDERR_FILENO; break;
        default: fd = -1; break;
        }
        if (fd >= 0) write(fd, &v, 1);
        break;
    case IO_CONSOLE_CHANNEL:
        update_channel(cns, v);
        break;
    }
}
