#ifndef __VM_QUIVM_H
#define __VM_QUIVM_H

#include <stdint.h>

/* Constants. */
/* Contants for instructions */
#define INSN_LIT_BASE                 0x80
#define INSN_REG_BASE                 0xC0
#define INSN_RET                      0xC0
#define INSN_JSR                      0xC1
#define INSN_JMP                      0xC2
#define INSN_JZ                       0xC3
#define INSN_EQ0                      0xC4
#define INSN_EQ                       0xC5
#define INSN_ULT                      0xC6
#define INSN_LT                       0xC7
#define INSN_NOP                      0xC8
#define INSN_AND                      0xC9
#define INSN_OR                       0xCA
#define INSN_XOR                      0xCB
#define INSN_ADD                      0xCC
#define INSN_SUB                      0xCD
#define INSN_UMUL                     0xCE
#define INSN_UDIV                     0xCF
#define INSN_RD                       0xD0
#define INSN_WRT                      0xD1
#define INSN_RDB                      0xD2
#define INSN_WRTB                     0xD3
#define INSN_SGE                      0xD4
#define INSN_SHL                      0xD5
#define INSN_SHR                      0xD6
#define INSN_SAR                      0xD7
#define INSN_DUP                      0xD8
#define INSN_DROP                     0xD9
#define INSN_SWAP                     0xDA
#define INSN_OVER                     0xDB
#define INSN_ROT                      0xDC
#define INSN_RTO                      0xDD
#define INSN_RFROM                    0xDE
#define INSN_RPEEK                    0xDF
#define INSN_INVL                     0xFF

/* Some special addresses */
#define INITIAL_PC              0x00000100

/* Cell with stack pointer */
#define CELL_STACK_POINTER      0xFFFFFFFF

/* Status of the VM */
#define STS_TERMINATED          0x00000001
#define STS_HALTED              0x00000002
#define STS_EXCEPTION           0x00000004

/* Exception codes */
#define EX_RESET                0x00000000
#define EX_INVALID_INSN         0x00000001
#define EX_DIVIDE_BY_ZERO       0x00000002

/* The I/O address for the system device */
#define IO_BASE                 0xFFF00000
#define IO_SYS_BASE             0xFFFFFF00

/* Relative addresses within the system device */
#define IO_SYS_SCELL                  0xF8
#define IO_SYS_DSTACK                 0xF4
#define IO_SYS_RSTACK                 0xF0
#define IO_SYS_STATUS                 0xEC
#define IO_SYS_TERMINATE              0xE8
#define IO_SYS_STACKSIZE              0xE4
#define IO_SYS_MEMSIZE                0xE0
#define IO_SYS_CELLSIZE               0xDC
#define IO_SYS_ID                     0xD8

/* Data structures and types */

struct quivm;                       /* forward pre-declaration */

/* Callback for read calls (on the external I/O region).
 * The parameter `address` is the address to read from.
 * The `address` is guaranteed to be aligned to 4-bytes.
 * Returns the value read.
 */
typedef uint32_t (*quivm_read_cb)(const struct quivm *qvm,
                                  uint32_t address);

/* Callback for write calls (on the external I/O region).
 * The parameter `address` is the address to write, and `v` is the value.
 * The `address` is guaranteed to be aligned to 4-bytes.
 */
typedef void (*quivm_write_cb)(struct quivm *qvm,
                               uint32_t address, uint32_t v);

/* Structure for a QUI virtual machine */
struct quivm {
    uint32_t *dstack;               /* data stack */
    uint32_t *rstack;               /* return stack */

    uint8_t *mem;                   /* VM memory */
    uint32_t pc;                    /* program counter */
    uint32_t acc;                   /* accumulator */
    uint32_t dsp, rsp;              /* data and return stack pointers */
    uint32_t stacksize;             /* stack size (in cells) */
    uint32_t memsize;               /* memory size (in bytes) */

    uint32_t scell;                 /* The cell for the stack read/write */
    uint32_t status;                /* The status of the VM */
    int termvalue;                  /* termination value */

    quivm_read_cb read_cb;          /* read callback function */
    quivm_write_cb write_cb;        /* write callback function */
    void *arg;                      /* extra argument for callbacks */
};

/* Functions */

/* Creates and initializes the QUI vm.
 * The `read_cb` is the callback for I/O, and the
 * `write_cb` is the callback for I/O writes. The parameter
 * `arg` is an extra argument for the callbacks.
 * Returns zero on success.
 */
int quivm_init(struct quivm *qvm, quivm_read_cb read_cb,
               quivm_write_cb write_cb, void *arg);

/* Destroys the QUI vm and release the resources. */
void quivm_destroy(struct quivm *qvm);

/* Resets the QUI vm. */
void quivm_reset(struct quivm *qvm);

/* Loads an image from a file named `filename`
 * at address `address`, loading at most `length` words.
 * The parameter `length` is populated with the number of
 * words read on return. If `length` is zero, all the
 * words in the file are read.
 * Returns zero on success.
 */
int quivm_load(struct quivm *qvm, const char *filename,
               uint32_t address, uint32_t *length);

/* Dumps an image to a file named `filename`.
 * The image start at address `address`, and has `length` words.
 * The parameter `length` is populated with the number of
 * words written on return.
 * Returns zero on success.
 */
int quivm_dump(struct quivm *qvm, const char *filename,
               uint32_t address, uint32_t *length);

/* Runs one step of the virtual machine.
 * Returns true if a step was executed.
 */
int quivm_step(struct quivm *qvm);

/* Runs the virtual machine for `max_steps` (or until halted).
 * If `max_steps` is zero, this function runs until the
 * virtual machine is halted.
 * Returns true if the machine has not yet halted.
 */
int quivm_run(struct quivm *qvm, unsigned int max_steps);

/* Reads a value from one of the stack memories.
 * The appropriate stack is selected by the parameter `use_rstack`.
 * The cell to read is given by `cell`.
 * Returns the value.
 */
uint32_t quivm_stack_read(const struct quivm *qvm,
                          int use_rstack, uint32_t cell);

/* Writes a value to one of the stack memories.
 * The appropriate stack is selected by the parameter `use_rstack`.
 * The cell to write is given by `cell`, and the value
 * to write is given by `v`.
 */
void quivm_stack_write(struct quivm *qvm, int use_rstack,
                       uint32_t cell, uint32_t v);

/* Reads the value of a cell in memory at `address`.
 * Returns the value read.
 */
uint32_t quivm_read(const struct quivm *qvm, uint32_t address);

/* Writes a value `v` to a cell in memory at `address`. */
void quivm_write(struct quivm *qvm, uint32_t address, uint32_t v);

/* Reads a byte from memory at `address`.
 * Returns the byte read.
 */
uint8_t quivm_read_byte(const struct quivm *qvm, uint32_t address);

/* Writes a byte `v` to  memory at `address`. */
void quivm_write_byte(struct quivm *qvm, uint32_t address, uint8_t b);


#endif /* __VM_QUIVM_H */
