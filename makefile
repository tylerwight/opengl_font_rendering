#gcc main.c -I /usr/include/freetype2 -I /usr/include/libpng16 -lm -lglfw -lGLEW -lGL -lfreetype

# Compiler
CC = gcc

# Source files
SRCS = main.c

# Output binary
TARGET = font-rendering

# Include directories
INCLUDES = -I/usr/include/freetype2 -I/usr/include/libpng16

# Libraries
LIBS = -lm -lglfw -lGLEW -lGL -lfreetype

# Compilation flags
CFLAGS = $(INCLUDES)

# Linker flags
LDFLAGS = $(LIBS)

# Build target
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

# Clean target
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: clean
