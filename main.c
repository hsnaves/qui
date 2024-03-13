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

/* Constants */
#define NUM_INSN_PER_FRAME      1000000

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
    uint8_t *pixels;
    int stride, ret;
    uint32_t i;

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

    for (i = 0; i < dpl->height; i++) {
        memcpy(&pixels[stride * i],
               &dpl->buffer[dpl->width * i],
               dpl->width * sizeof(uint8_t));
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

/* Auxiliary function to run the VM with a given set of I/O devices. */
static
int run(struct quivm *qvm, int argc, char **argv, char **envp)
{
    struct devio *io;
    struct console *cns;
    struct display *dpl;

    io = (struct devio *) qvm->arg;
    cns = io->cns;
    dpl = io->dpl;

    /* set up the arguments and enviroment variables */
    cns->argc = argc;
    cns->argv = argv;
    cns->envp = envp;

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
    printf("  %s [-r <romfile>] [-h|--help] args...\n", execname);
    printf("where:\n");
    printf("  -r <romfile>    Specify the rom file to use\n");
    printf("  -h|--help       Print this help\n");
}

/* main function */
int main(int argc, char **argv, char **envp)
{
    struct quivm qvm;
    struct devio io;
    const char *filename;
    uint32_t length;
    int i, ret;

    filename = "rom.bin";
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "main: "
                        "missing argument for `-r`\n");
                return 1;
            }
            filename = argv[++i];
        } else if ((strcmp(argv[i], "-h") == 0)
                   || (strcmp(argv[i], "--help") == 0)) {
            print_help(argv[0]);
            return 0;
        } else {
            break;
        }
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

    ret = run(&qvm, argc, argv, envp);

#ifdef USE_SDL
    destroy_window();
    SDL_Quit();
#endif

    quivm_destroy(&qvm);
    devio_destroy(&io);
    return ret;
}
