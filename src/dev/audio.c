#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/audio.h"

/* Constants */
#define NUM_CHANNELS                    16
#define BUFFER_SIZE                   8192
#define SAMPLES_PER_UPDATE             735

/* Data structures and types */
/* Structure representing an ADSR envelope.
 * The values represented here are in 20-bit fixed point (fp) notation.
 * So for instance, if the number is in 10-bit fp notation, then
 * it uses 10-bit for the fractional part. That means that
 * the value 1024 represents real number 1.0.
 */
struct envelope {
    uint32_t attack;            /* the attack rate (24-bit fp) */
    uint32_t decay;             /* the decay rate (24-bit fp)  */
    uint32_t sustain;           /* the sustain value (24-bit fp) */
    uint32_t release;           /* the release rate (24-bit fp) */
    uint32_t value;             /* the envelope value (24-bit fp) */
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
    uint32_t position;          /* the current position (12-bit fp) */
    uint32_t increment;         /* Increment per sample (12-bit fp) */
    uint32_t duration;          /* The remaining duration of the sample
                                 * (in samples).
                                 */
    uint32_t vol_left;          /* left speaker volume (4-bit fp) */
    uint32_t vol_right;         /* right speaker volume (4-bit fp) */
    struct envelope env;        /* The envelope */
};

/* Structure reprensenting an audio channel */
struct channel {
    struct sample smpl[2];      /* current and next playing sample */
};

/* The structure for the internal audio device */
struct audio_internal {
    struct channel channels[NUM_CHANNELS]; /* the audio channels */

    int8_t buf[BUFFER_SIZE];    /* the audio buffer */
    uint32_t head, tail;        /* for the circular buffer */
};

/* Pre-compiled table of pitch to frequency multiplier
 * The formula is roundf(2 ** ((n - 57.) / 12.) * 4096)
 * using 12-bit fixed point notation.
 */
