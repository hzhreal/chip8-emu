#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

#include "chip8.h"
#include "graphics.h"
#include "debugger.h"

static int str_to_u16_1(const char *s, uint16_t *i) {
    long l;
    errno = 0;

    l = strtol(s, NULL, 16);
    if (errno != 0 || l > UINT16_MAX || l < 0) {
        return -1;
    }
    *i = (uint16_t)l;

    return 0;
}

static int str_to_u16_2(const char *s, uint16_t *x, uint16_t *y) {
    long l;
    char *end = NULL;
    errno = 0;

    l = strtol(s, &end, 16);
    if (errno != 0 || l > UINT16_MAX || l < 0 || *end != ' ') {
        return -1;
    }
    *x = (uint16_t)l;
    
    l = strtol(end, NULL, 16);
    if (errno != 0 || l > UINT16_MAX || l < 0) {
        return -2;
    }
    *y = (uint16_t)l;

    return 0;
}

void debugger_cli(Debugger_t *dbg, Chip8_t *system, Chip8_Graphics *gfx) {
    char line[DEBUGGER_BUF] = {0};
    char arg;
    char *delim;
    uint16_t v1 = 0, v2 = 0;
    if (!dbg->run) {
        for (;;) {
            printf("> ");
            if (fgets(line, sizeof(line), stdin)) {
                arg = line[0];
                switch (arg) {
                    case 's':
                        break;
                    case 'b':
                        system->pc -= sizeof(uint16_t);
                        break;
                    case 'd':
                        chip8_print(system);
                        break;
                    case 'g':
                        delim = strchr(line + 1, ' ');
                        if (delim) {
                            if (str_to_u16_1(delim + 1, &system->pc) != 0) {
                                printf("Failed to parse.\n");
                            }
                        }
                        else {
                            printf("g <0x0>\n");
                        }
                        break;
                    case 'm':
                        delim = strchr(line + 1, ' ');
                        if (delim) {
                            if (str_to_u16_1(delim + 1, &v1) == 0) {
                                printf("%" PRIX8 "\n", system->memory[v1]);
                            }
                            else {
                                printf("Failed to parse.\n");
                            }
                        }
                        else {
                            printf("m <0x0>\n");
                        }
                        break;
                    case 'e':
                        delim = strchr(line + 1, ' ');
                        if (delim) {
                            if (str_to_u16_1(delim + 1, &dbg->exec_max) == 0) {
                                if (dbg->exec_max > 0) {
                                    dbg->run = true;
                                }
                                else {
                                    printf("Number of instructions to execute is not set.\n");
                                }
                            }
                            else {
                                printf("Failed to parse\n");
                            }
                        }
                        else {
                            printf("e <0x0>\n");
                        }
                        break;
                    case 'q':
                        system->EMU_flags.exit = 1;
                        dbg->run = true;
                        break;
                    case 'c':
                        memset(system->gfx, 0, sizeof(system->gfx));
                        graphics_update(gfx, system);
                        break;
                    case 'p':
                        delim = strchr(line + 1, ' ');
                        if (delim) {
                            if (str_to_u16_1(delim + 1, &v1) == 0) {
                                if (system->sp >= STACK_SIZE) {
                                    printf("Stack is too big.\n");
                                    break;
                                }
                                system->stack[system->sp] = v1;
                                system->sp++;
                            }
                            else {
                                printf("Failed to parse.\n");
                            }
                        }
                        else {
                            printf("p <0x0>\n");
                        }
                        break;
                    case 'P':
                        if (system->sp > 0) {
                            printf("%" PRIX16 "\n", system->stack[system->sp]);
                            system->sp--;
                        }
                        break;
                    case 'r':
                        delim = strchr(line + 1, ' ');
                        if (delim) {
                            if (str_to_u16_2(delim + 1, &v1, &v2) == 0) {
                                if (v1 >= REGISTER_COUNT) {
                                    printf("Register out of range.\n");
                                    break;
                                }
                                system->V[v1] = (uint8_t)v2;
                            }
                            else {
                                printf("Failed to parse.\n");
                            }
                        }
                        else {
                            printf("r <0x0> <0x0>\n");
                        }
                        break;
                    case 'h':
                        printf("'s'              - Step Forward\n");
                        printf("'b'              - Step Back\n");
                        printf("'d'              - Dump registers, stack, and opcode\n");
                        printf("'g <0x0>'        - Go to location\n");
                        printf("'m <0x0>'        - Print value at memory location\n");
                        printf("'e <0x0>'        - Execute amount of instructions\n");
                        printf("'q'              - Quit\n");
                        printf("'c'              - Clear the display\n");
                        printf("'p <0x0>'        - Push a value onto the stack\n");
                        printf("'P'              - Pop value off the stack\n");
                        printf("'r <V0x0> <0x0>' - Set register to a value\n");
                        printf("'h'              - Show all commands\n");
                        break;
                    default:
                        printf("Unknown command.\n");
                        break;
                }
            }
            if (arg == 's' || arg == 'b' || arg == 'q' || arg == 'g' || arg == 'e') {
                break;
            }
        }
    }
    return;
}