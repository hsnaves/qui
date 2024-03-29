#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#ifdef USE_SDL
#    include <SDL.h>
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
#define NUM_INSN_PER_TICK       100000

/* Global variables */
#ifdef USE_SDL
static SDL_Window *window;      /* the interface window */
static SDL_Renderer *renderer;  /* the renderer for the window */
static SDL_Texture *texture;    /* the texture for drawing */
static int zoom = 1;            /* the zoom level */
static int mouse_captured;      /* the mouse was captured */
#endif

/* Functions */

#ifdef USE_SDL
/* Auxiliary function to destroy the SDL window */
static
void destroy_window(void)
{
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    texture = NULL;
    renderer = NULL;
    window = NULL;
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
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              zoom * width, zoom * height,
                              SDL_WINDOW_SHOWN);

    if (!window) {
        fprintf(stderr, "main: create_window: "
                "could not create window (SDL_Error: %s)\n",
                SDL_GetError());
        return;
    }

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_PRESENTVSYNC
                                  | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1,
                                      SDL_RENDERER_PRESENTVSYNC
                                      | SDL_RENDERER_SOFTWARE);
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
                                SDL_PIXELFORMAT_RGB332,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, height);
    if (!texture) {
        fprintf(stderr, "main: create_window: "
                "could not create texture (SDL_Error: %s)\n",
                SDL_GetError());
        destroy_window();
        return;
    }

    mouse_captured = 0;
}

/* Auxiliary function to update the screen.
 * The QUI virtual machine is given by parameter `qvm`.
 */
static
void update_screen(struct quivm *qvm)
{
    struct devio *io;
    struct display *dpl;
    uint32_t i, address, length;
    uint8_t *pixels;
    int stride, ret;

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
    for (i = 0; i < dpl->height; i++) {
        if (!(address < qvm->memsize)) break;

        length = dpl->width;
        if (length > (qvm->memsize - address))
            length = qvm->memsize - address;

        memcpy(&pixels[stride * i], &qvm->mem[address], length);
        address += dpl->stride;
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

/* To capture the mouse movements (and keyboard).
 * The `capture` indicates whether we should capture or release
 * the mouse movements.
 */
static
void capture_mouse(int capture)
{
    if (capture) {
        SDL_ShowCursor(0);
        SDL_SetWindowGrab(window, SDL_TRUE);
        SDL_SetWindowTitle(window,
                           "QUIVM - Mouse captured. "
                           "Press 'Ctrl+Alt' to release.");
    } else {
        SDL_ShowCursor(1);
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_SetWindowTitle(window, "QUIVM");
    }

    mouse_captured = capture;
}

/* Auxiliary function to process events.
 * The QUI virtual machine is given by parameter `qvm`.
 */
static
void process_events(struct quivm *qvm)
{
    struct devio *io;
    struct keyboard *kbd;
    struct display *dpl;
    uint32_t button;
    SDL_Event e;
    SDL_Keymod mod;
    int x, y;

    if (!window) return;

    io = (struct devio *) qvm->arg;
    kbd = io->kbd;
    dpl = io->dpl;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            break;

        case SDL_MOUSEMOTION:
            if (!mouse_captured) break;

            x = e.motion.x;
            if (x < 0) x = 0;
            if (x >= ((int) dpl->width))
                x = dpl->width - 1;
            kbd->x = x;

            y = e.motion.y;
            if (y < 0) y = 0;
            if (y >= ((int) dpl->height))
                y = dpl->height - 1;
            kbd->y = y;
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (!mouse_captured)
                capture_mouse(1);
            /* fall through */
        case SDL_MOUSEBUTTONUP:
            if (!mouse_captured) break;

            if (e.button.button == SDL_BUTTON_LEFT) {
                button = KEYBOARD_BTN_LEFT;
            } else if (e.button.button == SDL_BUTTON_RIGHT) {
                button = KEYBOARD_BTN_RIGHT;
            } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                button = KEYBOARD_BTN_MIDDLE;
            } else {
                button = 0;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                kbd->button |= button;
            } else {
                kbd->button &= ~button;
            }
            break;

        case SDL_KEYDOWN:
            mod = SDL_GetModState();
            if ((mod & KMOD_CTRL) && (mod & KMOD_ALT)) {
                capture_mouse(0);
            }
            if (e.key.keysym.sym == SDLK_F5) {
                zoom = (zoom == 4) ? 1 : zoom + 1;
                SDL_SetWindowSize(window,
                                  dpl->width * zoom,
                                  dpl->height * zoom);
            }
            /* fall through */

        case SDL_KEYUP:
            if (!mouse_captured) break;
            break;
        }
    }
}

#endif /* USE_SDL */

/* Auxiliary function to run the VM with a given set of I/O devices.
 * Returns zero on success.
 */
