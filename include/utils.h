#ifndef UTILS_H
#define UTILS_H

#include <log.h>

#include "chip8.h"

#define LOG_FLAGS (LOG_ALL)

int load_rom(Chip8_t *system, const char *path);

#endif // UTILS_H