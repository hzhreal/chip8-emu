#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL2/SDL.h>

#include "graphics.h"
#include "chip8.h"

void await_keypress(SDL_Event *event, Chip8_t *system);
void await_unpause(SDL_Event *event, Chip8_t *system);

#endif // KEYBOARD_H