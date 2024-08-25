#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <inttypes.h>

#include <log.h>

#include "utils.h"
#include "chip8.h"

const uint8_t chip8_fontset[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static inline void beep(void) {
    printf("\a");
    fflush(stdout);
    return;
}

/* 
Fetch 16-bit opcode from memory 
    - The opcode consists of the high byte which is the program counter (pc) then the low byte which is program counter (pc) + 1
    - We shift the high byte to the left by 8 bits so we can combine with low byte using bitwise OR 
*/
static inline uint16_t fetch_opcode(Chip8_t *system) {
    return system->memory[system->pc] << 8 | system->memory[system->pc + 1];
}

/*
Clear the graphics screen
    - Set all bytes in the gfx array to 0
*/
static inline void clear_screen(Chip8_t *system) {
    return (void)memset(system->gfx, 0, sizeof(system->gfx));
}

/*
Return from a function:
    - Decrement the stack pointer (sp) to point to the return address
    - Set the program counter (pc) to the return address stored at the new stack pointer location
    - The return address was previously pushed onto the stack when the subroutine was called
*/
static inline void return_from_subroutine(Chip8_t *system) {
    system->sp--;
    #if defined(DEBUG)
    if (system->sp >= STACK_SIZE) {
        printf("PREVENTED STACK OVERFLOW! SP: %" PRIX16 "\nPROGRAM SHOULD HAVE RETURNED FROM SUBROUTINE, SO UNEXPECTED BEHAVIOR MIGHT OCCUR!\n", system->sp);
        return;
    }
    #endif
    system->pc = system->stack[system->sp];
    return;
}

/* 
Jump to an address
    - Set the program counter (pc) to the new address where program execution will continue
*/
static inline void jump_to_address(Chip8_t *system, uint16_t addr) {
    system->pc = addr;
    return;
}

/*
Invoke function
    - Set the return address in the current stack position and push the stack
    - Set the program counter (pc) to the subroutine address
*/
static inline void call_subroutine(Chip8_t *system, uint16_t addr) {
    #if defined(DEBUG)
    if (system->sp >= STACK_SIZE) {
        printf("PREVENTED STACK OVERFLOW! SP: %" PRIX16 "\nPROGRAM SHOULD HAVE CALLED A SUBROUTINE, SO UNEXPECTED BEHAVIOR MIGHT OCCUR!\n", system->sp);
        return;
    }
    #endif
    system->stack[system->sp] = system->pc;
    system->sp++;
    system->pc = addr;
    return;
}

/*
Skip instruction if Vx == y
    - Compare Vx and y, if equal we increase program counter (pc) by 2 bytes
*/
static inline void skip_instru_if_equal(Chip8_t *system, uint8_t x, uint8_t y) {
    if (system->V[x] == y) {
        system->pc += sizeof(uint16_t);
    }
    return;
}

/*
Skip instruction if Vx != y
    - Compare Vx and y, if not equal we increase program counter (pc) by 2 bytes
*/
static inline void skip_instru_if_not_equal(Chip8_t *system, uint8_t x, uint8_t y) {
    if (system->V[x] != y) {
        system->pc += sizeof(uint16_t);
    }
    return;
}

/*
Skip instruction if Vx == Vy
    - Compare Vx and Vy, if equal we increase program counter (pc) by 2 bytes
*/
static inline void skip_instru_if_equal_1(Chip8_t *system, uint8_t x, uint8_t y) {
    if (system->V[x] == system->V[y]) {
        system->pc += sizeof(uint16_t);
    }
    return;
}

/*
Set register
    - Set the register Vx to the value y
*/
static inline void set_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    system->V[x] = y;
    return;
}

/*
Add to register
    - Add y to the Vx register
    - Carry flag will not be set onto VF
*/
static inline void add_to_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    system->V[x] += y;
    return;
}

/*
Copy a registers value to another
    - Vx will be set to the value of Vy
*/
static inline void mov_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    system->V[x] = system->V[y];
    return;
}

/*
Bitwise OR on register
    - Vx will be set to Vx OR Vy
*/
static inline void or_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    system->V[x] |= system->V[y];
    return;
}

/*
Bitwise AND on register
    - Vx will be set to Vx AND Vy
*/
static inline void and_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    system->V[x] &= system->V[y];
    return;
}

