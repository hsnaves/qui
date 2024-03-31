#ifndef __DEV_DISPLAY_H
#define __DEV_DISPLAY_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the display device */
#define IO_DISPLAY_BASE         0xFFFFFE80
#define IO_DISPLAY_END          0xFFFFFEC0

/* Addresses within the display device */
#define IO_DISPLAY_COMMAND      0xFFFFFEBC
#define IO_DISPLAY_PARAM0       0xFFFFFEB8
#define IO_DISPLAY_PARAM1       0xFFFFFEB4
#define IO_DISPLAY_PARAM2       0xFFFFFEB0
#define IO_DISPLAY_PARAM3       0xFFFFFEAC
#define IO_DISPLAY_PARAM4       0xFFFFFEA8
#define IO_DISPLAY_PARAM5       0xFFFFFEA4
#define IO_DISPLAY_PARAM6       0xFFFFFEA0
#define IO_DISPLAY_PARAM7       0xFFFFFE9C

/* Display commands */
#define DISPLAY_CMD_INIT                 1
#define DISPLAY_CMD_SETBUF               2
#define DISPLAY_CMD_WAITSYNC             3
#define DISPLAY_CMD_FRAMECOUNT           4
#define DISPLAY_CMD_BLT                  5

/* Display return values */
#define DISPLAY_SUCCESS                  0
#define DISPLAY_ERROR                   -1

/* Data structures and types */
/* A structure representing the display device */
struct display {
    int initialized;            /* device was initialized */
    int waitsync;               /* the vm is waiting for a sync */
    uint32_t mode;              /* display mode */
    uint32_t width;             /* display width */
    uint32_t height;            /* display height */
    uint32_t buffer;            /* address of the framebuffer in memory */
    uint32_t stride;            /* the row stride for the framebuffer */
    uint32_t framecount;        /* counter for frames */

    uint32_t command;           /* the command */
    uint32_t params[8];         /* the parameters for the command */
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
