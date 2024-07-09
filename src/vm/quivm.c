#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vm/quivm.h"

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

int quivm_init(struct quivm *qvm, uint32_t memsize)
{
    qvm->dstack = NULL;
    qvm->rstack = NULL;
    qvm->mem = NULL;

    /* validate the memsize parameter */
    if ((memsize % 4) != 0) {
        quivm_destroy(qvm);
        fprintf(stderr, "vm/quivm: init: "
                "memsize is not multiple of 4\n");
        return 1;
    }

    if (memsize < 0x80000) {
        quivm_destroy(qvm);
        fprintf(stderr, "vm/quivm: init: "
                "memsize too small\n");
        return 1;
    }

    if (memsize > IO_BASE) {
        quivm_destroy(qvm);
        fprintf(stderr, "vm/quivm: init: "
                "memsize too large\n");
        return 1;
    }

    qvm->memsize = memsize;

    qvm->dstack = (uint32_t *) malloc(STACK_SIZE * sizeof(uint32_t));
    qvm->rstack = (uint32_t *) malloc(STACK_SIZE * sizeof(uint32_t));
    qvm->mem = (uint8_t *) malloc(qvm->memsize);

    if (!qvm->dstack || !qvm->rstack || !qvm->mem) {
        quivm_destroy(qvm);
        fprintf(stderr, "vm/quivm: init: "
                "memory exhausted\n");
        return 1;
    }

    qvm->arg = NULL;
    qvm->read_cb = NULL;
    qvm->write_cb = NULL;

    /* reset the memory, and the stacks (with -1) */
    memset(qvm->dstack, -1, STACK_SIZE * sizeof(uint32_t));
    memset(qvm->rstack, -1, STACK_SIZE * sizeof(uint32_t));
    memset(qvm->mem, -1, qvm->memsize);

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
    qvm->scell = CELL_STACK_POINTER;
    qvm->selector = 0;
    qvm->status = 0;
    qvm->termvalue = 0;
    qvm->cycles = 0;
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
        if ((c == EOF) || !(address < qvm->memsize)) break;
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
    if (check_buffer(address, len, qvm->memsize)) {
        *length = 0;
        return;
    }
    memcpy(&qvm->mem[address], data, len);
    *length = len;
}

