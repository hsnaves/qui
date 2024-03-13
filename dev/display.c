#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/display.h"

/* Functions */

int display_init(struct display *dpl)
{
    dpl->width = 0;
    dpl->height = 0;
    dpl->buffer = NULL;
    dpl->initialized = 0;
    return 0;
}

void display_destroy(struct display *dpl)
{
    if (dpl->buffer) free(dpl->buffer);
    dpl->buffer = NULL;
}

void display_update(struct display *dpl, struct quivm *qvm)
{
    (void)(dpl); /* UNUSED */
    (void)(qvm); /* UNUSED */
}

uint32_t display_read_callback(const struct display *dpl,
                               const struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_DISPLAY_WIDTH:
        v = dpl->width;
        break;
    case IO_DISPLAY_HEIGHT:
        v = dpl->height;
        break;
    default:
        v = -1;
        break;
    }
    return v;
}

void display_write_callback(struct display *dpl,  struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    switch (address) {
    case IO_DISPLAY_WIDTH:
        if (!dpl->initialized) {
            dpl->width = v;
        }
        break;
    case IO_DISPLAY_HEIGHT:
        if (!dpl->initialized) {
            dpl->height = v;
            dpl->buffer = (uint8_t *) malloc(dpl->width * dpl->height);
            if (!dpl->buffer) {
                fprintf(stderr, "display: write_callback: "
                        "memory exhausted\n");
                qvm->status |= STS_TERMINATED;
                qvm->termvalue = 1;
            }
        }
        dpl->initialized = 1;
        break;
    case IO_DISPLAY_BUFFER:
        if (dpl->initialized) {
            uint32_t i, size, address;

            address = v;
            size = dpl->width * dpl->height;

            /* TODO: speed-up this copy (also in storage device) */
            for (i = 0; i < size; i++) {
                if (!(address < qvm->memsize)) break;
                dpl->buffer[i] = quivm_read_byte(qvm, address++);
            }
        }
        break;
    }
}