/*
Bitwise XOR on register
    - Vx will be set to Vx XOR Vy
*/
static inline void xor_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    system->V[x] ^= system->V[y];
    return;
}

/*
Add register onto register
    - Vx will be set to Vx + Vy
    - Carry flag can be set onto VF
*/
static inline void add_reg_to_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    if (system->V[x] + system->V[y] > UINT8_MAX) {
        /* CARRY */
        system->V[x] += system->V[y];
        system->V[0xF] = CARRY_FLAG;
    }
    else {
        /* NO CARRY*/
        system->V[x] += system->V[y];
        system->V[0xF] = 0;
    }
    return;
}

/*
Sub register from register
    - Vx will be set to Vx - Vy
    - No borrow flag can be set onto VF
*/
static inline void sub_reg_from_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    if (system->V[x] >= system->V[y]) {
        /* NOBORROW */
        system->V[x] -= system->V[y];
        system->V[0xF] = NOBORROW_FLAG;
    }
    else {
        /* NO NOBORROW */
        system->V[x] -= system->V[y];
        system->V[0xF] = 0;
    }
    return;
}

/*
Right shift register by 1
    - Store least significant bit (LSB) before shift in VF
*/
static inline void rsh_reg(Chip8_t *system, uint8_t x) {
    uint8_t lsb = system->V[x] & 1;
    system->V[x] >>= 1;
    system->V[0xF] = lsb;
    return;
}

/*
Sub register from register
    - Vx will be set to Vy - Vx
    - No borrow flag can be set onto VF
*/
static inline void sub_reg_from_reg_1(Chip8_t *system, uint8_t x, uint8_t y) {
    if (system->V[y] >= system->V[x]) {
        /* NOBORROW */
        system->V[x] = system->V[y] - system->V[x];
        system->V[0xF] = NOBORROW_FLAG;
    }
    else {
        /* NO NOBORROW */
        system->V[x] = system->V[y] - system->V[x];
        system->V[0xF] = 0;
    }
    return;
}

/*
Left shift register by 1
    - Set VF to 1 if the most significant bit (MSB) of Vx was set before shift, 0 otherwise
*/
static inline void lsh_reg(Chip8_t *system, uint8_t x) {
    uint8_t msb = system->V[x] >> 7;
    system->V[x] <<= 1;
    if (msb) {
        /* SET */
        system->V[0xF] = 1;
    }
    else {
        /* UNSET */
        system->V[0xF] = 0;
    }
    return;
}

/*
Skip instruction if register does not equal reqister
    - If Vx != Vy, skip instruction
*/
static inline void skip_instru_if_req_not_equal_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    if (system->V[x] != system->V[y]) {
        system->pc += sizeof(uint16_t);
    }
    return;
}

/*
Set index register (I)
    - Index register (I) will be set to the address (addr)
*/
static inline void set_idx_reg(Chip8_t *system, uint16_t addr) {
    system->I = addr;
    return;
}

/*
Jump to address + V0
    - Program counter (pc) will be set to the address (addr) + V0
*/
static inline void jump_to_address_1(Chip8_t *system, uint16_t addr) {
    system->pc = system->V[0x0] + addr;
    return;
}

/*
Set register to random number
    - We will generate a random number and perform bitwise AND with y, Vx will be set to the result
    - The generated number is between 0-255
*/
static inline void rand_reg(Chip8_t *system, uint8_t x, uint8_t y) {
    system->V[x] = (rand() % 256) & y;
    return;
}

/*
Draw
    - We draw a sprite at coordinate (Vx, Vy)
    - Width of 8 pixels, height of z pixels
    - We set VF to draw flag if a pixel colission occurs
*/
static inline void draw(Chip8_t *system, uint8_t x, uint8_t y, uint8_t z) {
    uint16_t pixel;
    int yline, xline;

    uint8_t xx = system->V[x];
    uint8_t yy = system->V[y];

    system->V[0xF] = 0;
    for (yline = 0; yline < z; yline++) {
        pixel = system->memory[system->I + yline];
        for (xline = 0; xline < 8; xline++) {
            if ((pixel & (0x80 >> xline)) != 0) {
                if (system->gfx[(xx + xline + ((yy + yline) * DISPLAY_WIDTH))]) {
                    /* PIXEL COLISSION */
                    system->V[0xF] = PIXELCOLLISION_FLAG;
                }
                system->gfx[xx + xline + ((yy + yline) * DISPLAY_WIDTH)] ^= 1;
            }
        }
    }
    system->EMU_flags.draw_to_screen = 1;
    return;
}

