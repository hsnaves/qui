#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm/quivm.h"
#include "dev/devio.h"
#include "dev/console.h"
#include "dev/storage.h"
#include "dev/network.h"
#include "dev/rtclock.h"
#include "dev/display.h"
#include "dev/audio.h"

/* Functions */

int devio_init(struct devio *io)
{
    io->cns = NULL;
    io->stg = NULL;
    io->ntw = NULL;
    io->rtc = NULL;
    io->dpl = NULL;
    io->aud = NULL;

    /* initialize the console device */
    io->cns = (struct console *) malloc(sizeof(struct console));
    if (!io->cns) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (console_init(io->cns)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the console device\n");
        devio_destroy(io);
        return 1;
    }

    /* initialize the storage device */
    io->stg = (struct storage *) malloc(sizeof(struct storage));
    if (!io->stg) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (storage_init(io->stg)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the storage device\n");
        devio_destroy(io);
        return 1;
    }

    /* initialize the network device */
    io->ntw = (struct network *) malloc(sizeof(struct network));
    if (!io->ntw) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (network_init(io->ntw)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the network device\n");
        devio_destroy(io);
        return 1;
    }

    /* initialize the clock device */
    io->rtc = (struct rtclock *) malloc(sizeof(struct rtclock));
    if (!io->rtc) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (rtclock_init(io->rtc)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the clock device\n");
        devio_destroy(io);
        return 1;
    }

    /* initialize the display device */
    io->dpl = (struct display *) malloc(sizeof(struct display));
    if (!io->dpl) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (display_init(io->dpl)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the display device\n");
        devio_destroy(io);
        return 1;
    }

    /* initialize the audio device */
    io->aud = (struct audio *) malloc(sizeof(struct audio));
    if (!io->aud) {
        fprintf(stderr, "dev/devio: init: "
                "memory exhausted\n");
        devio_destroy(io);
        return 1;
    }

    if (audio_init(io->aud)) {
        fprintf(stderr, "dev/devio: init: "
                "error while initializing the audio device\n");
        devio_destroy(io);
        return 1;
    }

    return 0;
}

void devio_destroy(struct devio *io)
{
    /* destroy the console device */
    if (io->cns) {
        console_destroy(io->cns);
        free(io->cns);
    }
    io->cns = NULL;

    /* destroy the storage device */
    if (io->stg) {
        storage_destroy(io->stg);
        free(io->stg);
    }
    io->stg = NULL;

    /* destroy the network device */
    if (io->ntw) {
        network_destroy(io->ntw);
        free(io->ntw);
    }
    io->ntw = NULL;

    /* destroy the clock device */
    if (io->rtc) {
        rtclock_destroy(io->rtc);
        free(io->rtc);
    }
    io->rtc = NULL;

    /* destroy the display device */
    if (io->dpl) {
        display_destroy(io->dpl);
        free(io->dpl);
    }
    io->dpl = NULL;

    /* destroy the audio device */
    if (io->aud) {
        audio_destroy(io->aud);
        free(io->aud);
    }
    io->aud = NULL;
}

void devio_update(struct quivm *qvm)
{
    struct devio *io;
    io = (struct devio *) qvm->arg;

    display_update(io->dpl, qvm);
}

uint32_t devio_read_callback(struct quivm *qvm, uint32_t address)
{
    struct devio *io;
    io = (struct devio *) qvm->arg;

    if ((address >= IO_CONSOLE_BASE) && (address < IO_CONSOLE_END)) {
        return console_read_callback(io->cns, qvm, address);
    }
    if ((address >= IO_STORAGE_BASE) && (address < IO_STORAGE_END)) {
        return storage_read_callback(io->stg, qvm, address);
    }
    if ((address >= IO_NETWORK_BASE) && (address < IO_NETWORK_END)) {
        return network_read_callback(io->ntw, qvm, address);
    }
    if ((address >= IO_RTCLOCK_BASE) && (address < IO_RTCLOCK_END)) {
        return rtclock_read_callback(io->rtc, qvm, address);
    }
    if ((address >= IO_DISPLAY_BASE) && (address < IO_DISPLAY_END)) {
        return display_read_callback(io->dpl, qvm, address);
    }
    if ((address >= IO_AUDIO_BASE) && (address < IO_AUDIO_END)) {
        return audio_read_callback(io->aud, qvm, address);
    }

    return -1;
}

void devio_write_callback(struct quivm *qvm, uint32_t address, uint32_t v)
{
    struct devio *io;
    io = (struct devio *) qvm->arg;

    if ((address >= IO_CONSOLE_BASE) && (address < IO_CONSOLE_END)) {
        console_write_callback(io->cns, qvm, address, v);
        return;
    }
    if ((address >= IO_STORAGE_BASE) && (address < IO_STORAGE_END)) {
        storage_write_callback(io->stg, qvm, address, v);
        return;
    }
    if ((address >= IO_NETWORK_BASE) && (address < IO_NETWORK_END)) {
        network_write_callback(io->ntw, qvm, address, v);
        return;
    }
    if ((address >= IO_RTCLOCK_BASE) && (address < IO_RTCLOCK_END)) {
        rtclock_write_callback(io->rtc, qvm, address, v);
        return;
    }
    if ((address >= IO_DISPLAY_BASE) && (address < IO_DISPLAY_END)) {
        display_write_callback(io->dpl, qvm, address, v);
        return;
    }
    if ((address >= IO_AUDIO_BASE) && (address < IO_AUDIO_END)) {
        audio_write_callback(io->aud, qvm, address, v);
        return;
    }
}

