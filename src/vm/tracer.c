#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "vm/quivm.h"
#include "vm/internal.h"

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
    struct page **traced;           /* traced pages */
    struct page *free;              /* free pages (linked list) */
    struct node *allocated;         /* allocated pages (linked list) */
    struct page *sentinel;          /* the sentinel (only "do_trace" code) */
    int memory_error;               /* indicates lack of memory */
};

/* Helpful macros */
#define get_tracer(qvm) \
    ((struct tracer *) (((char *) (qvm)) + offsetof(struct quivm, tracer)))

#define get_quivm(tr) \
    ((struct quivm *) (((char *) (tr)) - offsetof(struct quivm, tracer)))

/* Static declarations */
static int tracer_allocate(struct tracer *tr);
static struct page *tracer_get_free_page(struct tracer *tr);
static void do_trace(struct quivm *qvm, uint64_t data);
static void tracer_trace(struct tracer *tr, uint32_t address);

/* Functions */

int tracer_init(struct quivm *qvm)
{
    struct tracer *tr;
    uint32_t i;

    tr = get_tracer(qvm);
    tr->traced = NULL;
    tr->free = NULL;
    tr->allocated = NULL;
    tr->sentinel = NULL;
    tr->memory_error = 0;

    tr->traced = (struct page **) malloc(NUM_PAGES * sizeof(struct page *));
    if (!tr->traced) {
        fprintf(stderr, "vm/tracer: init: memory exhausted\n");
        return 1;
    }
    memset(tr->traced, 0, NUM_PAGES * sizeof(struct page *));

    if (tracer_allocate(tr)) {
        tracer_destroy(qvm);
        fprintf(stderr, "vm/tracer: init: could not reserve pages\n");
        return 1;
    }

    tr->sentinel = tracer_get_free_page(tr);
    for (i = 0; i < NUM_PAGES; i++)
        tr->traced[i] = tr->sentinel;

    return 0;
}

void tracer_destroy(struct quivm *qvm)
{
    struct tracer *tr;
    tr = get_tracer(qvm);

    if (tr->traced) free(tr->traced);
    tr->traced = NULL;

    while (tr->allocated) {
        struct node *next;
        next = tr->allocated->next;
        free(tr->allocated);
        tr->allocated = next;
    }
}

void tracer_invalidate(struct quivm *qvm, uint32_t istart, uint32_t iend)
{
    struct tracer *tr;
    uint32_t start_pg_index, end_pg_index;
    uint32_t pg_index;

    tr = get_tracer(qvm);
    start_pg_index = istart / PAGE_SIZE;
    end_pg_index = 1 + ((iend - 1) / PAGE_SIZE);

    for (pg_index = start_pg_index; pg_index < end_pg_index; pg_index++) {
        struct page *pg = tr->traced[pg_index];
        if (pg != tr->sentinel) {
            struct node *n = (struct node *) pg;
            n->next = (struct node *) tr->free;
            tr->free = pg;
            tr->traced[pg_index] = tr->sentinel;
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
    uint32_t i;

    if (!tr->free) {
        if (tracer_allocate(tr)) {
            struct quivm *qvm;
            tr->memory_error = 1;
            qvm = get_quivm(tr);
            quivm_raise(qvm);
            return NULL; /* should never return */
        }
    }

    pg = tr->free;
    n = (struct node *) pg;
    tr->free = (struct page *) n->next;

    /* Initialize the page with code for tracing */
    for (i = 0; i < PAGE_SIZE; i++)
        pg->code[i] = &do_trace;

    memset(pg->data, 0, PAGE_SIZE * sizeof(uint64_t));
    return pg;
}

#define DISPATCH(qvm) \
{                                                \
    struct tracer *tr;                           \
    struct page *pg;                             \
    uint32_t address, pg_index, offset;          \
                                                 \
    tr = get_tracer(qvm);                        \
    address = (qvm)->pc % MEMORY_SIZE;           \
    pg_index = address / PAGE_SIZE;              \
    pg = tr->traced[pg_index];                   \
    offset = address % PAGE_SIZE;                \
    pg->code[offset]((qvm), pg->data[offset]);   \
}

int tracer_run(struct quivm *qvm)
{
    /* check if it has terminated */
    if (!(qvm->status & STS_RUNNING))
        return 0;

    qvm->status |= STS_JMPBUF;
    if (setjmp(((jmp_buf *) qvm->jmpbuf)[0])) {
        struct tracer *tr;
        qvm->status &= ~STS_JMPBUF;
        tr = get_tracer(qvm);
        if (tr->memory_error) return 1;
        return tracer_run(qvm);
    }

    DISPATCH(qvm);
    return 0;
}


/* Implementation of the trace operations */

static
void do_trace(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    tracer_trace((struct tracer *) qvm->tracer, qvm->pc);
    DISPATCH(qvm);
}

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
    } else {
        dstack_push(qvm, qvm->acc);
        qvm->acc = err_cond;

        /* rstack points to the faulting instruction */
        rstack_push(qvm, qvm->pc);
        qvm->pc = INITIAL_PC;
        qvm->status &= ~STS_OKAY;
    }
    DISPATCH(qvm);
}

static
void do_lit(struct quivm *qvm, uint64_t data)
{
    dstack_push(qvm, qvm->acc);
    qvm->acc = (uint32_t) data;
    qvm->pc += (uint32_t) (data >> 32);
    DISPATCH(qvm);
}

static
void do_lits(struct quivm *qvm, uint64_t data)
{
    qvm->acc <<= 7;
    qvm->acc |= (uint32_t) data;
    qvm->pc += (uint32_t) (data >> 32);
    DISPATCH(qvm);
}

