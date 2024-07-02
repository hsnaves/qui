#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef USE_SDL
#include <SDL.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "vm/quivm.h"
#include "dev/devio.h"
#include "dev/console.h"
#include "dev/storage.h"
#include "dev/network.h"
#include "dev/rtclock.h"
#include "dev/display.h"
#include "dev/audio.h"
#include "dev/keyboard.h"
#include "dev/timer.h"

/* Constants */
#define NUM_INSN_PER_FRAME         1000000

/* Global variables */
#ifdef USE_SDL
static SDL_Window *window;      /* the interface window */
static SDL_Renderer *renderer;  /* the renderer for the window */
static SDL_Texture *texture;    /* the texture for drawing */
static SDL_AudioDeviceID audio_id; /* the id of the audio device */
static int zoom = 1;            /* the zoom level */
static int mouse_captured;      /* the mouse was captured */
static SDL_Joystick *joystick;  /* the joystick device */
#ifndef __EMSCRIPTEN__
static int disable_throttle;    /* to disable the cpu throttle */
#endif
#endif

#ifdef INCLUDE_DEFAULT_ROM
#    include "default_rom.c"
#else
/* just an infinite loop here */
static const uint8_t default_rom[] = { 0xBE, 0xC2 };
#endif

/* Functions */

#ifdef USE_SDL
/* Auxiliary function to destroy the SDL window */
static
void destroy_window(void)
{
    if (joystick) SDL_JoystickClose(joystick);
    if (window) SDL_SetWindowTitle(window, "QUIVM - Terminated");
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    joystick = NULL;
    texture = NULL;
    renderer = NULL;
    window = NULL;
}

/* To capture the mouse movements (and keyboard).
 * The `capture` indicates whether we should capture or release
 * the mouse movements.
 */
static
void capture_mouse(int capture)
{
    if (capture) {
        SDL_SetRelativeMouseMode(1);
        SDL_SetWindowTitle(window,
                           "QUIVM - Mouse captured. "
                           "Press 'Ctrl+Alt' to release.");
    } else {
        SDL_SetRelativeMouseMode(0);
        SDL_SetWindowTitle(window, "QUIVM");
    }

    mouse_captured = capture;
}

/* Auxiliary function to create an SDL window
 * The width and height are specified by the parameters
 * `width` and `height`, respectively.
 */
static
void create_window(uint32_t width, uint32_t height)
{
    if (window) return;
    window = SDL_CreateWindow("QUIVM",
                              SDL_WINDOWPOS_CENTERED_DISPLAY(0),
                              SDL_WINDOWPOS_CENTERED_DISPLAY(0),
                              zoom * width, zoom * height,
                              SDL_WINDOW_SHOWN);

    if (!window) {
        fprintf(stderr, "main: create_window: "
                "could not create window (SDL_Error: %s)\n",
                SDL_GetError());
        return;
    }

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "main: create_window: "
                "using software renderer\n");
        renderer = SDL_CreateRenderer(window, -1,
                                      SDL_RENDERER_SOFTWARE);
    }

    if (!renderer) {
        fprintf(stderr, "main: create_window: "
                "could not create renderer (SDL_Error: %s)",
                SDL_GetError());
        destroy_window();
        return;
    }

    SDL_RenderSetLogicalSize(renderer, width, height);
    SDL_RenderSetIntegerScale(renderer, 1);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_RGB24,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, height);
    if (!texture) {
        fprintf(stderr, "main: create_window: "
                "could not create texture (SDL_Error: %s)\n",
                SDL_GetError());
        destroy_window();
        return;
    }

    if (SDL_NumJoysticks() > 0) {
        joystick = SDL_JoystickOpen(0);
        if (joystick == NULL) {
            fprintf(stderr,"main: unable to open game controller: "
                    " SDL Error: %s\n", SDL_GetError());
        }
    }

    capture_mouse(1);
}

/* Auxiliary function to update the screen.
 * The QUI virtual machine is given by parameter `qvm`.
 */
