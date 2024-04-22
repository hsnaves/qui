#include <stdint.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/keyboard.h"

/* Functions */

int keyboard_init(struct keyboard *kbd)
{
    memset(kbd->state, 0, sizeof(kbd->state));
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
    }
}
