#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vm/quivm.h"

/* Some constants */
#define DEFAULT_STACKSIZE      0x00000400
#define DEFAULT_MEMSIZE        0x00100000

/* Macros */
#define BSWAP(n) ((uint32_t) ((((n) & 0xFFU) << 24) |  \
                              (((n) & 0xFF00U) << 8) | \
                              (((n) >> 8) & 0xFF00U) | \
                              (((n) >> 24) & 0xFFU)))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#    define CONVERT_LE(n) (n)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#    define CONVERT_LE(n) BSWAP(n)
#else
#    error "undefined endianness"
#endif

/* Functions */

int quivm_init(struct quivm *qvm, quivm_read_cb read_cb,
               quivm_write_cb write_cb, void *arg)
{
    qvm->stacksize = DEFAULT_STACKSIZE;
    qvm->memsize = DEFAULT_MEMSIZE;

    qvm->dstack = (uint32_t *) malloc(qvm->stacksize * sizeof(uint32_t));
    qvm->rstack = (uint32_t *) malloc(qvm->stacksize * sizeof(uint32_t));
    qvm->mem = (uint8_t *) malloc(qvm->memsize);

    if (!qvm->dstack || !qvm->rstack || !qvm->mem) {
        quivm_destroy(qvm);
        fprintf(stderr, "vm/quivm: init: "
                "memory exhausted\n");
        return 1;
    }

    qvm->read_cb = read_cb;
    qvm->write_cb = write_cb;
    qvm->arg = arg;

    /* zero the memory, the stack and the device memory */
    memset(qvm->dstack, 0, qvm->stacksize * sizeof(uint32_t));
    memset(qvm->rstack, 0, qvm->stacksize * sizeof(uint32_t));
    memset(qvm->mem, 0, qvm->memsize);

    quivm_reset(qvm);
    return 0;
}

void quivm_destroy(struct quivm *qvm)
{
    if (qvm->dstack) free(qvm->dstack);
    if (qvm->rstack) free(qvm->rstack);
    if (qvm->mem) free(qvm->mem);

    qvm->dstack = NULL;
    qvm->rstack = NULL;
    qvm->mem = NULL;
}

void quivm_reset(struct quivm *qvm)
{
    qvm->pc = INITIAL_PC;
    qvm->acc = EX_RESET;
    qvm->dsp = 1; /* leave a sentinel value of zero */
    qvm->rsp = 0;
    qvm->scell = CELL_STACK_POINTER;
    qvm->status = 0;
    qvm->termvalue = 0;
}

int quivm_load(struct quivm *qvm, const char *filename,
               uint32_t address, uint32_t *length)
{
    FILE *fp;
    uint32_t i, len;
    int c;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "vm/quivm: load: "
                "could not open file `%s` for reading\n",
                filename);
        *length = 0;
        return 1;
    }

    len = *length;

    for (i = 0; (i < len) || (len == 0); i++) {
        /* reads the word in little endinan format */
        c = fgetc(fp);
        if (c == EOF) break;
        quivm_write_byte(qvm, address + i, (uint8_t) c);
    }
    *length = i;
    fclose(fp);
    return 0;
}

/* Pushes a value `v` onto a stack selected by `use_rstack`. */
static
void stack_push(struct quivm *qvm, int use_rstack, uint32_t v)
{
    uint32_t *sp;
    sp = (use_rstack) ? &qvm->rsp : &qvm->dsp;
    quivm_stack_write(qvm, use_rstack, sp[0], v);
    if (++sp[0] == qvm->stacksize) sp[0] = 0;
}

/* Pops a value from the selected by `use_rstack`.
 * Returns the popped value.
 */
static
uint32_t stack_pop(struct quivm *qvm, int use_rstack)
{
    uint32_t *sp;
    sp = (use_rstack) ? &qvm->rsp : &qvm->dsp;
    if (sp[0]-- == 0) sp[0] = qvm->stacksize - 1;
    return quivm_stack_read(qvm, use_rstack, sp[0]);
}

/* Auxiliary function to run when an exception is found.
 * The error condition is given by `err_cond`.
 */
static
void on_exception(struct quivm *qvm, uint32_t err_cond)
{
    if (qvm->status & STS_EXCEPTION) {
        qvm->status |= STS_TERMINATED;
        qvm->termvalue = 1;
        return;
    }

    stack_push(qvm, 0, qvm->acc);
    qvm->acc = err_cond;

    /* rstack points to the faulting instruction */
    stack_push(qvm, 1, qvm->pc);
    qvm->pc = INITIAL_PC;

    qvm->status |= STS_EXCEPTION;
}

