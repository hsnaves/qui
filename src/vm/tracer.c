#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "vm/quivm.h"
#include "vm/internal.h"

/* The dispatch methods are
 * 0 -> switch
 * 1 -> computed gotos (threaded)
 * 2 -> functions with tail call (threaded)
 */
#ifndef DISPATCH_METHOD
#define DISPATCH_METHOD 1
#endif

/* computed gotos are only used when __GNUC__ is defined */
#if !defined(__GNUC__) && (DISPATCH_METHOD == 1)
#define DISPATCH_METHOD 0
#endif

/* Constants for the tracer */
#define PAGE_SIZE                     4096
#define NUM_PAGES  (MEMORY_SIZE/PAGE_SIZE)
#define NUM_ALLOC_PAGES                 16

/* Constants for other instructions */
#define INSN_BASE                     0xB0
#define INSN_TRACE                    0xB0
#define INSN_EXCEPTION                0xB1
#define INSN_JSR_CONST                0xB2
#define INSN_JMP_CONST                0xB3
#define INSN_JZ_CONST                 0xB4
#define INSN_ADD_CONST                0xB5
#define INSN_RGET_CONST               0xB6
#define INSN_RSET_CONST               0xB7
#define INSN_TWODUP                   0xB8
#define INSN_LIT                      0xBE
#define INSN_LITS                     0xBF
#define NUM_INSN ((INSN_RSET+1)-INSN_BASE)

/* Macro for list of instructions */
#define INSN_TABLE(MACRO) \
    MACRO(TRACE) \
    MACRO(EXCEPTION) \
    MACRO(JSR_CONST) \
    MACRO(JMP_CONST) \
    MACRO(JZ_CONST) \
    MACRO(ADD_CONST) \
    MACRO(RGET_CONST) \
    MACRO(RSET_CONST) \
    MACRO(TWODUP) \
    MACRO(TRACE) \
    MACRO(TRACE) \
    MACRO(TRACE) \
    MACRO(TRACE) \
    MACRO(TRACE) \
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
    MACRO(RSET)

#if DISPATCH_METHOD == 0 /* switch */

/* Callback function to run the traced code. */
typedef uint8_t trace_code;

/* Macros for VM registers and memory */
#define r_pc     pc
#define r_acc    acc
#define r_dsp    dsp
#define r_rsp    rsp
#define m_dstack dstack
#define m_rstack rstack

