#include <stdint.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/display.h"

/* Functions */

int display_init(struct display *dpl)
{
    dpl->initialized = 0;
    dpl->bpp = 0;
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
void do_initialize(struct display *dpl, struct quivm *qvm)
{
    uint32_t bpp;

    (void)(qvm); /* UNUSED */
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
    dpl->width = (dpl->params[1] & 0xFFFF);
    dpl->height = (dpl->params[1] >> 16);

    dpl->initialized = 1;
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform the set buffer command */
static
void do_set_buffer(struct display *dpl, struct quivm *qvm)
{
    uint32_t buffer;
    buffer = dpl->params[0];
    if (!(buffer < qvm->memsize)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }
    dpl->buffer = buffer;
    dpl->stride = (dpl->params[1] & 0xFFFF);
    if (dpl->stride & 0x8000)
        dpl->stride |= 0xFFFF0000;
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform the set palette command */
static
void do_set_palette(struct display *dpl, struct quivm *qvm)
{
    uint32_t palette;
    palette = dpl->params[0];
    if (!(palette < qvm->memsize) || !((palette + 768) < qvm->memsize)) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }
    dpl->palette = palette;
    dpl->params[0] = DISPLAY_SUCCESS;
}

/* perform a block transfer operation */
static
void do_block_transfer(struct display *dpl, struct quivm *qvm)
{
    uint32_t src, src_s;
    uint32_t src_w, src_h;
    uint32_t mask, mask_w, mask_s;
    uint32_t dst, dst_s;
    uint32_t dst_w, dst_h;
    uint32_t orig_src, orig_mask;
    uint32_t quot, rem;
    uint32_t i, j, pos;
    uint32_t row, count;
    int flip;

    src = dpl->params[0];
    mask = dpl->params[1];
    src_w = (dpl->params[2] & 0xFFFF);
    src_h = (dpl->params[2] >> 16);
    mask_w = (mask) ? (src_w + 7) >> 3 : 0;
    src_s = (dpl->params[3] & 0xFFFF);
    mask_s = (dpl->params[3] >> 16);
    dst = dpl->params[4];
    dst_w = (dpl->params[5] & 0xFFFF);
    dst_h = (dpl->params[5] >> 16);
    dst_s = (dpl->params[6] & 0xFFFF);
    flip = (dpl->params[6] & 0x80000000);

    /* if there is nothing to do, terminate the operation */
    if (dst_w == 0 || dst_h == 0) {
        dpl->params[0] = DISPLAY_SUCCESS;
        return;
    }

    /* sign extend the strides */
    if (src_s & 0x8000) src_s |= 0xFFFF0000;
    if (mask_s & 0x8000) mask_s |= 0xFFFF0000;
    if (dst_s & 0x8000) dst_s |= 0xFFFF0000;

    if (flip) {
        /* flip the order of the block transfer */
        dst = - dst;
        src += (src_h - 1) * src_s;
        mask += (src_h - 1) * mask_s;
        dst += (dst_h - 1) * dst_s;
        src_s = -src_s;
        mask_s = -mask_s;
        dst_s = -dst_s;
    }

    /* check if the buffers fit in memory */
    if (check_buffer2d(dst, dst_s, dst_w, dst_h, qvm->memsize)
        || check_buffer2d(src, src_s, src_w, src_h, qvm->memsize)
        || check_buffer2d(mask, mask_s, mask_w, src_h, qvm->memsize)
        || ((src_w == 0) || (src_h == 0))) {
        dpl->params[0] = DISPLAY_ERROR;
        return;
    }

    orig_src = src;
    orig_mask = mask;
    quot = dst_w / src_w;
    rem = dst_w % src_w;
    row = 0;
    for (i = 0; i < dst_h; i++) {
        pos = dst;
        if (src_w == 1) {
            /* handle special case */
            if (!mask || (qvm->mem[mask] & 1)) {
                memset(&qvm->mem[pos], qvm->mem[src], dst_w);
            }
        } else if (mask) {
            uint32_t k, l;
            uint32_t bitmask;
            for (j = 0; j <= quot; j++) {
                bitmask = 0;
                count = (j < quot) ? src_w : rem;
                for (k = l = 0; k < count; k++) {
                    if ((k & 7) == 0)
                        bitmask = qvm->mem[mask + (l++)];
                    if (bitmask & 1)
                        qvm->mem[pos] = qvm->mem[src + k];
                    bitmask >>= 1;
                    pos++;
                }
            }
        } else {
            for (j = 0; j <= quot; j++) {
                count = (j < quot) ? src_w : rem;
                memcpy(&qvm->mem[pos], &qvm->mem[src], count);
                pos += count;
            }
        }

        dst += dst_s;
        if (++row == src_h) {
            src = orig_src;
            mask = orig_mask;
            row = 0;
        } else {
            src += src_s;
            mask += mask_s;
        }
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
        do_set_buffer(dpl, qvm);
        break;
    case DISPLAY_CMD_SETPALETTE:
        do_set_palette(dpl, qvm);
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