static
void update_screen(struct quivm *qvm)
{
    struct devio *io;
    struct display *dpl;
    uint32_t i, j;
    uint32_t address, width;
    uint8_t *pixels;
    int stride, ret;
    int valid;

    /* Check if texture and renderer are defined. */
    if (!texture || !renderer) return;

    io = (struct devio *) qvm->arg;
    dpl = io->dpl;

    ret = SDL_LockTexture(texture, NULL, (void **) &pixels, &stride);
    if (ret < 0) {
        fprintf(stderr, "main: update_screen: "
                "could not lock texture (SDL_Error(%d): %s)",
                ret, SDL_GetError());
        return;
    }

    address = dpl->buffer;
    if (dpl->bpp == 24) {
        width = 3 * dpl->width;
        valid = 1;
    } else {
        /* 8 bits per pixel */
        width = dpl->width;
        valid = (dpl->palette != 0);
    }
    valid = valid && (!check_buffer2d(address, dpl->stride,
                                      width, dpl->height,
                                      qvm->memsize));

    if (valid) {
        if (dpl->bpp == 24) {
            for (i = 0; i < dpl->height; i++) {
                memcpy(pixels, &qvm->mem[address], width);
                pixels += stride;
                address += dpl->stride;
            }
        } else { /* 8 bits per pixel */
            for (i = 0; i < dpl->height; i++) {
                uint32_t palette_address;
                uint32_t pos;

                pos = 0;
                for (j = 0; j < width; j++) {
                    palette_address = dpl->palette;
                    palette_address += 3 * ((uint32_t) qvm->mem[address + j]);
                    pixels[pos++] = qvm->mem[palette_address++];
                    pixels[pos++] = qvm->mem[palette_address++];
                    pixels[pos++] = qvm->mem[palette_address];
                }
                pixels += stride;
                address += dpl->stride;
            }
        }
    }

    SDL_UnlockTexture(texture);

    ret = SDL_RenderCopy(renderer, texture, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "main: update_screen: "
                "could not copy texture (SDL_Error(%d): %s)",
                ret, SDL_GetError());
        return;
    }

    SDL_RenderPresent(renderer);
}

/* Auxiliary function to process events.
 * The QUI virtual machine is given by parameter `qvm`.
 */
