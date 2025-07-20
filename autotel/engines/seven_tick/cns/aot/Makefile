# AOT Memory Calculator and Arena Code Generator Makefile
# Simple build for the memory calculation library and code generator

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
INCLUDES = -I.
TARGET = libaot_calculator.a
CODEGEN_TARGET = libarena_codegen.a
OBJS = aot_calculator.o
CODEGEN_OBJS = arena_codegen.o

# 7-tick performance optimizations
CFLAGS += -march=native -mtune=native -ffast-math -funroll-loops

all: $(TARGET) $(CODEGEN_TARGET) test_calculator test_arena_codegen

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(CODEGEN_TARGET): $(CODEGEN_OBJS)
	ar rcs $@ $^

aot_calculator.o: aot_calculator.c aot_calculator.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

arena_codegen.o: arena_codegen.c arena_codegen.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test_calculator: test_calculator.c $(TARGET)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -laot_calculator -o $@

test_arena_codegen: test_arena_codegen.c $(CODEGEN_TARGET)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -larena_codegen -o $@

test_calculator.c:
	@echo "Creating simple test file..."
	@echo '#include <stdio.h>' > $@
	@echo '#include "aot_calculator.h"' >> $@
	@echo '' >> $@
	@echo 'int main(void) {' >> $@
	@echo '    printf("AOT Calculator Test\\n");' >> $@
	@echo '    printf("Small graph: %zu bytes\\n", aot_calculate_for_small_graph());' >> $@
	@echo '    printf("Medium graph: %zu bytes\\n", aot_calculate_for_medium_graph());' >> $@
	@echo '    printf("Large graph: %zu bytes\\n", aot_calculate_for_large_graph());' >> $@
	@echo '    return 0;' >> $@
	@echo '}' >> $@

clean:
	rm -f *.o $(TARGET) $(CODEGEN_TARGET) test_calculator test_arena_codegen test_calculator.c generated_arena.h simple_arena.h

install: $(TARGET) $(CODEGEN_TARGET)
	cp $(TARGET) ../lib/
	cp $(CODEGEN_TARGET) ../lib/
	cp aot_calculator.h ../include/cns/
	cp arena_codegen.h ../include/cns/

test: test_calculator test_arena_codegen
	./test_calculator
	./test_arena_codegen

.PHONY: all clean install test