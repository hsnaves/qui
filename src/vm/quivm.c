#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "vm/quivm.h"
#include "vm/internal.h"

/* Macros for stack operations */
#define dstack_push(v) qvm->dstack[qvm->dsp++] = (v)
#define rstack_push(v) qvm->rstack[qvm->rsp++] = (v)
#define dstack_pop() qvm->dstack[--qvm->dsp]
#define rstack_pop() qvm->rstack[--qvm->rsp]

/* Functions */

int quivm_init(struct quivm *qvm, int use_tracer)
{
    qvm->jmpbuf = NULL;
    qvm->dstack = NULL;
    qvm->rstack = NULL;
    qvm->mem = NULL;

    if (tracer_init(qvm)) {
        quivm_destroy(qvm);
        fprintf(stderr, "vm/quivm: init: "
                "could not initialize tracer\n");
        return 1;
    }

    qvm->jmpbuf = (void *) malloc(sizeof(jmp_buf));
    qvm->dstack = (uint32_t *) malloc(STACK_SIZE * sizeof(uint32_t));
    qvm->rstack = (uint32_t *) malloc(STACK_SIZE * sizeof(uint32_t));
    qvm->mem = (uint8_t *) malloc(MEMORY_SIZE);

    if (!qvm->jmpbuf || !qvm->dstack || !qvm->rstack || !qvm->mem) {
        quivm_destroy(qvm);
        fprintf(stderr, "vm/quivm: init: "
                "memory exhausted\n");
        return 1;
    }

    qvm->arg = NULL;
    qvm->read_cb = NULL;
    qvm->write_cb = NULL;
    qvm->use_tracer = use_tracer;

    /* reset the memory, and the stacks (with -1) */
    memset(qvm->dstack, -1, STACK_SIZE * sizeof(uint32_t));
    memset(qvm->rstack, -1, STACK_SIZE * sizeof(uint32_t));
    memset(qvm->mem, -1, MEMORY_SIZE);

    quivm_reset(qvm);
    return 0;
}

void quivm_destroy(struct quivm *qvm)
{
    tracer_destroy(qvm);

    if (qvm->jmpbuf) free(qvm->jmpbuf);
    if (qvm->dstack) free(qvm->dstack);
    if (qvm->rstack) free(qvm->rstack);
    if (qvm->mem) free(qvm->mem);

    qvm->jmpbuf = NULL;
    qvm->dstack = NULL;
    qvm->rstack = NULL;
    qvm->mem = NULL;
}

void quivm_configure(struct quivm *qvm, void *arg,
                     quivm_read_cb read_cb, quivm_write_cb write_cb)
{
    qvm->arg = arg;
    qvm->read_cb = read_cb;
    qvm->write_cb = write_cb;
}

void quivm_reset(struct quivm *qvm)
{
    qvm->pc = INITIAL_PC;
    qvm->acc = EX_RESET;
    qvm->dsp = 1; /* leave a sentinel value of zero */
    qvm->rsp = 0;
    qvm->selector = 0;
    qvm->status = (STS_RUNNING | STS_OKAY);
    qvm->istart = 0;
    qvm->iend = 0;
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
        if ((c == EOF) || !(address < MEMORY_SIZE)) break;
        qvm->mem[address++] = (uint8_t) c;
    }
    *length = i;
    fclose(fp);
    return 0;
}

void quivm_load_array(struct quivm *qvm, const uint8_t *data,
                      uint32_t address, uint32_t *length)
{
    uint32_t len;
    len = *length;
    if (check_buffer(address, len, MEMORY_SIZE)) {
        *length = 0;
        return;
    }
    memcpy(&qvm->mem[address], data, len);
    *length = len;
}

