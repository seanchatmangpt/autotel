# 8T Test Infrastructure Makefile
# Comprehensive build system with SIMD optimizations and performance flags

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -march=native -mtune=native
LDFLAGS = -lm -lpthread

# SIMD optimization flags
SIMD_FLAGS = -mavx2 -mfma -msse4.2 -mpopcnt

# 8T specific flags
CNS_8T_FLAGS = -DCNS_8T_ENABLED -DCNS_8T_TICK_LIMIT=8 -DCNS_8T_SIMD_WIDTH=32

# Performance flags
PERF_FLAGS = -ffast-math -funroll-loops -finline-functions -flto

# Debug flags (commented out for performance builds)
# DEBUG_FLAGS = -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined

# Cache optimization flags
CACHE_FLAGS = -falign-functions=64 -falign-loops=32 -falign-jumps=16

# Include directories
INCLUDES = -I../../include -I../../src

# Source files
ARENA_TEST_SRC = test_arena_l1.c
NUMERICAL_TEST_SRC = test_numerical.c
GRAPH_TEST_SRC = test_graph_l1.c
PERFORMANCE_TEST_SRC = test_l1_performance.c
BENCHMARK_SRC = benchmark_8t.c

# Object files
ARENA_TEST_OBJ = $(ARENA_TEST_SRC:.c=.o)
NUMERICAL_TEST_OBJ = $(NUMERICAL_TEST_SRC:.c=.o)
GRAPH_TEST_OBJ = $(GRAPH_TEST_SRC:.c=.o)
PERFORMANCE_TEST_OBJ = $(PERFORMANCE_TEST_SRC:.c=.o)
BENCHMARK_OBJ = $(BENCHMARK_SRC:.c=.o)

# Executables
ARENA_TEST_BIN = test_arena_l1
NUMERICAL_TEST_BIN = test_numerical
GRAPH_TEST_BIN = test_graph_l1
PERFORMANCE_TEST_BIN = test_l1_performance
BENCHMARK_BIN = benchmark_8t

# All targets
ALL_BINS = $(ARENA_TEST_BIN) $(NUMERICAL_TEST_BIN) $(GRAPH_TEST_BIN) \
           $(PERFORMANCE_TEST_BIN) $(BENCHMARK_BIN)

# Default target
all: $(ALL_BINS)

# Arena L1 allocator tests
$(ARENA_TEST_BIN): $(ARENA_TEST_OBJ)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      -o $@ $^ $(LDFLAGS)

$(ARENA_TEST_OBJ): $(ARENA_TEST_SRC)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      $(INCLUDES) -c $< -o $@

# Numerical precision and SIMD tests
$(NUMERICAL_TEST_BIN): $(NUMERICAL_TEST_OBJ)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      -o $@ $^ $(LDFLAGS)

$(NUMERICAL_TEST_OBJ): $(NUMERICAL_TEST_SRC)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      $(INCLUDES) -c $< -o $@

# Cache-optimized graph tests
$(GRAPH_TEST_BIN): $(GRAPH_TEST_OBJ)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      -o $@ $^ $(LDFLAGS)

$(GRAPH_TEST_OBJ): $(GRAPH_TEST_SRC)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      $(INCLUDES) -c $< -o $@

# L1 cache performance validation
$(PERFORMANCE_TEST_BIN): $(PERFORMANCE_TEST_OBJ)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      -o $@ $^ $(LDFLAGS)

$(PERFORMANCE_TEST_OBJ): $(PERFORMANCE_TEST_SRC)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      $(INCLUDES) -c $< -o $@

# 8T vs 7T performance comparison
$(BENCHMARK_BIN): $(BENCHMARK_OBJ)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      -o $@ $^ $(LDFLAGS)

$(BENCHMARK_OBJ): $(BENCHMARK_SRC)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      $(INCLUDES) -c $< -o $@

# Debug builds
debug: CFLAGS = -std=c11 -Wall -Wextra -g -O0 -DDEBUG
debug: PERF_FLAGS = 
debug: $(ALL_BINS)

# Profile builds
profile: CFLAGS += -pg -fno-omit-frame-pointer
profile: LDFLAGS += -pg
profile: $(ALL_BINS)

# Coverage builds
coverage: CFLAGS += --coverage -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += --coverage
coverage: $(ALL_BINS)

# Static analysis builds
analyze: CFLAGS += -fanalyzer
analyze: $(ALL_BINS)

# Test execution targets
test: test-arena test-numerical test-graph test-performance

test-arena: $(ARENA_TEST_BIN)
	@echo "=== Running Arena L1 Tests ==="
	./$(ARENA_TEST_BIN)
	@echo

test-numerical: $(NUMERICAL_TEST_BIN)
	@echo "=== Running Numerical Tests ==="
	./$(NUMERICAL_TEST_BIN)
	@echo

test-graph: $(GRAPH_TEST_BIN)
	@echo "=== Running Graph L1 Tests ==="
	./$(GRAPH_TEST_BIN)
	@echo

test-performance: $(PERFORMANCE_TEST_BIN)
	@echo "=== Running L1 Performance Tests ==="
	./$(PERFORMANCE_TEST_BIN)
	@echo

# Benchmark execution
benchmark: $(BENCHMARK_BIN)
	@echo "=== Running 8T vs 7T Benchmark ==="
	./$(BENCHMARK_BIN)
	@echo

# Performance regression testing
regression-test: $(BENCHMARK_BIN)
	@echo "=== Performance Regression Testing ==="
	@for i in 1 2 3 4 5; do \
		echo "Run $$i/5:"; \
		./$(BENCHMARK_BIN) | grep -E "(Speedup|efficiency|regression)"; \
		echo; \
	done

