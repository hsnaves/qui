#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "quivm.h"

/* Constants for the tracer */
#define PAGE_SIZE                     4096
#define NUM_PAGES  (MEMORY_SIZE/PAGE_SIZE)
#define NUM_ALLOC_PAGES                 16

/* Data structures and types */

/* Callback function to run the traced code. */
typedef void (*run_trace_cb)(struct quivm *qvm, uint64_t data);

/* Structure representing a page of the traced code */
struct page {
    run_trace_cb code[PAGE_SIZE];   /* translated code */
    uint64_t data[PAGE_SIZE];       /* translated code data */
};

/* Structure for representing a linked list */
struct node {
    struct node *next;              /* the next node */
};

/* Structure for the tracer */
struct tracer {
    struct page *traced[NUM_PAGES]; /* traced pages */
    struct page *free;              /* free pages */
    struct node *allocated;         /* allocated pages */
    struct quivm *qvm;              /* pointer to the VM */
    int memory_error;               /* indicates lack of memory */
};

/* Static declarations */
static int tracer_allocate(struct tracer *tr);
static struct page *tracer_get_free_page(struct tracer *tr);
static void tracer_trace(struct tracer *tr,
                         struct page *pg, uint32_t offset);


/* Functions */

/* Destroys the tracer and release the resources. */
void tracer_destroy(struct quivm *qvm)
{
    struct tracer *tr;

    tr = (struct tracer *) qvm->tracer;
    qvm->tracer = NULL;

    if (tr) {
        while (tr->allocated) {
            struct node *next;
            next = tr->allocated->next;
            free(tr->allocated);
            tr->allocated = next;
        }

        free(tr);
    }
}

/* Initializes the tracer.
 * Returns zero on success.
 */
int tracer_init(struct quivm *qvm)
{
    struct tracer *tr;

    tr = (struct tracer *) malloc(sizeof(struct tracer));
    if (!tr) {
        fprintf(stderr, "vm/tracer: init: memory exhausted\n");
        return 1;
    }

    memset(tr->traced, 0, NUM_PAGES * sizeof(struct page *));
    tr->free = NULL;
    tr->allocated = NULL;
    tr->qvm = qvm;
    tr->memory_error = 0;
    qvm->tracer = tr;

    if (tracer_allocate(tr)) {
        tracer_destroy(qvm);
        fprintf(stderr, "vm/tracer: init: could not reserve pages\n");
        return 1;
    }

    return 0;
}

/* Flushes the pages of the tracer cache */
void tracer_flush(struct quivm *qvm,
                  uint32_t start_address, uint32_t end_address)
{
    struct tracer *tr;
    uint32_t start_pg_index, end_pg_index;
    uint32_t pg_index;

    tr = (struct tracer *) qvm->tracer;
    start_pg_index = start_address / PAGE_SIZE;
    end_pg_index = 1 + (end_address / PAGE_SIZE);

    for (pg_index = start_pg_index; pg_index != end_pg_index; pg_index++) {
        struct page *pg = tr->traced[pg_index];
        if (pg) {
            struct node *n = (struct node *) pg;
            n->next = (struct node *) tr->free;
            tr->free = pg;
            tr->traced[pg_index] = NULL;
        }
    }
}

/* Allocates pages for the tracer.
 * Returns zero on success.
 */
static
int tracer_allocate(struct tracer *tr)
{
    struct node *allocated;
    struct page *pages;
    size_t size;
    int i;

    size = sizeof(struct node) + NUM_ALLOC_PAGES * sizeof(struct page);
    allocated = (struct node *) malloc(size);

    if (!allocated) {
        fprintf(stderr, "vm/tracer: allocate: memory exhausted\n");
        return 1;
    }

    pages = (struct page *) &allocated[1];
    for (i = 0; i < NUM_ALLOC_PAGES; i++) {
        struct node *n;
        n = (struct node *) &pages[i];
        n->next = (struct node *) tr->free;
        tr->free = (struct page *) n;
    }

    allocated->next = tr->allocated;
    tr->allocated = allocated;

    return 0;
}

