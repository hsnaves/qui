#include <stdint.h>

#include "vm/quivm.h"
#include "dev/keyboard.h"

/* Functions */

int keyboard_init(struct keyboard *kbd)
{
    kbd->key[0] = 0;
    kbd->key[1] = 0;
    kbd->x = 0;
    kbd->y = 0;
    kbd->button = 0;
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
    case IO_KEYBOARD_KEY0:
        v = kbd->key[0];
        break;
    case IO_KEYBOARD_KEY1:
        v = kbd->key[1];
        break;
    case IO_KEYBOARD_X:
        v = kbd->x;
        break;
    case IO_KEYBOARD_Y:
        v = kbd->y;
        break;
    case IO_KEYBOARD_BUTTON:
        v = kbd->button;
        break;
    default:
        v = -1;
        break;
    }
    return v;
}

void keyboard_write_callback(struct keyboard *kbd,  struct quivm *qvm,
                             uint32_t address, uint32_t v)
{
    (void)(kbd); /* UNUSED */
    (void)(qvm); /* UNUSED */
    (void)(address); /* UNUSED */
    (void)(v); /* UNUSED */
}
