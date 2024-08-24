#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>

#include <config.h>

#include "chip8.h"

#define CLOCK_FREQUENCY 60
#define CLOCK_PERIOD (1000.0 / CLOCK_FREQUENCY)

#define DEFAULT_SCALING 10
#define DEFAULT_IPS 540

#define MAX_SCALING 20
#define MAX_IPS 1000

extern SDL_Rect pos;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    RGBA_t *background;
    RGBA_t *pixel;
    uint32_t pixels[DISPLAY_WIDTH * DISPLAY_HEIGHT];
} Chip8_Graphics;

void graphics_delay(uint32_t ms);
int graphics_init(Chip8_Graphics *gfx, int scaling, const char *rom);
void graphics_update(Chip8_Graphics *gfx, Chip8_t *system);
void graphics_cleanup(Chip8_Graphics *gfx);

#endif // GRAPHICS_H