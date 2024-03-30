#ifndef __DEV_DEVIO_H
#define __DEV_DEVIO_H

#include <stdint.h>

#include "vm/quivm.h"
#include "dev/console.h"
#include "dev/storage.h"
#include "dev/network.h"
#include "dev/rtclock.h"
#include "dev/display.h"
#include "dev/audio.h"
#include "dev/keyboard.h"
#include "dev/timer.h"


/* Data structures and types */
/* A structure for the VM I/O devices */
struct devio {
    struct console *cns;        /* A reference to the console device */
    struct storage *stg;        /* A reference to the storage device */
    struct network *ntw;        /* A reference to the network device */
    struct rtclock *rtc;        /* A reference to the clock device */
    struct display *dpl;        /* A reference to the display device */
    struct audio *aud;          /* A reference to the audio device */
    struct keyboard *kbd;       /* A reference to the keyboard device */
    struct timer *tmr;          /* A reference to the timer device */

    struct quivm *qvm;          /* A reference to the QUI vm */
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

/* Configures the callbacks for a given QUI vm `qvm`. */
void devio_configure(struct devio *io, struct quivm *qvm);

/* Updates the state of the I/O.
 * This function should be called periodically at each new tick.
 */
void devio_update(struct devio *io);

#endif /* __DEV_DEVIO_H */
