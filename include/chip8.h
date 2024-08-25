#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

#define MEMORY_SIZE 0x1000
#define REGISTER_COUNT 16
#define STACK_SIZE 16
#define NUM_KEYS 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define PROGRAM_START 0x200

/* We will use this in the VF register. */
#define CARRY_FLAG          (0x01) // 0b00000001
#define NOBORROW_FLAG       (0x01) // 0b00000001
#define PIXELCOLLISION_FLAG (0x01) // 0b00000001

/* Systems memory map
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM */

typedef struct {
/* CHIP-8 has 35 opcodes (2 bytes long). */
    uint16_t opcode;

/* Index register and program counter that will hold a value from 0x000 to 0xFFF. */
    uint16_t I;
    uint16_t pc;

/* CHIP-8 as two timer registers that count at 60 Hz. They will will count down to zero, and the system will buzz when sound timer reaches zero. */
    uint8_t delay_timer;
    uint8_t sound_timer;

/* CHIP-8 has 4K memory. */
    uint8_t memory[MEMORY_SIZE];

/* CHIP-8 has 16 8-bit data registers named V0 to VF. VF is used as a flag in some instructions. */
    uint8_t V[REGISTER_COUNT];

/* Graphics are black and white with 2048 pixels (64 x 32). */
    uint8_t gfx[DISPLAY_WIDTH * DISPLAY_HEIGHT];

/* The interpreter will need a stack to because CHIP-8 has opcodes that will allow the program to jump to an address or call a subroutine. We need a stack to remember the location before performing a jump. The system has 16 levels of stack and to remember which level we will create a seperate pointer. */
    uint16_t stack[STACK_SIZE];
    uint16_t sp;

/* CHIP-8 has a HEX based keypad (0x0-0xF). */
    uint8_t key[NUM_KEYS];
    
/* This bitfield will be used for flags that are specific to the emulator implementation. */
    struct {
        unsigned int draw_to_screen : 1;
        unsigned int pause          : 1;
        unsigned int restart        : 1;
        unsigned int exit           : 1;
    } EMU_flags;
} Chip8_t;

/* Each number or character is 4 pixels wide and 5 pixels high. */
extern const uint8_t chip8_fontset[];

void chip8_initialize(Chip8_t *system);
void chip8_update_timers(Chip8_t *system);
void chip8_emulatecycle(Chip8_t *system);

#if defined(DEBUG)
void chip8_print(Chip8_t *system);
#endif

#endif // CHIP8_H