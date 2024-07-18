#ifndef __VM_QUIVM_H
#define __VM_QUIVM_H

#include <stdint.h>

/* Constants. */
/* Constants for instructions */
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
#define INSN_AND                      0xC8
#define INSN_OR                       0xC9
#define INSN_XOR                      0xCA
#define INSN_ADD                      0xCB
#define INSN_SUB                      0xCC
#define INSN_CSEL                     0xCD
#define INSN_UMUL                     0xCE
#define INSN_UDIV                     0xCF
#define INSN_RD                       0xD0
#define INSN_WRT                      0xD1
#define INSN_RDB                      0xD2
#define INSN_WRTB                     0xD3
#define INSN_SIGNE                    0xD4
#define INSN_SHL                      0xD5
#define INSN_USHR                     0xD6
#define INSN_SHR                      0xD7
#define INSN_NOP                      0xD8
#define INSN_DUP                      0xD9
#define INSN_DROP                     0xDA
#define INSN_SWAP                     0xDB
#define INSN_OVER                     0xDC
#define INSN_ROT                      0xDD
#define INSN_RTO                      0xDE
#define INSN_RFROM                    0xDF
#define INSN_RGET                     0xE0
#define INSN_RSET                     0xE1
#define INSN_INVL                     0xFF

/* Some special addresses */
#define INITIAL_PC              0x00000000

/* Status of the VM */
#define STS_RUNNING             0x80000000
#define STS_OKAY                0x40000000
#define STS_JMPBUF              0x20000000 /* used internally */
#define STS_WAITING             0x00000100
#define STS_RETVAL_MASK         0x000000FF

/* Exception codes */
#define EX_RESET                0x00000000
#define EX_INVALID_INSN         0x00000001
#define EX_DIVIDE_BY_ZERO       0x00000002

/* The constants for memory and stack */
#define MEMORY_SIZE             0x00400000
#define STACK_SIZE                     256

/* Threshold number of stack cells close
 * to the stack limit to declarate stack
 * overflow.
 */
#define STACK_THRESHOLD                 16

/* The I/O address for the system device */
#define IO_BASE                 0xFFF00000
#define IO_SYS_BASE             0xFFFFFFE0

/* Addresses within the system device */
#define IO_SYS_SELECTOR         0xFFFFFFF8
#define IO_SYS_VALUE            0xFFFFFFF4

/* Possible values of the selector */
#define SYS_STATUS                       1
#define SYS_DSP                          2
#define SYS_RSP                          3
#define SYS_STACKSIZE                    4
#define SYS_MEMSIZE                      5
#define SYS_CELLSIZE                     6
#define SYS_ID                           7
#define SYS_ISTART                       8
#define SYS_IEND                         9

/* Data structures and types */

struct quivm;                       /* forward pre-declaration */

/* Callback for read calls (on the external I/O region).
 * The `arg` parameter is the extra parameter passed in quivm_configure().
 * The parameter `address` is the address to read from.
 * The `address` is guaranteed to be aligned to 4-bytes.
 * Returns the value read.
 */
typedef uint32_t (*quivm_read_cb)(struct quivm *qvm, void *arg,
                                  uint32_t address);

/* Callback for write calls (on the external I/O region).
 * The `arg` parameter is the extra parameter passed in quivm_configure().
 * The parameter `address` is the address to write, and `v` is the value.
 * The `address` is guaranteed to be aligned to 4-bytes.
 */
typedef void (*quivm_write_cb)(struct quivm *qvm, void *arg,
                               uint32_t address, uint32_t v);

/* Structure for a QUI virtual machine */
struct quivm {
    void *tracer;                   /* pointer to the tracer data */
    void *jmpbuf;                   /* for handling exceptions */

    void *arg;                      /* extra argument for callbacks */
    quivm_read_cb read_cb;          /* read callback function */
    quivm_write_cb write_cb;        /* write callback function */