static
void do_ret(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc = rstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_jsr(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    rstack_push(qvm, ++qvm->pc);
    qvm->pc += qvm->acc;
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_jmp(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc += qvm->acc + 1;
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_jz(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    if (v == 0) qvm->pc += qvm->acc;
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_eq0(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->acc = (0 == qvm->acc) ? -1 : 0;
    qvm->pc++;
    DISPATCH(qvm);
}

static
void do_eq(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v == qvm->acc) ? -1 : 0;
    DISPATCH(qvm);
}

static
void do_ult(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v < qvm->acc) ? -1 : 0;
    DISPATCH(qvm);
}

static
void do_lt(struct quivm *qvm, uint64_t data)
{
    int32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = (int32_t) dstack_pop(qvm);
    qvm->acc = (v < ((int32_t) qvm->acc)) ? -1 : 0;
    DISPATCH(qvm);
}

static
void do_and(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc &= v;
    DISPATCH(qvm);
}

static
void do_or(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc |= v;
    DISPATCH(qvm);
}

static
void do_xor(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc ^= v;
    DISPATCH(qvm);
}

static
void do_add(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc += v;
    DISPATCH(qvm);
}

static
void do_sub(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = v - qvm->acc;
    DISPATCH(qvm);
}

static
void do_csel(struct quivm *qvm, uint64_t data)
{
    uint32_t v, w;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm); w = dstack_pop(qvm);
    qvm->acc = (qvm->acc) ? v : w;
    DISPATCH(qvm);
}

static
void do_umul(struct quivm *qvm, uint64_t data)
{
    uint64_t dv, dw;
    (void)(data); /* UNUSED */

    qvm->pc++;
    dw = (uint64_t) dstack_pop(qvm);
    dv = (uint64_t) qvm->acc;
    dv *= dw;
    dstack_push(qvm, (uint32_t) dv);
    qvm->acc = (uint32_t) (dv >> 32);
    DISPATCH(qvm);
}

static
void do_udiv(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */

    /* check for division by zero */
    if (qvm->acc == 0) {
        do_exception(qvm, EX_DIVIDE_BY_ZERO);
    } else {
        qvm->pc++; v = dstack_pop(qvm);
        dstack_push(qvm, (v % qvm->acc));
        qvm->acc = v / qvm->acc;
    }
    DISPATCH(qvm);
}

static
void do_rd(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = quivm_read(qvm, qvm->acc);
    DISPATCH(qvm);
}

static
void do_wrt(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    quivm_write(qvm, qvm->acc, v);
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_rdb(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = (uint32_t) quivm_read_byte(qvm, qvm->acc);
    DISPATCH(qvm);
}

static
void do_wrtb(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    quivm_write_byte(qvm, qvm->acc, v);
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_signe(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    if (qvm->acc < 32) {
        qvm->acc = 32 - qvm->acc;
        v <<= qvm->acc;
        qvm->acc = (((int32_t) v) >> qvm->acc);
    } else {
        qvm->acc = v;
    }
    DISPATCH(qvm);
}

static
void do_shl(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v << (qvm->acc & 0x1F));
    DISPATCH(qvm);
}

static
void do_ushr(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v >> (qvm->acc & 0x1F));
    DISPATCH(qvm);
}

static
void do_shr(struct quivm *qvm, uint64_t data)
{
    int32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = (int32_t) dstack_pop(qvm);
    qvm->acc = (uint32_t) (v >> (qvm->acc & 0x1F));
    DISPATCH(qvm);
}

static
void do_nop(struct quivm *qvm, uint64_t data)
{
    qvm->pc += (uint32_t) (data >> 32);
    DISPATCH(qvm);
}

static
void do_dup(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    dstack_push(qvm, qvm->acc);
    DISPATCH(qvm);
}

static
void do_drop(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_swap(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    dstack_push(qvm, qvm->acc);
    qvm->acc = v;
    DISPATCH(qvm);
}

static
void do_over(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    dstack_push(qvm, v);
    dstack_push(qvm, qvm->acc);
    qvm->acc = v;
    DISPATCH(qvm);
}

static
void do_rot(struct quivm *qvm, uint64_t data)
{
    uint32_t v, w;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm); w = dstack_pop(qvm);
    dstack_push(qvm, v);
    dstack_push(qvm, qvm->acc);
    qvm->acc = w;
    DISPATCH(qvm);
}

static
void do_rto(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++; rstack_push(qvm, qvm->acc);
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_rfrom(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++; dstack_push(qvm, qvm->acc);
    qvm->acc = rstack_pop(qvm);
    DISPATCH(qvm);
}

static
void do_rget(struct quivm *qvm, uint64_t data)
{
    (void)(data); /* UNUSED */
    qvm->pc++;
    qvm->acc = qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)];
    DISPATCH(qvm);
}

static
void do_rset(struct quivm *qvm, uint64_t data)
{
    uint32_t v;
    (void)(data); /* UNUSED */
    qvm->pc++; v = dstack_pop(qvm);
    qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)] = v;
    qvm->acc = dstack_pop(qvm);
    DISPATCH(qvm);
}


/* Traces the code in a given address. */
static
void tracer_trace(struct tracer *tr, uint32_t address)
{
    struct quivm *qvm;
    struct page *pg;
    uint32_t pg_index, offset, v;
    uint8_t insn;

    qvm = get_quivm(tr);
    pg_index = address / PAGE_SIZE;
    pg = tr->traced[pg_index];
    if (pg == tr->sentinel) {
        pg = tracer_get_free_page(tr);
        tr->traced[pg_index] = pg;
    }
    offset = address % PAGE_SIZE;

    insn = qvm->mem[address];
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