/* 
Skip instruction if key is pressed
    - If key stored in Vx is pressed we skip instruction
*/
static inline void skip_instru_if_key_pressed(Chip8_t *system, uint8_t x) {
    if (system->key[system->V[x]]) {
        system->pc += sizeof(uint16_t);
    }
    return;
}

/* 
Skip instruction if key is not pressed
    - If key stored in Vx is not pressed we skip instruction
*/
static inline void skip_instru_if_key_not_pressed(Chip8_t *system, uint8_t x) {
    if (!system->key[system->V[x]]) {
        system->pc += sizeof(uint16_t);
    }
    return;
}

/* 
Set register to delay timer
    - Vx will be set to the value of the delay timer
*/
static inline void set_reg_to_delay_timer(Chip8_t *system, uint8_t x) {
    system->V[x] = system->delay_timer;
    return;
}

/*
Await key press and store in register
    - Check if any keys are pressed, if not come back to the instruction
*/
static inline void get_key(Chip8_t *system, uint8_t x) {
    int i;
    for (i = 0; i < NUM_KEYS; i++) {
        if (system->key[i]) {
            system->V[x] = i;
            return;
        }
    }
    system->pc -= sizeof(uint16_t);
    return;
}

/*
Set delay timer to value of register
    - Delay timer will be set to Vx
*/
static inline void set_delay_timer_to_reg(Chip8_t *system, uint8_t x) {
    system->delay_timer = system->V[x];
    return;
}

/* 
Set sound timer to value of register
    - Sound timer will be set to Vx
*/
static inline void set_sound_timer_to_reg(Chip8_t *system, uint8_t x) {
    system->sound_timer = system->V[x];
    return;
}

/*
Add register value to index register (I)
    - Index register (I) will be set to I + Vx
    - Carry flag will not be set onto VF
*/
static inline void add_reg_to_i(Chip8_t *system, uint8_t x) {
    system->I += system->V[x];
    return;
}

/*
Set index register (I) to the location of the sprite for character in register
    - Index register (I) will be set to the address of the sprite according to Vx
    - Essentially set I to Vx * 5
*/
static inline void set_i_to_sprite_addr(Chip8_t *system, uint8_t x) {
    system->I = system->V[x] * 5;
    return;
}

/*
Store the binary-coded decimal (BCD) of register in the location of in the index register (I)
    - Hundreds digit at location in I
    - Tens digit at I+1
    - Ones digit at I+2
    - Obtain from Vx
*/
static inline void store_bcd_reg(Chip8_t *system, uint8_t x) {
    system->memory[system->I    ] = system->V[x] / 100;
    system->memory[system->I + 1] = (system->V[x] / 10) % 10;
    system->memory[system->I + 2] = (system->V[x] / 100) % 10;
    return;
}

/* 
Store registers in memory starting at address in index register (I)
    - V0 to and including Vx will be stored in memory starting from address I
*/
static inline void reg_dump(Chip8_t *system, uint8_t x) {
    memcpy(system->memory + system->I, system->V, x * sizeof(uint8_t));
    return;
}

/*
Fill registers with values from memory starting at address in index register (I)
    - V0 to and including Vx will be filled with values stored in memory starting from address I
*/
static inline void reg_load(Chip8_t *system, uint8_t x) {
    memcpy(system->V, system->memory + system->I, x * sizeof(uint8_t));
    return;
}

void chip8_initialize(Chip8_t *system) {
    system->opcode      = 0;
    system->I           = 0;
    system->sp          = 0;
    system->delay_timer = 0;
    system->sound_timer = 0;
    system->pc          = PROGRAM_START;
    
    memset(system->memory, 0, sizeof(system->memory));
    memset(system->V,      0, sizeof(system->V     ));
    memset(system->gfx,    0, sizeof(system->gfx   ));
    memset(system->stack,  0, sizeof(system->stack ));
    memset(system->key,    0, sizeof(system->key   ));

    system->EMU_flags.draw_to_screen = 0;
    system->EMU_flags.pause          = 0;
    system->EMU_flags.restart        = 0;
    system->EMU_flags.exit           = 0;

    memcpy(system->memory, chip8_fontset, sizeof(chip8_fontset));

    return;
}

