#include <stdint.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/keyboard.h"

/* Functions */

int keyboard_init(struct keyboard *kbd)
{
    memset(kbd, 0, sizeof(*kbd));
    return 0;
}

void keyboard_destroy(struct keyboard *kbd)
{
    (void)(kbd); /* UNUSED */
}

uint32_t keyboard_read_callback(struct keyboard *kbd,
                                struct quivm *qvm, uint32_t address)
{
    uint32_t v;

    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_KEYBOARD_SELECTOR:
        v = kbd->selector;
        break;
    case IO_KEYBOARD_STATE:
        if (kbd->selector < KEYBOARD_SIZE) {
            v = kbd->state[kbd->selector];
        } else {
            v = -1;
        }
        break;
    case IO_KEYBOARD_LED:
        v = kbd->led;
        break;
    default:
        v = -1;
        break;
    }
    return v;
}

void keyboard_write_callback(struct keyboard *kbd, struct quivm *qvm,
                             uint32_t address, uint32_t v)
{
    (void)(qvm); /* UNUSED */
    switch (address) {
    case IO_KEYBOARD_SELECTOR:
        kbd->selector = v;
        break;
    case IO_KEYBOARD_LED:
        kbd->led = v;
        break;
    }
}

void keyboard_set_bit(struct keyboard *kbd, uint32_t bit, int v)
{
    uint32_t idx;
    idx = (bit >> 5);
    bit = (1 << (bit & 31));
    if (!(idx < KEYBOARD_SIZE)) return;
    if (v) {
        kbd->state[idx] |= bit;
    } else {
        kbd->state[idx] &= ~bit;
    }
}

void keyboard_clear_mouse(struct keyboard *kbd)
{
    kbd->state[KEYBOARD_MOUSE_DX] = 0;
    kbd->state[KEYBOARD_MOUSE_DY] = 0;
}

void keyboard_move_mouse(struct keyboard *kbd, int dx, int dy)
{
    kbd->state[KEYBOARD_MOUSE_DX] += dx;
    kbd->state[KEYBOARD_MOUSE_DY] += dy;
}
