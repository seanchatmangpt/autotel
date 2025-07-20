# CNS Minimal Benchmark Build System
# Optimized for honest performance measurement

CC = clang
CFLAGS = -O3 -march=native -Wall -Wextra -std=c11
LDFLAGS = -lm

# Architecture-specific flags
ifeq ($(shell uname -m),x86_64)
    CFLAGS += -mavx2 -mavx512f -mfma
endif

# Debug build with assembly output
DEBUG_CFLAGS = -O3 -g -S -march=native -Wall -Wextra -std=c11

# Target binaries
TARGETS = cns_minimal_benchmark 7tick_impl

# Source files
BENCHMARK_SRC = cns_minimal_benchmark.c
BASELINE_SRC = 7tick_impl.c

all: $(TARGETS)

cns_minimal_benchmark: $(BENCHMARK_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

7tick_impl: $(BASELINE_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Generate assembly for inspection
assembly: $(BENCHMARK_SRC)
	$(CC) $(DEBUG_CFLAGS) -o cns_minimal_benchmark.s $(BENCHMARK_SRC)
	@echo "Assembly generated in cns_minimal_benchmark.s"

# Run comprehensive benchmarks
benchmark: cns_minimal_benchmark
	@echo "Running CNS Minimal vs 7C Benchmarks..."
	@echo "========================================"
	@./cns_minimal_benchmark

# Run baseline for comparison
baseline: 7tick_impl
	@echo "Running 7C Baseline Implementation..."
	@echo "===================================="
	@./7tick_impl

# Performance comparison run
compare: cns_minimal_benchmark 7tick_impl
	@echo "=== BASELINE 7C PERFORMANCE ==="
	@./7tick_impl
	@echo ""
	@echo "=== CNS MINIMAL COMPARISON ==="
	@./cns_minimal_benchmark

# Quick single test
quick: cns_minimal_benchmark
	@echo "Quick benchmark (reduced iterations)..."
	@BENCHMARK_ITERATIONS=10000 ./cns_minimal_benchmark

# Clean build artifacts
clean:
	rm -f $(TARGETS) *.s *.o *.cnsb

# Install dependencies (if needed)
deps:
	@echo "Checking dependencies..."
	@which clang || (echo "Install clang compiler" && exit 1)
	@echo "Dependencies OK"

# Generate performance report
report: cns_minimal_benchmark
	@echo "Generating Performance Report..."
	@echo "===============================" > performance_report.txt
	@echo "Date: $$(date)" >> performance_report.txt
	@echo "System: $$(uname -a)" >> performance_report.txt
	@echo "CPU: $$(sysctl -n machdep.cpu.brand_string 2>/dev/null || cat /proc/cpuinfo | grep 'model name' | head -1)" >> performance_report.txt
	@echo "" >> performance_report.txt
	@./cns_minimal_benchmark >> performance_report.txt 2>&1
	@echo "Report saved to performance_report.txt"

# Validate build environment
validate:
	@echo "Validating build environment..."
	@$(CC) --version
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Architecture: $$(uname -m)"
	@echo "Validation complete"

# Help target
help:
	@echo "CNS Minimal Benchmark Build System"
	@echo "=================================="
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build all benchmarks"
	@echo "  benchmark  - Run CNS minimal benchmarks"
	@echo "  baseline   - Run 7C baseline benchmarks"
	@echo "  compare    - Run both for comparison"
	@echo "  quick      - Quick benchmark test"
	@echo "  assembly   - Generate assembly output"
	@echo "  report     - Generate performance report"
	@echo "  clean      - Clean build artifacts"
	@echo "  validate   - Check build environment"
	@echo "  help       - Show this help"

.PHONY: all benchmark baseline compare quick assembly clean deps report validate help