static
int run(struct quivm *qvm)
{
    struct devio *io;
    struct display *dpl;
    unsigned int i;
#ifdef USE_SDL
    uint32_t time0, delta;

    /* multiply time by 3 for higher resolution */
    time0 = 3 * SDL_GetTicks();
#endif

    io = (struct devio *) qvm->arg;
    dpl = io->dpl;

    while (1) {
        for (i = 0; i < NUM_TICKS_PER_FRAME; i++) {
            if (!quivm_run(qvm, NUM_INSN_PER_TICK))
                break;
            devio_update(io);
        }
        if (qvm->status & STS_TERMINATED)
            break;

#ifdef USE_SDL
        if (dpl->initialized && !window) {
            create_window(dpl->width, dpl->height);
            if (!window) {
                qvm->status |= STS_TERMINATED;
                qvm->termvalue = 1;
                break;
            }
        }
        process_events(qvm);
        update_screen(qvm);

        delta = (3 * SDL_GetTicks()) - time0;
        time0 += delta;

        /* For 60 FPS, 50 / 3 = 16.666ms */
        if (delta < 50) {
            /* round up the delay */
            SDL_Delay((50 - delta + 2) / 3);
            time0 += (50 - delta);
        }
#else
        /* No support for display here */
        if (dpl->initialized) {
            qvm->status |= STS_TERMINATED;
            qvm->termvalue = 1;
            break;
        }
#endif
    }

    return qvm->termvalue;
}

/* Prints the help text to the console.
 * The name of the executable is given in `execname`.
 */
static
void print_help(const char *execname)
{
    printf("Usage:\n");
    printf("  %s [-r <romfile>] [--readonly] [--bind <addr>]\n", execname);
    printf("        [--target <addr>] [--port <port> ] [--utc]\n");
    printf("        [-h|--help] args...\n");

    printf("where:\n");
    printf("  -r <romfile>    Specify the rom file to use\n");
    printf("  --readonly      To not allow writes in the storage device\n");
    printf("  --bind <addr>   Binds the UDP socket to a given address\n");
    printf("  --target <addr> The address of the target socket\n");
    printf("  --port <port>   The UDP port to bind to\n");
    printf("  --utc           To use UTC for the real time clock\n");
    printf("  -h|--help       Print this help\n");
}

/* main function */
int main(int argc, char **argv, char **envp)
{
    struct quivm qvm;
    struct devio io;
    const char *filename;
    const char *bind_address;
    const char *target_address;
    uint32_t length;
    int use_utc;
    int disable_write;
    int port;
    int i, ret;

    filename = "rom.bin";
    bind_address = NULL;
    target_address = NULL;
    use_utc = 0;
    disable_write = 0;
    port = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i == argc - 1) goto missing_argument;
            filename = argv[++i];
        } else if (strcmp(argv[i], "--readonly") == 0) {
            disable_write = 1;
        } else if (strcmp(argv[i], "--utc") == 0) {
            use_utc = 1;
        } else if (strcmp(argv[i], "--port") == 0) {
            char *end;
            if (i == argc - 1) goto missing_argument;
            port = strtol(argv[++i], &end, 10);
            if (end[0] != '\0') {
                fprintf(stderr, "main: "
                        "invalid port `%s`\n",
                        argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--bind") == 0) {
            if (i == argc - 1) goto missing_argument;
            bind_address = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0) {
            if (i == argc - 1) goto missing_argument;
            target_address = argv[++i];
        } else if ((strcmp(argv[i], "-h") == 0)
                   || (strcmp(argv[i], "--help") == 0)) {
            print_help(argv[0]);
            return 0;
        } else {
            break;
        }
        continue;

    missing_argument:
        fprintf(stderr, "main: "
                "missing argument for `%s`\n",
                argv[i]);
        return 1;
    }

    /* Move the arguments forward */
    argc -= i;
    argv = &argv[i];

    if (quivm_init(&qvm)) {
        fprintf(stderr, "main: could not initialize the VM\n");
        return 1;
    }

    if (devio_init(&io)) {
        fprintf(stderr, "main: could not initialize I/O\n");
        quivm_destroy(&qvm);
        return 1;
    }

    devio_configure(&io, &qvm);

    length = 0;
    if (quivm_load(&qvm, filename, 0, &length)) {
        fprintf(stderr, "main: could not load image `%s`\n", filename);
        quivm_destroy(&qvm);
        devio_destroy(&io);
        return 1;
    }

#ifdef USE_SDL
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
    if (ret < 0) {
        fprintf(stderr, "main: "
                "could not initialize SDL (SDL_Error(%d): %s)\n",
                ret, SDL_GetError());

        quivm_destroy(&qvm);
        devio_destroy(&io);
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
#endif

    /* set up the options */
    io.stg->disable_write = disable_write;
    io.rtc->use_utc = use_utc;
    if (port != 0) io.ntw->port = port;
    if (bind_address) io.ntw->bind_address = bind_address;
    if (target_address) io.ntw->target_address = target_address;

    /* set up the arguments and enviroment variables */
    io.cns->argc = argc;
    io.cns->argv = argv;
    io.cns->envp = envp;

    ret = run(&qvm);

#ifdef USE_SDL
    destroy_window();
    SDL_Quit();
#endif

    quivm_destroy(&qvm);
    devio_destroy(&io);
    return ret;
}
