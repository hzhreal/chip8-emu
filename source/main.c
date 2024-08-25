#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#if defined(DEBUG)
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>

static int str_to_u16(const char *s, uint16_t *i) {
    long l;
    char *end = NULL;
    errno = 0;
    l = strtol(s, &end, 16);
    if (errno != 0 || l > UINT16_MAX || l < 0 || *end != '\0') {
        return -1;
    }
    *i = (uint16_t)l;
    return 0;
}
#endif

#include <config.h>
#include <log.h>

#include "chip8.h"
#include "graphics.h"
#include "keyboard.h"
#include "utils.h"

#define CONFIG_FILE_PATH "chip8-emu.conf"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "%s <path to ROM>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    /* INITIALIZE THE CHIP-8 SYSTEM */
    Chip8_t sys;
    chip8_initialize(&sys);
    int res = load_rom(&sys, argv[1]);
    switch (res) {
        case -1:
            fprintf(stderr, "INVALID ROM PATH!\n");
            return 1;
        case -2:
            fprintf(stderr, "ROM IS TOO BIG!\n");
            return 1;
        case -3:
            fprintf(stderr, "FAILED TO LOAD ROM!\n");
            return 1;
        default:
            printf("%s loaded!\n", argv[1]);
            break;
    }

    /* USER-CONFIGURATION */
    int scaling;
    int ips;
    RGBA_t background, pixel;
    ConfigTable *table = config_parse_file(CONFIG_FILE_PATH);

    if (table) {
        if (config_get_int(table, "scaling", "graphics", 10, &scaling) != 0) {
            scaling = DEFAULT_SCALING;
        }
        else {
            if (scaling < 1) scaling = DEFAULT_SCALING;
            if (scaling > MAX_SCALING) scaling = MAX_SCALING;
        }

        if (config_get_int(table, "instructions", "ips", 10, &ips) != 0) {
            ips = DEFAULT_IPS;
        }
        else {
            if (ips < 1) ips = DEFAULT_IPS;
            if (ips > MAX_IPS) ips = MAX_IPS;
        }
    }
    else {
        scaling = DEFAULT_SCALING;
        ips = DEFAULT_IPS;
    }

    /* INITIALIZE GRAPHICS */
    Chip8_Graphics gfx;
    if (table) {
        if (config_get_rgba(table, "background", "color", &background) != 0) {
            background.red = 0;
            background.green = 0;
            background.blue = 0;
            background.alpha = 255;
        }
        if (config_get_rgba(table, "pixel", "color", &pixel) != 0) {
            pixel.red = 255;
            pixel.green = 255;
            pixel.blue = 255;
            pixel.alpha = 255;
        }
    }
    else {
        background.red = 0;
        background.green = 0;
        background.blue = 0;
        background.alpha = 255;

        pixel.red = 255;
        pixel.green = 255;
        pixel.blue = 255;
        pixel.alpha = 255;
    }
    gfx.background = &background;
    gfx.pixel = &pixel;

    if (graphics_init(&gfx, scaling, argv[1]) < 0) {
        LOG_TO_STREAM(stderr, LOG_LEVEL_CRITICAL, LOG_FLAGS, "FAILED TO INITIALIZE GRAPHICS: %s", SDL_GetError());
        graphics_cleanup(&gfx);
    }
    SDL_Event event;

    /* We need to control execution by time */
    int i;
    uint64_t start, end;
    double elapsed_time;
    const double freq = SDL_GetPerformanceFrequency();
    int ipf = ips / CLOCK_FREQUENCY;

    #if defined(DEBUG)
    bool run = false;
    uint16_t executed = 0;
    uint16_t exec_max = 0;
    #endif

    /* EMU LOOP*/
    for (;;) {
        start = SDL_GetPerformanceCounter();

        await_keypress(&event, &sys);

        #if defined(DEBUG)
        char line[0x100] = {0};
        char arg;
        char *delim;
        if (!run) {
            for (;;) {
                printf("> ");
                if (fgets(line, sizeof(line), stdin)) {
                    arg = line[0];
                    switch (arg) {
                        case 's':
                            break;
                        case 'b':
                            sys.pc -= sizeof(uint16_t);
                            break;
                        case 'd':
                            chip8_print(&sys);
                            break;
                        case 'g':
                            delim = strtok(line + 1, " \n");
                            if (delim) {
                                if (str_to_u16(delim, &sys.pc) != 0) {
                                    printf("Failed to parse.\n");
                                }
                            }
                            else {
                                printf("g <0x0>\n");
                            }
                            break;
                        case 'm':
                            uint16_t p = 0;
                            delim = strtok(line + 1, " \n");
                            if (delim) {
                                if (str_to_u16(delim, &p) == 0) {
                                    printf("%" PRIX8 "\n", sys.memory[p]);
                                }
                                else {
                                    printf("Failed to parse.\n");
                                }
                            }
                            else {
                                printf("m <0x0>\n");
                            }
                            break;
                        case 'r':
                            delim = strtok(line + 1, " \n");
                            if (delim) {
                                if (str_to_u16(delim, &exec_max) == 0) {
                                    if (exec_max > 0) {
                                        run = true;
                                    }
                                    else {
                                        printf("Number of instructions to execute is not set.\n");
                                    }
                                }
                                else {
                                    printf("Failed to parse\n");
                                }
                            }
                            else {
                                printf("r <0x0>\n");
                            }
                            break;
                        case 'q':
                            sys.EMU_flags.exit = 1;
                            run = true;
                            break;
                        case 'h':
                            printf("'s'       - Step Forward\n");
                            printf("'b'       - Step Back\n");
                            printf("'d'       - Dump registers, stack, and opcode\n");
                            printf("'g <0x0>' - Go to location\n");
                            printf("'m <0x0>' - Print value at memory location\n");
                            printf("'r <0x0>' - Run amount of instructions\n");
                            printf("'q'       - Quit\n");
                            printf("'h'       - Show all commands\n");
                            break;
                        default:
                            printf("Unknown command.\n");
                            break;
                    }
                }
                if (arg == 's' || arg == 'b' || arg == 'q' || arg == 'g' || arg == 'r') {
                    break;
                }
            }
        }
        #endif

        /* Execute the amount of instructions per frame*/
        for (i = 0; i < ipf; i++) {
            chip8_emulatecycle(&sys);
            #if defined(DEBUG)
            executed++;
            if (executed >= exec_max) {
                break;
            }
            #endif
        }

        chip8_update_timers(&sys);

        if (sys.EMU_flags.draw_to_screen) {
            graphics_update(&gfx, &sys);
        }

        if (sys.EMU_flags.exit) {
            printf("Exiting...\n");
            break;
        }
        else if (sys.EMU_flags.pause) {
            printf("Paused\n");
            await_unpause(&event, &sys);
            printf("Unpaused\n");
        }
        else if (sys.EMU_flags.restart) {
            printf("Restarting...\n");
            chip8_initialize(&sys);
            load_rom(&sys, argv[1]);
            printf("Restarted\n");
        }

        /* Maintain within the clock period */
        end = SDL_GetPerformanceCounter();
        elapsed_time = ((end - start) * 1000) / freq;

        if (elapsed_time < CLOCK_PERIOD) {
            SDL_Delay((uint32_t)(CLOCK_PERIOD - elapsed_time));
        }

        #if defined(DEBUG)
        if (executed >= exec_max) {
            run = false;
            executed = 0;
        }
        #endif
    }

    graphics_cleanup(&gfx);
    if (table) {
        config_cleanup(table);
    }

    return 0;
}