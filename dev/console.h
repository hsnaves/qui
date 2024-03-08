#ifndef __DEV_CONSOLE_H
#define __DEV_CONSOLE_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
#define IO_CONSOLE_BASE         0xFFFFFE00
#define IO_CONSOLE_MAX          0xFFFFFF00
#define CONSOLE_CELL_IN               0xFC
#define CONSOLE_CELL_OUT              0xF8

/* Data structures and types */
struct console {
    void *internal;
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
