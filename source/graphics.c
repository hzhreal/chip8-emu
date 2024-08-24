#include <SDL2/SDL.h>
#include <inttypes.h>

#include "graphics.h"
#include "chip8.h"

SDL_Rect pos = {.h = DISPLAY_HEIGHT, .w = DISPLAY_WIDTH, .x = 0, .y = 0};

void graphics_delay(uint32_t ms) {
    SDL_Delay(ms);
    return;
}

void graphics_update(Chip8_Graphics *gfx, Chip8_t *system) {
    int x, y;

    uint32_t pixel_color = (gfx->pixel->red << 24) | (gfx->pixel->green << 16) | (gfx->pixel->blue << 8) | (gfx->pixel->alpha);
    uint32_t background_color = (gfx->background->red << 24) | (gfx->background->green << 16) | (gfx->background->blue << 8) | (gfx->background->alpha);
    
    memset(gfx->pixels, 0, sizeof(gfx->pixels));

    for (x = 0; x < DISPLAY_WIDTH; x++) {
        for (y = 0; y < DISPLAY_HEIGHT; y++) {
            if (system->gfx[x + (y * DISPLAY_WIDTH)]) {
                gfx->pixels[x + (y * DISPLAY_WIDTH)] = pixel_color;
            }
            else {
                gfx->pixels[x + (y * DISPLAY_WIDTH)] = background_color;
            }
        }
    }

    SDL_UpdateTexture(gfx->texture, NULL, gfx->pixels, DISPLAY_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(gfx->renderer);
    SDL_RenderCopy(gfx->renderer, gfx->texture, NULL, &pos);
    SDL_RenderPresent(gfx->renderer);

    system->EMU_flags.draw_to_screen = 0;
    return;
}

int graphics_init(Chip8_Graphics *gfx, int scaling, const char *rom) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        return -1;
    }

    gfx->window = SDL_CreateWindow(rom, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH * scaling, DISPLAY_HEIGHT * scaling, 0);
    if (!gfx->window) {
        return -2;
    }

    gfx->renderer = SDL_CreateRenderer(gfx->window, -1, SDL_RENDERER_ACCELERATED);
    if (!gfx->renderer) {
        return -3;
    };

    gfx->texture = SDL_CreateTexture(gfx->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!gfx->texture) {
        return -4;
    }

    SDL_RenderSetLogicalSize(gfx->renderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    SDL_SetRenderDrawColor(gfx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(gfx->renderer);

    return 0;
}

void graphics_cleanup(Chip8_Graphics *gfx) {
    if (!gfx) return;

    if (gfx->texture) {
        SDL_DestroyTexture(gfx->texture);
        gfx->texture = NULL;
    }
    if (gfx->renderer) {
        SDL_DestroyRenderer(gfx->renderer);
        gfx->renderer = NULL;
    }
    if (gfx->window) {
        SDL_DestroyWindow(gfx->window);
        gfx->window = NULL;
    }

    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_Quit();
    return;
}