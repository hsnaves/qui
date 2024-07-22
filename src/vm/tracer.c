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
    struct page *sentinel;          /* the sentinel (only "do_TRACE" code) */
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
static void do_TRACE(struct quivm *qvm, uint64_t data);
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
        pg->code[i] = &do_TRACE;

    memset(pg->data, 0, PAGE_SIZE * sizeof(uint64_t));
    return pg;
}

/* Macros for stack operations */
#define dstack_push(qvm, v) (qvm)->dstack[(qvm)->dsp++] = (v)
#define rstack_push(qvm, v) (qvm)->rstack[(qvm)->rsp++] = (v)
#define dstack_pop(qvm) ((qvm)->dstack[--(qvm)->dsp])
#define rstack_pop(qvm) ((qvm)->rstack[--(qvm)->rsp])

/* Other macros */
#define PROLOGUE(insn) \
    static void do_ ## insn(struct quivm *qvm, uint64_t data)
#define EPILOGUE
#define UNUSED_DATA (void)(data)
#define DISPATCH \
{                                                \
    struct tracer *tr;                           \
    struct page *pg;                             \
    uint32_t address, pg_index, offset;          \
                                                 \
    tr = get_tracer(qvm);                        \
    address = qvm->pc % MEMORY_SIZE;             \
    pg_index = address / PAGE_SIZE;              \
    pg = tr->traced[pg_index];                   \
    offset = address % PAGE_SIZE;                \
    pg->code[offset](qvm, pg->data[offset]);     \
}
#define XT(insn) &do_ ## insn

#define ALL_INSNS(MACRO) \
    MACRO(TRACE) \
    MACRO(EXCEPTION) \
    MACRO(LIT) \
    MACRO(LITS) \
    MACRO(RET) \
    MACRO(JSR) \
    MACRO(JMP) \
    MACRO(JZ) \
    MACRO(EQ0) \
    MACRO(EQ) \
    MACRO(ULT) \
    MACRO(LT) \
    MACRO(AND) \
    MACRO(OR) \
    MACRO(XOR) \
    MACRO(ADD) \
    MACRO(SUB) \
    MACRO(CSEL) \
    MACRO(UMUL) \
    MACRO(UDIV) \
    MACRO(RD) \
    MACRO(WRT) \
    MACRO(RDB) \
    MACRO(WRTB) \
    MACRO(SIGNE) \
    MACRO(SHL) \
    MACRO(USHR) \
    MACRO(SHR) \
    MACRO(NOP) \
    MACRO(DUP) \
    MACRO(DROP) \
    MACRO(SWAP) \
    MACRO(OVER) \
    MACRO(ROT) \
    MACRO(RTO) \
    MACRO(RFROM) \
    MACRO(RGET) \
    MACRO(RSET) \
    MACRO(JSR_CONST) \
    MACRO(JMP_CONST) \
    MACRO(JZ_CONST) \
    MACRO(ADD_CONST) \
    MACRO(RGET_CONST)

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

    DISPATCH;
    return 0;
}


/* Implementation of the trace operations */