int quivm_run(struct quivm *qvm)
{
    uint32_t v, w;
    uint64_t dv, dw;
    uint32_t err_cond;
    uint8_t insn;

    if (qvm->use_tracer) tracer_run(qvm);

    /* check if it has terminated */
    if (!(qvm->status & STS_RUNNING))
        return !!(qvm->status & STS_WAITING);

    qvm->status |= STS_JMPBUF;
    if (setjmp(((jmp_buf *) qvm->jmpbuf)[0])) {
        qvm->status &= ~STS_JMPBUF;
        return quivm_run(qvm);
    }

    err_cond = 0;
    while (1) {
        insn = qvm->mem[qvm->pc % MEMORY_SIZE];
        qvm->pc++;

        if (insn < INSN_LIT_BASE) {
            qvm->acc <<= 7;
            qvm->acc |= (uint32_t) insn;
            continue;
        }

        if (insn < INSN_REG_BASE) {
            dstack_push(qvm->acc);
            qvm->acc = (uint32_t) (insn - INSN_LIT_BASE);
            if (qvm->acc & 0x20) {
                /* sign-extend the literal value */
                qvm->acc |= ~0x3F;
            }
            continue;
        }

        /* process regular instructions */
        switch (insn) {
        case INSN_RET:
            qvm->pc = rstack_pop();
            break;
        case INSN_JSR:
            rstack_push(qvm->pc);
            /* fall through */
        case INSN_JMP:
            qvm->pc += qvm->acc;
            qvm->acc = dstack_pop();
            break;
        case INSN_JZ:
            v = dstack_pop();
            if (v == 0) qvm->pc += qvm->acc;
            qvm->acc = dstack_pop();
            break;
        case INSN_EQ0:
            qvm->acc = (0 == qvm->acc) ? -1 : 0;
            break;
        case INSN_EQ:
            v = dstack_pop();
            qvm->acc = (v == qvm->acc) ? -1 : 0;
            break;
        case INSN_ULT:
            v = dstack_pop();
            qvm->acc = (v < qvm->acc) ? -1 : 0;
            break;
        case INSN_LT:
            v = dstack_pop();
            qvm->acc = (((int32_t) v) < ((int32_t) qvm->acc)) ? -1 : 0;
            break;
        case INSN_AND:
            v = dstack_pop();
            qvm->acc &= v;
            break;
        case INSN_OR:
            v = dstack_pop();
            qvm->acc |= v;
            break;
        case INSN_XOR:
            v = dstack_pop();
            qvm->acc ^= v;
            break;
        case INSN_SHL:
            v = dstack_pop();
            qvm->acc = (v << (qvm->acc & 0x1F));
            break;
        case INSN_USHR:
            v = dstack_pop();
            qvm->acc = (v >> (qvm->acc & 0x1F));
            break;
        case INSN_SHR:
            v = dstack_pop();
            qvm->acc = (((int32_t) v) >> (qvm->acc & 0x1F));
            break;
        case INSN_ADD:
            v = dstack_pop();
            qvm->acc += v;
            break;
        case INSN_SUB:
            v = dstack_pop();
            qvm->acc = v - qvm->acc;
            break;
        case INSN_NOP:
            break;
        case INSN_DUP:
            dstack_push(qvm->acc);
            break;
        case INSN_DROP:
            qvm->acc = dstack_pop();
            break;
        case INSN_SWAP:
            v = dstack_pop();
            dstack_push(qvm->acc);
            qvm->acc = v;
            break;
        case INSN_OVER:
            v = dstack_pop();
            dstack_push(v);
            dstack_push(qvm->acc);
            qvm->acc = v;
            break;
        case INSN_ROT:
            v = dstack_pop();
            w = dstack_pop();
            dstack_push(v);
            dstack_push(qvm->acc);
            qvm->acc = w;
            break;
        case INSN_RTO:
            rstack_push(qvm->acc);
            qvm->acc = dstack_pop();
            break;
        case INSN_RFROM:
            dstack_push(qvm->acc);
            qvm->acc = rstack_pop();
            break;
        case INSN_RD:
            qvm->acc = quivm_read(qvm, qvm->acc);
            break;
        case INSN_WRT:
            v = dstack_pop();
            quivm_write(qvm, qvm->acc, v);
            qvm->acc = dstack_pop();
            break;
        case INSN_RDB:
            qvm->acc = (uint32_t) quivm_read_byte(qvm, qvm->acc);
            break;
        case INSN_WRTB:
            v = dstack_pop();
            quivm_write_byte(qvm, qvm->acc, v);
            qvm->acc = dstack_pop();
            break;
        case INSN_RGET:
            qvm->acc = qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)];
            break;
        case INSN_RSET:
            v = dstack_pop();
            qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)] = v;
            qvm->acc = dstack_pop();
            break;
        case INSN_UMUL:
            dw = (uint64_t) dstack_pop();
            dv = (uint64_t) qvm->acc;
            dv *= dw;
            dstack_push((uint32_t) dv);
            qvm->acc = (uint32_t) (dv >> 32);
            break;
        case INSN_UDIV:
            /* check for division by zero */
            if (qvm->acc == 0) {
                qvm->pc--;
                err_cond = EX_DIVIDE_BY_ZERO;
                goto check_exception;
            }
            v = dstack_pop();
            dstack_push((v % qvm->acc));
            qvm->acc = v / qvm->acc;
            break;
        case INSN_INVL: /* fall through */
        default:
            qvm->pc--;
            err_cond = EX_INVALID_INSN;

        check_exception:
            if (!(qvm->status & STS_OKAY)) {
                fprintf(stderr, "vm/quivm: on_exception: "
                        "unhandled exception: %d at 0x%08X\n",
                        err_cond, qvm->pc);
                quivm_terminate(qvm, err_cond);
                quivm_raise(qvm);
                return 0;
            }

            dstack_push(qvm->acc);
            qvm->acc = err_cond;

            /* rstack points to the faulting instruction */
            rstack_push(qvm->pc);
            qvm->pc = INITIAL_PC;
            qvm->status &= ~STS_OKAY;
            break;
        }
    }
}