int quivm_step(struct quivm *qvm)
{
    uint32_t v, w;
    uint64_t dv, dw;
    uint32_t old_pc;
    uint8_t insn;

    if (qvm->status & (STS_HALTED | STS_TERMINATED))
        return 0;

    old_pc = qvm->pc;
    insn = quivm_read_byte(qvm, qvm->pc++);

    if (insn < INSN_LIT_BASE) {
        qvm->acc <<= 7;
        qvm->acc |= (uint32_t) insn;
    } else if (insn < INSN_REG_BASE) {
        stack_push(qvm, 0, qvm->acc);
        qvm->acc = (uint32_t) (insn - INSN_LIT_BASE);
        if (qvm->acc & 0x20) {
            /* sign-extend the literal value */
            qvm->acc |= ~0x3F;
        }
    } else {
        /* process regular instructions */
        switch (insn) {
        case INSN_RET:
            qvm->pc = stack_pop(qvm, 1);
            break;
        case INSN_JSR:
            stack_push(qvm, 1, qvm->pc);
            /* fall through */
        case INSN_JMP:
            qvm->pc += qvm->acc;
            qvm->acc = stack_pop(qvm, 0);
            break;
        case INSN_JZ:
            v = stack_pop(qvm, 0);
            if (v == 0) qvm->pc += qvm->acc;
            qvm->acc = stack_pop(qvm, 0);
            break;
        case INSN_EQ0:
            qvm->acc = (0 == qvm->acc) ? -1 : 0;
            break;
        case INSN_EQ:
            v = stack_pop(qvm, 0);
            qvm->acc = (v == qvm->acc) ? -1 : 0;
            break;
        case INSN_ULT:
            v = stack_pop(qvm, 0);
            qvm->acc = (v < qvm->acc) ? -1 : 0;
            break;
        case INSN_LT:
            v = stack_pop(qvm, 0);
            qvm->acc = (((int32_t) v) < ((int32_t) qvm->acc)) ? -1 : 0;
            break;
        case INSN_NOP:
            break;
        case INSN_AND:
            v = stack_pop(qvm, 0);
            qvm->acc &= v;
            break;
        case INSN_OR:
            v = stack_pop(qvm, 0);
            qvm->acc |= v;
            break;
        case INSN_XOR:
            v = stack_pop(qvm, 0);
            qvm->acc ^= v;
            break;
        case INSN_ADD:
            v = stack_pop(qvm, 0);
            qvm->acc += v;
            break;
        case INSN_SUB:
            v = stack_pop(qvm, 0);
            qvm->acc = v - qvm->acc;
            break;
        case INSN_UMUL:
            dw = (uint64_t) stack_pop(qvm, 0);
            dv = (uint64_t) qvm->acc;
            dv *= dw;
            stack_push(qvm, 0, (uint32_t) dv);
            qvm->acc = (uint32_t) (dv >> 32);
            break;
        case INSN_UDIV:
            /* check for division by zero */
            if (qvm->acc == 0) {
                qvm->pc = old_pc;
                on_exception(qvm, EX_DIVIDE_BY_ZERO);
                return 1;
            }
            v = stack_pop(qvm, 0);
            stack_push(qvm, 0, (v % qvm->acc));
            qvm->acc = v / qvm->acc;
            break;
        case INSN_RD:
            qvm->acc = quivm_read(qvm, qvm->acc);
            break;
        case INSN_WRT:
            v = stack_pop(qvm, 0);
            quivm_write(qvm, qvm->acc, v);
            qvm->acc = stack_pop(qvm, 0);
            break;
        case INSN_RDB:
            qvm->acc = quivm_read_byte(qvm, qvm->acc);
            break;
        case INSN_WRTB:
            v = stack_pop(qvm, 0);
            quivm_write_byte(qvm, qvm->acc, v);
            qvm->acc = stack_pop(qvm, 0);
            break;
        case INSN_SGE8:
            qvm->acc &= 0xFF;
            if (qvm->acc & 0x80)
                qvm->acc |= ~0xFF;
            break;
        case INSN_SHL:
            v = stack_pop(qvm, 0);
            qvm->acc = (v << (qvm->acc & 0x1F));
            break;
        case INSN_SHR:
            v = stack_pop(qvm, 0);
            qvm->acc = (v >> (qvm->acc & 0x1F));
            break;
        case INSN_SAR:
            v = stack_pop(qvm, 0);
            qvm->acc = (((int32_t) v) >> (qvm->acc & 0x1F));
            break;
        case INSN_DUP:
            stack_push(qvm, 0, qvm->acc);
            break;
        case INSN_DROP:
            qvm->acc = stack_pop(qvm, 0);
            break;
        case INSN_SWAP:
            v = stack_pop(qvm, 0);
            stack_push(qvm, 0, qvm->acc);
            qvm->acc = v;
            break;
        case INSN_OVER:
            v = stack_pop(qvm, 0);
            stack_push(qvm, 0, v);
            stack_push(qvm, 0, qvm->acc);
            qvm->acc = v;
            break;
        case INSN_ROT:
            v = stack_pop(qvm, 0);
            w = stack_pop(qvm, 0);
            stack_push(qvm, 0, v);
            stack_push(qvm, 0, qvm->acc);
            qvm->acc = w;
            break;
        case INSN_RTO:
            stack_push(qvm, 1, qvm->acc);
            qvm->acc = stack_pop(qvm, 0);
            break;
        case INSN_RFROM:
            stack_push(qvm, 0, qvm->acc);
            qvm->acc = stack_pop(qvm, 1);
            break;
        case INSN_RPEEK:
            stack_push(qvm, 0, qvm->acc);
            qvm->acc = stack_pop(qvm, 1);
            stack_push(qvm, 1, qvm->acc);
            break;
        case INSN_INVL: /* fall through */
        default:
            qvm->pc = old_pc;
            on_exception(qvm, EX_INVALID_INSN);
            return 1;
        }
    }

    /* Detect stack overflow
     * Overflows are detected even if the last
     * instruction halted or terminated the VM
     */
    if (!(qvm->status & STS_EXCEPTION)
        && ((qvm->dsp >= qvm->stacksize - STACK_THRESHOLD)
            || (qvm->rsp >= qvm->stacksize - STACK_THRESHOLD))) {

        stack_push(qvm, 0, qvm->acc);
        qvm->acc = old_pc;
        on_exception(qvm, EX_STACK_OVERFLOW);
    }

    return 1;
}