int quivm_run(struct quivm *qvm, uint32_t num_cycles)
{
    uint32_t v, w;
    uint64_t dv, dw;
    uint32_t cyc, first_pc;
    uint32_t err_cond;
    uint8_t insn;

    /* check if it has terminated */
    if (qvm->status & STS_TERMINATED)
        return 0;

    /* If halted, update the cycle count. */
    if (qvm->status & STS_HALTED) {
        qvm->cycles += num_cycles;
        return 1;
    }

    if (num_cycles == 0) return 1;

    err_cond = 0;
    first_pc = qvm->pc;

    while (1) {
        if (qvm->pc < qvm->memsize) {
            insn = qvm->mem[qvm->pc];
        } else {
            /* prevent the VM from executing outside the memory space */
            insn = INSN_INVL;
        }
        qvm->pc++;

        if (insn < INSN_LIT_BASE) {
            qvm->acc <<= 7;
            qvm->acc |= (uint32_t) insn;
            continue;
        }

        if (insn < INSN_REG_BASE) {
            quivm_dstack_push(qvm, qvm->acc);
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
            cyc = qvm->pc - first_pc;
            qvm->pc = quivm_rstack_pop(qvm);
            goto check_branch;
        case INSN_JSR:
            quivm_rstack_push(qvm, qvm->pc);
            /* fall through */
        case INSN_JMP:
            cyc = qvm->pc - first_pc;
            qvm->pc += qvm->acc;
            qvm->acc = quivm_dstack_pop(qvm);
            goto check_branch;
        case INSN_JZ:
            cyc = qvm->pc - first_pc;
            v = quivm_dstack_pop(qvm);
            if (v == 0) qvm->pc += qvm->acc;
            qvm->acc = quivm_dstack_pop(qvm);
            goto check_branch;
        case INSN_EQ0:
            qvm->acc = (0 == qvm->acc) ? -1 : 0;
            break;
        case INSN_EQ:
            v = quivm_dstack_pop(qvm);
            qvm->acc = (v == qvm->acc) ? -1 : 0;
            break;
        case INSN_ULT:
            v = quivm_dstack_pop(qvm);
            qvm->acc = (v < qvm->acc) ? -1 : 0;
            break;
        case INSN_LT:
            v = quivm_dstack_pop(qvm);
            qvm->acc = (((int32_t) v) < ((int32_t) qvm->acc)) ? -1 : 0;
            break;
        case INSN_NOP:
            break;
        case INSN_AND:
            v = quivm_dstack_pop(qvm);
            qvm->acc &= v;
            break;
        case INSN_OR:
            v = quivm_dstack_pop(qvm);
            qvm->acc |= v;
            break;
        case INSN_XOR:
            v = quivm_dstack_pop(qvm);
            qvm->acc ^= v;
            break;
        case INSN_ADD:
            v = quivm_dstack_pop(qvm);
            qvm->acc += v;
            break;
        case INSN_SUB:
            v = quivm_dstack_pop(qvm);
            qvm->acc = v - qvm->acc;
            break;
        case INSN_UMUL:
            dw = (uint64_t) quivm_dstack_pop(qvm);
            dv = (uint64_t) qvm->acc;
            dv *= dw;
            quivm_dstack_push(qvm, (uint32_t) dv);
            qvm->acc = (uint32_t) (dv >> 32);
            break;
        case INSN_UDIV:
            /* check for division by zero */
            if (qvm->acc == 0) {
                qvm->pc--; first_pc--;
                err_cond = EX_DIVIDE_BY_ZERO;
                goto check_exception;
            }
            v = quivm_dstack_pop(qvm);
            quivm_dstack_push(qvm, (v % qvm->acc));
            qvm->acc = v / qvm->acc;
            break;
        case INSN_RD:
            v = quivm_read(qvm, qvm->acc);
            goto check_rewind;
        case INSN_WRT:
            v = quivm_dstack_pop(qvm);
            quivm_write(qvm, qvm->acc, v);
            qvm->acc = quivm_dstack_pop(qvm);
            goto check_read_write;
        case INSN_RDB:
            v = quivm_read_byte(qvm, qvm->acc);
        check_rewind:
            if (qvm->status & STS_REWIND) {
                qvm->pc--;
                qvm->status &= ~STS_REWIND;
            } else {
                qvm->acc = v;
            }
            goto check_read_write;
        case INSN_WRTB:
            v = quivm_dstack_pop(qvm);
            quivm_write_byte(qvm, qvm->acc, v);
            qvm->acc = quivm_dstack_pop(qvm);
            goto check_read_write;
        case INSN_SIGNE:
            v = quivm_dstack_pop(qvm);
            if (qvm->acc < 32) {
                qvm->acc = 32 - qvm->acc;
                v <<= qvm->acc;
                qvm->acc = (((int32_t) v) >> qvm->acc);
            } else {
                qvm->acc = v;
            }
            break;
        case INSN_SHL:
            v = quivm_dstack_pop(qvm);
            qvm->acc = (v << (qvm->acc & 0x1F));
            break;
        case INSN_USHR:
            v = quivm_dstack_pop(qvm);
            qvm->acc = (v >> (qvm->acc & 0x1F));
            break;
        case INSN_SHR:
            v = quivm_dstack_pop(qvm);
            qvm->acc = (((int32_t) v) >> (qvm->acc & 0x1F));
            break;
        case INSN_DUP:
            quivm_dstack_push(qvm, qvm->acc);
            break;
        case INSN_DROP:
            qvm->acc = quivm_dstack_pop(qvm);
            break;
        case INSN_SWAP:
            v = quivm_dstack_pop(qvm);
            quivm_dstack_push(qvm, qvm->acc);
            qvm->acc = v;
            break;
        case INSN_OVER:
            v = quivm_dstack_pop(qvm);
            quivm_dstack_push(qvm, v);
            quivm_dstack_push(qvm, qvm->acc);
            qvm->acc = v;
            break;
        case INSN_ROT:
            v = quivm_dstack_pop(qvm);
            w = quivm_dstack_pop(qvm);
            quivm_dstack_push(qvm, v);
            quivm_dstack_push(qvm, qvm->acc);
            qvm->acc = w;
            break;
        case INSN_RTO:
            quivm_rstack_push(qvm, qvm->acc);
            qvm->acc = quivm_dstack_pop(qvm);
            break;
        case INSN_RFROM:
            quivm_dstack_push(qvm, qvm->acc);
            qvm->acc = quivm_rstack_pop(qvm);
            break;
        case INSN_RPEEK:
            quivm_dstack_push(qvm, qvm->acc);
            qvm->acc = quivm_rstack_pop(qvm);
            quivm_rstack_push(qvm, qvm->acc);
            break;
        case INSN_INVL: /* fall through */
        default:
            qvm->pc--; first_pc--;
            err_cond = EX_INVALID_INSN;
            goto check_exception;
        }
        continue;

    check_read_write:
        cyc = qvm->pc - first_pc;
        if (qvm->status & STS_TERMINATED) {
            qvm->cycles += cyc;
            return 0;
        }

        if (qvm->status & STS_HALTED) {
            if (num_cycles > cyc) {
                qvm->cycles += num_cycles;
            } else {
                qvm->cycles += cyc;
            }
            return 1;
        }

    check_branch:
        qvm->cycles += cyc;
        first_pc = qvm->pc;
        if (num_cycles > cyc)
            num_cycles -= cyc;
        else
            break;

        /* Detect stack overflow
         * Overflows are detected even if the last
         * instruction halted or terminated the VM
         */
        if (!(qvm->status & STS_EXCEPTION)
            && ((qvm->dsp >= STACK_SIZE - STACK_THRESHOLD)
                || (qvm->rsp >= STACK_SIZE - STACK_THRESHOLD))) {

            err_cond = EX_STACK_OVERFLOW;
            goto check_exception;
        }
        continue;

    check_exception:
        cyc = qvm->pc - first_pc + 1;
        qvm->cycles += cyc;
        if (qvm->status & STS_EXCEPTION) {
            fprintf(stderr, "vm/quivm: on_exception: "
                    "unhandled exception: %d at 0x%08X\n",
                    err_cond, qvm->pc);
            qvm->status |= STS_TERMINATED;
            qvm->termvalue = 1;
            return 0;
        }

        quivm_dstack_push(qvm, qvm->acc);
        qvm->acc = err_cond;

        /* rstack points to the faulting instruction */
        quivm_rstack_push(qvm, qvm->pc);
        qvm->pc = first_pc = INITIAL_PC;
        qvm->status |= STS_EXCEPTION;

        if (num_cycles > cyc)
            num_cycles -= cyc;
        else
            break;
        continue;
    }

    return 1;
}