/* Obtains a free page from the tracer
 * Always returns a valid page.
 */
static
struct page *tracer_get_free_page(struct tracer *tr)
{
    struct page *pg;
    struct node *n;

    if (!tr->free) {
        if (tracer_allocate(tr)) {
            tr->memory_error = 1;
            quivm_raise(tr->qvm);
            return NULL; /* should never return */
        }
    }

    pg = tr->free;
    n = (struct node *) pg;
    tr->free = (struct page *) n->next;

    memset(pg->code, 0, PAGE_SIZE * sizeof(run_trace_cb));
    memset(pg->data, 0, PAGE_SIZE * sizeof(uint64_t));
    return pg;
}

/* Runs the VM using the tracer */
int tracer_run(struct quivm *qvm)
{
    struct tracer *tr;
    struct page *last_pg;
    uint32_t last_pg_index;

    /* check if it has terminated */
    if (!(qvm->status & STS_RUNNING))
        return 0;

    qvm->status |= STS_JMPBUF;
    if (setjmp(((jmp_buf *) qvm->jmpbuf)[0])) {
        qvm->status &= ~STS_JMPBUF;
        tr = (struct tracer *) qvm->tracer;
        if (tr->memory_error) return 1;
        return tracer_run(qvm);
    }

    tr = (struct tracer *) qvm->tracer;

    last_pg_index = NUM_PAGES;
    last_pg = NULL;
    while (1) {
        struct page *pg;
        uint32_t pg_index, offset;
        run_trace_cb runcb;

        pg_index = qvm->pc / PAGE_SIZE;
        if (pg_index != last_pg_index) {
            pg = tr->traced[pg_index];
            if (!pg) {
                pg = tracer_get_free_page(tr);
                tr->traced[pg_index] = pg;
            }

            last_pg_index = pg_index;
            last_pg = pg;
        } else {
            pg = last_pg;
        }

        offset = qvm->pc % PAGE_SIZE;
        runcb = pg->code[offset];
        if (!runcb) {
            tracer_trace(tr, pg, offset);
            runcb = pg->code[offset];
        }

        (*runcb)(qvm, pg->data[offset]);
    }

    return 0;
}

/* Implementation of the trace operations */
static
void do_exception(struct quivm *qvm, uint64_t data)
{
    int err_cond;

    err_cond = (int) data;
    if (!(qvm->status & STS_OKAY)) {
        fprintf(stderr, "vm/quivm: on_exception: "
                "unhandled exception: %d at 0x%08X\n",
                err_cond, qvm->pc);
        quivm_terminate(qvm, err_cond);
        quivm_raise(qvm);
        return;
    }

    quivm_dstack_push(qvm, qvm->acc);
    qvm->acc = err_cond;

    /* rstack points to the faulting instruction */
    quivm_rstack_push(qvm, qvm->pc);
    qvm->pc = INITIAL_PC;
    qvm->status &= ~STS_OKAY;
}

static
void do_lit(struct quivm *qvm, uint64_t data)
{
    quivm_dstack_push(qvm, qvm->acc);
    qvm->acc = (uint32_t) data;
    qvm->pc += (uint32_t) (data >> 32);
}

static
void do_lits(struct quivm *qvm, uint64_t data)
{
    qvm->acc <<= 7;
    qvm->acc |= (uint32_t) data;
    qvm->pc += (uint32_t) (data >> 32);
}

static
void do_ret(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc = quivm_rstack_pop(qvm);
}

static
void do_jsr(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    quivm_rstack_push(qvm, ++qvm->pc);
    qvm->pc += qvm->acc;
    qvm->acc = quivm_dstack_pop(qvm);
}

static
void do_jmp(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc += qvm->acc + 1;
    qvm->acc = quivm_dstack_pop(qvm);
}

static
void do_jz(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    if (v == 0) qvm->pc += qvm->acc;
    qvm->acc = quivm_dstack_pop(qvm);
}

static
void do_eq0(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->acc = (0 == qvm->acc) ? -1 : 0;
    qvm->pc++;
}

