#include <stddef.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/network.h"

/* Functions */

int network_init(struct network *ntw)
{
    ntw->unused = NULL;
    return 0;
}

void network_destroy(struct network *ntw)
{
    (void)(ntw); /* UNUSED */
}

uint32_t network_read_callback(struct network *ntw,
                               struct quivm *qvm, uint32_t address)
{
    uint32_t v;

    (void)(ntw); /* UNUSED */
    (void)(qvm); /* UNUSED */

    (void)(address); /* UNUSED */

    v = -1;
    return v;
}

void network_write_callback(struct network *ntw,  struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    (void)(ntw); /* UNUSED */
    (void)(qvm); /* UNUSED */

    (void)(address); /* UNUSED */
    (void)(v); /* UNUSED */
}

