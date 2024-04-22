#ifndef __DEV_RTCLOCK_H
#define __DEV_RTCLOCK_H

#include <stdint.h>
#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the rtclock device */
#define IO_RTCLOCK_BASE         0xFFFFFF70
#define IO_RTCLOCK_END          0xFFFFFF80

/* Addresses within the rtclock device */
#define IO_RTCLOCK_DATE         0xFFFFFF7C
#define IO_RTCLOCK_TIME         0xFFFFFF78
#define IO_RTCLOCK_OTHER        0xFFFFFF74

/* Data structures and types */
/* A structure representing the external rtclock device */
struct rtclock {
    int use_utc;                /* to use UTC time instead of local time */
};

/* Functions */

/* Function to initialize the external rtclock device.
 * Returns zero on success.
 */
int rtclock_init(struct rtclock *rtc);

/* Function to finalize and release the resources
 * used by the rtclock device.
 */
void rtclock_destroy(struct rtclock *rtc);

/* Configures the rtclock device.
 * The `use_utc` is flag indicating to use UTC time instead
 * of local time.
 */
void rtclock_configure(struct rtclock *rtc, int use_utc);

/* Implementation of the read callback for the rtclock device.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t rtclock_read_callback(struct rtclock *rtc,
                               struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the rtclock device.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void rtclock_write_callback(struct rtclock *rtc, struct quivm *qvm,
                            uint32_t address, uint32_t v);

#endif /* __DEV_RTCLOCK_H */
