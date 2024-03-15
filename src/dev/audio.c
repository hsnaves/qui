#include <stdint.h>

#include "vm/quivm.h"
#include "dev/audio.h"

/* Functions */

int audio_init(struct audio *aud)
{
    aud->initialized = 0;
    return 0;
}

void audio_destroy(struct audio *aud)
{
    (void)(aud); /* UNUSED */
}

uint32_t audio_read_callback(struct audio *aud,
                             struct quivm *qvm, uint32_t address)
{
    uint32_t v;

    (void)(aud); /* UNUSED */
    (void)(qvm); /* UNUSED */

    switch (address) {
    default:
        v = -1;
        break;
    }
    return v;
}

void audio_write_callback(struct audio *aud,  struct quivm *qvm,
                          uint32_t address, uint32_t v)
{
    (void)(aud); /* UNUSED */
    (void)(qvm); /* UNUSED */
    (void)(address); /* UNUSED */
    (void)(v); /* UNUSED */
}
