#ifndef __DEV_AUDIO_H
#define __DEV_AUDIO_H

#include <stdint.h>

#include "vm/quivm.h"

/* Constants */
/* The I/O address space for the audio device */
#define IO_AUDIO_BASE           0xFFFFFE40
#define IO_AUDIO_END            0xFFFFFE80

/* Addresses within the audio device */

/* Data structures and types */
/* A structure representing the audio device */
struct audio {
    int initialized;            /* device was initialized */
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


#endif /* __DEV_AUDIO_H */