void quivm_terminate(struct quivm *qvm, int retval)
{
    qvm->status &= ~(STS_RUNNING | STS_WAITING | STS_RETVAL_MASK);
    qvm->status |= retval & (STS_RETVAL_MASK);
}

void quivm_raise(struct quivm *qvm)
{
    if (qvm->status & STS_JMPBUF) {
        longjmp(((jmp_buf *) qvm->jmpbuf)[0], 1);
    }
}


/* Performs an aligned read at `address`, which must be a multiple
 * of 4-bytes. Returns the value read.
 */
static
uint32_t aligned_read(struct quivm *qvm, uint32_t address)
{
    uint32_t v;

    if (address < MEMORY_SIZE) {
        v = ((uint32_t *) &qvm->mem[address])[0];
        v = CONVERT_LE(v);
        return v;
    }

    if (address < IO_BASE)
        return -1;

    if (address >= IO_SYS_BASE) {
        switch (address) {
        case IO_SYS_SELECTOR:
            v = qvm->selector;
            break;
        case IO_SYS_VALUE:
            switch (qvm->selector) {
            case SYS_STATUS:    v = qvm->status; break;
            case SYS_DSP:       v = qvm->dsp; break;
            case SYS_RSP:       v = qvm->rsp; break;
            case SYS_STACKSIZE: v = STACK_SIZE; break;
            case SYS_MEMSIZE:   v = MEMORY_SIZE; break;
            case SYS_CELLSIZE:  v = 4; break;
            case SYS_ID:        v = 0; break; /* TODO: set proper value */
            case SYS_ISTART:    v = qvm->istart; break;
            case SYS_IEND:      v = qvm->iend; break;
            default:
                if (qvm->selector >= STACK_SIZE
                    && qvm->selector < (2 * STACK_SIZE)) {
                    v = qvm->dstack[qvm->selector - STACK_SIZE];
                } else if (qvm->selector >= (2 * STACK_SIZE)
                           && qvm->selector < (3 * STACK_SIZE)) {
                    v = qvm->rstack[qvm->selector - (2 * STACK_SIZE)];
                } else {
                    v = -1;
                }
                break;
            }
            break;
        default:
            v = -1;
            break;
        }
        return v;
    }

    if (qvm->read_cb)
        return (qvm->read_cb)(qvm, qvm->arg, address);

    return -1;
}

