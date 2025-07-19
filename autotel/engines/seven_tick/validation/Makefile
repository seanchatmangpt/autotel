# S7T Performance Validation Framework Makefile

CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lm -lpthread

# Include paths
INCLUDES = -I../lib -I../include

# Source files
VALIDATION_SRCS = test_s7t_validation.c ../lib/s7t_perf.c
ENGINE_LIBS = ../lib/lib7t_pm.a ../lib/lib7t_mcts.a ../lib/lib7t_sparql.a \
              ../lib/lib7t_common.a ../lib/lib7t_memory.a

# Output
TARGET = test_s7t_validation

# Validation report targets
REPORTS = validation_report.md validation_report.json validation_heatmap.md

.PHONY: all clean test validate report

all: $(TARGET)

$(TARGET): $(VALIDATION_SRCS)
	@echo "Building S7T Validation Framework..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(VALIDATION_SRCS) $(ENGINE_LIBS) $(LDFLAGS)
	@echo "Build complete!"

test: $(TARGET)
	@echo "Running S7T Validation Tests..."
	./$(TARGET)

validate: test
	@echo "Validation complete. Check generated reports."

report:
	@if [ -f validation_report.md ]; then \
		echo "=== VALIDATION SUMMARY ==="; \
		grep -A 5 "## Summary" validation_report.md; \
		echo ""; \
		grep "Overall Physics Compliance" validation_report.md; \
	else \
		echo "No validation report found. Run 'make test' first."; \
	fi

clean:
	rm -f $(TARGET) $(REPORTS)
	@echo "Cleaned validation artifacts."

# Individual test targets
test-cycles: $(TARGET)
	./$(TARGET) --test-cycles

test-memory: $(TARGET)
	./$(TARGET) --test-memory

test-branches: $(TARGET)
	./$(TARGET) --test-branches

test-cache: $(TARGET)
	./$(TARGET) --test-cache

# Performance profiling with perf
perf-profile: $(TARGET)
	@echo "Running with perf profiling..."
	perf record -g ./$(TARGET)
	perf report

# Valgrind memory check
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Generate assembly for inspection
asm: $(VALIDATION_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -S -o validation.s test_s7t_validation.c

help:
	@echo "S7T Validation Framework"
	@echo "======================="
	@echo "make all      - Build validation framework"
	@echo "make test     - Run all validation tests"
	@echo "make validate - Run tests and check compliance"
	@echo "make report   - Show validation summary"
	@echo "make clean    - Clean build artifacts"
	@echo ""
	@echo "Individual tests:"
	@echo "make test-cycles   - Test cycle compliance"
	@echo "make test-memory   - Test memory patterns"
	@echo "make test-branches - Test branch prediction"
	@echo "make test-cache    - Test cache behavior"
	@echo ""
	@echo "Analysis tools:"
	@echo "make perf-profile - Profile with perf"
	@echo "make memcheck     - Check memory with valgrind"
	@echo "make asm          - Generate assembly code"