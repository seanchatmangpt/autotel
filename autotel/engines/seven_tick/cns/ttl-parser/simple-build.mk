# Simple build for TTL parser without CNS conflicts
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -Iinclude
LDFLAGS = -lm

# Source files needed for TTL parsing
SOURCES = src/main.c src/lexer.c src/parser.c src/token.c src/ast.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = ttl-parser-simple

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./$(TARGET) examples/simple.ttl