static
void process_events(struct quivm *qvm)
{
    static int quit_counter;
    struct devio *io;
    struct keyboard *kbd;
    struct display *dpl;
    uint32_t bit;
    SDL_Event e;
    SDL_Keymod mod;

    io = (struct devio *) qvm->arg;
    kbd = io->kbd;
    dpl = io->dpl;

    keyboard_clear_mouse(kbd);
    if (quit_counter > 0) quit_counter--;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            quit_counter += 30;
            if (!window || (quit_counter > 30)) {
               /* Ctrl-C was pressed and SDL captured the SIGINT */
               qvm->status |= STS_TERMINATED;
               qvm->termvalue = 1;
               break;
            }
            keyboard_set_bit(kbd, KEYBOARD_QUIT, 1);
            break;
        case SDL_KEYDOWN:
            mod = SDL_GetModState();
            if ((mod & KMOD_CTRL) && (mod & KMOD_ALT)) {
                capture_mouse(0);
            }
            if ((e.key.keysym.sym == SDLK_F2) && (mod & KMOD_CTRL)) {
                zoom = (zoom == 4) ? 1 : zoom + 1;
                if (window) {
                    SDL_SetWindowSize(window,
                                      dpl->width * zoom,
                                      dpl->height * zoom);
                }
            }
            /* fall through */
        case SDL_KEYUP:
            if (!mouse_captured) break;

            if (e.key.keysym.scancode >= 0x04
                && e.key.keysym.scancode <= 0x63) {
                bit = (e.key.keysym.scancode - 04);
                bit += KEYBOARD_KEY_A;
            } else if (e.key.keysym.scancode >= 0xE0
                       && e.key.keysym.scancode <= 0xE7) {
                bit = (e.key.keysym.scancode - 0xE0);
                bit += KEYBOARD_KEY_LCTRL;
            } else {
                break;
            }

            keyboard_set_bit(kbd, bit, (e.type == SDL_KEYDOWN));
            break;
        case SDL_MOUSEMOTION:
            if (!mouse_captured) break;
            keyboard_move_mouse(kbd, e.motion.xrel, e.motion.yrel);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (!mouse_captured) {
                capture_mouse(1);
                break;
            }
            /* fall through */
        case SDL_MOUSEBUTTONUP:
            if (!mouse_captured) break;

            if (e.button.button == SDL_BUTTON_LEFT) {
                bit = KEYBOARD_MOUSE_LEFT;
            } else if (e.button.button == SDL_BUTTON_RIGHT) {
                bit = KEYBOARD_MOUSE_RIGHT;
            } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                bit = KEYBOARD_MOUSE_MIDDLE;
            } else {
                break;
            }

            keyboard_set_bit(kbd, bit, (e.type == SDL_MOUSEBUTTONDOWN));
            break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            if (e.jbutton.button < 12) {
                bit = e.jbutton.button;
                bit += KEYBOARD_JOY_BTN0;
                keyboard_set_bit(kbd, bit, (e.type == SDL_JOYBUTTONDOWN));
            }
            break;
        case SDL_JOYAXISMOTION:
            if (e.jaxis.axis < 2) {
                bit = 140 + 2 * e.jaxis.axis;
                keyboard_set_bit(kbd, bit, 0);
                keyboard_set_bit(kbd, bit + 1, 0);

                if (e.jaxis.value > 5200) {
                    bit++;
                } else if (e.jaxis.value >= -5200) {
                    break;
                }
                keyboard_set_bit(kbd, bit, 1);
            }
            break;
        }
    }
}

/* Auxiliary function to stop the SDL audio device */
static
void stop_audio(void)
{
    if (audio_id) SDL_CloseAudioDevice(audio_id);
    audio_id = 0;
}

/* Starts the audio device */
static
void start_audio(struct audio *aud)
{
    SDL_AudioSpec as;

    SDL_zero(as);
    as.freq = AUDIO_FREQUENCY;
    as.format = AUDIO_S8;
    as.channels = 1;
    as.callback = &audio_stream_callback;
    as.samples = 2048;
    as.userdata = aud;
    audio_id = SDL_OpenAudioDevice(NULL, 0, &as, NULL, 0);
    if (!audio_id) {
        fprintf(stderr, "main: start_audio: "
                "could not start audio (SDL_Error: %s)",
                SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audio_id, 1);
}

#endif /* USE_SDL */

/* Runs a single frame of simulation (implementation)
 * Returns zero on success.
 */
static
int do_run_one_frame(struct quivm *qvm)
{
    struct devio *io;
    struct display *dpl;
    struct audio *aud;

    io = (struct devio *) qvm->arg;
    dpl = io->dpl;
    aud = io->aud;

    if (!quivm_run(qvm, NUM_INSN_PER_FRAME))
        return 1;

#ifdef USE_SDL
    if (audio_id) {
        SDL_LockAudioDevice(audio_id);
        devio_update(io);
        SDL_UnlockAudioDevice(audio_id);
    } else {
        devio_update(io);
    }
#else
    devio_update(io);
#endif

    /* check if some device terminated the VM */
    if (qvm->status & STS_TERMINATED)
        return 1;

#ifdef USE_SDL
    if (dpl->initialized && !window) {
        create_window(dpl->width, dpl->height);
        if (!window) {
            qvm->status |= STS_TERMINATED;
            qvm->termvalue = 1;
            return 1;
        }
    }
    if (aud->initialized) {
        if (!audio_id) {
            start_audio(aud);
            if (!audio_id) {
                qvm->status |= STS_TERMINATED;
                qvm->termvalue = 1;
                return 1;
            }
        }
        SDL_PauseAudioDevice(audio_id, aud->paused);
    }

    process_events(qvm);
    update_screen(qvm);
#else /* !USE_SDL */
    /* No support for display or audio */
    if (dpl->initialized || aud->initialized) {
        qvm->status |= STS_TERMINATED;
        qvm->termvalue = 1;
        return 1;
    }
#endif
    return 0;
}

/* Runs a single frame of simulation */
static
void run_one_frame(void *arg)
{
    struct quivm *qvm;
    struct devio *io;

    qvm = (struct quivm *) arg;
    io = (struct devio *) qvm->arg;

    if (do_run_one_frame(qvm)) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
#ifdef USE_SDL
        stop_audio();
        destroy_window();
        SDL_Quit();
#endif
        quivm_destroy(qvm);
        devio_destroy(io);
    }
}


