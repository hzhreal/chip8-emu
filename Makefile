# Compiler and flags
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -O3 -fomit-frame-pointer
# CFLAGS = -Iinclude -Wall -Wextra -DDEBUG -g
LDLIBS = -lSDL2 -lNeatLogger -lNeatConfig

# Directories and files
SRCDIR = source
INCDIR = include
OBJDIR = obj
BINDIR = bin
TARGET = $(BINDIR)/chip8-emu

# Source and object files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

# Default target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDLIBS)

# Rule to compile .c files into .o files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create bin and obj directories if they don't exist
$(BINDIR):
	mkdir -p $(BINDIR)
	cp config/chip8-emu.conf bin

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean up build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean
