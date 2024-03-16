#include <stddef.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/timer.h"

/* Functions */

int timer_init(struct timer *tmr)
{
    tmr->tickcount = 0;
    tmr->oninterrupt = 0;
    tmr->enabled = 0;
    return 0;
}

void timer_destroy(struct timer *tmr)
{
    (void)(tmr); /* UNUSED */
}

void timer_update(struct timer *tmr, struct quivm *qvm)
{
    tmr->tickcount++;
    if (tmr->enabled) {
        quivm_stack_push(qvm, 1, qvm->pc);
        qvm->pc = tmr->oninterrupt;
    }
}

uint32_t timer_read_callback(struct timer *tmr,
                             struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_TIMER_TICKCOUNT:
        v = tmr->tickcount;
        break;
    case IO_TIMER_ONINTERRUPT:
        v = tmr->oninterrupt;
        break;
    case IO_TIMER_ENABLED:
        v = tmr->enabled;
        break;
    default:
        v = -1;
        break;
    }

    return v;
}

void timer_write_callback(struct timer *tmr,  struct quivm *qvm,
                          uint32_t address, uint32_t v)
{
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_TIMER_TICKCOUNT:
        tmr->tickcount = v;
        break;
    case IO_TIMER_ONINTERRUPT:
        tmr->oninterrupt = v;
        break;
    case IO_TIMER_ENABLED:
        tmr->enabled = v;
        break;
    }
}


