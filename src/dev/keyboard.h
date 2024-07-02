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
#define IO_KEYBOARD_LED         0xFFFFFF14

/* Possible values for the selector */
#define KEYBOARD_KEY_BASE                0
#define KEYBOARD_MOUSE_DX                5
#define KEYBOARD_MOUSE_DY                6
#define KEYBOARD_SIZE                    7

/* Keyboard keys */
#define KEYBOARD_KEY_A                   0
#define KEYBOARD_KEY_B                   1
#define KEYBOARD_KEY_C                   2
#define KEYBOARD_KEY_D                   3
#define KEYBOARD_KEY_E                   4
#define KEYBOARD_KEY_F                   5
#define KEYBOARD_KEY_G                   6
#define KEYBOARD_KEY_H                   7
#define KEYBOARD_KEY_I                   8
#define KEYBOARD_KEY_J                   9
#define KEYBOARD_KEY_K                  10
#define KEYBOARD_KEY_L                  11
#define KEYBOARD_KEY_M                  12
#define KEYBOARD_KEY_N                  13
#define KEYBOARD_KEY_O                  14
#define KEYBOARD_KEY_P                  15
#define KEYBOARD_KEY_Q                  16
#define KEYBOARD_KEY_R                  17
#define KEYBOARD_KEY_S                  18
#define KEYBOARD_KEY_T                  19
#define KEYBOARD_KEY_U                  20
#define KEYBOARD_KEY_V                  21
#define KEYBOARD_KEY_W                  22
#define KEYBOARD_KEY_X                  23
#define KEYBOARD_KEY_Y                  24
#define KEYBOARD_KEY_Z                  25
#define KEYBOARD_KEY_1                  26
#define KEYBOARD_KEY_2                  27
#define KEYBOARD_KEY_3                  28
#define KEYBOARD_KEY_4                  29
#define KEYBOARD_KEY_5                  30
#define KEYBOARD_KEY_6                  31
#define KEYBOARD_KEY_7                  32
#define KEYBOARD_KEY_8                  33
#define KEYBOARD_KEY_9                  34
#define KEYBOARD_KEY_0                  35
#define KEYBOARD_KEY_RETURN             36
#define KEYBOARD_KEY_ESC                37
#define KEYBOARD_KEY_BS                 38
#define KEYBOARD_KEY_TAB                39
#define KEYBOARD_KEY_SPACE              40
#define KEYBOARD_KEY_MINUS              41
#define KEYBOARD_KEY_EQ                 42
#define KEYBOARD_KEY_LBRACKET           43
#define KEYBOARD_KEY_RBRACKET           44
#define KEYBOARD_KEY_BSLASH             45
#define KEYBOARD_KEY_NONUSHASH          46
#define KEYBOARD_KEY_SEMICOLON          47
#define KEYBOARD_KEY_QUOTE              48
#define KEYBOARD_KEY_BACKQUOTE          49
#define KEYBOARD_KEY_COMMA              50
#define KEYBOARD_KEY_PERIOD             51
#define KEYBOARD_KEY_SLASH              52
#define KEYBOARD_KEY_CAPSLOCK           53
#define KEYBOARD_KEY_F1                 54
#define KEYBOARD_KEY_F2                 55
#define KEYBOARD_KEY_F3                 56
#define KEYBOARD_KEY_F4                 57
#define KEYBOARD_KEY_F5                 58
#define KEYBOARD_KEY_F6                 59
#define KEYBOARD_KEY_F7                 60
#define KEYBOARD_KEY_F8                 61
#define KEYBOARD_KEY_F9                 62
#define KEYBOARD_KEY_F10                63
#define KEYBOARD_KEY_F11                64
#define KEYBOARD_KEY_F12                65
#define KEYBOARD_KEY_PRINTSCR           66
#define KEYBOARD_KEY_SCROLL             67
#define KEYBOARD_KEY_PAUSE              68
#define KEYBOARD_KEY_INSERT             69
#define KEYBOARD_KEY_HOME               70
#define KEYBOARD_KEY_PGUP               71
#define KEYBOARD_KEY_DEL                72
#define KEYBOARD_KEY_END                73
#define KEYBOARD_KEY_PGDOWN             74
#define KEYBOARD_KEY_RIGHT              75
#define KEYBOARD_KEY_LEFT               76
#define KEYBOARD_KEY_DOWN               77
#define KEYBOARD_KEY_UP                 78
#define KEYBOARD_KEY_NUMLOCK            79
#define KEYBOARD_KEY_NSLASH             80
#define KEYBOARD_KEY_NSTAR              81
#define KEYBOARD_KEY_NMINUS             82
#define KEYBOARD_KEY_NPLUS              83
#define KEYBOARD_KEY_NENTER             84
#define KEYBOARD_KEY_N1                 85
#define KEYBOARD_KEY_N2                 86
#define KEYBOARD_KEY_N3                 87
#define KEYBOARD_KEY_N4                 88
#define KEYBOARD_KEY_N5                 89
#define KEYBOARD_KEY_N6                 90
#define KEYBOARD_KEY_N7                 91
#define KEYBOARD_KEY_N8                 92
#define KEYBOARD_KEY_N9                 93
#define KEYBOARD_KEY_N0                 94
#define KEYBOARD_KEY_NPERIOD            95
#define KEYBOARD_KEY_LCTRL              96
#define KEYBOARD_KEY_LSHIFT             97
#define KEYBOARD_KEY_LALT               98
#define KEYBOARD_KEY_LGUI               99
#define KEYBOARD_KEY_RCTRL             100
#define KEYBOARD_KEY_RSHIFT            101
#define KEYBOARD_KEY_RALT              102
#define KEYBOARD_KEY_RGUI              103

