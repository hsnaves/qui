#include <stdint.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/display.h"

/* Functions */

int display_init(struct display *dpl)
{
    dpl->initialized = 0;
    dpl->width = 0;
    dpl->height = 0;
    dpl->buffer = 0;
    dpl->stride = 0;
    dpl->palette = 0;
    dpl->framecount = 0;
    dpl->command = 0;
    memset(dpl->params, 0, sizeof(dpl->params));
    return 0;
}

void display_destroy(struct display *dpl)
{
    (void)(dpl); /* UNUSED */
}

void display_update(struct display *dpl, struct quivm *qvm)
{
    /* resume the VM */
    qvm->status &= ~STS_HALTED;
    dpl->framecount++;
}

uint32_t display_read_callback(struct display *dpl,
                               struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_DISPLAY_COMMAND:
        v = dpl->command;
        break;
    default:
        if (((IO_DISPLAY_PARAM0 - address) % 4 == 0)
            && (address >= IO_DISPLAY_PARAM6)
            && (address <= IO_DISPLAY_PARAM0)) {

            v = dpl->params[(IO_DISPLAY_PARAM0 - address) >> 2];
        } else {
            v = -1;
        }
        break;
    }
    return v;
}

/* perform the initialization of the display */
static
void do_initialize(struct display *dpl, struct quivm *qvm)
{
    (void)(qvm); /* UNUSED */
    if (dpl->initialized) {
        /* already initialized */
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    dpl->mode = dpl->params[0];
    dpl->width = dpl->params[1];
    dpl->height = dpl->params[2];

    switch (dpl->mode & 0xFF) {
    case MODE_1BPP:
        if ((dpl->width % 8) != 0)
            goto invalid_mode;
        break;
    case MODE_4BPP:
        if ((dpl->width % 2) != 0)
            goto invalid_mode;
        break;
    case MODE_8BPP:
    case MODE_24BPP:
        break;
    default:
    invalid_mode:
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    dpl->initialized = 1;
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform a block transfer operation */
static
void do_block_transfer(struct display *dpl, struct quivm *qvm)
{
    uint32_t src, dst;
    uint32_t src_stride, dst_stride;
    uint32_t width, height;
    uint32_t i, length;

    src = dpl->params[0];
    src_stride = dpl->params[1];
    width = dpl->params[2];
    height = dpl->params[3];
    dst = dpl->params[4];
    dst_stride = dpl->params[5];

    for (i = 0; i < height; i++) {
        if (!(dst < qvm->memsize) || !(src < qvm->memsize))
            break;

        length = width;
        if (length > (qvm->memsize - dst))
            length = qvm->memsize - dst;
        if (length > (qvm->memsize - src))
            length = qvm->memsize - src;

        memcpy(&qvm->mem[dst], &qvm->mem[src], length);
        dst += dst_stride;
        src += src_stride;
    }
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* runs a command in dpl->command */
static
void do_command(struct display *dpl, struct quivm *qvm)
{
    switch (dpl->command) {
    case DISPLAY_CMD_INIT:
        do_initialize(dpl, qvm);
        break;
    case DISPLAY_CMD_FRAMECOUNT:
        dpl->params[1] = dpl->framecount;
        dpl->params[0] = DISPLAY_SUCCESS;
        break;
    case DISPLAY_CMD_SETBUF:
        dpl->buffer = dpl->params[0];
        dpl->stride = dpl->params[1];
        dpl->params[0] = DISPLAY_SUCCESS;
        break;
    case DISPLAY_CMD_SETPALETTE:
        dpl->palette = dpl->params[0];
        dpl->params[0] = DISPLAY_SUCCESS;
        break;
    case DISPLAY_CMD_BLT:
        do_block_transfer(dpl, qvm);
        break;
    default:
        dpl->params[0] = DISPLAY_ERROR;
        break;
    }
}

void display_write_callback(struct display *dpl,  struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    switch (address) {
    case IO_DISPLAY_COMMAND:
        dpl->command = v;
        do_command(dpl, qvm);
        break;
    default:
        if (((IO_DISPLAY_PARAM0 - address) % 4 == 0)
            && (address >= IO_DISPLAY_PARAM6)
            && (address <= IO_DISPLAY_PARAM0)) {

            dpl->params[(IO_DISPLAY_PARAM0 - address) >> 2] = v;
        }
        break;
    }
}
