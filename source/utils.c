#include <stdio.h>

#include "chip8.h"
#include "utils.h"

int load_rom(Chip8_t *system, const char *path) {
    FILE *fp;
    size_t len, bytes_read;

    fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    if (len > sizeof(system->memory) - PROGRAM_START || len % sizeof(uint16_t) != 0) {
        fclose(fp);
        return -2;
    }
    fseek(fp, 0, SEEK_SET);

    bytes_read = fread(system->memory + PROGRAM_START, 1, len, fp);
    if (bytes_read != len) {
        fclose(fp);
        return -3;
    }

    fclose(fp);
    return 0;
}