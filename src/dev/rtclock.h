#ifndef __DEV_RTCLOCK_H
#define __DEV_RTCLOCK_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the rtclock device */
#define IO_RTCLOCK_BASE         0xFFFFFEC0
#define IO_RTCLOCK_END          0xFFFFFF00

/* Addresses within the rtclock device */
#define IO_RTCLOCK_YEAR         0xFFFFFEFC
#define IO_RTCLOCK_MONTH        0xFFFFFEF8
#define IO_RTCLOCK_MDAY         0xFFFFFEF4
#define IO_RTCLOCK_HOUR         0xFFFFFEF0
#define IO_RTCLOCK_MIN          0xFFFFFEEC
#define IO_RTCLOCK_SEC          0xFFFFFEE8
#define IO_RTCLOCK_WDAY         0xFFFFFEE4
#define IO_RTCLOCK_YDAY         0xFFFFFEE0
#define IO_RTCLOCK_ISDST        0xFFFFFEDC

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
void rtclock_write_callback(struct rtclock *rtc,  struct quivm *qvm,
                            uint32_t address, uint32_t v);

#endif /* __DEV_RTCLOCK_H */