/* Auxiliary function to run the VM with a given set of I/O devices.
 * Returns zero on success.
 */
static
int run(struct quivm *qvm)
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(&run_one_frame, qvm, 60, 0);
    return 0;
#else
#ifdef USE_SDL
    uint32_t time0, delta;

    /* multiply time by 3 for higher resolution */
    time0 = 3 * SDL_GetTicks();
#endif

    while (1) {
        run_one_frame(qvm);
        if (qvm->status & STS_TERMINATED)
            break;

#ifdef USE_SDL
        delta = (3 * SDL_GetTicks()) - time0;
        time0 += delta;

        if (!disable_throttle && (delta < (3 * 1000 / FPS))) {
            /* For 60 FPS, 1000 / 60 = 50 / 3 = 16.666ms */
            SDL_Delay(((3 * 1000 / FPS) - delta + 2) / 3);
            time0 += ((3 * 1000 / FPS) - delta);
        }
#endif
    }
    return qvm->termvalue;
#endif /* ! __EMSCRIPTEN__ */
}

/* Prints the help text to the console.
 * The name of the executable is given in `execname`.
 */
static
void print_help(const char *execname)
{
    printf("Usage:\n");
    printf("  %s [OPTIONS] [--] args ...\n", execname);
    printf("where the available options are:\n");
    printf("  -r <romfile>       Specify the rom file to use\n");
    printf("  --stackize <size>  The stack size in cells\n");
    printf("  --memsize <size>   The memory size in bytes\n");
    printf("  --readonly         To not allow writes in the storage device\n");
    printf("  --bind <addr>      Binds the UDP socket to a given address\n");
    printf("  --target <addr>    The address of the target socket\n");
    printf("  --port <port>      The UDP port to bind to\n");
    printf("  --utc              To use UTC for the real time clock\n");
#ifdef USE_SDL
    printf("  --zoom <zoom>      Set the initial zoom level\n");
    printf("  --joystick         To enable joystick use\n");
#ifndef __EMSCRIPTEN__
    printf("  --nothrottle       To disable speed throttle\n");
#endif
#endif /* USE_SDL */
    printf("  -h|--help          Print this help\n");
}

