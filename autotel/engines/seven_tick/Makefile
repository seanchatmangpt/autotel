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

verification: $(GATEKEEPER_BIN)

$(GATEKEEPER_BIN): $(GATEKEEPER_SRC) $(RUNTIME_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -l7t_runtime -ldl -Wl,-rpath,$(PWD)/$(LIB_DIR)

clean:
	rm -f $(RUNTIME_LIB) $(COMPILER_BIN) $(GATEKEEPER_BIN)
	rm -f /tmp/seven_t_kernel.c /tmp/kernel.so /tmp/sprint_health_data.ttl

run-benchmark: all
	cd $(VERIFICATION_DIR) && ./gatekeeper
