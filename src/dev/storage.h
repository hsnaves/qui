#ifndef __DEV_STORAGE_H
#define __DEV_STORAGE_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the storage device */
#define IO_STORAGE_BASE         0xFFFFFFA0
#define IO_STORAGE_END          0xFFFFFFC0

/* Addresses within the storage device */
#define IO_STORAGE_NAME         0xFFFFFFBC
#define IO_STORAGE_DATA         0xFFFFFFB8
#define IO_STORAGE_LEN          0xFFFFFFB4
#define IO_STORAGE_OFFSET       0xFFFFFFB0
#define IO_STORAGE_OP           0xFFFFFFAC

/* The possible operations */
#define STORAGE_OP_READ                  1
#define STORAGE_OP_WRITE                 2

/* Data structures and types */
/* A structure representing the external storage device */
struct storage {
    uint32_t name;              /* address of the name (nul-terminated) */
    uint32_t data;              /* addres of the data */
    uint32_t len;               /* length of the data
                                 * at the end of the operation, it contains
                                 * the number of bytes read/written.
                                 */
    uint32_t offset;            /* the offset to seek, for read operations.
                                 * if writing, and offset is non-zero
                                 * then file is appended instead of truncated.
                                 */
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
uint32_t storage_read_callback(struct storage *stg,
                               struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the storage device.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void storage_write_callback(struct storage *stg,  struct quivm *qvm,
                            uint32_t address, uint32_t v);

#endif /* __DEV_STORAGE_H */
