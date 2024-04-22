#ifndef __DEV_NETWORK_H
#define __DEV_NETWORK_H

#include <stdint.h>
#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the network device */
#define IO_NETWORK_BASE         0xFFFFFF80
#define IO_NETWORK_END          0xFFFFFFA0

/* Addresses within the network device */
#define IO_NETWORK_DATA         0xFFFFFF9C
#define IO_NETWORK_LEN          0xFFFFFF98
#define IO_NETWORK_OP           0xFFFFFF94

/* The possible operations */
#define NETWORK_OP_RECEIVE               1
#define NETWORK_OP_SEND                  2

/* Data structures and types */
/* A structure representing the external network device */
struct network {
    uint32_t data;              /* address of the data buffer */
    uint32_t len;               /* the length of the data buffer */
    uint32_t op;                /* the operation */
    const char *bind_address;   /* the address to bind the port */
    const char *target_address; /* the address where to send messages */
    int port;                   /* the port used for the udp sockets */
    int initialized;            /* device was initialized */
    void *internal;             /* internal data used by the device */
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

/* Configures the network device.
 * The `bind_address` and `port` specify which UDP address and port
 * to bind and listen for connections. The `target_address` specifies
 * which address to connect (including the port specified by `port`).
 */
void network_configure(struct network *ntw, const char *bind_address,
                       const char *target_address, int port);

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
void network_write_callback(struct network *ntw, struct quivm *qvm,
                            uint32_t address, uint32_t v);

#endif /* __DEV_NETWORK_H */
