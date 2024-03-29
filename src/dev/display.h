#ifndef __DEV_DISPLAY_H
#define __DEV_DISPLAY_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
#define NUM_TICKS_PER_FRAME             16

/* The I/O address space for the display device */
#define IO_DISPLAY_BASE         0xFFFFFE80
#define IO_DISPLAY_END          0xFFFFFEC0

/* Addresses within the display device */
#define IO_DISPLAY_MODE         0xFFFFFEBC
#define IO_DISPLAY_WIDTH        0xFFFFFEB8
#define IO_DISPLAY_HEIGHT       0xFFFFFEB4
#define IO_DISPLAY_BUFFER       0xFFFFFEB0
#define IO_DISPLAY_STRIDE       0xFFFFFEAC
#define IO_DISPLAY_WAITSYNC     0xFFFFFEA8
#define IO_DISPLAY_FRAMECOUNT   0xFFFFFFA4

/* Data structures and types */
/* A structure representing the display device */
struct display {
    uint32_t mode;              /* display mode (specifies bpp, use of
                                 * palette, etc.)
                                 */
    uint32_t width;             /* display width */
    uint32_t height;            /* display height */
    uint32_t buffer;            /* address of the framebuffer in memory */
    uint32_t stride;            /* the row stride for the framebuffer */
    int waitsync;               /* the vm is waiting for a sync */
    uint32_t framecount;        /* counter for frames */
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
uint32_t display_read_callback(struct display *dpl,
                               struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the display.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void display_write_callback(struct display *dpl,  struct quivm *qvm,
                            uint32_t address, uint32_t v);


#endif /* __DEV_DISPLAY_H */