static
void do_eq(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc = (v == qvm->acc) ? -1 : 0;
}

static
void do_ult(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc = (v < qvm->acc) ? -1 : 0;
}

static
void do_lt(struct quivm *qvm, uint64_t data)
{
    int32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = (int32_t) quivm_dstack_pop(qvm);
    qvm->acc = (v < ((int32_t) qvm->acc)) ? -1 : 0;
}

static
void do_and(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc &= v;
}

static
void do_or(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc |= v;
}

static
void do_xor(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc ^= v;
}

static
void do_add(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc += v;
}

static
void do_sub(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc = v - qvm->acc;
}

static
void do_csel(struct quivm *qvm, uint64_t data)
{
    uint32_t v, w;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm); w = quivm_dstack_pop(qvm);
    qvm->acc = (qvm->acc) ? v : w;
}

static
void do_umul(struct quivm *qvm, uint64_t data)
{
    uint64_t dv, dw;
    (void)(data); /* UNUSED */

    qvm->pc++;
    dw = (uint64_t) quivm_dstack_pop(qvm);
    dv = (uint64_t) qvm->acc;
    dv *= dw;
    quivm_dstack_push(qvm, (uint32_t) dv);
    qvm->acc = (uint32_t) (dv >> 32);
}

static
void do_udiv(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */

    /* check for division by zero */
    if (qvm->acc == 0) {
        do_exception(qvm, EX_DIVIDE_BY_ZERO);
        return;
    }
    qvm->pc++; v = quivm_dstack_pop(qvm);
    quivm_dstack_push(qvm, (v % qvm->acc));
    qvm->acc = v / qvm->acc;
}

static
void do_rd(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = quivm_read(qvm, qvm->acc);
}

static
void do_wrt(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    quivm_write(qvm, qvm->acc, v);
    qvm->acc = quivm_dstack_pop(qvm);
}

static
void do_rdb(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = (uint32_t) quivm_read_byte(qvm, qvm->acc);
}

static
void do_wrtb(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    quivm_write_byte(qvm, qvm->acc, v);
    qvm->acc = quivm_dstack_pop(qvm);
}

static
void do_signe(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    if (qvm->acc < 32) {
        qvm->acc = 32 - qvm->acc;
        v <<= qvm->acc;
        qvm->acc = (((int32_t) v) >> qvm->acc);
    } else {
        qvm->acc = v;
    }
}

static
void do_shl(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc = (v << (qvm->acc & 0x1F));
}

static
void do_ushr(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->acc = (v >> (qvm->acc & 0x1F));
}

static
void do_shr(struct quivm *qvm, uint64_t data)
{
    int32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = (int32_t) quivm_dstack_pop(qvm);
    qvm->acc = (uint32_t) (v >> (qvm->acc & 0x1F));
}

static
void do_nop(struct quivm *qvm, uint64_t data)
{
    qvm->pc += (uint32_t) (data >> 32);
}

static
void do_dup(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    quivm_dstack_push(qvm, qvm->acc);
}

static
void do_drop(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = quivm_dstack_pop(qvm);
}

static
void do_swap(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    quivm_dstack_push(qvm, qvm->acc);
    qvm->acc = v;
}

static
void do_over(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    quivm_dstack_push(qvm, v);
    quivm_dstack_push(qvm, qvm->acc);
    qvm->acc = v;
}

static
void do_rot(struct quivm *qvm, uint64_t data)
{
    uint32_t v, w;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm); w = quivm_dstack_pop(qvm);
    quivm_dstack_push(qvm, v);
    quivm_dstack_push(qvm, qvm->acc);
    qvm->acc = w;
}

static
void do_rto(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++; quivm_rstack_push(qvm, qvm->acc);
    qvm->acc = quivm_dstack_pop(qvm);
}

static
void do_rfrom(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++; quivm_dstack_push(qvm, qvm->acc);
    qvm->acc = quivm_rstack_pop(qvm);
}

static
void do_rget(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)];
}

