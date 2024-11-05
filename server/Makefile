# Makefile for server program

# Check if CROSS is defined
ifndef CROSS
	CROSS=
endif

# Compiler to use
CC=$(CROSS)gcc

# Compiler flags
CFLAGS=-Wall -g

# Source files
SOURCES=server.c

# Object files
OBJECTS=$(SOURCES:.c=.o)

# Executable name
EXECUTABLE=aesdsocket

default: $(EXECUTABLE)
all: $(EXECUTABLE)

$(EXECUTABLE):$(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

.PHONY: all clean