/* Performs an aligned write of `v` to a cell at `address`, which must
 * be a multiple of 4-bytes.
 */
static
void aligned_write(struct quivm *qvm, uint32_t address, uint32_t v)
{
    if (address < MEMORY_SIZE) {
        v = CONVERT_LE(v);
        ((uint32_t *) &qvm->mem[address])[0] = v;
        return;
    }

    if (address < IO_BASE)
        return;

    if (address >= IO_SYS_BASE) {
        switch (address) {
        case IO_SYS_SELECTOR:
            qvm->selector = v;
            break;
        case IO_SYS_VALUE:
            switch (qvm->selector) {
            case SYS_STATUS:
                qvm->status &= ~(STS_RUNNING
                                 | STS_WAITING
                                 | STS_RETVAL_MASK);
                qvm->status |= v & (STS_RUNNING
                                    | STS_OKAY
                                    | STS_WAITING
                                    | STS_RETVAL_MASK);

                if ((qvm->status & (STS_RUNNING
                                    | STS_JMPBUF)) == STS_JMPBUF) {
                    qvm->acc = dstack_pop();
                    quivm_raise(qvm);
                }
                break;
            case SYS_DSP:
                qvm->dsp = (uint8_t) v;
                break;
            case SYS_RSP:
                qvm->rsp = (uint8_t) v;
                break;
            case SYS_ISTART:
                qvm->istart = v;
                break;
            case SYS_IEND:
                qvm->iend = v;
                tracer_invalidate(qvm, qvm->istart, qvm->iend);
                break;
            default:
                if (qvm->selector >= STACK_SIZE
                    && qvm->selector < (2 * STACK_SIZE)) {
                    qvm->dstack[qvm->selector - STACK_SIZE] = v;
                } else if (qvm->selector >= (2 * STACK_SIZE)
                           && qvm->selector < (3 * STACK_SIZE)) {
                    qvm->rstack[qvm->selector - (2 * STACK_SIZE)] = v;
                }
                break;
            }
            break;
        }
        return;
    }

    if (qvm->write_cb)
        (qvm->write_cb)(qvm, qvm->arg, address, v);
}

uint32_t quivm_read(struct quivm *qvm, uint32_t address)
{
    uint32_t v, shift;

    /* small optimization */
    if (address < MEMORY_SIZE - 3) {
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
    if (address < MEMORY_SIZE - 3) {
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

uint8_t quivm_read_byte(struct quivm *qvm, uint32_t address)
{
    uint32_t v, shift;

    if (address < MEMORY_SIZE)
        return qvm->mem[address];

    shift = (address & 3) << 3;
    address &= ~3;
    v = aligned_read(qvm, address);
    return (v >> shift) & 0xFF;
}

void quivm_write_byte(struct quivm *qvm, uint32_t address, uint8_t b)
{
    uint32_t v, shift, mask;

    if (address < MEMORY_SIZE) {
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

void quivm_dstack_push(struct quivm *qvm, uint32_t v)
{
    dstack_push(v);
}

void quivm_rstack_push(struct quivm *qvm, uint32_t v)
{
    rstack_push(v);
}

uint32_t quivm_dstack_pop(struct quivm *qvm)
{
    return dstack_pop();
}

uint32_t quivm_rstack_pop(struct quivm *qvm)
{
    return rstack_pop();
}
