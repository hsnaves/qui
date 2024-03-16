#include <stdint.h>

#include "vm/quivm.h"
#include "dev/display.h"

/* Functions */

int display_init(struct display *dpl)
{
    dpl->width = 0;
    dpl->height = 0;
    dpl->buffer = 0;
    dpl->stride = 0;
    dpl->waitsync = 0;
    dpl->initialized = 0;
    return 0;
}

void display_destroy(struct display *dpl)
{
    (void)(dpl); /* UNUSED */
}

void display_update(struct display *dpl, struct quivm *qvm)
{
    if (dpl->waitsync) {
        /* resume the VM */
        qvm->status &= ~STS_HALTED;
    }
    dpl->waitsync = 0;
}

uint32_t display_read_callback(struct display *dpl,
                               struct quivm *qvm, uint32_t address)
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
    case IO_DISPLAY_BUFFER:
        v = dpl->buffer;
        break;
    case IO_DISPLAY_STRIDE:
        v = dpl->stride;
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
        }
        break;
    case IO_DISPLAY_BUFFER:
        dpl->buffer = v;
        break;
    case IO_DISPLAY_STRIDE:
        dpl->stride = v;
        break;
    case IO_DISPLAY_WAITSYNC:
        dpl->initialized = 1;
        dpl->waitsync = !!v;

        /* halts the VM */
        if (dpl->waitsync) qvm->status |= STS_HALTED;
        break;
    }
}
