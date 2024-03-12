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
#define IO_CONSOLE_USE_ERR      0xFFFFFFB4

/* Data structures and types */
/* A structure representing the console device */
struct console {
    int use_err;                /* To use the standar error as output */
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
uint32_t console_read_callback(const struct console *cns,
                               const struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the console.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void console_write_callback(struct console *cns,  struct quivm *qvm,
                            uint32_t address, uint32_t v);


#endif /* __DEV_CONSOLE_H */
