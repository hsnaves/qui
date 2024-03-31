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
    float position;             /* the current playing position */
    float increment;            /* Increment of position per sample */
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

/* Pre-compiled table of pitch to frequency multiplier
 * The formula is 2 ** ((n - 57.) / 12.)
 */
static const float PITCH_TO_FREQ[108] = {
     0.0371627,  0.0393725,  0.0417137,  0.0441942,
     0.0468221,  0.0496063,  0.0525560,  0.0556812,
     0.0589921,  0.0625000,  0.0662164,  0.0701539,

     0.0743254,  0.0787451,  0.0834275,  0.0883883,
     0.0936442,  0.0992126,  0.1051121,  0.1113623,
     0.1179843,  0.1250000,  0.1324329,  0.1403078,

     0.1486509,  0.1574901,  0.1668550,  0.1767767,
     0.1872884,  0.1984251,  0.2102241,  0.2227247,
     0.2359686,  0.2500000,  0.2648658,  0.2806155,

     0.2973018,  0.3149803,  0.3337100,  0.3535534,
     0.3745768,  0.3968503,  0.4204482,  0.4454494,
     0.4719371,  0.5000000,  0.5297316,  0.5612310,

     0.5946035,  0.6299605,  0.6674199,  0.7071068,
     0.7491536,  0.7937005,  0.8408964,  0.8908987,
     0.9438743,  1.0000000,  1.0594631,  1.1224620,

     1.1892071,  1.2599211,  1.3348398,  1.4142135,
     1.4983071,  1.5874010,  1.6817929,  1.7817974,
     1.8877486,  2.0000000,  2.1189263,  2.2449241,

     2.3784142,  2.5198421,  2.6696796,  2.8284271,
     2.9966142,  3.1748021,  3.3635857,  3.5635948,
     3.7754972,  4.0000000,  4.2378526,  4.4898481,

     4.7568283,  5.0396843,  5.3393593,  5.6568542,
     5.9932284,  6.3496041,  6.7271714,  7.1271896,
     7.5509944,  8.0000000,  8.4757051,  8.9796963,

     9.5136566, 10.0793686, 10.6787186, 11.3137083,
    11.9864569, 12.6992083, 13.4543428, 14.2543793,
    15.1019888, 16.0000000, 16.9514103, 17.9593925,
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
    uint32_t v, pos, x0, address;
    float d, y0, y1, y;

    audi = (struct audio_internal *) aud->internal;
    for (i = 0; i < SAMPLES_PER_UPDATE; i++) {
        /* check for buffer full */
        if (audi->tail >= audi->head + BUFFER_SIZE) break;

        v = 0;
        for (ch = 0; ch < NUM_CHANNELS; ch++) {
            chn = &audi->channels[ch];
            smpl = &chn->smpl;
            if (smpl->length == 0) continue;

            x0 = ((uint32_t) smpl->position);
            if (x0 >= smpl->length) {
                /* something went wrong here */
                continue;
            }

            /* interpolate the samples linearly */
            address = smpl->address + x0;
            y0 = qvm->mem[address];
            y1 = qvm->mem[address + 1];
            d = smpl->position - ((float) x0);
            y = y0 + d * (y1 - y0);

            v += (uint8_t) y;

            smpl->position += smpl->increment;
            while (smpl->position >= smpl->length)
                smpl->position -= smpl->length;
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
    unsigned int ch, pitch;

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
        pitch = ((aud->params[0] >> 8) & 0xFF);
        if (pitch >= 108) pitch = 107;

        chn = &audi->channels[ch];
        smpl = &chn->smpl;
        smpl->address = aud->params[1];
        smpl->length = aud->params[2];
        smpl->position = 0.0;
        smpl->increment = PITCH_TO_FREQ[pitch];

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
