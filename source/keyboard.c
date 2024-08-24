#include <SDL2/SDL.h>

#include "chip8.h"
#include "graphics.h"
#include "keyboard.h"

void await_keypress(SDL_Event *event, Chip8_t *system) {
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                system->EMU_flags.exit = 1;
                break;

            case SDL_KEYDOWN:
                switch (event->key.keysym.sym) {
                    case SDLK_SPACE:
                        system->EMU_flags.pause = 1;
                        break;
                    case SDLK_BACKSPACE:
                        system->EMU_flags.restart = 1;
                        break;
                    case SDLK_ESCAPE:
                        system->EMU_flags.exit = 1;
                        break;
                    
                    case SDLK_x:
                        system->key[0x0] = 1;
                        break;
                    case SDLK_1:
                        system->key[0x1] = 1;
                        break;
                    case SDLK_2:
                        system->key[0x2] = 1;
                        break;
                    case SDLK_3:
                        system->key[0x3] = 1;
                        break;
                    case SDLK_q:
                        system->key[0x4] = 1;
                        break;
                    case SDLK_w:
                        system->key[0x5] = 1;
                        break;
                    case SDLK_e:
                        system->key[0x6] = 1;
                        break;
                    case SDLK_a:
                        system->key[0x7] = 1;
                        break;
                    case SDLK_s:
                        system->key[0x8] = 1;
                        break;
                    case SDLK_d:
                        system->key[0x9] = 1;
                        break;
                    case SDLK_z:
                        system->key[0xA] = 1;
                        break;
                    case SDLK_c:
                        system->key[0xB] = 1;
                        break;
                    case SDLK_4:
                        system->key[0xC] = 1;
                        break;
                    case SDLK_r:
                        system->key[0xD] = 1;
                        break;
                    case SDLK_f:
                        system->key[0xE] = 1;
                        break;
                    case SDLK_v:
                        system->key[0xF] = 1;
                        break;
                    
                }
                break;

            case SDL_KEYUP:
                switch (event->key.keysym.sym) {
                    case SDLK_x:
                        system->key[0x0] = 0;
                        break;
                    case SDLK_1:
                        system->key[0x1] = 0;
                        break;
                    case SDLK_2:
                        system->key[0x2] = 0;
                        break;
                    case SDLK_3:
                        system->key[0x3] = 0;
                        break;
                    case SDLK_q:
                        system->key[0x4] = 0;
                        break;
                    case SDLK_w:
                        system->key[0x5] = 0;
                        break;
                    case SDLK_e:
                        system->key[0x6] = 0;
                        break;
                    case SDLK_a:
                        system->key[0x7] = 0;
                        break;
                    case SDLK_s:
                        system->key[0x8] = 0;
                        break;
                    case SDLK_d:
                        system->key[0x9] = 0;
                        break;
                    case SDLK_z:
                        system->key[0xA] = 0;
                        break;
                    case SDLK_c:
                        system->key[0xB] = 0;
                        break;
                    case SDLK_4:
                        system->key[0xC] = 0;
                        break;
                    case SDLK_r:
                        system->key[0xD] = 0;
                        break;
                    case SDLK_f:
                        system->key[0xE] = 0;
                        break;
                    case SDLK_v:
                        system->key[0xF] = 0;
                        break;
                }
                break;
        }
    }
    return;
}

void await_unpause(SDL_Event *event, Chip8_t *system) {
    while (system->EMU_flags.pause) {
        while (SDL_PollEvent(event)) {
            switch (event->type) {
                case SDL_QUIT:
                    system->EMU_flags.exit = 1;
                    break;

                case SDL_KEYDOWN:
                    switch (event->key.keysym.sym) {
                        case SDLK_SPACE:
                            system->EMU_flags.pause = 0;
                            break;
                    }
                    break;
            }
        }
    }
    return;
}