#ifndef __DEV_CONSOLE_H
#define __DEV_CONSOLE_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the console device */
#define IO_CONSOLE_BASE         0xFFFFFF80
#define IO_CONSOLE_END          0xFFFFFFC0

/* Addresses within the console device */
#define IO_CONSOLE_IN           0xFFFFFFBC
#define IO_CONSOLE_OUT          0xFFFFFFB8
#define IO_CONSOLE_CHANNEL      0xFFFFFFB4

/* Possible channels */
#define CONSOLE_ICHANNEL_STDIN          00
#define CONSOLE_ICHANNEL_ARGS           01
#define CONSOLE_ICHANNEL_ENV            02
#define CONSOLE_OCHANNEL_STDOUT         00
#define CONSOLE_OCHANNEL_STDERR         01


/* Data structures and types */
/* A structure representing the console device */
struct console {
    int argc;                   /* number of arguments in the input */
    char **argv;                /* arguments passed to the program */
    char **envp;                /* pointer to enviroment variables */

    int channel;                /* which channel to use (stderr or stdout) */
    int argi, argii;            /* indices to traverse the arguments */
    int envi, envii;            /* indices to traverse the env vars */
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
void console_write_callback(struct console *cns,  struct quivm *qvm,
                            uint32_t address, uint32_t v);


#endif /* __DEV_CONSOLE_H */
