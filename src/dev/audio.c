#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm/quivm.h"
#include "dev/audio.h"

/* Constants */
#define NUM_CHANNELS                     4
#define MAX_COMMANDS                    16

/* Data structures and types */

/* Structure representing a sample to be played (or already playing), with
 * the sample data, the length of the sample, etc.
 */
struct audio_sample {
    uint32_t address;           /* the address of the sample in memory */
    uint32_t length;            /* number of data points in the sample */
    uint32_t position;          /* the current playing position */
    uint8_t buffer[AUDIO_SAMPLE_SIZE]; /* buffered data extracted from QVM */
    int loaded;                 /* the buffer was loaded */
};

/* Structure reprensenting an audio channel */
struct audio_channel {
    struct audio_sample smpl;   /* current playing sample */
};

/* The structure for the internal audio device */
struct audio_internal {
    /* the audio channels */
    struct audio_channel channels[NUM_CHANNELS];

    unsigned int num_cmds;      /* number of buffered commands */
    struct audio_command cmds[MAX_COMMANDS]; /* command buffer */
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
    memset(&aud->cmd, 0, sizeof(aud->cmd));
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
    struct audio_command *cmd;
    struct audio_channel *chn;
    struct audio_sample *smpl;
    unsigned int i, channel;
    uint32_t address, length;

    audi = (struct audio_internal *) aud->internal;

    for (i = 0; i < audi->num_cmds; i++) {
        cmd = &audi->cmds[i];
        switch (cmd->command) {
        case AUDIO_CMD_PLAY:
            channel = (cmd->params[0] & 0xFF);
            if (channel >= NUM_CHANNELS) break;

            chn = &audi->channels[channel];
            smpl = &chn->smpl;
            smpl->address = cmd->params[1];
            smpl->length = cmd->params[2];
            smpl->position = 0;
            smpl->loaded = 0;

            if (smpl->length > (qvm->memsize - smpl->address))
                smpl->length = qvm->memsize - smpl->address;

            break;
        default:
            /* Ignore wrong command */
            break;
        }
    }
    audi->num_cmds = 0;

    for (channel = 0; channel < NUM_CHANNELS; channel++) {
        chn = &audi->channels[channel];
        smpl = &chn->smpl;
        if (smpl->length == 0) continue;

        if (smpl->length < AUDIO_SAMPLE_SIZE) {
            if (!smpl->loaded) {
                memcpy(smpl->buffer,
                       &qvm->mem[smpl->address],
                       smpl->length);
            }
        } else {
            address = smpl->address + smpl->position;
            length = smpl->length - smpl->position;
            if (length < AUDIO_SAMPLE_SIZE) {
                memcpy(smpl->buffer,
                       &qvm->mem[address],
                       length);
                memcpy(&smpl->buffer[length],
                       &qvm->mem[smpl->address],
                       AUDIO_SAMPLE_SIZE - length);
            } else {
                memcpy(smpl->buffer,
                       &qvm->mem[address],
                       AUDIO_SAMPLE_SIZE);
            }
        }
        smpl->loaded = 1;
    }
}

uint32_t audio_read_callback(struct audio *aud,
                             struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_AUDIO_COMMAND:
        v = aud->cmd.command;
        break;
    default:
        if (((IO_AUDIO_PARAM0 - address) % 4 == 0)
            && (address >= IO_AUDIO_PARAM7)
            && (address <= IO_AUDIO_PARAM0)) {

            v = aud->cmd.params[(IO_AUDIO_PARAM0 - address) >> 2];
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
    struct audio_command *cmd;

    (void)(qvm); /* UNUSED */

    audi = (struct audio_internal *) aud->internal;
    switch (aud->cmd.command) {
    case AUDIO_CMD_INIT:
        if (!aud->initialized) {
            aud->initialized = 1;
            aud->cmd.params[0] = AUDIO_SUCCESS;
        } else {
            /* already initialized */
            aud->cmd.params[0] = AUDIO_ERROR;
        }
        break;
    case AUDIO_CMD_PLAY:
        if (audi->num_cmds < MAX_COMMANDS) {
            cmd = &audi->cmds[audi->num_cmds++];
            memcpy(cmd, &aud->cmd, sizeof(*cmd));
            aud->cmd.params[0] = AUDIO_SUCCESS;
        } else {
            aud->cmd.params[0] = AUDIO_ERROR;
        }
        break;
    default:
        aud->cmd.params[0] = AUDIO_ERROR;
        break;
    }
}

void audio_write_callback(struct audio *aud,  struct quivm *qvm,
                          uint32_t address, uint32_t v)
{
    switch (address) {
    case IO_AUDIO_COMMAND:
        aud->cmd.command = v;
        do_command(aud, qvm);
        break;
    default:
        if (((IO_AUDIO_PARAM0 - address) % 4 == 0)
            && (address >= IO_AUDIO_PARAM7)
            && (address <= IO_AUDIO_PARAM0)) {

            aud->cmd.params[(IO_AUDIO_PARAM0 - address) >> 2] = v;
        }
        break;
    }
}

void audio_stream_callback(void *arg, uint8_t *stream, int len)
{
    struct audio *aud;
    struct audio_internal *audi;
    struct audio_channel *chn;
    struct audio_sample *smpl;
    unsigned int channel;
    int i;

    aud = (struct audio *) arg;
    audi = aud->internal;

    memset(stream, 0, len);
    if (len > AUDIO_SAMPLE_SIZE) len = AUDIO_SAMPLE_SIZE;

    for (channel = 0; channel < NUM_CHANNELS; channel++) {
        chn = &audi->channels[channel];
        smpl = &chn->smpl;
        if (smpl->length == 0 || !smpl->loaded) continue;

        if (smpl->length < AUDIO_SAMPLE_SIZE) {
            for (i = 0; i < len; i++) {
                stream[i] += smpl->buffer[smpl->position++];
                if (smpl->position == smpl->length)
                    smpl->position = 0;
            }
        } else {
            for (i = 0; i < len; i++) {
                stream[i] += smpl->buffer[i];

                smpl->position++;
                if (smpl->position == smpl->length)
                    smpl->position = 0;
            }
        }
        smpl->loaded = 0;
    }
}
