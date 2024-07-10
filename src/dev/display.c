#include <stdint.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/display.h"

/* Functions */

int display_init(struct display *dpl)
{
    memset(dpl, 0, sizeof(struct display));
    return 0;
}

void display_destroy(struct display *dpl)
{
    (void)(dpl); /* UNUSED */
}

void display_update(struct display *dpl, struct quivm *qvm)
{
    (void)(qvm); /* UNUSED */
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
void do_initialize(struct display *dpl)
{
    uint32_t bpp;

    if (dpl->initialized) {
        /* already initialized */
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    /* check if bpp is valid */
    bpp = dpl->params[0];
    if (bpp != 8 && bpp != 24) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    dpl->bpp = bpp;
    dpl->width = dpl->params[1];
    dpl->height = dpl->params[2];

    dpl->initialized = 1;
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform the set buffer command */
static
void do_set_buffer(struct display *dpl)
{
    uint32_t buffer;
    buffer = dpl->params[0];
    if (!(buffer < MEMORY_SIZE)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }
    dpl->buffer = buffer;
    dpl->stride = dpl->params[1];
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform the set palette command */
static
void do_set_palette(struct display *dpl)
{
    uint32_t palette;
    palette = dpl->params[0];
    if (!(palette < MEMORY_SIZE) || !((palette + 768) < MEMORY_SIZE)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }
    dpl->palette = palette;
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* sets the source buffer */
static
void do_set_source(struct display *dpl)
{
    uint32_t src, src_s;
    uint32_t src_w, src_h;

    src = dpl->params[0];
    src_s = dpl->params[1];
    src_w = dpl->params[2];
    src_h = dpl->params[3];

    /* check if the buffer fits in memory */
    if (check_buffer2d(src, src_s, src_w, src_h, MEMORY_SIZE)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    dpl->src = src;
    dpl->src_s = src_s;
    dpl->src_w = src_w;
    dpl->src_h = src_h;
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* sets the destination buffer */
static
void do_set_destination(struct display *dpl)
{
    dpl->dst = dpl->params[0];
    dpl->dst_s = dpl->params[1];
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform a block transfer operation */
static
void do_block_transfer(struct display *dpl, struct quivm *qvm)
{
    uint32_t src, src_s;
    uint32_t dst, dst_s;
    uint32_t i;
    int flip;

    flip = dpl->params[0];
    src = dpl->src;
    src_s = dpl->src_s;
    dst = dpl->dst;
    dst_s = dpl->dst_s;

    /* check if the buffer fits in memory */
    if (check_buffer2d(dst, dst_s, dpl->src_w, dpl->src_h, MEMORY_SIZE)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    if (flip) {
        /* flip the order of the block transfer */
        src += (dpl->src_h - 1) * src_s;
        dst += (dpl->src_h - 1) * dst_s;
        src_s = -src_s;
        dst_s = -dst_s;
    }

    for (i = 0; i < dpl->src_h; i++) {
        memcpy(&qvm->mem[dst], &qvm->mem[src], dpl->src_w);
        dst += dst_s;
        src += src_s;
    }

    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform a tiled block transfer operation */
static
void do_tiled_block_transfer(struct display *dpl, struct quivm *qvm)
{
    uint32_t src, src_s;
    uint32_t dst, dst_s;
    uint32_t dst_w, dst_h;
    uint32_t orig_src;
    uint32_t i, row;
    int flip;

    flip = dpl->params[0];
    dst_w = dpl->params[1];
    dst_h = dpl->params[2];
    src = dpl->src;
    src_s = dpl->src_s;
    dst = dpl->dst;
    dst_s = dpl->dst_s;

    /* check if the buffer fits in memory */
    if (check_buffer2d(dst, dst_s, dst_w, dst_h, MEMORY_SIZE)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    if (flip) {
        /* flip the order of the block transfer */
        src += (dpl->src_h - 1) * src_s;
        dst += (dst_h - 1) * dst_s;
        src_s = -src_s;
        dst_s = -dst_s;
    }

    orig_src = src;
    row = 0;

    if (dpl->src_w == 1) {
        /* handle special case */
        for (i = 0; i < dst_h; i++) {
            memset(&qvm->mem[dst], qvm->mem[src], dst_w);
            dst += dst_s;
            if (++row == dpl->src_h) {
                src = orig_src;
                row = 0;
            } else {
                src += src_s;
            }
        }
    } else {
        uint32_t quot, rem;
        uint32_t j, pos, count;

        quot = dst_w / dpl->src_w;
        rem = dst_w % dpl->src_w;
        for (i = 0; i < dst_h; i++) {
            pos = dst;
            for (j = 0; j <= quot; j++) {
                count = (j < quot) ? dpl->src_w : rem;
                memcpy(&qvm->mem[pos], &qvm->mem[src], count);
                pos += count;
            }

            dst += dst_s;
            if (++row == dpl->src_h) {
                src = orig_src;
                row = 0;
            } else {
                src += src_s;
            }
        }
    }

    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform a masked block transfer operation */
static
void do_masked_block_transfer(struct display *dpl, struct quivm *qvm)
{
    uint32_t src, src_s;
    uint32_t mask, mask_s, mask_w;
    uint32_t dst, dst_s;
    uint32_t i;
    int flip;

    flip = dpl->params[0];
    mask = dpl->params[1];
    mask_s = dpl->params[2];
    mask_w = (dpl->src_w + 7) >> 3;
    src = dpl->src;
    src_s = dpl->src_s;
    dst = dpl->dst;
    dst_s = dpl->dst_s;

    /* check if the buffers fit in memory */
    if (check_buffer2d(mask, mask_s, mask_w, dpl->src_h, MEMORY_SIZE)
        || check_buffer2d(dst, dst_s, dpl->src_w, dpl->src_h, MEMORY_SIZE)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    if (flip) {
        /* flip the order of the block transfer */
        src += (dpl->src_h - 1) * src_s;
        mask += (dpl->src_h - 1) * mask_s;
        dst += (dpl->src_h - 1) * dst_s;
        src_s = -src_s;
        mask_s = -mask_s;
        dst_s = -dst_s;
    }

    for (i = 0; i < dpl->src_h; i++) {
        uint32_t j, k;
        uint32_t pos, bitmask;

        pos = dst;
        bitmask = 0;
        k = 0;
        for (j = 0; j < dpl->src_w; j++) {
            if ((j & 7) == 0)
                bitmask = qvm->mem[mask + (k++)];
            if (bitmask & 1)
                qvm->mem[pos] = qvm->mem[src + j];
            bitmask >>= 1;
            pos++;
        }
        dst += dst_s;
        src += src_s;
        mask += mask_s;
    }

    dpl->params[0] = DISPLAY_SUCCESS;
}

/* runs a command in dpl->command */
static
void do_command(struct display *dpl, struct quivm *qvm)
{
    switch (dpl->command) {
    case DISPLAY_CMD_INIT:
        do_initialize(dpl);
        break;
    case DISPLAY_CMD_FRAMECOUNT:
        dpl->params[1] = dpl->framecount;
        dpl->params[0] = DISPLAY_SUCCESS;
        break;
    case DISPLAY_CMD_SET_BUFFER:
        do_set_buffer(dpl);
        break;
    case DISPLAY_CMD_SET_PALETTE:
        do_set_palette(dpl);
        break;
    case DISPLAY_CMD_SET_SOURCE:
        do_set_source(dpl);
        break;
    case DISPLAY_CMD_SET_DESTINATION:
        do_set_destination(dpl);
        break;
    case DISPLAY_CMD_BLT:
        do_block_transfer(dpl, qvm);
        break;
    case DISPLAY_CMD_TILED_BLT:
        do_tiled_block_transfer(dpl, qvm);
        break;
    case DISPLAY_CMD_MASKED_BLT:
        do_masked_block_transfer(dpl, qvm);
        break;
    default:
        dpl->params[0] = DISPLAY_ERROR;
        break;
    }
}

void display_write_callback(struct display *dpl, struct quivm *qvm,
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

int check_buffer2d(uint32_t address, uint32_t stride,
                   uint32_t width, uint32_t height,
                   uint32_t memsize)
{
    uint32_t remaining;
    uint64_t diff;

    if (!height || !width) return 0;
    if (!(address <= memsize)) return 1;
    remaining = memsize - address;
    if (!(width <= remaining)) return 1;

    diff = (uint64_t) (height - 1);
    if (stride & 0x80000000) {
        stride = -stride;
        remaining = address;
        width = 0;
    }
    diff *= (uint64_t) stride;
    diff += (uint64_t) width;
    if (!(diff <= ((uint64_t) remaining)))
        return 1;

    return 0;
}
