#ifndef __VM_INTERNAL_H
#define __VM_INTERNAL_H

#include <stdint.h>

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

/* Macros for stack operations */
#define dstack_push(qvm, v) (qvm)->dstack[(qvm)->dsp++] = (v)
#define rstack_push(qvm, v) (qvm)->rstack[(qvm)->rsp++] = (v)
#define dstack_pop(qvm) ((qvm)->dstack[--(qvm)->dsp])
#define rstack_pop(qvm) ((qvm)->rstack[--(qvm)->rsp])

/* Functions */

/* Initializes the tracer.
 * Returns zero on success.
 */
int tracer_init(struct quivm *qvm);

/* Destroys the tracer and release the resources. */
void tracer_destroy(struct quivm *qvm);

/* Flushes the pages of the tracer cache
 * The address range is defined by the parameters `istart` (the
 * start address) and `iend` (the end address, which is one plus
 * the last valid address).
 */
void tracer_invalidate(struct quivm *qvm, uint32_t istart, uint32_t iend);

/* Runs the VM using the tracer */
int tracer_run(struct quivm *qvm);

#endif