int quivm_run(struct quivm *qvm, unsigned int max_steps)
{
    unsigned int step = 0;
    while ((step++ < max_steps) || (max_steps == 0)) {
        if (!quivm_step(qvm))
            return 0;
    }
    return 1;
}

uint32_t quivm_stack_read(const struct quivm *qvm,
                          int use_rstack, uint32_t cell)
{
    const uint32_t *stack;
    if (cell >= qvm->stacksize) return 0xFFFFFFFF;
    stack = (use_rstack) ? qvm->rstack : qvm->dstack;
    return stack[cell];
}

void quivm_stack_write(struct quivm *qvm, int use_rstack,
                       uint32_t cell, uint32_t v)
{
    uint32_t *stack;
    if (cell >= qvm->stacksize) return;
    stack = (use_rstack) ? qvm->rstack : qvm->dstack;
    stack[cell] = v;
}

/* Performs an aligned read at `address`, which must be a multiple
 * of 4-bytes. Returns the value read.
 */
static
uint32_t aligned_read(const struct quivm *qvm, uint32_t address)
{
    uint32_t v;

    if (address < qvm->memsize) {
        v = ((uint32_t *) &qvm->mem[address])[0];
        v = CONVERT_LE(v);
        return v;
    }

    if (address < IO_BASE)
        return -1;

    if (address >= IO_SYS_BASE) {
        switch (address) {
        case IO_SYS_SCELL:
            v = qvm->scell;
            break;
        case IO_SYS_DSTACK:
            if (qvm->scell == CELL_STACK_POINTER) {
                v = qvm->dsp;
            } else {
                v = quivm_stack_read(qvm, 0, qvm->scell);
            }
            break;
        case IO_SYS_RSTACK:
            if (qvm->scell == CELL_STACK_POINTER) {
                v = qvm->rsp;
            } else {
                v = quivm_stack_read(qvm, 1, qvm->scell);
            }
            break;
        case IO_SYS_STATUS:
            v = qvm->status;
            break;
        case IO_SYS_STACKSIZE:
            v = qvm->stacksize;
            break;
        case IO_SYS_MEMSIZE:
            v = qvm->memsize;
            break;
        case IO_SYS_CELLSIZE:
            v = 4;
            break;
        case IO_SYS_ID:
            v = 0;
            break;
        default:
            v = -1;
            break;
        }
        return v;
    }

    if (qvm->read_cb)
        return (qvm->read_cb)(qvm, address);

    return -1;
}