static
void do_rset(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = quivm_dstack_pop(qvm);
    qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)] = v;
    qvm->acc = quivm_dstack_pop(qvm);
}


/* Traces the code in a given page, and at given offset. */
static
void tracer_trace(struct tracer *tr, struct page *pg, uint32_t offset)
{
    struct quivm *qvm;
    uint32_t v;
    uint8_t insn;

    qvm = tr->qvm;
    if (qvm->pc < MEMORY_SIZE) {
        insn = qvm->mem[qvm->pc];
    } else {
        /* prevent the VM from executing outside the memory space */
        insn = INSN_INVL;
    }

    if (insn < INSN_LIT_BASE) {
        pg->code[offset] = &do_lits;
        pg->data[offset] = ((uint64_t) insn) | (1UL << 32UL);
        return;
    }

    if (insn < INSN_REG_BASE) {
        v = (uint32_t) (insn - INSN_LIT_BASE);
        if (v & 0x20) {
            /* sign-extend the literal value */
            v |= ~0x3F;
        }
        pg->code[offset] = &do_lit;
        pg->data[offset] = ((uint64_t) v) | (1UL << 32UL);
        return;
    }

    /* process regular instructions */
    pg->data[offset] = (1UL << 32UL);
    switch (insn) {
    case INSN_RET:   pg->code[offset] = &do_ret;     break;
    case INSN_JSR:   pg->code[offset] = &do_jsr;     break;
    case INSN_JMP:   pg->code[offset] = &do_jmp;     break;
    case INSN_JZ:    pg->code[offset] = &do_jz;      break;
    case INSN_EQ0:   pg->code[offset] = &do_eq0;     break;
    case INSN_EQ:    pg->code[offset] = &do_eq;      break;
    case INSN_ULT:   pg->code[offset] = &do_ult;     break;
    case INSN_LT:    pg->code[offset] = &do_lt;      break;
    case INSN_AND:   pg->code[offset] = &do_and;     break;
    case INSN_OR:    pg->code[offset] = &do_or;      break;
    case INSN_XOR:   pg->code[offset] = &do_xor;     break;
    case INSN_ADD:   pg->code[offset] = &do_add;     break;
    case INSN_SUB:   pg->code[offset] = &do_sub;     break;
    case INSN_CSEL:  pg->code[offset] = &do_csel;    break;
    case INSN_UMUL:  pg->code[offset] = &do_umul;    break;
    case INSN_UDIV:  pg->code[offset] = &do_udiv;    break;
    case INSN_RD:    pg->code[offset] = &do_rd;      break;
    case INSN_WRT:   pg->code[offset] = &do_wrt;     break;
    case INSN_RDB:   pg->code[offset] = &do_rdb;     break;
    case INSN_WRTB:  pg->code[offset] = &do_wrtb;    break;
    case INSN_SIGNE: pg->code[offset] = &do_signe;   break;
    case INSN_SHL:   pg->code[offset] = &do_shl;     break;
    case INSN_USHR:  pg->code[offset] = &do_ushr;    break;
    case INSN_SHR:   pg->code[offset] = &do_shr;     break;
    case INSN_NOP:   pg->code[offset] = &do_nop;     break;
    case INSN_DUP:   pg->code[offset] = &do_dup;     break;
    case INSN_DROP:  pg->code[offset] = &do_drop;    break;
    case INSN_SWAP:  pg->code[offset] = &do_swap;    break;
    case INSN_OVER:  pg->code[offset] = &do_over;    break;
    case INSN_ROT:   pg->code[offset] = &do_rot;     break;
    case INSN_RTO:   pg->code[offset] = &do_rto;     break;
    case INSN_RFROM: pg->code[offset] = &do_rfrom;   break;
    case INSN_RGET:  pg->code[offset] = &do_rget;    break;
    case INSN_RSET:  pg->code[offset] = &do_rset;    break;
    case INSN_INVL: /* fall through */
    default:
        pg->code[offset] = &do_exception;
        pg->data[offset] = EX_INVALID_INSN;
        break;
    }
}
