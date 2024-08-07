#ifndef __DEV_DISPLAY_H
#define __DEV_DISPLAY_H

#include <stdint.h>
#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the display device */
#define IO_DISPLAY_BASE         0xFFFFFF40
#define IO_DISPLAY_END          0xFFFFFF60

/* Addresses within the display device */
#define IO_DISPLAY_COMMAND      0xFFFFFF5C
#define IO_DISPLAY_PARAM0       0xFFFFFF58
#define IO_DISPLAY_PARAM1       0xFFFFFF54
#define IO_DISPLAY_PARAM2       0xFFFFFF50
#define IO_DISPLAY_PARAM3       0xFFFFFF4C
#define IO_DISPLAY_PARAM4       0xFFFFFF48
#define IO_DISPLAY_PARAM5       0xFFFFFF44
#define IO_DISPLAY_PARAM6       0xFFFFFF40

/* Display commands */
#define DISPLAY_CMD_INIT                 1
#define DISPLAY_CMD_FRAMECOUNT           2
#define DISPLAY_CMD_SET_BUFFER           3
#define DISPLAY_CMD_SET_PALETTE          4
#define DISPLAY_CMD_SET_SOURCE           5
#define DISPLAY_CMD_SET_DESTINATION      6
#define DISPLAY_CMD_BLT                  7
#define DISPLAY_CMD_TILED_BLT            8
#define DISPLAY_CMD_MASKED_BLT           9

/* Display return values */
#define DISPLAY_SUCCESS                  0
#define DISPLAY_ERROR                   -1

/* Data structures and types */
/* A structure representing the display device */
struct display {
    int initialized;            /* device was initialized */
    uint32_t framecount;        /* counter for frames */

    uint32_t bpp;               /* bits per pixel */
    uint32_t width;             /* display width */
    uint32_t height;            /* display height */
    uint32_t buffer;            /* address of the framebuffer in memory */
    uint32_t stride;            /* the row stride for the framebuffer */
    uint32_t palette;           /* the address of the palette in memory */

    uint32_t src, src_s;        /* source buffer and stride */
    uint32_t src_w, src_h;      /* the dimensions of the source */
    uint32_t dst, dst_s;        /* destination buffer and stride */

    uint32_t command;           /* the command */
    uint32_t params[7];         /* the parameters for the command */
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
void display_write_callback(struct display *dpl, struct quivm *qvm,
                            uint32_t address, uint32_t v);

/* Auxiliary function to check if a given two dimensional buffer fits
 * the memory. The buffer is specified by the parameters:
 *  - `address` : the starting address of the buffer;
 *  - `stride` : the row stride of the buffer;
 *  - `width` : the width of the buffer;
 *  - `height` : the height of the buffer;
 * and the memory size is given by `memsize`.
 * Returns zero if the buffer is valid.
 */
int check_buffer2d(uint32_t address, uint32_t stride,
                   uint32_t width, uint32_t height,
                   uint32_t memsize);

#endif /* !defined(__DEV_DISPLAY_H) */
