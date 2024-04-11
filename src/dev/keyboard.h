#ifndef __DEV_KEYBOARD_H
#define __DEV_KEYBOARD_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the keyboard device */
#define IO_KEYBOARD_BASE        0xFFFFFF00
#define IO_KEYBOARD_END         0xFFFFFF20

/* Addresses within the keyboard device */
#define IO_KEYBOARD_SELECTOR    0xFFFFFF1C
#define IO_KEYBOARD_STATE       0xFFFFFF18
#define IO_KEYBOARD_PREV_STATE  0xFFFFFF14

/* Possible values for the selector */
#define KEYBOARD_KEY0                    0
#define KEYBOARD_KEY1                    1
#define KEYBOARD_KEY2                    2
#define KEYBOARD_X                       3
#define KEYBOARD_Y                       4
#define KEYBOARD_BUTTON                  5
#define KEYBOARD_SIZE                    6

/* Keyboard keys */
#define KEYBOARD_KEY0_A         0x00000001
#define KEYBOARD_KEY0_B         0x00000002
#define KEYBOARD_KEY0_C         0x00000004
#define KEYBOARD_KEY0_D         0x00000008
#define KEYBOARD_KEY0_E         0x00000010
#define KEYBOARD_KEY0_F         0x00000020
#define KEYBOARD_KEY0_G         0x00000040
#define KEYBOARD_KEY0_H         0x00000080
#define KEYBOARD_KEY0_I         0x00000100
#define KEYBOARD_KEY0_J         0x00000200
#define KEYBOARD_KEY0_K         0x00000400
#define KEYBOARD_KEY0_L         0x00000800
#define KEYBOARD_KEY0_M         0x00001000
#define KEYBOARD_KEY0_N         0x00002000
#define KEYBOARD_KEY0_O         0x00004000
#define KEYBOARD_KEY0_P         0x00008000
#define KEYBOARD_KEY0_Q         0x00010000
#define KEYBOARD_KEY0_R         0x00020000
#define KEYBOARD_KEY0_S         0x00040000
#define KEYBOARD_KEY0_T         0x00080000
#define KEYBOARD_KEY0_U         0x00100000
#define KEYBOARD_KEY0_V         0x00200000
#define KEYBOARD_KEY0_W         0x00400000
#define KEYBOARD_KEY0_X         0x00800000
#define KEYBOARD_KEY0_Y         0x01000000
#define KEYBOARD_KEY0_Z         0x02000000
#define KEYBOARD_KEY0_1         0x04000000
#define KEYBOARD_KEY0_2         0x08000000
#define KEYBOARD_KEY0_3         0x10000000
#define KEYBOARD_KEY0_4         0x20000000
#define KEYBOARD_KEY0_5         0x40000000
#define KEYBOARD_KEY0_6         0x80000000

#define KEYBOARD_KEY1_7         0x00000001
#define KEYBOARD_KEY1_8         0x00000002
#define KEYBOARD_KEY1_9         0x00000004
#define KEYBOARD_KEY1_0         0x00000008
#define KEYBOARD_KEY1_RETURN    0x00000010
#define KEYBOARD_KEY1_ESC       0x00000020
#define KEYBOARD_KEY1_BS        0x00000040
#define KEYBOARD_KEY1_TAB       0x00000080
#define KEYBOARD_KEY1_SPACE     0x00000100
#define KEYBOARD_KEY1_MINUS     0x00000200
#define KEYBOARD_KEY1_EQ        0x00000400
#define KEYBOARD_KEY1_LBRACKET  0x00000800
#define KEYBOARD_KEY1_RBRACKET  0x00001000
#define KEYBOARD_KEY1_BSLASH    0x00002000
#define KEYBOARD_KEY1_NONUSHASH 0x00004000
#define KEYBOARD_KEY1_SEMICOLON 0x00008000
#define KEYBOARD_KEY1_QUOTE     0x00010000
#define KEYBOARD_KEY1_BACKQUOTE 0x00020000
#define KEYBOARD_KEY1_COMMA     0x00040000
#define KEYBOARD_KEY1_PERIOD    0x00080000
#define KEYBOARD_KEY1_SLASH     0x00100000
#define KEYBOARD_KEY1_CAPSLOCK  0x00200000
#define KEYBOARD_KEY1_F1        0x00400000
#define KEYBOARD_KEY1_F2        0x00800000
#define KEYBOARD_KEY1_F3        0x01000000
#define KEYBOARD_KEY1_F4        0x02000000
#define KEYBOARD_KEY1_F5        0x04000000
#define KEYBOARD_KEY1_F6        0x08000000
#define KEYBOARD_KEY1_F7        0x10000000
#define KEYBOARD_KEY1_F8        0x20000000
#define KEYBOARD_KEY1_F9        0x40000000
#define KEYBOARD_KEY1_F10       0x80000000

#define KEYBOARD_KEY2_F11       0x00000001
#define KEYBOARD_KEY2_F12       0x00000002
#define KEYBOARD_KEY2_PRINTSCR  0x00000004
#define KEYBOARD_KEY2_SCROLL    0x00000008
#define KEYBOARD_KEY2_PAUSE     0x00000010
#define KEYBOARD_KEY2_INSERT    0x00000020
#define KEYBOARD_KEY2_HOME      0x00000040
#define KEYBOARD_KEY2_PGUP      0x00000080
#define KEYBOARD_KEY2_DEL       0x00000100
#define KEYBOARD_KEY2_END       0x00000200
#define KEYBOARD_KEY2_PGDOWN    0x00000400
#define KEYBOARD_KEY2_RIGHT     0x00000800
#define KEYBOARD_KEY2_LEFT      0x00001000
#define KEYBOARD_KEY2_DOWN      0x00002000
#define KEYBOARD_KEY2_UP        0x00004000
#define KEYBOARD_KEY2_LCTRL     0x00008000
#define KEYBOARD_KEY2_LSHIFT    0x00010000
#define KEYBOARD_KEY2_LALT      0x00020000
#define KEYBOARD_KEY2_LGUI      0x00040000
#define KEYBOARD_KEY2_RCTRL     0x00080000
#define KEYBOARD_KEY2_RSHIFT    0x00100000
#define KEYBOARD_KEY2_RALT      0x00200000
#define KEYBOARD_KEY2_RGUI      0x00400000
#define KEYBOARD_KEY2_QUIT      0x80000000

/* mouse buttons */
#define KEYBOARD_BTN_LEFT       0x00000001
#define KEYBOARD_BTN_MIDDLE     0x00000002
#define KEYBOARD_BTN_RIGHT      0x00000004


/* Data structures and types */
/* A structure representing the keyboard device */
struct keyboard {
    uint32_t state[KEYBOARD_SIZE]; /* state of the keyboard keys */
    uint32_t prev_state[KEYBOARD_SIZE]; /* previous state of keys */
    uint32_t selector;          /* to select which state to read */
};

/* Functions */

/* Function to initialize the keyboard.
 * Returns zero on success.
 */
int keyboard_init(struct keyboard *kbd);

/* Function to finalize and release the resources
 * used by the keyboard.
 */
void keyboard_destroy(struct keyboard *kbd);

/* Refreshes the keyboard data.
 * This is to take the snapshot of the keyboard state.
 */
void keyboard_update(struct keyboard *kbd, struct quivm *qvm);

/* Implementation of the read callback for the keyboard.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t keyboard_read_callback(struct keyboard *kbd,
                                struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the keyboard.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void keyboard_write_callback(struct keyboard *kbd,  struct quivm *qvm,
                             uint32_t address, uint32_t v);


#endif /* __DEV_KEYBOARD_H */
