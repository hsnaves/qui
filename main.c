#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

/* Constants */
#define NUM_INSN_PER_FRAME      6000000

/* Global variables */
#ifdef USE_SDL
SDL_Window *window;             /* the interface window */
SDL_Renderer *renderer;         /* the renderer for the window */
SDL_Texture *texture;           /* the texture for drawing */
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
                              width, height,
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

/* Auxiliary function to process events.
 * The QUI virtual machine is given by parameter `qvm`.
 */
static
void process_events(struct quivm *qvm)
{
    SDL_Event e;
    (void)(qvm); /* UNUSED */

    if (!window) return;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            break;

        case SDL_MOUSEMOTION:
            break;

        case SDL_MOUSEBUTTONDOWN:
            break;

        case SDL_MOUSEBUTTONUP:
            break;

        case SDL_KEYDOWN:
            break;

        case SDL_KEYUP:
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
#ifdef USE_SDL
    uint32_t time0_3x, time_3x, delta_3x;
#endif

    io = (struct devio *) qvm->arg;
    dpl = io->dpl;

#ifdef USE_SDL
    time0_3x = 3 * SDL_GetTicks();
#endif

    while (quivm_run(qvm, NUM_INSN_PER_FRAME)) {
        devio_update(qvm);
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

        time_3x = 3 * SDL_GetTicks();
        delta_3x = time_3x - time0_3x;
        time0_3x = time_3x;

        /* For 30 FPS, 100 / 3 = 33.3333ms */
        if (delta_3x < 100) {
            SDL_Delay((100 - delta_3x + 2) / 3);
            time0_3x += (100 - delta_3x);
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

    if (devio_init(&io)) {
        fprintf(stderr, "main: could not initialize I/O\n");
        return 1;
    }

    if (quivm_init(&qvm, &devio_read_callback,
                   &devio_write_callback, &io)) {
        fprintf(stderr, "main: could not initialize the VM\n");
        devio_destroy(&io);
        return 1;
    }

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