/* mouse buttons */
#define KEYBOARD_MOUSE_LEFT            112
#define KEYBOARD_MOUSE_MIDDLE          113
#define KEYBOARD_MOUSE_RIGHT           114

/* Special QUIT button */
#define KEYBOARD_QUIT                  127

/* joystick buttons */
#define KEYBOARD_JOY_BTN0              128
#define KEYBOARD_JOY_BTN1              129
#define KEYBOARD_JOY_BTN2              130
#define KEYBOARD_JOY_BTN3              131
#define KEYBOARD_JOY_BTN4              132
#define KEYBOARD_JOY_BTN5              133
#define KEYBOARD_JOY_BTN6              134
#define KEYBOARD_JOY_BTN7              135
#define KEYBOARD_JOY_BTN8              136
#define KEYBOARD_JOY_BTN9              137
#define KEYBOARD_JOY_BTN10             138
#define KEYBOARD_JOY_BTN11             139
#define KEYBOARD_JOY_LEFT              140
#define KEYBOARD_JOY_RIGHT             141
#define KEYBOARD_JOY_UP                142
#define KEYBOARD_JOY_DOWN              143


/* Data structures and types */
/* A structure representing the keyboard device */
struct keyboard {
    uint32_t state[KEYBOARD_SIZE]; /* state of the keyboard keys */
    uint32_t selector;          /* to select which state to read */
    uint32_t led;               /* state of the LEDs */
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
void keyboard_write_callback(struct keyboard *kbd, struct quivm *qvm,
                             uint32_t address, uint32_t v);

/* Set a particular bit of the state of the keyboard.
 * The bit to set is given by `bit` and the value of the bit
 * (either zero or 1) is given in `v`.
 */
void keyboard_set_bit(struct keyboard *kbd, uint32_t bit, int v);

/* Clear the relative mouse movement. */
void keyboard_clear_mouse(struct keyboard *kbd);

/* Add the mouse movement.
 * The movement is indicated by the number of horizonal pixels in `dx`
 * and the number of vertical pixels in `dy`.
 */
void keyboard_move_mouse(struct keyboard *kbd, int dx, int dy);

#endif
