CC = gcc
CFLAGS = -O3 -march=native -fPIC -Wall -Wextra
LDFLAGS = -lpthread -ldl -lm

# All Seven Tick components
COMPONENTS = sparql7t shacl7t owl7t_compiler qop cjinja pico_jit

# Source files
SPARQL7T_SRC = c_src/sparql7t.c
SHACL7T_SRC = c_src/shacl7t.c
OWL7T_SRC = c_src/owl7t_compiler.c
QOP_SRC = c_src/qop.c
CJINJA_SRC = c_src/cjinja.c
JIT_SRC = c_src/pico_jit.c

# Shared libraries
LIBS = libsparql7t.so libshacl7t.so libow17t.so libqop.so libcjinja.so libpico_jit.so

# Default target
all: $(LIBS) test_seven_tick

# SPARQL-7T
libsparql7t.so: $(SPARQL7T_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

# SHACL-7T
libshacl7t.so: $(SHACL7T_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

# OWL-7T Compiler
libow17t.so: $(OWL7T_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

# Query Optimization Planner
libqop.so: $(QOP_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

# CJinja Template Engine
libcjinja.so: $(CJINJA_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

# Adaptive JIT Daemon
libpico_jit.so: $(JIT_SRC)
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

# Unified test program
test_seven_tick: c_src/test_seven_tick.c $(SPARQL7T_SRC) $(SHACL7T_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Benchmark suite
benchmark: test_seven_tick
	@echo "=== Seven Tick Benchmark Suite ==="
	@echo "Running on: $$(uname -a)"
	@echo "CPU: $$(sysctl -n machdep.cpu.brand_string 2>/dev/null || lscpu | grep 'Model name' | cut -d: -f2 | xargs)"
	@echo ""
	./test_seven_tick benchmark

# Unit tests
test: test_seven_tick
	./test_seven_tick

# Performance analysis
perf: test_seven_tick
	@echo "Note: perf not available on macOS, using time instead"
	time ./test_seven_tick benchmark

# Clean
clean:
	rm -f $(LIBS) test_seven_tick /tmp/jit_* *.o

# Install (for Python integration)
install: $(LIBS)
	mkdir -p ../lib
	cp $(LIBS) ../lib/

.PHONY: all test benchmark perf clean install
