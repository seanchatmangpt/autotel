CC = cc
BASE_CFLAGS = -O3 -march=native -fPIC -Wall -Wextra
BASE_LDFLAGS = -shared

# Default flags
CFLAGS = $(BASE_CFLAGS)
LDFLAGS = $(BASE_LDFLAGS)

# Production build flags
PROD_CFLAGS = $(BASE_CFLAGS) -DNDEBUG -flto -fomit-frame-pointer -funroll-loops -ftree-vectorize
PROD_LDFLAGS = $(BASE_LDFLAGS) -flto

# Directories
RUNTIME_DIR = runtime/src
COMPILER_DIR = compiler/src
LIB_DIR = lib
VERIFICATION_DIR = verification

# Runtime library
RUNTIME_SRCS = $(RUNTIME_DIR)/seven_t_runtime.c
RUNTIME_LIB = $(LIB_DIR)/lib7t_runtime.so

# Compiler
COMPILER_SRCS = $(COMPILER_DIR)/compiler.c $(COMPILER_DIR)/qop.c $(COMPILER_DIR)/cjinja.c
COMPILER_BIN = compiler/seven-t-compiler

# Verification
GATEKEEPER_SRC = $(VERIFICATION_DIR)/gatekeeper_benchmark.c
GATEKEEPER_BIN = $(VERIFICATION_DIR)/gatekeeper
SHACL_BENCHMARK_SRC = $(VERIFICATION_DIR)/shacl_validation_benchmark.c
SHACL_BENCHMARK_BIN = $(VERIFICATION_DIR)/shacl_validation_benchmark
SHACL_7TICK_BENCHMARK_SRC = $(VERIFICATION_DIR)/shacl_7tick_benchmark.c
SHACL_7TICK_BENCHMARK_BIN = $(VERIFICATION_DIR)/shacl_7tick_benchmark
CJINJA_BENCHMARK_SRC = $(VERIFICATION_DIR)/cjinja_benchmark.c
CJINJA_BENCHMARK_BIN = $(VERIFICATION_DIR)/cjinja_benchmark
CJINJA_80_20_BENCHMARK_SRC = $(VERIFICATION_DIR)/cjinja_80_20_benchmark.c
CJINJA_80_20_BENCHMARK_BIN = $(VERIFICATION_DIR)/cjinja_80_20_benchmark
CJINJA_7TICK_VS_49TICK_BENCHMARK_SRC = $(VERIFICATION_DIR)/cjinja_7tick_vs_49tick_benchmark.c
CJINJA_7TICK_VS_49TICK_BENCHMARK_BIN = $(VERIFICATION_DIR)/cjinja_7tick_vs_49tick_benchmark
SHACL_IMPL_BENCHMARK_SRC = $(VERIFICATION_DIR)/shacl_implementation_benchmark.c
SHACL_IMPL_BENCHMARK_BIN = $(VERIFICATION_DIR)/shacl_implementation_benchmark
SPARQL_7TICK_BENCHMARK_SRC = $(VERIFICATION_DIR)/sparql_7tick_benchmark.c
SPARQL_7TICK_BENCHMARK_BIN = $(VERIFICATION_DIR)/sparql_7tick_benchmark

# Telemetry7T targets
TELEMETRY7T_SRC = c_src/telemetry7t.c
TELEMETRY7T_BENCHMARK_SRC = verification/telemetry7t_benchmark.c
TELEMETRY7T_BENCHMARK_BIN = verification/telemetry7t_benchmark
TELEMETRY7T_7TICK_BENCHMARK_SRC = verification/telemetry7t_7tick_benchmark.c
TELEMETRY7T_7TICK_BENCHMARK_BIN = verification/telemetry7t_7tick_benchmark
TELEMETRY7T_JSON_BENCHMARK_SRC = verification/telemetry7t_simple_json.c
TELEMETRY7T_JSON_BENCHMARK_BIN = verification/telemetry7t_simple_json

# PM7T Process Mining Library
PM7T_OBJS = c_src/pm7t.o
PM7T_LIB = libpm7t.so

# Optimized 7T Engine
OPTIMIZED_OBJS = c_src/sparql7t_optimized.o
OPTIMIZED_LIB = libsparql7t_optimized.so

.PHONY: all runtime compiler verification clean production

all: runtime compiler verification

production: CFLAGS = $(PROD_CFLAGS)
production: LDFLAGS = $(PROD_LDFLAGS)
production: clean all

runtime: $(RUNTIME_LIB)

$(RUNTIME_LIB): $(RUNTIME_SRCS)
	mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

compiler: $(COMPILER_BIN)