/* Other macros */
#define PROLOGUE(insn) \
    case (INSN_ ## insn - INSN_BASE): \
    goto do_ ## insn; do_ ## insn:
#define EPILOGUE        break;
#define UNUSED_DATA
#define INVOKE(insn, d) data = d; goto do_ ## insn
#define CTABLE(insn)    INSN_ ## insn,
#define DISPATCH        continue;
#define FLUSH \
    qvm->pc = pc;   qvm->acc = acc; \
    qvm->dsp = dsp; qvm->rsp = rsp

#elif DISPATCH_METHOD == 1 /* computed gotos */

/* Callback function to run the traced code. */
typedef void *trace_code;

/* Macros for VM registers and memory */
#define r_pc     pc
#define r_acc    acc
#define r_dsp    dsp
#define r_rsp    rsp
#define m_dstack dstack
#define m_rstack rstack

/* Other macros */
#define PROLOGUE(insn)  do_ ## insn:
#define EPILOGUE
#define UNUSED_DATA
#define INVOKE(insn, d) data = d; goto do_ ## insn
#define CTABLE(insn)    &&do_ ## insn,
#define DISPATCH \
{                                                \
    struct tracer *tr;                           \
    struct page *pg;                             \
    uint32_t address, pg_index, offset;          \
                                                 \
    tr = get_tracer(qvm);                        \
    address = r_pc % MEMORY_SIZE;                \
    pg_index = address / PAGE_SIZE;              \
    pg = tr->traced[pg_index];                   \
    offset = address % PAGE_SIZE;                \
    data = pg->data[offset];                     \
    goto *pg->code[offset];                      \
}
#define FLUSH \
    qvm->pc = pc;   qvm->acc = acc; \
    qvm->dsp = dsp; qvm->rsp = rsp

#elif DISPATCH_METHOD == 2 /* functions (tail call) */

/* Callback function to run the traced code. */
typedef void (*trace_code)(struct quivm *qvm, uint64_t data);

/* Macros for VM registers and memory */
#define r_pc     qvm->pc
#define r_acc    qvm->acc
#define r_dsp    qvm->dsp
#define r_rsp    qvm->rsp
#define m_dstack qvm->dstack
#define m_rstack qvm->rstack

/* Other macros */
#define PROLOGUE(insn) \
    static void do_ ## insn(struct quivm *qvm, uint64_t data)
#define EPILOGUE
#define UNUSED_DATA     (void)(data)
#define INVOKE(insn, d) do_ ## insn(qvm, d)
#define CTABLE(insn)    &do_ ## insn,
#define DISPATCH \
{                                                \
    struct tracer *tr;                           \
    struct page *pg;                             \
    uint32_t address, pg_index, offset;          \
                                                 \
    tr = get_tracer(qvm);                        \
    address = r_pc % MEMORY_SIZE;                \
    pg_index = address / PAGE_SIZE;              \
    pg = tr->traced[pg_index];                   \
    offset = address % PAGE_SIZE;                \
    pg->code[offset](qvm, pg->data[offset]);     \
}
#define FLUSH

#else /* DISPATCH_METHOD > 2 */
#error "invalid dispatch method"
#endif

/* Macros for stack operations */
#define dstack_push(v) m_dstack[r_dsp++] = (v)
#define rstack_push(v) m_rstack[r_rsp++] = (v)
#define dstack_pop() m_dstack[--r_dsp]
#define rstack_pop() m_rstack[--r_rsp]


/* Data structures and types */

/* Structure representing a page of the traced code */
struct page {
    trace_code code[PAGE_SIZE];     /* translated code */
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
    struct page *sentinel;          /* the sentinel (only "TRACE" code) */
    trace_code *ctable;             /* table of codes */
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
static void tracer_populate_ctable(struct tracer *tr);
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
    tr->ctable = NULL;
    tr->memory_error = 0;

    tr->traced = (struct page **) malloc(NUM_PAGES * sizeof(struct page *));
    tr->ctable = (trace_code *) malloc(NUM_INSN * sizeof(trace_code));
    if (!tr->traced || !tr->ctable) {
        fprintf(stderr, "vm/tracer: init: memory exhausted\n");
        return 1;
    }
    memset(tr->traced, 0, NUM_PAGES * sizeof(struct page *));
    memset(tr->ctable, 0, NUM_INSN * sizeof(trace_code));

    if (tracer_allocate(tr)) {
        tracer_destroy(qvm);
        fprintf(stderr, "vm/tracer: init: could not reserve pages\n");
        return 1;
    }

    tracer_populate_ctable(tr);
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

    if (tr->ctable) free(tr->ctable);
    tr->ctable = NULL;

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
    start_pg_index = (istart % MEMORY_SIZE) / PAGE_SIZE;
    end_pg_index = 1 + (((iend - 1) % MEMORY_SIZE) / PAGE_SIZE);

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
        pg->code[i] = tr->ctable[INSN_TRACE - INSN_BASE];

    memset(pg->data, 0, PAGE_SIZE * sizeof(uint64_t));
    return pg;
}

int tracer_run(struct quivm *qvm)
{
#if DISPATCH_METHOD != 2
    uint32_t pc, acc;
    uint32_t *dstack, *rstack;
    uint64_t data;
    uint8_t dsp, rsp;
#endif
#if DISPATCH_METHOD == 1
    static trace_code ctable[] = {
        INSN_TABLE(CTABLE)
    };
    if (!get_tracer(qvm)->ctable[0]) {
        memcpy(get_tracer(qvm)->ctable, ctable, sizeof(ctable));
        return 0;
    }
#endif

    /* check if it has terminated */
    if (!(qvm->status & STS_RUNNING))
        return 0;

    qvm->status |= STS_JMPBUF;
    if (setjmp(((jmp_buf *) qvm->jmpbuf)[0])) {
        qvm->status &= ~STS_JMPBUF;
        if (get_tracer(qvm)->memory_error) return 1;
        return tracer_run(qvm);
    }

#if DISPATCH_METHOD == 0
    pc = qvm->pc;
    acc = qvm->acc;
    dstack = qvm->dstack;
    rstack = qvm->rstack;
    dsp = qvm->dsp;
    rsp = qvm->rsp;

    while (1) {
        struct tracer *tr;
        struct page *pg;
        uint32_t address, pg_index, offset;

        tr = get_tracer(qvm);
        address = r_pc % MEMORY_SIZE;
        pg_index = address / PAGE_SIZE;
        pg = tr->traced[pg_index];
        offset = address % PAGE_SIZE;
        data = pg->data[offset];
        switch (pg->code[offset] - INSN_BASE) {
#elif DISPATCH_METHOD == 1
    pc = qvm->pc;
    acc = qvm->acc;
    dstack = qvm->dstack;
    rstack = qvm->rstack;
    dsp = qvm->dsp;
    rsp = qvm->rsp;
    DISPATCH;
#elif DISPATCH_METHOD == 2
    DISPATCH;
    return 0;
}

#define DECLARATION(insn) \
    static void do_ ## insn(struct quivm *qvm, uint64_t data);
INSN_TABLE(DECLARATION)

#endif /* DISPATCH_METHOD == 2 */

/* Implementation of the trace operations */
PROLOGUE(TRACE)
{
    UNUSED_DATA;
    FLUSH;
    tracer_trace(get_tracer(qvm), r_pc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(EXCEPTION)
{
    int err_cond;

    FLUSH;
    err_cond = (int) data;
    if (!(qvm->status & STS_OKAY)) {
        fprintf(stderr, "vm/quivm: on_exception: "
                "unhandled exception: %d at 0x%08X\n",
                err_cond, r_pc);
        quivm_terminate(qvm, err_cond);
        quivm_raise(qvm);
    } else {
        dstack_push(r_acc);
        r_acc = err_cond;

        /* rstack points to the faulting instruction */
        rstack_push(r_pc);
        r_pc = INITIAL_PC;
        qvm->status &= ~STS_OKAY;
    }
    DISPATCH;
}
EPILOGUE

PROLOGUE(JSR_CONST)
{
    r_pc += (uint32_t) (data >> 32);
    rstack_push(r_pc);
    r_pc = (uint32_t) data;
    DISPATCH;
}
EPILOGUE

PROLOGUE(JMP_CONST)
{
    r_pc = (uint32_t) data;
    DISPATCH;
}
EPILOGUE

PROLOGUE(JZ_CONST)
{
    r_pc += (uint32_t) (data >> 32);
    if (r_acc == 0) r_pc = (uint32_t) data;
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(ADD_CONST)
{
    r_pc += (uint32_t) (data >> 32);
    r_acc += (uint32_t) data;
    DISPATCH;
}
EPILOGUE

PROLOGUE(RGET_CONST)
{
    dstack_push(r_acc);
    r_pc += (uint32_t) (data >> 32);
    r_acc = m_rstack[(uint8_t) (r_rsp - 1 - ((uint32_t) data))];
    DISPATCH;
}
EPILOGUE

PROLOGUE(RSET_CONST)
{
    r_pc += (uint32_t) (data >> 32);
    m_rstack[(uint8_t) (r_rsp - 1 - ((uint8_t) data))] = r_acc;
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(TWODUP)
{
    uint32_t v;
    r_pc += 2;
    v = dstack_pop();
    dstack_push(v);
    dstack_push(r_acc);
    dstack_push(v);
    DISPATCH;
}
EPILOGUE

PROLOGUE(LIT)
{
    dstack_push(r_acc);
    r_acc = (uint32_t) data;
    r_pc += (uint32_t) (data >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(LITS)
{
    r_acc <<= 7;
    r_acc |= (uint32_t) data;
    r_pc += (uint32_t) (data >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(RET)
{
    UNUSED_DATA;
    r_pc = rstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(JSR)
{
    UNUSED_DATA;
    rstack_push(++r_pc);
    r_pc += r_acc;
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(JMP)
{
    UNUSED_DATA;
    r_pc += r_acc + 1;
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(JZ)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    if (v == 0) r_pc += r_acc;
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(EQ0)
{
    UNUSED_DATA;
    r_pc++;
    r_acc = (0 == r_acc) ? -1 : 0;
    DISPATCH;
}
EPILOGUE

PROLOGUE(EQ)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc = (v == r_acc) ? -1 : 0;
    DISPATCH;
}
EPILOGUE

PROLOGUE(ULT)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc = (v < r_acc) ? -1 : 0;
    DISPATCH;
}
EPILOGUE

PROLOGUE(LT)
{
    int32_t v;
    UNUSED_DATA;
    r_pc++; v = (int32_t) dstack_pop();
    r_acc = (v < ((int32_t) r_acc)) ? -1 : 0;
    DISPATCH;
}
EPILOGUE

PROLOGUE(AND)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc &= v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(OR)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc |= v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(XOR)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc ^= v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(ADD)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc += v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(SUB)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc = v - r_acc;
    DISPATCH;
}
EPILOGUE

PROLOGUE(CSEL)
{
    uint32_t v, w;
    UNUSED_DATA;
    r_pc++; v = dstack_pop(); w = dstack_pop();
    r_acc = (r_acc) ? v : w;
    DISPATCH;
}
EPILOGUE

PROLOGUE(UMUL)
{
    uint64_t dv, dw;
    UNUSED_DATA;

    r_pc++;
    dw = (uint64_t) dstack_pop();
    dv = (uint64_t) r_acc;
    dv *= dw;
    dstack_push((uint32_t) dv);
    r_acc = (uint32_t) (dv >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(UDIV)
{
    uint32_t v;
    UNUSED_DATA;

    /* check for division by zero */
    if (r_acc == 0) {
        INVOKE(EXCEPTION, EX_DIVIDE_BY_ZERO);
    } else {
        r_pc++; v = dstack_pop();
        dstack_push((v % r_acc));
        r_acc = v / r_acc;
    }
    DISPATCH;
}
EPILOGUE

PROLOGUE(RD)
{
    UNUSED_DATA;
    r_pc++;
    FLUSH;
    r_acc = quivm_read(qvm, r_acc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(WRT)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    FLUSH;
    quivm_write(qvm, r_acc, v);
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(RDB)
{
    UNUSED_DATA;
    r_pc++;
    FLUSH;
    r_acc = (uint32_t) quivm_read_byte(qvm, r_acc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(WRTB)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    FLUSH;
    quivm_write_byte(qvm, r_acc, v);
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(SIGNE)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    if (r_acc < 32) {
        r_acc = 32 - r_acc;
        v <<= r_acc;
        r_acc = (((int32_t) v) >> r_acc);
    } else {
        r_acc = v;
    }
    DISPATCH;
}
EPILOGUE

PROLOGUE(SHL)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc = (v << (r_acc & 0x1F));
    DISPATCH;
}
EPILOGUE

PROLOGUE(USHR)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    r_acc = (v >> (r_acc & 0x1F));
    DISPATCH;
}
EPILOGUE

PROLOGUE(SHR)
{
    int32_t v;
    UNUSED_DATA;
    r_pc++; v = (int32_t) dstack_pop();
    r_acc = (uint32_t) (v >> (r_acc & 0x1F));
    DISPATCH;
}
EPILOGUE

PROLOGUE(NOP)
{
    r_pc += (uint32_t) (data >> 32);
    DISPATCH;
}
EPILOGUE

PROLOGUE(DUP)
{
    UNUSED_DATA;
    r_pc++;
    dstack_push(r_acc);
    DISPATCH;
}
EPILOGUE

PROLOGUE(DROP)
{
    UNUSED_DATA;
    r_pc++;
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(SWAP)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    dstack_push(r_acc);
    r_acc = v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(OVER)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    dstack_push(v);
    dstack_push(r_acc);
    r_acc = v;
    DISPATCH;
}
EPILOGUE

PROLOGUE(ROT)
{
    uint32_t v, w;
    UNUSED_DATA;
    r_pc++; v = dstack_pop(); w = dstack_pop();
    dstack_push(v);
    dstack_push(r_acc);
    r_acc = w;
    DISPATCH;
}
EPILOGUE

PROLOGUE(RTO)
{
    UNUSED_DATA;
    r_pc++; rstack_push(r_acc);
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(RFROM)
{
    UNUSED_DATA;
    r_pc++; dstack_push(r_acc);
    r_acc = rstack_pop();
    DISPATCH;
}
EPILOGUE

PROLOGUE(RGET)
{
    UNUSED_DATA;
    r_pc++;
    r_acc = m_rstack[(uint8_t) (r_rsp - 1 - r_acc)];
    DISPATCH;
}
EPILOGUE

PROLOGUE(RSET)
{
    uint32_t v;
    UNUSED_DATA;
    r_pc++; v = dstack_pop();
    m_rstack[(uint8_t) (r_rsp - 1 - r_acc)] = v;
    r_acc = dstack_pop();
    DISPATCH;
}
EPILOGUE

#if DISPATCH_METHOD == 0
        default:
            break;
        }
    }
    return 0;
}
#elif DISPATCH_METHOD == 1
    return 0;
}
#endif

/* Populates the code table */
static
void tracer_populate_ctable(struct tracer *tr)
{
#if DISPATCH_METHOD == 1
    tr->ctable[0] = NULL;
    tracer_run(get_quivm(tr));
#else
    static trace_code ctable[] = {
        INSN_TABLE(CTABLE)
    };
    memcpy(tr->ctable, ctable, sizeof(ctable));
#endif
}

/* Traces the code in a given address. */
static
void tracer_trace(struct tracer *tr, uint32_t address)
{
    struct quivm *qvm;
    struct page *pg;
    uint32_t pg_index, offset, v;
    uint32_t addr, max_addr;
    uint64_t data;
    uint8_t insn;

    qvm = get_quivm(tr);
    pg_index = address / PAGE_SIZE;

    addr = address;
    max_addr = (pg_index + 1) * PAGE_SIZE;

    insn = qvm->mem[addr++];
    if (insn < INSN_LIT_BASE) {
        data = ((uint64_t) insn) | (1ULL << 32ULL);
        insn = INSN_LITS;
        goto write_code;
    }

    if (insn < INSN_REG_BASE) {
        v = (uint32_t) (insn - INSN_LIT_BASE);
        if (v & 0x20) {
            /* sign-extend the literal value */
            v |= ~0x3F;
        }

        while (addr < max_addr) {
            insn = qvm->mem[addr++];
            if (!(insn < INSN_LIT_BASE)) break;
            v <<= 7; v |= (uint32_t) insn;
        }

        if (insn == INSN_JSR) {
            insn = INSN_JSR_CONST;
            v += addr;
        } else if (insn == INSN_JMP) {
            insn = INSN_JMP_CONST;
            v += addr;
        } else if (insn == INSN_JZ) {
            insn = INSN_JZ_CONST;
            v += addr;
        } else if (insn == INSN_ADD) {
            insn = INSN_ADD_CONST;
        } else if (insn == INSN_SUB) {
            insn = INSN_ADD_CONST;
            v = -v;
        } else if (insn == INSN_RGET) {
            insn = INSN_RGET_CONST;
        } else if (insn == INSN_RSET) {
            insn = INSN_RSET_CONST;
        } else {
            if ((insn >= INSN_LIT_BASE) && (addr > address + 1))
                addr--;
            insn = INSN_LIT;
        }
        data = ((uint64_t) v);
        data |= (((uint64_t) (addr - address)) << 32ULL);
        goto write_code;
    }

    /* process regular instructions */
    data = (1ULL << 32ULL);
    if (insn == INSN_OVER) {
        if (addr < max_addr) {
            insn = qvm->mem[addr++];
            if (insn == INSN_OVER) {
                insn = INSN_TWODUP;
            } else {
                insn = INSN_OVER;
                addr--;
            }
        }
    } else if (insn > INSN_RSET) {
        insn = INSN_EXCEPTION;
        data = EX_INVALID_INSN;
    }

write_code:
    pg = tr->traced[pg_index];
    if (pg == tr->sentinel) {
        pg = tracer_get_free_page(tr);
        tr->traced[pg_index] = pg;
    }
    offset = address % PAGE_SIZE;
    pg->code[offset] = tr->ctable[insn - INSN_BASE];
    pg->data[offset] = data;
}
