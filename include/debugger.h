#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdbool.h>
#include <stdint.h>

#include "graphics.h"
#include "chip8.h"

#define DEBUGGER_BUF 0x100

typedef struct {
    bool run;
    uint16_t executed;
    uint16_t exec_max;
} Debugger_t;

void debugger_cli(Debugger_t *dbg, Chip8_t *system, Chip8_Graphics *gfx);

#endif // DEBUGGER_H