static const uint32_t PITCH_TO_FREQ[108] = {
      152,   161,   171,   181,
      192,   203,   215,   228,
      242,   256,   271,   287,

      304,   323,   342,   362,
      384,   406,   431,   456,
      483,   512,   542,   575,

      609,   645,   683,   724,
      767,   813,   861,   912,
      967,  1024,  1085,  1149,

     1218,  1290,  1367,  1448,
     1534,  1625,  1722,  1825,
     1933,  2048,  2170,  2299,

     2435,  2580,  2734,  2896,
     3069,  3251,  3444,  3649,
     3866,  4096,  4340,  4598,

     4871,  5161,  5468,  5793,
     6137,  6502,  6889,  7298,
     7732,  8192,  8679,  9195,

     9742, 10321, 10935, 11585,
    12274, 13004, 13777, 14596,
    15464, 16384, 17358, 18390,

    19484, 20643, 21870, 23170,
    24548, 26008, 27554, 29193,
    30929, 32768, 34716, 36781,

    38968, 41285, 43740, 46341,
    49097, 52016, 55109, 58386,
    61858, 65536, 69433, 73562,
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
    unsigned int i, ch, sn;
    unsigned int num_samples;
    uint32_t pos, x0;
    int32_t d, y0, y1, y;
    int32_t v, total;
    int active;

    if (!aud->initialized) return;
    audi = (struct audio_internal *) aud->internal;

    num_samples = SAMPLES_PER_UPDATE;
    if (audi->tail - audi->head < 2048)
        num_samples = 2048;

    active = 0;
    for (i = 0; i < num_samples; i++) {
        /* check for buffer full */
        if (audi->tail >= audi->head + BUFFER_SIZE) break;

        total = 0;
        for (ch = 0; ch < NUM_CHANNELS; ch++) {
            chn = &audi->channels[ch];

            for (sn = 0; sn < 2; sn++) {
                smpl = &chn->smpl[sn];
                env = &smpl->env;

                if (smpl->length == 0) {
                    if (sn == 0) {
                        chn->smpl[0] = chn->smpl[1];
                        chn->smpl[1].length = 0;
                        if (smpl->length != 0)
                            sn--;
                    }
                    continue;
                }

                x0 = (smpl->position >> 12);
                if (x0 >= smpl->length) {
                    /* something went wrong here */
                    continue;
                }

                /* interpolate the samples linearly */
                d = (int32_t) (smpl->position & 0xFFF);
                y0 = (int8_t) qvm->mem[smpl->address + x0];
                if (++x0 == smpl->length) x0 = 0;
                y1 = (int8_t) qvm->mem[smpl->address + x0];
                y = y0 + ((d * (y1 - y0)) >> 12);

                /* advance the envelope */
                switch (env->stage) {
                case ENV_ATTACK:
                    env->value += env->attack;
                    if (env->value >= (1 << 24)) {
                        env->value = (1 << 24);
                        env->stage = ENV_DECAY;
                    }
                    break;
                case ENV_DECAY:
                    if (env->value < env->decay) {
                        env->value = env->sustain;
                    } else{
                        env->value -= env->decay;
                    }
                    if (env->value <= env->sustain) {
                        env->value = env->sustain;
                        env->stage = ENV_SUSTAIN;
                    }
                    break;
                case ENV_SUSTAIN:
                    break;
                case ENV_RELEASE:
                default:
                    if (env->value <= env->release) {
                        env->value = 0;
                        smpl->length = 0;
                    } else {
                        env->value -= env->release;
                    }
                    break;
                }
                v = env->value >> 14;
                v *= smpl->vol_left;
                v >>= 4;
                v *= y;
                total += v;

                smpl->position += smpl->increment;
                if (smpl->length > 0) {
                    uint32_t scaled_length = (smpl->length << 12);
                    while (smpl->position >= scaled_length)
                        smpl->position -= scaled_length;
                }

                if (!(smpl->duration & 0x80000000)) {
                    /* if it reached the end, stop playing sample */
                    smpl->duration--;
                    if (smpl->duration == 0) {
                        env->stage = ENV_RELEASE;
                        smpl->duration = 0x80000000;
                    }
                }
                active = 1;
            }
        }

        if (!active) break;
        total >>= 10;

        pos = audi->tail++;
        if (pos >= BUFFER_SIZE) pos -= BUFFER_SIZE;

        v = total;
        if (v < -128) v = -128;
        if (v > 127) v = 127;
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
            && (address >= IO_AUDIO_PARAM6)
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
void do_command(struct audio *aud)
{
    struct audio_internal *audi;
    struct channel *chn;
    struct sample *smpl;
    struct envelope *env;
    unsigned int ch, pitch, dur, vol;
    unsigned int attack, decay, sustain, release;
    uint32_t address, length;

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
        pitch = ((aud->params[0] >> 8) & 0xFF);
        address = aud->params[1];
        length = aud->params[2];

        /* validate the parameters of the command */
        if ((ch >= NUM_CHANNELS) || (pitch >= 108)
            || (check_buffer(address, length, MEMORY_SIZE))) {
            aud->params[0] = AUDIO_ERROR;
            break;
        }

        dur = ((aud->params[0] >> 16) & 0xFF);
        vol = ((aud->params[0] >> 24) & 0xFF);

        attack = (aud->params[3] & 0xFF); /* 4-bit fp */
        decay = ((aud->params[3] >> 8) & 0xFF); /* 4-bit fp */
        sustain = ((aud->params[3] >> 16) & 0xFF); /* 8-bit fp */
        release = ((aud->params[3] >> 24) & 0xFF); /* 4-bit fp */

        chn = &audi->channels[ch];
        smpl = &chn->smpl[0];
        if (smpl->length != 0) {
            /* if a note is playing, use the smpl[1] */
            smpl->env.stage = ENV_RELEASE;
            smpl->duration = 0x80000000;
            smpl = &chn->smpl[1];
        }
        env = &smpl->env;

        env->attack = (attack)
            ? (1 << 28) / (AUDIO_FREQUENCY * attack) : (1 << 24);
        env->decay = (decay)
            ? (1 << 28) / (AUDIO_FREQUENCY * decay) : (1 << 24);
        env->sustain = (sustain << 16);
        env->release = (release)
            ? (1 << 28) / (AUDIO_FREQUENCY * release) : (1 << 24);
        env->value = 0;
        env->stage = ENV_ATTACK;

        smpl->address = address;
        smpl->length = length;
        smpl->position = 0;
        smpl->increment = PITCH_TO_FREQ[pitch];
        smpl->vol_left = vol;
        smpl->vol_right = vol;

        if (dur == 255) {
            /* infinite duration */
            smpl->duration = 0x80000000;
        } else if (dur > 0) {
            /* converts from 1/16th of seconds to samples */
            smpl->duration = (AUDIO_FREQUENCY * dur) >> 4;
        } else {
            /* compute duration automatically */
            uint64_t duration;
            duration = smpl->length;
            duration <<= 10;
            duration /= smpl->increment;

            if (duration > 0x7FFFFFFF) duration = 0x80000000;
            smpl->duration = (uint32_t) duration;
        }

        aud->params[0] = AUDIO_SUCCESS;
        break;
    default:
        aud->params[0] = AUDIO_ERROR;
        break;
    }
}

void audio_write_callback(struct audio *aud, struct quivm *qvm,
                          uint32_t address, uint32_t v)
{
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_AUDIO_COMMAND:
        aud->command = v;
        do_command(aud);
        break;
    default:
        if (((IO_AUDIO_PARAM0 - address) % 4 == 0)
            && (address >= IO_AUDIO_PARAM6)
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
