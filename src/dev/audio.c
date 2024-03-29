#include <stdint.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/audio.h"

/* Functions */

int audio_init(struct audio *aud)
{
    aud->initialized = 0;
    aud->command = 0;
    memset(aud->params, 0, sizeof(aud->params));
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
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_AUDIO_COMMAND:
        v = aud->command;
        break;
    default:
        if (((IO_AUDIO_PARAM0 - address) % 4 == 0)
            && (address >= IO_AUDIO_PARAM7)
            && (address <= IO_AUDIO_PARAM0)) {

            v = aud->params[(IO_AUDIO_PARAM0 - address) >> 2];
        } else {
            v = -1;
        }
        break;
    }
    return v;
}

/* runs a command in aud->command */
static
void do_command(struct audio *aud, struct quivm *qvm)
{
    (void)(qvm); /* UNUSED */

    switch (aud->command) {
    case AUDIO_CMD_INIT:
        if (!aud->initialized) {
            aud->initialized = 1;
            aud->params[0] = AUDIO_SUCCESS;
        } else {
            /* already initialized */
            aud->params[0] = AUDIO_ERROR;
        }
        break;
    default:
        aud->params[0] = AUDIO_ERROR;
        break;
    }
}

void audio_write_callback(struct audio *aud,  struct quivm *qvm,
                          uint32_t address, uint32_t v)
{
    switch (address) {
    case IO_AUDIO_COMMAND:
        aud->command = v;
        do_command(aud, qvm);
        break;
    default:
        if (((IO_AUDIO_PARAM0 - address) % 4 == 0)
            && (address >= IO_AUDIO_PARAM7)
            && (address <= IO_AUDIO_PARAM0)) {

            aud->params[(IO_AUDIO_PARAM0 - address) >> 2] = v;
        }
        break;
    }
}
