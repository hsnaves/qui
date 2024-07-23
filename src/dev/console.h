#ifndef __DEV_CONSOLE_H
#define __DEV_CONSOLE_H

#include <stdint.h>
#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the console device */
#define IO_CONSOLE_BASE         0xFFFFFFC0
#define IO_CONSOLE_END          0xFFFFFFE0

/* Addresses within the console device */
#define IO_CONSOLE_IN           0xFFFFFFDC
#define IO_CONSOLE_OUT          0xFFFFFFD8
#define IO_CONSOLE_CHANNEL      0xFFFFFFD4

/* Channel values */
#define CONSOLE_OCHANNEL_MASK         0xFF
#define CONSOLE_ICHANNEL_MASK       0xFF00
#define CONSOLE_FLAGS_MASK        0xFF0000
#define CONSOLE_OCHANNEL_STDOUT          0
#define CONSOLE_OCHANNEL_STDERR          1
#define CONSOLE_ICHANNEL_STDIN    (0 << 8)
#define CONSOLE_ICHANNEL_ARGS     (1 << 8)
#define CONSOLE_ICHANNEL_ENV      (2 << 8)
#define CONSOLE_FLAGS_NOECHO     (1 << 16)
#define CONSOLE_FLAGS_RAW        (1 << 17)
#define CONSOLE_FLAGS_NOWAIT     (1 << 18)

/* Data structures and types */
/* A structure representing the console device */
struct console {
    int argc;                   /* number of arguments in the input */
    const char *const *argv;    /* arguments passed to the program */
    const char *const *envp;    /* pointer to enviroment variables */

    int channel;                /* which channel to use (stderr or stdout) */
    int argi, argii;            /* indices to traverse the arguments */
    int envi, envii;            /* indices to traverse the env vars */
    void *internal;             /* internal data used by the device */
};

/* Functions */

/* Function to initialize the console.
 * Returns zero on success.
 */
int console_init(struct console *cns);

/* Function to finalize and release the resources
 * used by the console.
 */
void console_destroy(struct console *cns);

/* Configure the arguments and environment variables for the
 * console device. The arguments are specified by `argc` and `argv`.
 * (the counter and the array of pointers). The enviroment variables
 * are specified by `envp` (where the last string pointer is NULL).
 */
void console_configure(struct console *cns, int argc,
                       const char *const *argv,
                       const char *const *envp);

/* Implementation of the read callback for the console.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t console_read_callback(struct console *cns,
                               struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the console.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void console_write_callback(struct console *cns, struct quivm *qvm,
                            uint32_t address, uint32_t v);

#endif /* !defined(__DEV_CONSOLE_H) */