/* Performs an aligned read at `address`, which must be a multiple
 * of 4-bytes. Returns the value read.
 */
static
uint32_t aligned_read(struct quivm *qvm, uint32_t address)
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
            } else if (qvm->scell < STACK_SIZE) {
                v = qvm->dstack[qvm->scell];
            } else {
                v = -1;
            }
            break;
        case IO_SYS_RSTACK:
            if (qvm->scell == CELL_STACK_POINTER) {
                v = qvm->rsp;
            } else if (qvm->scell < STACK_SIZE) {
                v = qvm->rstack[qvm->scell];
            } else {
                v = -1;
            }
            break;
        case IO_SYS_SELECTOR:
            v = qvm->selector;
            break;
        case IO_SYS_VALUE:
            switch (qvm->selector) {
            case SYS_STATUS:    v = qvm->status; break;
            case SYS_STACKSIZE: v = STACK_SIZE; break;
            case SYS_MEMSIZE:   v = qvm->memsize; break;
            case SYS_CELLSIZE:  v = 4; break;
            case SYS_ID:        v = 0; break; /* TODO: set proper value */
            case SYS_CYCLES:    v=  qvm->cycles; break;
            default: v = -1; break;
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
                qvm->dsp = (uint8_t) v;
            } else if (qvm->scell < STACK_SIZE) {
                qvm->dstack[qvm->scell] = v;
            }
            break;
        case IO_SYS_RSTACK:
            if (qvm->scell == CELL_STACK_POINTER) {
                qvm->rsp = (uint8_t) v;
            } else if (qvm->scell < STACK_SIZE) {
                qvm->rstack[qvm->scell] = v;
            }
            break;
        case IO_SYS_SELECTOR:
            qvm->selector = v;
            break;
        case IO_SYS_VALUE:
            switch (qvm->selector) {
            case SYS_STATUS:
                qvm->status &= ~(STS_EXCEPTION | STS_HALTED);
                qvm->status |= v & STS_HALTED;
                break;
            case SYS_TERMINATE:
                qvm->termvalue = v;
                qvm->status |= STS_TERMINATED;
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

uint8_t quivm_read_byte(struct quivm *qvm, uint32_t address)
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

void quivm_dstack_push(struct quivm *qvm, uint32_t v)
{
    qvm->dstack[qvm->dsp++] = v;
}

void quivm_rstack_push(struct quivm *qvm, uint32_t v)
{
    qvm->rstack[qvm->rsp++] = v;
}

uint32_t quivm_dstack_pop(struct quivm *qvm)
{
    return qvm->dstack[--qvm->dsp];
}

uint32_t quivm_rstack_pop(struct quivm *qvm)
{
    return qvm->rstack[--qvm->rsp];
}
