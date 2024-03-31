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
struct envelope {
    float attack;               /* the attack rate */
    float decay;                /* the decay rate */
    float sustain;              /* the sustain value */
    float release;              /* the release rate */
    float value;                /* the envelope value */
    enum envelope_stage {
        ENV_ATTACK,
        ENV_DECAY,
        ENV_SUSTAIN,
        ENV_RELEASE,
    } stage;                    /* the envelope stage */
};

/* Structure representing a sample to be played (or already playing), with
 * the sample data, the length of the sample, etc.
 */
struct sample {
    uint32_t address;           /* the address of the sample in memory */
    uint32_t length;            /* number of data points in the sample */
    float position;             /* the current playing position */
    float increment;            /* Increment of position per sample */
    struct envelope env;        /* The envelope */
};

/* Structure reprensenting an audio channel */
struct channel {
    struct sample smpl;         /* current playing sample */
    float volume;               /* the volume of the channel */
    uint32_t duration;          /* the remaining duration for the sample */
};

/* The structure for the internal audio device */
struct audio_internal {
    struct channel channels[NUM_CHANNELS]; /* the audio channels */

    int8_t buf[BUFFER_SIZE];    /* the audio buffer */
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
    aud->paused = 0;
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
    struct envelope *env;
    unsigned int i, ch, num_samples;
    uint32_t pos, x0;
    float v, d, y0, y1, y;
    int active;

    audi = (struct audio_internal *) aud->internal;

    num_samples = SAMPLES_PER_UPDATE;
    if (audi->tail - audi->head < 2048)
        num_samples = 2048;

    active = 0;
    for (i = 0; i < num_samples; i++) {
        /* check for buffer full */
        if (audi->tail >= audi->head + BUFFER_SIZE) break;

        v = 0.0f;
        for (ch = 0; ch < NUM_CHANNELS; ch++) {
            chn = &audi->channels[ch];
            smpl = &chn->smpl;
            env = &smpl->env;

            if (smpl->length == 0) continue;

            x0 = (uint32_t) smpl->position;
            if (x0 >= smpl->length) {
                /* something went wrong here */
                continue;
            }

            /* interpolate the samples linearly */
            d = smpl->position - ((float) x0);
            y0 = (float) ((int8_t) qvm->mem[smpl->address + x0]);
            if (++x0 == smpl->length) x0 = 0;
            y1 = (float) ((int8_t) qvm->mem[smpl->address + x0]);
            y = y0 + d * (y1 - y0);

            /* advance the envelope */
            switch (env->stage) {
            case ENV_ATTACK:
                env->value += env->attack;
                if (env->value > 1.0f) {
                    env->value = 1.0f;
                    env->stage = ENV_DECAY;
                }
                break;
            case ENV_DECAY:
                env->value -= env->decay;
                if (env->value < env->sustain) {
                    env->value = env->sustain;
                    env->stage = ENV_SUSTAIN;
                }
                break;
            case ENV_SUSTAIN:
                break;
            case ENV_RELEASE:
            default:
                env->value -= env->release;
                if (env->value <= 0.0f) {
                    env->value = 0.0;
                    smpl->length = 0;
                }
                break;
            }
            v += chn->volume * env->value * y;

            smpl->position += smpl->increment;
            if (smpl->length > 0) {
                while (smpl->position >= smpl->length)
                    smpl->position -= smpl->length;
            }

            if (!(chn->duration & 0x80000000)) {
                /* if it reached the end, stop playing sample */
                chn->duration--;
                if (chn->duration == 0) {
                    env->stage = ENV_RELEASE;
                    chn->duration = 0x80000000;
                }
            }
            active = 1;
        }

        if (!active) break;

        pos = audi->tail++;
        if (pos >= BUFFER_SIZE) pos -= BUFFER_SIZE;
        if (v < -128.0f) v = -128.0f;
        if (v > 127.0f) v = 127.0f;
        audi->buf[pos] = (int8_t) v;
    }

    aud->paused = (audi->tail == audi->head);
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
    struct envelope *env;
    unsigned int ch, pitch, dur, vol;
    unsigned int attack, decay, sustain, release;

    audi = (struct audio_internal *) aud->internal;
    switch (aud->command) {
    case AUDIO_CMD_INIT:
        if (!aud->initialized) {
            aud->initialized = 1;
            aud->paused = 1;
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

        dur = ((aud->params[0] >> 16) & 0xFF);
        vol = ((aud->params[0] >> 24) & 0xFF);

        attack = (aud->params[3] & 0xFF);
        decay = ((aud->params[3] >> 8) & 0xFF);
        sustain = ((aud->params[3] >> 16) & 0xFF);
        release = ((aud->params[3] >> 24) & 0xFF);

        chn = &audi->channels[ch];
        smpl = &chn->smpl;
        env = &smpl->env;

        smpl->address = aud->params[1];
        smpl->length = aud->params[2];
        smpl->position = 0.0;
        smpl->increment = PITCH_TO_FREQ[pitch];

        env->attack = 1.0f / (1.0f + ((AUDIO_FREQUENCY * attack) >> 4));
        env->decay = 1.0f / (1.0f + ((AUDIO_FREQUENCY * decay) >> 4));
        env->sustain = sustain / 255.0f;
        env->release = 1.0f / (1.0f + ((AUDIO_FREQUENCY * release) >> 4));

        env->value = 0.0f;
        env->stage = ENV_ATTACK;

        chn->volume = ((float) vol) / 16.0f;
        if (dur > 0) {
            if (dur == 255) {
                /* infinite duration */
                chn->duration = 0x80000000;
            } else {
                /* converts from 1/16th of seconds to samples */
                chn->duration = (AUDIO_FREQUENCY * dur) >> 4;
            }
        } else {
            /* compute duration automatically */
            chn->duration =
                (uint32_t) (((float) smpl->length) / smpl->increment);
        }

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
    /* FILE *fp; */
    int i;

    aud = (struct audio *) arg;
    audi = aud->internal;

    for (i = 0; i < len; i++) {
        /* check for buffer empty */
        if (audi->head >= audi->tail) break;

        stream[i] = (uint8_t) audi->buf[audi->head++];
        if (audi->head == BUFFER_SIZE) {
            audi->head = 0;
            audi->tail -= BUFFER_SIZE;
        }
    }
    memset(&stream[i], 0, len - i);
}