/* main function */
int main(int argc, char **argv, char **envp)
{
    /* static variables are not destroyed at the end of main
     * this is necessary when using emscripten.
     */
    static struct quivm qvm;
    static struct devio io;

    const char *filename;
    const char *bind_address;
    const char *target_address;
    uint32_t stacksize, memsize;
    uint32_t length;
    int use_utc;
    int disable_write;
    int port;
#ifdef USE_SDL
    int enable_joystick;
#endif
    int i, ret;
    char *end;

    stacksize = 0x0400;
    memsize = 0x100000;

    filename = NULL;
    bind_address = NULL;
    target_address = NULL;
    use_utc = 0;
    disable_write = 0;
    port = 0;
#ifdef USE_SDL
    enable_joystick = 0;
#ifndef __EMSCRIPTEN__
    disable_throttle = 0;
#endif
#endif

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i == argc - 1) goto missing_argument;
            filename = argv[++i];
        } else if (strcmp(argv[i], "--stacksize") == 0) {
            if (i == argc - 1) goto missing_argument;
            stacksize = strtol(argv[++i], &end, 10);
            if (end[0] != '\0') goto invalid_argument;
        } else if (strcmp(argv[i], "--memsize") == 0) {
            if (i == argc - 1) goto missing_argument;
            memsize = strtol(argv[++i], &end, 10);
            if (end[0] != '\0') goto invalid_argument;
        } else if (strcmp(argv[i], "--readonly") == 0) {
            disable_write = 1;
        } else if (strcmp(argv[i], "--utc") == 0) {
            use_utc = 1;
        } else if (strcmp(argv[i], "--port") == 0) {
            if (i == argc - 1) goto missing_argument;
            port = strtol(argv[++i], &end, 10);
            if (end[0] != '\0') goto invalid_argument;
        } else if (strcmp(argv[i], "--bind") == 0) {
            if (i == argc - 1) goto missing_argument;
            bind_address = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0) {
            if (i == argc - 1) goto missing_argument;
            target_address = argv[++i];
#ifdef USE_SDL
        } else if (strcmp(argv[i], "--zoom") == 0) {
            if (i == argc - 1) goto missing_argument;
            zoom = strtol(argv[++i], &end, 10);
            if ((end[0] != '\0') || (zoom < 1) || (zoom > 4))
                goto invalid_argument;
        } else if (strcmp(argv[i], "--joystick") == 0) {
            enable_joystick = 1;
#ifndef __EMSCRIPTEN__
        } else if (strcmp(argv[i], "--nothrottle") == 0) {
            disable_throttle = 1;
#endif
#endif
        } else if ((strcmp(argv[i], "-h") == 0)
                   || (strcmp(argv[i], "--help") == 0)) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--") == 0) {
            i++; break;
        } else {
            break;
        }
        continue;

    missing_argument:
        fprintf(stderr, "main: "
                "missing argument for `%s`\n", argv[i]);
        return 1;

    invalid_argument:
        fprintf(stderr, "main: "
                "invalid argument for `%s`: `%s`\n",
                argv[i - 1], argv[i]);
        return 1;
    }

    /* Move the arguments forward */
    argc -= i;
    argv = &argv[i];

    if (quivm_init(&qvm, stacksize, memsize)) {
        fprintf(stderr, "main: could not initialize the VM\n");
        return 1;
    }

    if (devio_init(&io)) {
        fprintf(stderr, "main: could not initialize I/O\n");
        quivm_destroy(&qvm);
        return 1;
    }

    devio_configure(&io, &qvm);

    if (filename) {
        length = 0;
        if (quivm_load(&qvm, filename, 0, &length)) {
            fprintf(stderr, "main: could not load image `%s`\n",
                    filename);
            quivm_destroy(&qvm);
            devio_destroy(&io);
            return 1;
        }
    } else {
        length = sizeof(default_rom);
        quivm_load_array(&qvm, default_rom, 0, &length);
    }

#ifdef USE_SDL
    ret = SDL_Init(SDL_INIT_VIDEO
                   | SDL_INIT_AUDIO
                   | ((enable_joystick) ? SDL_INIT_JOYSTICK : 0)
                   | SDL_INIT_NOPARACHUTE);
    if (ret < 0) {
        fprintf(stderr, "main: "
                "could not initialize SDL (SDL_Error(%d): %s)\n",
                ret, SDL_GetError());

        quivm_destroy(&qvm);
        devio_destroy(&io);
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SCALING, "1");
#endif

    /* configure the devices */
    storage_configure(io.stg, disable_write);
    rtclock_configure(io.rtc, use_utc);
    network_configure(io.ntw, bind_address, target_address, port);
    console_configure(io.cns, argc,
                      (const char * const *) argv,
                      (const char * const *) envp);

    ret = run(&qvm);
    return ret;
}