    uint32_t *dstack, *rstack;      /* data and return stacks */
    uint8_t *mem;                   /* VM memory */

    int use_tracer;                 /* boolean flag to use the tracer */

    uint32_t status;                /* The status of the VM */
    uint32_t selector;              /* The selector for internal registers */
    uint32_t istart, iend;          /* The code cache invalidate address
                                     * range
                                     */

    uint32_t pc;                    /* program counter */
    uint32_t acc;                   /* accumulator */
    uint8_t dsp, rsp;               /* data and return stack pointers */
};

/* Functions */

/* Creates and initializes the QUI vm.
 * If `use_tracer` is nonzero, it uses the tracer to speed
 * up the virtual machine.
 * Returns zero on success.
 */
int quivm_init(struct quivm *qvm, int use_tracer);

/* Destroys the QUI vm and release the resources. */
void quivm_destroy(struct quivm *qvm);

/* Configures the VM callbacks.
 * This included the extra argument `arg` passed to the callbacks,
 * and the callbacks themselves: `read_cb` (for reading), and
 * `write_cb` (for writing).
 */
void quivm_configure(struct quivm *qvm, void *arg,
                     quivm_read_cb read_cb, quivm_write_cb write_cb);

/* Resets the QUI vm. */
void quivm_reset(struct quivm *qvm);

/* Loads an image from a file named `filename`
 * at address `address`, loading at most `length` words.
 * The parameter `length` is populated with the number of
 * bytes read on return. If `length` is initially provided
 * as zero, all the bytes in the file are read.
 * Returns zero on success.
 */
int quivm_load(struct quivm *qvm, const char *filename,
               uint32_t address, uint32_t *length);

/* Loads an image from an array named `data` at address
 * `address`, loading at most `length` words. The parameter
 * `length` is populated with the number of bytes copied on return.
 */
void quivm_load_array(struct quivm *qvm, const uint8_t *data,
                      uint32_t address, uint32_t *length);

/* Runs the virtual machine until halted or terminated.
 * Returns nonzero if the machine has not yet terminated.
 */
int quivm_run(struct quivm *qvm);

/* Terminates the VM using the return value `retval`. */
void quivm_terminate(struct quivm *qvm, int retval);

/* Signals that some exception happened within the VM
 * and if the VM is currently running, it might need
 * to stop. This function might never return.
 */
void quivm_raise(struct quivm *qvm);

/* Reads the value of a cell in memory at `address`.
 * Returns the value read.
 */
uint32_t quivm_read(struct quivm *qvm, uint32_t address);

/* Writes a value `v` to a cell in memory at `address`. */
void quivm_write(struct quivm *qvm, uint32_t address, uint32_t v);

/* Reads a byte from memory at `address`.
 * Returns the byte read.
 */
uint8_t quivm_read_byte(struct quivm *qvm, uint32_t address);

/* Writes a byte `v` to  memory at `address`. */
void quivm_write_byte(struct quivm *qvm, uint32_t address, uint8_t b);

/* Pushes a value `v` onto the data stack. */
void quivm_dstack_push(struct quivm *qvm, uint32_t v);

/* Pushes a value `v` onto the return stack. */
void quivm_rstack_push(struct quivm *qvm, uint32_t v);

/* Pops a value from the data stack.
 * Returns the popped value.
 */
uint32_t quivm_dstack_pop(struct quivm *qvm);

/* Pops a value from the return stack.
 * Returns the popped value.
 */
uint32_t quivm_rstack_pop(struct quivm *qvm);

/* Auxiliary macro to check if a given a buffer fits the memory.
 * The buffer is specified by the parameters:
 *  - `address` : the starting address of the buffer;
 *  - `length` : the length of the buffer;
 * and the memory size is given by `memsize`.
 * Returns zero if the buffer is valid.
 */
#define check_buffer(address, length, memsize) \
    (!(((address) < (memsize)) && ((length) < ((memsize) - (address)))))

#endif
