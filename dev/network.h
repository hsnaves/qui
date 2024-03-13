#ifndef __DEV_NETWORK_H
#define __DEV_NETWORK_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the network device */
#define IO_NETWORK_BASE         0xFFFFFF00
#define IO_NETWORK_END          0xFFFFFF40

/* Addresses within the network device */

/* Data structures and types */
/* A structure representing the external network device */
struct network {
    void *unused;               /* not used (so struct is not empty) */
};

/* Functions */

/* Function to initialize the external network device.
 * Returns zero on success.
 */
int network_init(struct network *ntw);

/* Function to finalize and release the resources
 * used by the network device.
 */
void network_destroy(struct network *ntw);

/* Implementation of the read callback for the network device.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t network_read_callback(struct network *ntw,
                               struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the network device.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void network_write_callback(struct network *ntw,  struct quivm *qvm,
                            uint32_t address, uint32_t v);

#endif /* __DEV_NETWORK_H */
