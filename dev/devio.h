#ifndef __DEV_DEVIO_H
#define __DEV_DEVIO_H

#include <stdint.h>

#include "vm/quivm.h"
#include "dev/console.h"
#include "dev/storage.h"
#include "dev/network.h"
#include "dev/rtclock.h"
#include "dev/display.h"

/* Data structures and types */
/* A structure for the VM I/O devices */
struct devio {
    struct console *cns;        /* A reference to the console device */
    struct storage *stg;        /* A reference to the storage device */
    struct network *ntw;        /* A reference to the network device */
    struct rtclock *rtc;        /* A reference to the clock device */
    struct display *dpl;        /* A reference to the display device */
};

/* Functions */

/* Function to create and initialize the I/O devices.
 * Returns zero on success.
 */
int devio_init(struct devio *io);

/* Function to finalize and release the resources
 * used by the devices. The device pointer is given
 * by `dev`.
 */
void devio_destroy(struct devio *io);

/* Updates the state of the I/O.
 * This function should be called periodically at each screen
 * refresh.
 */
void devio_update(struct quivm *qvm);

/* Main implementation of the QUI read callback.
 * The parameter `address` is the address to read.
 * Returns the value read.
 */
uint32_t devio_read_callback(struct quivm *qvm, uint32_t address);

/* Main implementation of the QUI write callback.
 * The parameter `address` is the address to write, and `v` is the value.
 */
void devio_write_callback(struct quivm *qvm, uint32_t address, uint32_t v);

#endif /* __DEV_DEVIO_H */
