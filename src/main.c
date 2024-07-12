#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/devio.h"
#include "dev/console.h"
#include "dev/storage.h"
#include "dev/network.h"
#include "dev/rtclock.h"
#include "dev/display.h"
#include "dev/audio.h"
#include "dev/keyboard.h"
#include "dev/timer.h"

/* Global variables */

#ifdef INCLUDE_DEFAULT_ROM
#    include "default_rom.c"
#else
/* Default rom contains just an infinite loop */
static const uint8_t default_rom[] = { 0xBE, 0xC2 };
#endif

/* Functions */

/* Runs a single frame of simulation */
static
void run_one_frame(struct quivm *qvm)
{
    struct devio *io;
    struct display *dpl;
    struct audio *aud;

    io = (struct devio *) qvm->arg;
    dpl = io->dpl;
    aud = io->aud;

    qvm->status |= STS_RUNNING;
    if (!quivm_run(qvm))
        goto destroy_vm;

    devio_update(io);

    /* check if some device terminated the VM */
    if (!(qvm->status & (STS_RUNNING | STS_WAITING)))
        goto destroy_vm;

    /* No support for display or audio */
    if (dpl->initialized || aud->initialized) {
        fprintf(stderr, "run_one_frame: "
                "no support for display or audio\n");
        quivm_terminate(qvm, 1);
        goto destroy_vm;
    }
    return;

destroy_vm:

    quivm_destroy(qvm);
    devio_destroy(io);
    return;
}

/* Auxiliary function to run the VM.
 * Returns zero on success.
 */
static
int run(struct quivm *qvm)
{
    while (1) {
        run_one_frame(qvm);
        if (!(qvm->status & (STS_RUNNING | STS_WAITING)))
            break;
    }
    return (qvm->status & (STS_RETVAL_MASK));
}

/* Prints the help text to the console.
 * The name of the executable is given in `execname`.
 */
static
void print_help(const char *execname)
{
    printf("Usage:\n");
    printf("  %s [OPTIONS] [--] args ...\n", execname);
    printf("where the available options are:\n");
    printf("  -r <romfile>       Specify the rom file to use\n");
    printf("  --readonly         To not allow writes in the storage device\n");
    printf("  --bind <addr>      Binds the UDP socket to a given address\n");
    printf("  --target <addr>    The address of the target socket\n");
    printf("  --port <port>      The UDP port to bind to\n");
    printf("  --utc              To use UTC for the real time clock\n");
    printf("  -h|--help          Print this help\n");
}

/* main function */
int main(int argc, char **argv, char **envp)
{
    /* static variables are not destroyed at the end of main
     * this is necessary when using emscripten.
     */
    static struct quivm qvm;
    static struct devio io;

    const char *filename;
    const char *bind_address;
    const char *target_address;
    uint32_t length;
    int use_utc;
    int disable_write;
    int port;
    int i, ret;
    char *end;

    filename = NULL;
    bind_address = NULL;
    target_address = NULL;
    use_utc = 0;
    disable_write = 0;
    port = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i == argc - 1) goto missing_argument;
            filename = argv[++i];
        } else if (strcmp(argv[i], "--readonly") == 0) {
            disable_write = 1;
        } else if (strcmp(argv[i], "--utc") == 0) {
            use_utc = 1;
        } else if (strcmp(argv[i], "--port") == 0) {
            if (i == argc - 1) goto missing_argument;
            port = strtol(argv[++i], &end, 10);
            if (end[0] != '\0') goto invalid_argument;
        } else if (strcmp(argv[i], "--bind") == 0) {
            if (i == argc - 1) goto missing_argument;
            bind_address = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0) {
            if (i == argc - 1) goto missing_argument;
            target_address = argv[++i];
        } else if ((strcmp(argv[i], "-h") == 0)
                   || (strcmp(argv[i], "--help") == 0)) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--") == 0) {
            i++; break;
        } else {
            break;
        }
        continue;

    missing_argument:
        fprintf(stderr, "main: "
                "missing argument for `%s`\n", argv[i]);
        return 1;

    invalid_argument:
        fprintf(stderr, "main: "
                "invalid argument for `%s`: `%s`\n",
                argv[i - 1], argv[i]);
        return 1;
    }

    /* Move the arguments forward */
    argc -= i;
    argv = &argv[i];

    if (quivm_init(&qvm)) {
        fprintf(stderr, "main: could not initialize the VM\n");
        return 1;
    }

    if (devio_init(&io)) {
        fprintf(stderr, "main: could not initialize I/O\n");
        quivm_destroy(&qvm);
        return 1;
    }

    devio_configure(&io, &qvm);

    if (filename) {
        length = 0;
        if (quivm_load(&qvm, filename, 0, &length)) {
            fprintf(stderr, "main: could not load image `%s`\n",
                    filename);
            quivm_destroy(&qvm);
            devio_destroy(&io);
            return 1;
        }
    } else {
        length = sizeof(default_rom);
        quivm_load_array(&qvm, default_rom, 0, &length);
    }

    /* configure the devices */
    storage_configure(io.stg, disable_write);
    rtclock_configure(io.rtc, use_utc);
    network_configure(io.ntw, bind_address, target_address, port);
    console_configure(io.cns, argc,
                      (const char * const *) argv,
                      (const char * const *) envp);

    ret = run(&qvm);
    return ret;
}