# Memory leak testing (requires valgrind)
memcheck: $(ALL_BINS)
	@echo "=== Memory Leak Testing ==="
	@for bin in $(ALL_BINS); do \
		echo "Testing $$bin:"; \
		valgrind --leak-check=full --error-exitcode=1 ./$$bin > /dev/null; \
		if [ $$? -eq 0 ]; then \
			echo "✅ $$bin: No memory leaks"; \
		else \
			echo "❌ $$bin: Memory leaks detected"; \
		fi; \
	done

# Performance profiling (requires perf)
perf-profile: $(BENCHMARK_BIN)
	@echo "=== Performance Profiling ==="
	perf record -g ./$(BENCHMARK_BIN)
	perf report

# Cache analysis (requires perf)
cache-analysis: $(PERFORMANCE_TEST_BIN)
	@echo "=== Cache Performance Analysis ==="
	perf stat -e cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses \
		./$(PERFORMANCE_TEST_BIN)

# Assembly output for optimization analysis
asm: $(BENCHMARK_SRC)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      $(INCLUDES) -S $< -o $(BENCHMARK_SRC:.c=.s)

# Optimization report
opt-report: $(BENCHMARK_SRC)
	$(CC) $(CFLAGS) $(SIMD_FLAGS) $(CNS_8T_FLAGS) $(PERF_FLAGS) $(CACHE_FLAGS) \
	      $(INCLUDES) -fopt-info-vec-optimized -fopt-info-loop-optimized \
	      -c $< -o /dev/null

# SIMD capability check
check-simd:
	@echo "=== Checking SIMD Capabilities ==="
	@echo "Compiler SIMD support:"
	@$(CC) -march=native -dM -E - < /dev/null | grep -E "(SSE|AVX|FMA)"
	@echo
	@echo "Runtime CPU features:"
	@cat /proc/cpuinfo | grep -E "(sse|avx|fma)" | head -1 || echo "CPU info not available"

# Size analysis
size-analysis: $(ALL_BINS)
	@echo "=== Binary Size Analysis ==="
	@for bin in $(ALL_BINS); do \
		echo "$$bin:"; \
		size $$bin; \
		echo; \
	done

# Dependency analysis
deps:
	@echo "=== Dependency Analysis ==="
	@for src in $(ARENA_TEST_SRC) $(NUMERICAL_TEST_SRC) $(GRAPH_TEST_SRC) \
	            $(PERFORMANCE_TEST_SRC) $(BENCHMARK_SRC); do \
		echo "$$src dependencies:"; \
		$(CC) $(INCLUDES) -MM $$src; \
		echo; \
	done

# Clean targets
clean:
	rm -f *.o *.s *.gcda *.gcno gmon.out

clean-all: clean
	rm -f $(ALL_BINS)

# Documentation generation
docs:
	@echo "=== Generating Documentation ==="
	doxygen -g 8t-tests.doxyfile 2>/dev/null || true
	@if [ -f 8t-tests.doxyfile ]; then \
		sed -i 's/PROJECT_NAME.*=.*/PROJECT_NAME = "8T Test Infrastructure"/' 8t-tests.doxyfile; \
		sed -i 's/INPUT.*=.*/INPUT = ./' 8t-tests.doxyfile; \
		sed -i 's/RECURSIVE.*=.*/RECURSIVE = YES/' 8t-tests.doxyfile; \
		doxygen 8t-tests.doxyfile; \
		echo "Documentation generated in html/"; \
	else \
		echo "Doxygen not available"; \
	fi

# Install targets
install: $(ALL_BINS)
	@echo "=== Installing 8T Test Suite ==="
	mkdir -p ../bin
	cp $(ALL_BINS) ../bin/
	@echo "Tests installed to ../bin/"

# Continuous integration target
ci: clean all test benchmark
	@echo "=== CI Pipeline Complete ==="

# Help target
help:
	@echo "8T Test Infrastructure Makefile"
	@echo "================================"
	@echo
	@echo "Targets:"
	@echo "  all              - Build all test binaries"
	@echo "  test             - Run all tests"
	@echo "  benchmark        - Run performance benchmarks"
	@echo "  regression-test  - Run performance regression tests"
	@echo "  memcheck         - Check for memory leaks (requires valgrind)"
	@echo "  perf-profile     - Profile performance (requires perf)"
	@echo "  cache-analysis   - Analyze cache performance (requires perf)"
	@echo "  check-simd       - Check SIMD capabilities"
	@echo "  size-analysis    - Analyze binary sizes"
	@echo "  asm              - Generate assembly output"
	@echo "  opt-report       - Generate optimization report"
	@echo "  docs             - Generate documentation (requires doxygen)"
	@echo "  clean            - Clean object files"
	@echo "  clean-all        - Clean all generated files"
	@echo "  install          - Install tests to ../bin/"
	@echo "  ci               - Run continuous integration pipeline"
	@echo
	@echo "Build variants:"
	@echo "  debug            - Build with debug symbols"
	@echo "  profile          - Build with profiling support"
	@echo "  coverage         - Build with coverage support"
	@echo "  analyze          - Build with static analysis"
	@echo
	@echo "Individual tests:"
	@echo "  test-arena       - Run arena allocator tests"
	@echo "  test-numerical   - Run numerical precision tests"
	@echo "  test-graph       - Run graph optimization tests"
	@echo "  test-performance - Run L1 cache performance tests"

# Make all targets phony except object files
.PHONY: all test test-arena test-numerical test-graph test-performance benchmark \
        regression-test memcheck perf-profile cache-analysis check-simd \
        size-analysis asm opt-report deps clean clean-all docs install ci help \
        debug profile coverage analyze