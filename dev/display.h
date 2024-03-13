#ifndef __DEV_DISPLAY_H
#define __DEV_DISPLAY_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the display device */
#define IO_DISPLAY_BASE         0xFFFFFF00
#define IO_DISPLAY_END          0xFFFFFF40

/* Addresses within the display device */
#define IO_DISPLAY_WIDTH        0xFFFFFF3C
#define IO_DISPLAY_HEIGHT       0xFFFFFF38
#define IO_DISPLAY_BUFFER       0xFFFFFF34

/* Data structures and types */
/* A structure representing the display device */
struct display {
    uint32_t width;             /* display width */
    uint32_t height;            /* display height */
    uint8_t *buffer;            /* frame buffer */
    int initialized;            /* device was initialized */
};

/* Functions */

/* Function to initialize the display.
 * Returns zero on success.
 */
int display_init(struct display *dpl);

/* Function to finalize and release the resources
 * used by the display.
 */
void display_destroy(struct display *dpl);

/* Updates the state of the display.
 * A reference to the QUI vm is given by `qvm`.
 * This function should be called periodically at each screen
 * refresh.
 */
void display_update(struct display *dpl, struct quivm *qvm);

/* Implementation of the read callback for the display.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t display_read_callback(const struct display *dpl,
                               const struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the display.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void display_write_callback(struct display *dpl,  struct quivm *qvm,
                            uint32_t address, uint32_t v);


#endif /* __DEV_DISPLAY_H */