/* Performs an aligned write of `v` to a cell at `address`, which must
 * be a multiple of 4-bytes.
 */
static
void aligned_write(struct quivm *qvm, uint32_t address, uint32_t v)
{
    if (address < qvm->memsize) {
        v = CONVERT_LE(v);
        ((uint32_t *) &qvm->mem[address])[0] = v;
        return;
    }

    if (address < IO_BASE)
        return;

    if (address >= IO_SYS_BASE) {
        switch (address) {
        case IO_SYS_SCELL:
            qvm->scell = v;
            break;
        case IO_SYS_DSTACK:
            if (qvm->scell == CELL_STACK_POINTER) {
                qvm->dsp = v % qvm->stacksize;
            } else {
                quivm_stack_write(qvm, 0, qvm->scell, v);
            }
            break;
        case IO_SYS_RSTACK:
            if (qvm->scell == CELL_STACK_POINTER) {
                qvm->rsp = v % qvm->stacksize;
            } else {
                quivm_stack_write(qvm, 1, qvm->scell, v);
            }
            break;
        case IO_SYS_STATUS:
            qvm->status &= ~(STS_EXCEPTION | STS_HALTED);
            qvm->status |= v & STS_HALTED;
            break;
        case IO_SYS_TERMINATE:
            qvm->termvalue = v;
            qvm->status |= STS_TERMINATED;
            break;
        }
        return;
    }

    if (qvm->write_cb)
        (qvm->write_cb)(qvm, address, v);
}


uint32_t quivm_read(const struct quivm *qvm, uint32_t address)
{
    uint32_t v, shift;

    /* small optimization */
    if (address <= qvm->memsize - 4) {
        v = ((uint32_t *) &qvm->mem[address])[0];
        v = CONVERT_LE(v);
        return v;
    }

    shift = (address & 3) << 3;
    address &= ~3;
    v = aligned_read(qvm, address);
    if (shift == 0) return v;

    /* handle misaligned addresses */
    v >>= shift;
    address += 4;
    shift = 32 - shift;
    v |= aligned_read(qvm, address) << shift;
    return v;
}

void quivm_write(struct quivm *qvm, uint32_t address, uint32_t v)
{
    uint32_t w, shift, mask;

    /* small optimization */
    if (address <= qvm->memsize - 4) {
        v = CONVERT_LE(v);
        ((uint32_t *) &qvm->mem[address])[0] = v;
        return;
    }

    shift = (address & 3) << 3;
    address &= ~3;
    if (shift == 0) {
        /* address is multiple of 4-bytes, so can call aligned_write() */
        aligned_write(qvm, address, v);
        return;
    }

    /* handle misaligned addresses */
    mask = (1 << shift) - 1;
    w = aligned_read(qvm, address);
    w &= mask;
    w |= (v << shift);
    aligned_write(qvm, address, w);

    mask = ~mask;
    address += 4;
    shift = 32 - shift;
    w = aligned_read(qvm, address);
    w &= mask;
    w |= (v >> shift);
    aligned_write(qvm, address, w);
}

uint8_t quivm_read_byte(const struct quivm *qvm, uint32_t address)
{
    uint32_t v, shift;

    if (address < qvm->memsize)
        return qvm->mem[address];

    shift = (address & 3) << 3;
    address &= ~3;
    v = aligned_read(qvm, address);
    return (v >> shift) & 0xFF;
}

void quivm_write_byte(struct quivm *qvm, uint32_t address, uint8_t b)
{
    uint32_t v, shift, mask;

    if (address < qvm->memsize) {
        qvm->mem[address] = b;
        return;
    }

    shift = (address & 3) << 3;
    address &= ~3;
    mask = 0xFF << shift;
    v = aligned_read(qvm, address);
    v = (v & (~mask)) | (((uint32_t) b) << shift);
    aligned_write(qvm, address, v);
}
