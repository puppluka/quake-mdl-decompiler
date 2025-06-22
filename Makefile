# Makefile for mdl_reverse_engineer (mdl2src)

# Define the C compiler to use
CC = gcc

# Define compiler flags:
# -Wall: Enable all standard warnings
# -Wextra: Enable extra warnings (recommended for robust code)
# -std=c99: Compile using the C99 standard (required for some features like inline functions if used)
# -g: Include debugging information
# -O2: Optimization level 2 (optional, can be removed for faster compilation during development)
# -lm: Link with the math library (for sqrt and other math functions)
# -D_DEFAULT_SOURCE: Enable certain POSIX/GNU extensions, often needed for sqrtf
CFLAGS = -Wall -Wextra -std=c99 -g -O2 -D_DEFAULT_SOURCE

# Define the name of the executable
TARGET = mdl2tri.exe

# Define the source file
SOURCE = mdl_reverse_engineer.c

# Default target: builds the executable
all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $< -o $@ -lm

# Clean target: removes compiled files and generated .lbm/.tri files
clean:
	rm -f $(TARGET)
	rm -f *.lbm
	rm -f *.tri
	rm -f *.o

.PHONY: all clean
