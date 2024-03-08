#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vm/quivm.h"

/* Some constants */
#define DEFAULT_STACKSIZE      0x00000400
#define DEFAULT_MEMSIZE        0x00100000

/* Functions */

int quivm_init(struct quivm *qvm, quivm_read_cb read_cb,
               quivm_write_cb write_cb, void *arg)
{
    qvm->stacksize = DEFAULT_STACKSIZE;
    qvm->memsize = DEFAULT_MEMSIZE;

    qvm->dstack = (uint32_t *) malloc(qvm->stacksize * sizeof(uint32_t));
    qvm->rstack = (uint32_t *) malloc(qvm->stacksize * sizeof(uint32_t));
    qvm->mem = (uint32_t *) malloc(qvm->memsize);

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
    qvm->acc = 0;
    qvm->dsp = 0;
    qvm->rsp = 0;
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

int quivm_dump(struct quivm *qvm, const char *filename,
               uint32_t address, uint32_t *length)
{
    FILE *fp;
    uint32_t i, len;
    uint8_t b;
    int c;

    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "vm/quivm: dump: "
                "could not open file `%s` for writing\n",
                filename);
        *length = 0;
        return 1;
    }

    len = *length;
    for (i = 0; i < len; i++) {
        b = quivm_read_byte(qvm, address + i);
        c = fputc(b, fp);
        if (c == EOF) break;
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
    quivm_stack_write(qvm, use_rstack, (*sp)++, v);
}

/* Pops a value from the selected by `use_rstack`.
 * Returns the popped value.
 */
static
uint32_t stack_pop(struct quivm *qvm, int use_rstack)
{
    uint32_t *sp;
    sp = (use_rstack) ? &qvm->rsp : &qvm->dsp;
    return quivm_stack_read(qvm, use_rstack, --(*sp));
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
    qvm->pc = EXCEPTION_HANDLER;
}

int quivm_step(struct quivm *qvm)
{
    uint32_t v, w;
    uint8_t insn;

    if (qvm->status & (STS_HALTED | STS_TERMINATED))
        return 0;

    insn = quivm_read_byte(qvm, qvm->pc++);
    if (insn < INSN_LIT_BASE) {
        qvm->acc <<= 7;
        qvm->acc |= (uint32_t) insn;
    } else if (insn < INSN_REG_BASE) {
        stack_push(qvm, 0, qvm->acc);
        qvm->acc = (uint32_t) (insn - INSN_LIT_BASE);
        if (qvm->acc & 0x20) {
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
            qvm->pc = qvm->acc;
            qvm->acc = stack_pop(qvm, 0);
            break;
        case INSN_JZ:
            v = stack_pop(qvm, 0);
            if (v == 0) qvm->pc = qvm->acc;
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
            v = stack_pop(qvm, 0);
            qvm->acc *= v;
            break;
        case INSN_UDIV:
            /* check for division by zero */
            if (qvm->acc == 0) {
                on_exception(qvm, EX_DIVIDE_BY_ZERO);
                return 1;
            }
            v = stack_pop(qvm, 0);
            stack_push(qvm, 0, (v % qvm->acc));
            qvm->acc = v / qvm->acc;
            break;
        case INSN_RD:
            if ((qvm->acc & 3) != 0) {
                on_exception(qvm, EX_UNALIGNED_MEMORY);
                return 1;
            }
            qvm->acc = quivm_read(qvm, qvm->acc);
            break;
        case INSN_WRT:
            if ((qvm->acc & 3) != 0) {
                on_exception(qvm, EX_UNALIGNED_MEMORY);
                return 1;
            }
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
        case INSN_SGE:
            qvm->acc &= 0xFF;
            if (qvm->acc & 0x80)
                qvm->acc |= ~0xFF;
            break;
        case INSN_SHL:
            v = stack_pop(qvm, 0);
            qvm->acc = (v << qvm->acc);
            break;
        case INSN_SHR:
            v = stack_pop(qvm, 0);
            qvm->acc = (v >> qvm->acc);
            break;
        case INSN_SAR:
            v = stack_pop(qvm, 0);
            qvm->acc = (((int32_t) v) >> qvm->acc);
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
            on_exception(qvm, EX_INVALID_INSN);
            return 1;
        }
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

uint32_t quivm_read(const struct quivm *qvm, uint32_t address)
{
    uint32_t v;

    /* align the address to multiple of 4 bytes */
    address &= ~0x03;

    if (address < IO_BASE) {
        if (address < qvm->memsize)
            return qvm->mem[(address >> 2)];
        return -1;
    }

    if (address >= IO_SYS_BASE) {
        switch (address - IO_SYS_BASE) {
        case SYS_CELL_SADDR:
            v = qvm->saddr;
            break;
        case SYS_CELL_DSTACK:
            if (qvm->saddr == 0xFFFFFFFF) {
                v = qvm->dsp;
            } else {
                v = quivm_stack_read(qvm, 0, qvm->saddr);
            }
            break;
        case SYS_CELL_RSTACK:
            if (qvm->saddr == 0xFFFFFFFF) {
                v = qvm->rsp;
            } else {
                v = quivm_stack_read(qvm, 1, qvm->saddr);
            }
            break;
        case SYS_CELL_STATUS:
            v = qvm->status;
            break;
        case SYS_CELL_STACKSIZE:
            v = qvm->stacksize;
            break;
        case SYS_CELL_MEMSIZE:
            v = qvm->memsize;
            break;
        case SYS_CELL_CELLSIZE:
            v = 4;
            break;
        case SYS_CELL_ID:
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

void quivm_write(struct quivm *qvm, uint32_t address, uint32_t v)
{
    /* align the address to multiple of 4 bytes */
    address &= ~0x03;

    if (address < IO_BASE) {
        if (address < qvm->memsize)
            qvm->mem[address >> 2] = v;
        return;
    }

    if (address >= IO_SYS_BASE) {
        switch (address - IO_SYS_BASE) {
        case SYS_CELL_SADDR:
            qvm->saddr = v;
            break;
        case SYS_CELL_DSTACK:
            if (qvm->saddr == 0xFFFFFFFF) {
                qvm->dsp = v;
            } else {
                quivm_stack_write(qvm, 0, qvm->saddr, v);
            }
            break;
        case SYS_CELL_RSTACK:
            if (qvm->saddr == 0xFFFFFFFF) {
                qvm->rsp = v;
            } else {
                quivm_stack_write(qvm, 1, qvm->saddr, v);
            }
            break;
        case SYS_CELL_STATUS:
            qvm->status &= ~STS_EXCEPTION;
            break;
        case SYS_CELL_TERMINATE:
            qvm->termvalue = v;
            qvm->status |= STS_TERMINATED;
            break;
        }
        return;
    }

    if (qvm->write_cb)
        (qvm->write_cb)(qvm, address, v);
}

uint8_t quivm_read_byte(const struct quivm *qvm, uint32_t address)
{
    uint32_t v = quivm_read(qvm, address);
    return (v >> ((address & 3) << 3)) & 0xFF;
}

void quivm_write_byte(struct quivm *qvm, uint32_t address, uint8_t b)
{
    uint32_t v, shift, mask;
    v = quivm_read(qvm, address);
    shift = (address & 3) << 3;
    mask = 0xFF << shift;
    v = (v & (~mask)) | (((uint32_t) b) << shift);
    quivm_write(qvm, address, v);
}
