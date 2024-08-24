## chip8-emu
A Chip8 emulator written in C with an interpreter approach.

### Key features
The emulator will allow you to run ROMs designed for the Chip8 virtual machine

### Usage
```
./chip8.emu <path to ROM>
```

### Keybinds
```
Keypad                   Keyboard
+-+-+-+-+                +-+-+-+-+
|1|2|3|C|                |1|2|3|4|
+-+-+-+-+                +-+-+-+-+
|4|5|6|D|                |Q|W|E|R|
+-+-+-+-+       =>       +-+-+-+-+
|7|8|9|E|                |A|S|D|F|
+-+-+-+-+                +-+-+-+-+
|A|0|B|F|                |Z|X|C|V|
+-+-+-+-+                +-+-+-+-+
```
- Pause the ROM using `SPACE`
- Restart the ROM using `BACKSPACE`
- Exit the ROM using `ESCAPE`

### Building
To compile the program, run
```
make
```

If you want to compile in debugging mode, uncomment
```
# CFLAGS = -Iinclude -Wall -Wextra -DDEBUG -g
```
In the Makefile

To build you will need to install:
- [SDL2](https://github.com/libsdl-org/SDL)
- [NeatLogger](https://github.com/hzhreal/NeatLogger)
- [NeatConfig](https://github.com/hzhreal/NeatConfig)

### Resources
- [How to write an emulator (CHIP-8 interpreter)](https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/)
- [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8)
