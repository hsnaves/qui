#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/audio.h"

/* Constants */
#define NUM_CHANNELS                     4
#define BUFFER_SIZE                   8192
#define SAMPLES_PER_UPDATE              46

/* Data structures and types */

/* Structure representing a sample to be played (or already playing), with
 * the sample data, the length of the sample, etc.
 */
struct sample {
    uint32_t address;           /* the address of the sample in memory */
    uint32_t length;            /* number of data points in the sample */
    uint32_t position;          /* the current playing position */
};

/* Structure reprensenting an audio channel */
struct channel {
    struct sample smpl;         /* current playing sample */
};

/* The structure for the internal audio device */
struct audio_internal {
    struct channel channels[NUM_CHANNELS]; /* the audio channels */

    uint8_t buf[BUFFER_SIZE];   /* the audio buffer */
    uint32_t head, tail;        /* for the circular buffer */
};
/* Functions */

int audio_init(struct audio *aud)
{
    struct audio_internal *audi;

    aud->internal = NULL;
    audi = (struct audio_internal *) malloc(sizeof(*audi));
    if (!audi) {
        fprintf(stderr, "audio: init: memory exhausted\n");
        return 1;
    }

    memset(audi, 0, sizeof(*audi));

    aud->initialized = 0;
    aud->command = 0;
    memset(&aud->params, 0, sizeof(aud->params));
    aud->internal = audi;
    return 0;
}

void audio_destroy(struct audio *aud)
{
    if (aud->internal) free(aud->internal);
    aud->internal = NULL;
}

void audio_update(struct audio *aud, struct quivm *qvm)
{
    struct audio_internal *audi;
    struct channel *chn;
    struct sample *smpl;
    unsigned int i, ch;
    uint32_t v, pos, address;

    audi = (struct audio_internal *) aud->internal;
    for (i = 0; i < SAMPLES_PER_UPDATE; i++) {
        /* check for buffer full */
        if (audi->tail >= audi->head + BUFFER_SIZE) break;

        v = 0;
        for (ch = 0; ch < NUM_CHANNELS; ch++) {
            chn = &audi->channels[ch];
            smpl = &chn->smpl;
            if (smpl->length == 0) continue;

            address = smpl->address + smpl->position;
            v += qvm->mem[address];

            smpl->position++;
            if (smpl->position == smpl->length)
                smpl->position = 0;
        }

        pos = audi->tail++;
        if (pos >= BUFFER_SIZE) pos -= BUFFER_SIZE;
        audi->buf[pos] = (v / NUM_CHANNELS);
    }
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
    struct audio_internal *audi;
    struct channel *chn;
    struct sample *smpl;
    unsigned int ch;

    audi = (struct audio_internal *) aud->internal;
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
    case AUDIO_CMD_PLAY:
        ch = (aud->params[0] & 0xFF);
        if (ch >= NUM_CHANNELS) {
            aud->params[0] = AUDIO_ERROR;
            break;
        }

        chn = &audi->channels[ch];
        smpl = &chn->smpl;
        smpl->address = aud->params[1];
        smpl->length = aud->params[2];
        smpl->position = 0;

        if (smpl->length > (qvm->memsize - smpl->address))
            smpl->length = qvm->memsize - smpl->address;

        aud->params[0] = AUDIO_SUCCESS;
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

void audio_stream_callback(void *arg, uint8_t *stream, int len)
{
    struct audio *aud;
    struct audio_internal *audi;
    int i;

    aud = (struct audio *) arg;
    audi = aud->internal;

    for (i = 0; i < len; i++) {
        /* check for buffer empty */
        if (audi->head >= audi->tail) break;

        stream[i] = audi->buf[audi->head++];
        if (audi->head == BUFFER_SIZE) {
            audi->head = 0;
            audi->tail -= BUFFER_SIZE;
        }
    }
    memset(&stream[i], 0, len - i);
}
