#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

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
    int executed = 0;
    #endif

    /* EMU LOOP*/
    for (;;) {
        start = SDL_GetPerformanceCounter();

        await_keypress(&event, &sys);

        /* Execute the amount of instructions per frame*/
        for (i = 0; i < ipf; i++) {
            chip8_emulatecycle(&sys);
            if (sys.EMU_flags.exit) {
                break;
            }
            #if defined(DEBUG)
            executed++;
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
        if (executed >= 100)
            break;
        #endif
    }

    graphics_cleanup(&gfx);
    if (table) {
        config_cleanup(table);
    }

    return 0;
}