PROLOGUE(TRACE)
{
    UNUSED_DATA;
    tracer_trace(get_tracer(qvm), qvm->pc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(EXCEPTION)
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
    DISPATCH;
}
EPILOGUE

PROLOGUE(LIT)
{
    dstack_push(qvm, qvm->acc);
    qvm->acc = (uint32_t) data;
    qvm->pc += (uint32_t) (data >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(LITS)
{
    qvm->acc <<= 7;
    qvm->acc |= (uint32_t) data;
    qvm->pc += (uint32_t) (data >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(RET)
{
    UNUSED_DATA;
    qvm->pc = rstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(JSR)
{
    UNUSED_DATA;
    rstack_push(qvm, ++qvm->pc);
    qvm->pc += qvm->acc;
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(JMP)
{
    UNUSED_DATA;
    qvm->pc += qvm->acc + 1;
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(JZ)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    if (v == 0) qvm->pc += qvm->acc;
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(EQ0)
{
    UNUSED_DATA;
    qvm->acc = (0 == qvm->acc) ? -1 : 0;
    qvm->pc++;
    DISPATCH;
}
EPILOGUE

PROLOGUE(EQ)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v == qvm->acc) ? -1 : 0;
    DISPATCH;
}
EPILOGUE

PROLOGUE(ULT)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v < qvm->acc) ? -1 : 0;
    DISPATCH;
}
EPILOGUE

PROLOGUE(LT)
{
    int32_t v;
    UNUSED_DATA;
    qvm->pc++; v = (int32_t) dstack_pop(qvm);
    qvm->acc = (v < ((int32_t) qvm->acc)) ? -1 : 0;
    DISPATCH;
}
EPILOGUE

PROLOGUE(AND)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc &= v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(OR)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc |= v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(XOR)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc ^= v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(ADD)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc += v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(SUB)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = v - qvm->acc;
    DISPATCH;
}
EPILOGUE

PROLOGUE(CSEL)
{
    uint32_t v, w;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm); w = dstack_pop(qvm);
    qvm->acc = (qvm->acc) ? v : w;
    DISPATCH;
}
EPILOGUE

PROLOGUE(UMUL)
{
    uint64_t dv, dw;
    UNUSED_DATA;

    qvm->pc++;
    dw = (uint64_t) dstack_pop(qvm);
    dv = (uint64_t) qvm->acc;
    dv *= dw;
    dstack_push(qvm, (uint32_t) dv);
    qvm->acc = (uint32_t) (dv >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(UDIV)
{
    uint32_t v;
    UNUSED_DATA;

    /* check for division by zero */
    if (qvm->acc == 0) {
        do_EXCEPTION(qvm, EX_DIVIDE_BY_ZERO);
    } else {
        qvm->pc++; v = dstack_pop(qvm);
        dstack_push(qvm, (v % qvm->acc));
        qvm->acc = v / qvm->acc;
    }
    DISPATCH;
}
EPILOGUE

PROLOGUE(RD)
{
    UNUSED_DATA;
    qvm->pc++;
    qvm->acc = quivm_read(qvm, qvm->acc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(WRT)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    quivm_write(qvm, qvm->acc, v);
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(RDB)
{
    UNUSED_DATA;
    qvm->pc++;
    qvm->acc = (uint32_t) quivm_read_byte(qvm, qvm->acc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(WRTB)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    quivm_write_byte(qvm, qvm->acc, v);
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(SIGNE)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    if (qvm->acc < 32) {
        qvm->acc = 32 - qvm->acc;
        v <<= qvm->acc;
        qvm->acc = (((int32_t) v) >> qvm->acc);
    } else {
        qvm->acc = v;
    }
    DISPATCH;
}
EPILOGUE

PROLOGUE(SHL)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v << (qvm->acc & 0x1F));
    DISPATCH;
}
EPILOGUE

PROLOGUE(USHR)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->acc = (v >> (qvm->acc & 0x1F));
    DISPATCH;
}
EPILOGUE

PROLOGUE(SHR)
{
    int32_t v;
    UNUSED_DATA;
    qvm->pc++; v = (int32_t) dstack_pop(qvm);
    qvm->acc = (uint32_t) (v >> (qvm->acc & 0x1F));
    DISPATCH;
}
EPILOGUE

PROLOGUE(NOP)
{
    qvm->pc += (uint32_t) (data >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(DUP)
{
    UNUSED_DATA;
    qvm->pc++;
    dstack_push(qvm, qvm->acc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(DROP)
{
    UNUSED_DATA;
    qvm->pc++;
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(SWAP)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    dstack_push(qvm, qvm->acc);
    qvm->acc = v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(OVER)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    dstack_push(qvm, v);
    dstack_push(qvm, qvm->acc);
    qvm->acc = v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(ROT)
{
    uint32_t v, w;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm); w = dstack_pop(qvm);
    dstack_push(qvm, v);
    dstack_push(qvm, qvm->acc);
    qvm->acc = w;
    DISPATCH;
}
EPILOGUE

PROLOGUE(RTO)
{
    UNUSED_DATA;
    qvm->pc++; rstack_push(qvm, qvm->acc);
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(RFROM)
{
    UNUSED_DATA;
    qvm->pc++; dstack_push(qvm, qvm->acc);
    qvm->acc = rstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(RGET)
{
    UNUSED_DATA;
    qvm->pc++;
    qvm->acc = qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)];
    DISPATCH;
}
EPILOGUE

PROLOGUE(RSET)
{
    uint32_t v;
    UNUSED_DATA;
    qvm->pc++; v = dstack_pop(qvm);
    qvm->rstack[(uint8_t) (qvm->rsp - 1 - qvm->acc)] = v;
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(JSR_CONST)
{
    qvm->pc += (uint32_t) (data >> 32);
    rstack_push(qvm, qvm->pc);
    qvm->pc = (uint32_t) data;
    DISPATCH;
}
EPILOGUE

PROLOGUE(JMP_CONST)
{
    qvm->pc = (uint32_t) data;
    DISPATCH;
}
EPILOGUE

PROLOGUE(JZ_CONST)
{
    qvm->pc += (uint32_t) (data >> 32);
    if (qvm->acc == 0) qvm->pc = (uint32_t) data;
    qvm->acc = dstack_pop(qvm);
    DISPATCH;
}
EPILOGUE

PROLOGUE(ADD_CONST)
{
    qvm->pc += (uint32_t) (data >> 32);
    qvm->acc += (uint32_t) data;
    DISPATCH;
}
EPILOGUE

PROLOGUE(RGET_CONST)
{
    dstack_push(qvm, qvm->acc);
    qvm->pc += (uint32_t) (data >> 32);
    qvm->acc = qvm->rstack[(uint8_t) (qvm->rsp - 1 - ((uint32_t) data))];
    DISPATCH;
}
EPILOGUE

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
        pg->code[offset] = XT(LITS);
        pg->data[offset] = ((uint64_t) insn) | (1UL << 32UL);
        return;
    }

    if (insn < INSN_REG_BASE) {
        uint32_t addr, max_addr;

        v = (uint32_t) (insn - INSN_LIT_BASE);
        if (v & 0x20) {
            /* sign-extend the literal value */
            v |= ~0x3F;
        }

        addr = address;
        max_addr =  (pg_index + 1) * PAGE_SIZE;
        while (++addr < max_addr) {
            insn = qvm->mem[addr];
            if (!(insn < INSN_LIT_BASE)) break;
            v <<= 7;
            v |= (uint32_t) insn;
        }

        addr++;
        if (insn == INSN_JSR) {
            pg->code[offset] = XT(JSR_CONST);
            v += addr;
        } else if (insn == INSN_JMP) {
            pg->code[offset] = XT(JMP_CONST);
            v += addr;
        } else if (insn == INSN_JZ) {
            pg->code[offset] = XT(JZ_CONST);
            v += addr;
        } else if (insn == INSN_ADD) {
            pg->code[offset] = XT(ADD_CONST);
        } else if (insn == INSN_SUB) {
            pg->code[offset] = XT(ADD_CONST);
            v = -v;
        } else if (insn == INSN_RGET) {
            pg->code[offset] = XT(RGET_CONST);
        } else {
            addr--;
            pg->code[offset] = XT(LIT);
        }
        pg->data[offset] = ((uint64_t) v)
                         | (((uint64_t) (addr - address)) << 32UL);
        return;
    }

    /* process regular instructions */
    pg->data[offset] = (1UL << 32UL);
    switch (insn) {
    case INSN_RET:   pg->code[offset] = XT(RET);     break;
    case INSN_JSR:   pg->code[offset] = XT(JSR);     break;
    case INSN_JMP:   pg->code[offset] = XT(JMP);     break;
    case INSN_JZ:    pg->code[offset] = XT(JZ);      break;
    case INSN_EQ0:   pg->code[offset] = XT(EQ0);     break;
    case INSN_EQ:    pg->code[offset] = XT(EQ);      break;
    case INSN_ULT:   pg->code[offset] = XT(ULT);     break;
    case INSN_LT:    pg->code[offset] = XT(LT);      break;
    case INSN_AND:   pg->code[offset] = XT(AND);     break;
    case INSN_OR:    pg->code[offset] = XT(OR);      break;
    case INSN_XOR:   pg->code[offset] = XT(XOR);     break;
    case INSN_ADD:   pg->code[offset] = XT(ADD);     break;
    case INSN_SUB:   pg->code[offset] = XT(SUB);     break;
    case INSN_CSEL:  pg->code[offset] = XT(CSEL);    break;
    case INSN_UMUL:  pg->code[offset] = XT(UMUL);    break;
    case INSN_UDIV:  pg->code[offset] = XT(UDIV);    break;
    case INSN_RD:    pg->code[offset] = XT(RD);      break;
    case INSN_WRT:   pg->code[offset] = XT(WRT);     break;
    case INSN_RDB:   pg->code[offset] = XT(RDB);     break;
    case INSN_WRTB:  pg->code[offset] = XT(WRTB);    break;
    case INSN_SIGNE: pg->code[offset] = XT(SIGNE);   break;
    case INSN_SHL:   pg->code[offset] = XT(SHL);     break;
    case INSN_USHR:  pg->code[offset] = XT(USHR);    break;
    case INSN_SHR:   pg->code[offset] = XT(SHR);     break;
    case INSN_NOP:   pg->code[offset] = XT(NOP);     break;
    case INSN_DUP:   pg->code[offset] = XT(DUP);     break;
    case INSN_DROP:  pg->code[offset] = XT(DROP);    break;
    case INSN_SWAP:  pg->code[offset] = XT(SWAP);    break;
    case INSN_OVER:  pg->code[offset] = XT(OVER);    break;
    case INSN_ROT:   pg->code[offset] = XT(ROT);     break;
    case INSN_RTO:   pg->code[offset] = XT(RTO);     break;
    case INSN_RFROM: pg->code[offset] = XT(RFROM);   break;
    case INSN_RGET:  pg->code[offset] = XT(RGET);    break;
    case INSN_RSET:  pg->code[offset] = XT(RSET);    break;
    case INSN_INVL: /* fall through */
    default:
        pg->code[offset] = XT(EXCEPTION);
        pg->data[offset] = EX_INVALID_INSN;
        break;
    }
}
