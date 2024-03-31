#ifndef __DEV_AUDIO_H
#define __DEV_AUDIO_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the audio device */
#define IO_AUDIO_BASE           0xFFFFFE40
#define IO_AUDIO_END            0xFFFFFE80

/* Addresses within the audio device */
#define IO_AUDIO_COMMAND        0xFFFFFE7C
#define IO_AUDIO_PARAM0         0xFFFFFE78
#define IO_AUDIO_PARAM1         0xFFFFFE74
#define IO_AUDIO_PARAM2         0xFFFFFE70
#define IO_AUDIO_PARAM3         0xFFFFFE6C
#define IO_AUDIO_PARAM4         0xFFFFFE68
#define IO_AUDIO_PARAM5         0xFFFFFE64
#define IO_AUDIO_PARAM6         0xFFFFFE60
#define IO_AUDIO_PARAM7         0xFFFFFE5C

/* Audio commands */
#define AUDIO_CMD_INIT                   1
#define AUDIO_CMD_PLAY                   2

/* Audio return values */
#define AUDIO_SUCCESS                    0
#define AUDIO_ERROR                     -1

/* Other constants */
#define AUDIO_SAMPLE_SIZE             2048
#define AUDIO_FREQUENCY              44100

/* Data structures and types */
/* A structure representing the audio command */
struct audio_command {
    uint32_t command;           /* the command */
    uint32_t params[8];         /* the parameters for the command */
};

/* A structure representing the audio device */
struct audio {
    int initialized;            /* device was initialized */
    struct audio_command cmd;   /* the command */
    void *internal;             /* A pointer to the internal data structure */
};

/* Functions */

/* Function to initialize the audio.
 * Returns zero on success.
 */
int audio_init(struct audio *aud);

/* Function to finalize and release the resources
 * used by the audio.
 */
void audio_destroy(struct audio *aud);

/* Refreshes the audio data.
 * This should be called periodically (and with the proper locks
 * to no interfere with `audio_stream_callback()`).
 */
void audio_update(struct audio *aud, struct quivm *qvm);

/* Implementation of the read callback for the audio.
 * The parameter `address` is the address to read. A reference to
 * the QUI vm is given by `qvm`.
 * Returns the value read.
 */
uint32_t audio_read_callback(struct audio *aud,
                             struct quivm *qvm, uint32_t address);

/* Implementation of the write callback for the audio.
 * The parameter `address` is the address to write, and `v` is the value.
 * A reference to the QUI vm is given by `qvm`.
 */
void audio_write_callback(struct audio *aud,  struct quivm *qvm,
                          uint32_t address, uint32_t v);

/* Callback to generate the samples for playback.
 * The stream to be filled is given by `stream`, and it has
 * size of `len` bytes.
 * The `arg` is a user defined parameter for the callback.
 */
void audio_stream_callback(void *arg, uint8_t *stream, int len);


#endif /* __DEV_AUDIO_H */
