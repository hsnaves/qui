#ifndef __DEV_STORAGE_H
#define __DEV_STORAGE_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the storage device */
#define IO_STORAGE_BASE         0xFFFFFF40
#define IO_STORAGE_END          0xFFFFFF80

/* Addresses within the storage device */
#define IO_STORAGE_NAME         0xFFFFFF7C
#define IO_STORAGE_DATA         0xFFFFFF78
#define IO_STORAGE_LEN          0xFFFFFF74
#define IO_STORAGE_OP           0xFFFFFF70

/* The possible operations */
#define STORAGE_OP_READ                  1
#define STORAGE_OP_WRITE                 2

/* Data structures and types */
/* A structure representing the external storage device */
struct storage {
    uint32_t name;              /* address of the name */
    uint32_t data;              /* addres of the data */
    uint32_t len;               /* length of the data */
    uint32_t op;                /* the operation */
    int disable_write;          /* to disable write operations */
};

/* Functions */

/* Function to initialize the external storage device.
 * Returns zero on success.
 */
int storage_init(struct storage *stg);

/* Function to finalize and release the resources
 * used by the storage device.
 */
void storage_destroy(struct storage *stg);

/* Implementation of the read callback for the storage device.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t storage_read_callback(const struct storage *stg,
                               const struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the storage device.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void storage_write_callback(struct storage *stg,  struct quivm *qvm,
                            uint32_t address, uint32_t v);

#endif /* __DEV_STORAGE_H */