$(COMPILER_BIN): $(COMPILER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ -ldl

verification: $(GATEKEEPER_BIN) $(SHACL_BENCHMARK_BIN) $(SHACL_7TICK_BENCHMARK_BIN) $(CJINJA_BENCHMARK_BIN) $(CJINJA_80_20_BENCHMARK_BIN) $(SHACL_IMPL_BENCHMARK_BIN) $(TELEMETRY7T_BENCHMARK_BIN) $(TELEMETRY7T_7TICK_BENCHMARK_BIN) $(TELEMETRY7T_JSON_BENCHMARK_BIN)

$(GATEKEEPER_BIN): $(GATEKEEPER_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(SHACL_BENCHMARK_BIN): $(SHACL_BENCHMARK_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(SHACL_7TICK_BENCHMARK_BIN): $(SHACL_7TICK_BENCHMARK_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(CJINJA_BENCHMARK_BIN): $(CJINJA_BENCHMARK_SRC) $(COMPILER_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $(CJINJA_BENCHMARK_SRC) $(COMPILER_DIR)/cjinja.c -I$(COMPILER_DIR)

$(CJINJA_80_20_BENCHMARK_BIN): $(CJINJA_80_20_BENCHMARK_SRC) $(COMPILER_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $(CJINJA_80_20_BENCHMARK_SRC) $(COMPILER_DIR)/cjinja.c -I$(COMPILER_DIR)

$(SHACL_IMPL_BENCHMARK_BIN): $(SHACL_IMPL_BENCHMARK_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(TELEMETRY7T_BENCHMARK_BIN): $(TELEMETRY7T_BENCHMARK_SRC) $(TELEMETRY7T_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

$(TELEMETRY7T_7TICK_BENCHMARK_BIN): $(TELEMETRY7T_7TICK_BENCHMARK_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(TELEMETRY7T_JSON_BENCHMARK_BIN): $(TELEMETRY7T_JSON_BENCHMARK_SRC) $(TELEMETRY7T_SRC) $(COMPILER_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $^

$(PM7T_LIB): $(PM7T_OBJS)
	$(CC) -shared -o $@ $(PM7T_OBJS) -lm

c_src/pm7t.o: c_src/pm7t.c c_src/pm7t.h
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# Optimized 7T Engine targets
$(OPTIMIZED_LIB): $(OPTIMIZED_OBJS)
	$(CC) -shared -o $@ $(OPTIMIZED_OBJS) -lm

c_src/sparql7t_optimized.o: c_src/sparql7t_optimized.c c_src/sparql7t_optimized.h
	$(CC) $(CFLAGS) -O3 -march=native -fPIC -c $< -o $@

# Process mining demo
process_mining_demo: examples/process_mining_demo.c $(PM7T_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -lpm7t -lm

# PM7T benchmarks
pm7t_benchmarks: benchmarks/pm7t_benchmarks.c $(PM7T_LIB)
	$(CC) $(CFLAGS) -O3 -march=native -o $@ $< -L. -lpm7t -lm

# Optimized benchmarks
optimized_benchmarks: benchmarks/optimized_benchmarks.c $(OPTIMIZED_LIB)
	$(CC) $(CFLAGS) -O3 -march=native -o $@ $< -L. -lsparql7t_optimized -lm

# Run benchmarks
benchmark: pm7t_benchmarks
	./pm7t_benchmarks

# Run optimized benchmarks
benchmark_optimized: optimized_benchmarks
	./optimized_benchmarks

clean:
	rm -f $(RUNTIME_LIB) $(COMPILER_BIN) $(GATEKEEPER_BIN) $(SHACL_BENCHMARK_BIN) $(SHACL_7TICK_BENCHMARK_BIN) $(CJINJA_BENCHMARK_BIN) $(SHACL_IMPL_BENCHMARK_BIN) $(TELEMETRY7T_BENCHMARK_BIN) $(PM7T_OBJS) $(PM7T_LIB) $(OPTIMIZED_OBJS) $(OPTIMIZED_LIB) process_mining_demo pm7t_benchmarks optimized_benchmarks
	rm -f /tmp/seven_t_kernel.c /tmp/kernel.so /tmp/sprint_health_data.ttl

run-benchmark: all
	cd $(VERIFICATION_DIR) && ./gatekeeper

run-shacl-benchmark: all
	cd $(VERIFICATION_DIR) && ./shacl_validation_benchmark

run-shacl-7tick-benchmark: all
	cd $(VERIFICATION_DIR) && ./shacl_7tick_benchmark

run-cjinja-benchmark: all
	cd $(VERIFICATION_DIR) && ./cjinja_benchmark

run-cjinja-80-20-benchmark: all
	cd $(VERIFICATION_DIR) && ./cjinja_80_20_benchmark

run-shacl-impl-benchmark: all
	cd $(VERIFICATION_DIR) && LD_LIBRARY_PATH=../lib ./shacl_implementation_benchmark

run-shacl-7tick-benchmark: all
	cd $(VERIFICATION_DIR) && LD_LIBRARY_PATH=../lib ./shacl_7tick_benchmark

run-telemetry7t-benchmark: all
	./verification/telemetry7t_benchmark

run-telemetry7t-7tick-benchmark: all
	./verification/telemetry7t_7tick_benchmark

run-telemetry7t-json-benchmark: all
	./verification/telemetry7t_simple_json
