#ifndef __DEV_KEYBOARD_H
#define __DEV_KEYBOARD_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the keyboard device */
#define IO_KEYBOARD_BASE        0xFFFFFF00
#define IO_KEYBOARD_END         0xFFFFFF20

/* Addresses within the keyboard device */
#define IO_KEYBOARD_KEY0        0xFFFFFF1C
#define IO_KEYBOARD_KEY1        0xFFFFFF18
#define IO_KEYBOARD_X           0xFFFFFF14
#define IO_KEYBOARD_Y           0xFFFFFF10
#define IO_KEYBOARD_BUTTON      0xFFFFFF0C

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
#define KEYBOARD_KEY0_SPACE     0x04000000
#define KEYBOARD_KEY0_TAB       0x08000000
#define KEYBOARD_KEY0_LSHIFT    0x10000000
#define KEYBOARD_KEY0_RSHIFT    0x20000000
#define KEYBOARD_KEY0_LALT      0x40000000
#define KEYBOARD_KEY0_RALT      0x80000000

#define KEYBOARD_KEY1_0         0x00000001
#define KEYBOARD_KEY1_1         0x00000002
#define KEYBOARD_KEY1_2         0x00000004
#define KEYBOARD_KEY1_3         0x00000008
#define KEYBOARD_KEY1_4         0x00000010
#define KEYBOARD_KEY1_5         0x00000020
#define KEYBOARD_KEY1_6         0x00000040
#define KEYBOARD_KEY1_7         0x00000080
#define KEYBOARD_KEY1_8         0x00000100
#define KEYBOARD_KEY1_9         0x00000200
#define KEYBOARD_KEY1_MINUS     0x00000400
#define KEYBOARD_KEY1_EQ        0x00000800
#define KEYBOARD_KEY1_LEFT      0x00001000
#define KEYBOARD_KEY1_RIGHT     0x00002000
#define KEYBOARD_KEY1_UP        0x00004000
#define KEYBOARD_KEY1_DOWN      0x00008000
#define KEYBOARD_KEY1_LBRACKET  0x00010000
#define KEYBOARD_KEY1_RBRACKET  0x00020000
#define KEYBOARD_KEY1_FSLASH    0x00040000
#define KEYBOARD_KEY1_BSLASH    0x00080000
#define KEYBOARD_KEY1_COMMA     0x00100000
#define KEYBOARD_KEY1_PERIOD    0x00200000
#define KEYBOARD_KEY1_SEMICOLON 0x00400000
#define KEYBOARD_KEY1_QUOTE     0x00800000
#define KEYBOARD_KEY1_ENTER     0x01000000
#define KEYBOARD_KEY1_CTRL      0x02000000
#define KEYBOARD_KEY1_BS        0x04000000
#define KEYBOARD_KEY1_DEL       0x08000000
#define KEYBOARD_KEY1_BACKQUOTE 0x10000000
#define KEYBOARD_KEY1_ESC       0x20000000

/* mouse buttons */
#define KEYBOARD_BTN_LEFT       0x00000001
#define KEYBOARD_BTN_MIDDLE     0x00000002
#define KEYBOARD_BTN_RIGHT      0x00000004


/* Data structures and types */
/* A structure representing the keyboard device */
struct keyboard {
    uint32_t key[2];            /* state of the keyboard keys */
    uint32_t x, y;              /* mouse x and y coordinates */
    uint32_t button;            /* mouse and joystick button state */
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
