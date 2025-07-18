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
SHACL_IMPL_BENCHMARK_SRC = $(VERIFICATION_DIR)/shacl_implementation_benchmark.c
SHACL_IMPL_BENCHMARK_BIN = $(VERIFICATION_DIR)/shacl_implementation_benchmark

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

verification: $(GATEKEEPER_BIN) $(SHACL_BENCHMARK_BIN) $(SHACL_7TICK_BENCHMARK_BIN) $(CJINJA_BENCHMARK_BIN) $(SHACL_IMPL_BENCHMARK_BIN)

$(GATEKEEPER_BIN): $(GATEKEEPER_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(SHACL_BENCHMARK_BIN): $(SHACL_BENCHMARK_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(SHACL_7TICK_BENCHMARK_BIN): $(SHACL_7TICK_BENCHMARK_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

$(CJINJA_BENCHMARK_BIN): $(CJINJA_BENCHMARK_SRC) $(COMPILER_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $(CJINJA_BENCHMARK_SRC) $(COMPILER_DIR)/cjinja.c -I$(COMPILER_DIR)

$(SHACL_IMPL_BENCHMARK_BIN): $(SHACL_IMPL_BENCHMARK_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

clean:
	rm -f $(RUNTIME_LIB) $(COMPILER_BIN) $(GATEKEEPER_BIN) $(SHACL_BENCHMARK_BIN) $(SHACL_7TICK_BENCHMARK_BIN) $(CJINJA_BENCHMARK_BIN) $(SHACL_IMPL_BENCHMARK_BIN)
	rm -f /tmp/seven_t_kernel.c /tmp/kernel.so /tmp/sprint_health_data.ttl

run-benchmark: all
	cd $(VERIFICATION_DIR) && ./gatekeeper

run-shacl-benchmark: all
	cd $(VERIFICATION_DIR) && ./shacl_validation_benchmark

run-shacl-7tick-benchmark: all
	cd $(VERIFICATION_DIR) && ./shacl_7tick_benchmark

run-cjinja-benchmark: all
	cd $(VERIFICATION_DIR) && ./cjinja_benchmark

run-shacl-impl-benchmark: all
	cd $(VERIFICATION_DIR) && LD_LIBRARY_PATH=../lib ./shacl_implementation_benchmark

run-shacl-7tick-benchmark: all
	cd $(VERIFICATION_DIR) && LD_LIBRARY_PATH=../lib ./shacl_7tick_benchmark