void chip8_update_timers(Chip8_t *system) {
    if (system->delay_timer > 0) {
        system->delay_timer--;
    }
    if (system->sound_timer > 0) {
        if (system->sound_timer == 1) {
            beep();
        }
        system->sound_timer--;
    }
    return;
}

void chip8_emulatecycle(Chip8_t *system) {
    uint8_t n, nn, x, y;
    uint16_t nnn;

    /* Fetch opcode and parse */
    system->opcode = fetch_opcode(system);
    n   =   system->opcode & 0x000F;
    nn  =   system->opcode & 0x00FF;
    nnn =   system->opcode & 0x0FFF;
    x   =  (system->opcode & 0x0F00) >> 8;
    y   =  (system->opcode & 0x00F0) >> 4;

    /* Store next instruction */
    system->pc += sizeof(uint16_t);

    /* Decode and execode opcode */
    switch (system->opcode & 0xF000) {
        case 0x0000:
            switch (system->opcode & 0x00FF) {
                /* 0x00E0: Clears the screen. */
                case 0x00E0:
                    clear_screen(system);
                    system->EMU_flags.draw_to_screen = 1;
                    break;

                /* 0x00EE: Returns from a subroutine. */
                case 0x00EE:
                    return_from_subroutine(system);
                    break;
                
                default:
                    LOG_TO_STREAM(stderr, LOG_LEVEL_WARNING, LOG_FLAGS, "INVALID OPCODE: %" PRIX16, system->opcode);
                    #if defined(DEBUG)
                    #else
                    system->EMU_flags.exit = 1;
                    #endif
                    break;
            }
            break;
        
        /* 0x1NNN: Jumps to address NNN. */
        case 0x1000:
            jump_to_address(system, nnn);
            break;

        /* 0x2NNN: Calls subroutine at NNN. */
        case 0x2000:
            call_subroutine(system, nnn);
            break;
        
        /* 0x3XNN: Skips the next instruction if VX equals NN. */
        case 0x3000:
            skip_instru_if_equal(system, x, nn);
            break;
        
        /* 0x4XNN: Skips the next instruction if VX does not equal NN. */
        case 0x4000:
            skip_instru_if_not_equal(system, x, nn);
            break;
        
        /* 0x5XNN: Skips the next instruction if VX equals VY. */
        case 0x5000:
            skip_instru_if_equal_1(system, x, nn);
            break;
        
        /* 6XNN: Sets VX to NN. */
        case 0x6000:
            set_reg(system, x, nn);
            break;
        
        /* 7XNN: Adds NN to VX. */
        case 0x7000:
            add_to_reg(system, x, nn);
            break;

        case 0x8000:
            switch (system->opcode & 0x000F) {
                /* 8XY0: Sets VX to the value of VY. */
                case 0x0000:
                    mov_reg(system, x, y);
                    break;

                /* 8XY1: Sets VX to VX OR VY. */
                case 0x0001:
                    or_reg(system, x, y);
                    break;
                
                /* 8XY2: Sets VX to VX AND VY. */
                case 0x0002:
                    and_reg(system, x, y);
                    break;
                
                /* 8XY3: Sets VX to VX XOR VY. */
                case 0x0003:
                    xor_reg(system, x, y);
                    break;
                
                /* 8XY4: Adds VY to VX. */
                case 0x0004:
                    add_reg_to_reg(system, x, y);
                    break;
                
                /* 8XY5: VY is subtracted from VX. */
                case 0x0005:
                    sub_reg_from_reg(system, x, y);
                    break;

                /* 8XY6: Shifts VX to the right by 1. */
                case 0x0006:
                    rsh_reg(system, x);
                    break;
                
                /* 8XY7: Sets VX to VY minus VX. */
                case 0x0007:
                    sub_reg_from_reg_1(system, x, y);
                    break;

                /* 8XYE: Shifts VX to the left by 1. */
                case 0x000E:
                    lsh_reg(system, x);
                    break;

                default:
                    LOG_TO_STREAM(stderr, LOG_LEVEL_WARNING, LOG_FLAGS, "INVALID OPCODE: %" PRIX16, system->opcode);
                    #if defined(DEBUG)
                    #else
                    system->EMU_flags.exit = 1;
                    #endif
                    break;
            }
            break;
        
        /* 9XY0: Skips the next instruction if VX does not equal VY. */
        case 0x9000:
            skip_instru_if_req_not_equal_reg(system, x, y);
            break;

        /* ANNN: Sets I to the address NNN. */
        case 0xA000:
            set_idx_reg(system, nnn);
            break;

        /* BNNN: Jumps to the address NNN plus V0. */
        case 0xB000:
            jump_to_address_1(system, nnn);
            break;

        /* CXNN: Sets VX to the result of a bitwise and operation on a random number. */
        case 0xC000:
            rand_reg(system, x, nn);
            break;

        /* DXYN: Draws a sprite at coordinate (VX, VY). */
        case 0xD000:
            draw(system, x, y, n);
            system->EMU_flags.draw_to_screen = 1;
            break;

        case 0xE000:
            switch (system->opcode & 0x00FF) {
                /* EX9E: Skips the next instruction if the key stored in VX is pressed. */
                case 0x009E:
                    skip_instru_if_key_pressed(system, x);
                    break;

                /* EXA1: Skips the next instruction if the key stored in VX is not pressed. */
                case 0x00A1:
                    skip_instru_if_key_not_pressed(system, x);
                    break;

                default:
                    LOG_TO_STREAM(stderr, LOG_LEVEL_WARNING, LOG_FLAGS, "INVALID OPCODE: %" PRIX16, system->opcode);
                    #if defined(DEBUG)
                    #else
                    system->EMU_flags.exit = 1;
                    #endif
                    break;
            }
            break;

        case 0xF000:
            switch (system->opcode & 0x00FF) {
                /* FX07: Sets VX to the value of the delay timer. */
                case 0x0007:
                    set_reg_to_delay_timer(system, x);
                    break;

                /* FX0A: A key press is awaited, and then stored in VX. */
                case 0x000A:
                    get_key(system, x);
                    break;

                /* FX15: Sets the delay timer to VX. */
                case 0x0015:
                    set_delay_timer_to_reg(system, x);
                    break;

                /* FX18: Sets the sound timer to VX. */
                case 0x0018:
                    set_sound_timer_to_reg(system, x);
                    break;

                /* FX1E: Adds VX to I. */
                case 0x001E:
                    add_reg_to_i(system, x);
                    break;

                /* FX29: Sets I to the location of the sprite for the character in VX. */
                case 0x0029:
                    set_i_to_sprite_addr(system, x);
                    break;

                /* FX33: Stores the binary-coded decimal representation of VX. */
                case 0x0033:
                    store_bcd_reg(system, x);
                    break;

                /* FX55: Stores from V0 to VX (including VX) in memory. */
                case 0x0055:
                    reg_dump(system, x);
                    break;

                /* FX65: Fills from V0 to VX (including VX) with values from memory. */
                case 0x0065:
                    reg_load(system, x);
                    break;
                
                default:
                    LOG_TO_STREAM(stderr, LOG_LEVEL_WARNING, LOG_FLAGS, "INVALID OPCODE: %" PRIX16, system->opcode);
                    #if defined(DEBUG)
                    #else
                    system->EMU_flags.exit = 1;
                    #endif
                    break;
            }
            break;

        default:
            LOG_TO_STREAM(stderr, LOG_LEVEL_WARNING, LOG_FLAGS, "INVALID OPCODE: %" PRIX16, system->opcode);
            #if defined(DEBUG)
            #else
            system->EMU_flags.exit = 1;
            #endif
            break;
    }
    return;
}

void chip8_print(Chip8_t *system) {
    int i;

    printf("EXECUTED OPCODE: %" PRIX16 "\n", system->opcode);
    printf("NEXT OPCODE %" PRIX16 "\n", fetch_opcode(system));
    printf("PC: %" PRIX16 "\n", system->pc);
    printf("I: %" PRIX16 "\n", system->I);
    printf("DELAY TIMER %" PRIX8 "\n", system->delay_timer);
    printf("SOUND TIMER: %" PRIX8 "\n\n", system->sound_timer);

    for (i = 0; i < REGISTER_COUNT; i++) {
        printf("V%" PRIX8 ":%" PRIX8 "\n", i, system->V[i]);
    }
    printf("\n");

    printf("STACK:\n");
    for (i = 0; i < STACK_SIZE; i++) {
        printf("0x%" PRIX8 ": %" PRIX16 "\n", i, system->stack[i]);
    }
    printf("SP: %" PRIX16 "\n", system->sp);
    return;
}