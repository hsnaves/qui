#ifndef __DEV_TIMER_H
#define __DEV_TIMER_H

#include <stdint.h>
#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the timer device */
#define IO_TIMER_BASE           0xFFFFFF60
#define IO_TIMER_END            0xFFFFFF70

/* Addresses within the timer device */
#define IO_TIMER_ONINTERRUPT    0xFFFFFF6C
#define IO_TIMER_ENABLED        0xFFFFFF68

/* Data structures and types */
/* A structure representing the timer device */
struct timer {
    uint32_t oninterrupt;       /* an address to be called in case
                                 * of interrupts (timer must be enabled)
                                 */
    int enabled;                /* if the timer is enabled */
};

/* Functions */

/* Function to initialize the external timer device.
 * Returns zero on success.
 */
int timer_init(struct timer *tmr);

/* Function to finalize and release the resources
 * used by the timer device.
 */
void timer_destroy(struct timer *tmr);

/* Updates the state of the timer.
 * This function should be called on every tick.
 */
void timer_update(struct timer *tmr, struct quivm *qvm);

/* Implementation of the read callback for the timer device.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t timer_read_callback(struct timer *tmr,
                               struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the timer device.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void timer_write_callback(struct timer *tmr, struct quivm *qvm,
                          uint32_t address, uint32_t v);

#endif
