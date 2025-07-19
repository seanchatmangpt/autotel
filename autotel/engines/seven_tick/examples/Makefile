CC = cc
CFLAGS = -O3 -march=native -fPIC -Wall -Wextra
CJINJA_DIR = ../compiler/src
SPARQL_DIR = ..

# Demo executables
DEMO_01 = demo_01_high_throughput_logging
DEMO_02 = demo_02_complex_web_templates
DEMO_03 = demo_03_sparql_result_formatting
DEMO_04 = demo_04_configuration_generation
DEMO_05 = demo_05_performance_comparison

# All demos
DEMOS = $(DEMO_01) $(DEMO_02) $(DEMO_03) $(DEMO_04) $(DEMO_05)

.PHONY: all clean run-all run-01 run-02 run-03 run-04 run-05

all: $(DEMOS)

# Demo 1: High-throughput logging (7-tick path)
$(DEMO_01): $(DEMO_01).c $(CJINJA_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $< $(CJINJA_DIR)/cjinja.c -I$(CJINJA_DIR)

# Demo 2: Complex web templates (49-tick path)
$(DEMO_02): $(DEMO_02).c $(CJINJA_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $< $(CJINJA_DIR)/cjinja.c -I$(CJINJA_DIR)

# Demo 3: SPARQL result formatting (both paths)
$(DEMO_03): $(DEMO_03).c $(CJINJA_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $< $(CJINJA_DIR)/cjinja.c -I$(CJINJA_DIR)

# Demo 4: Configuration generation (49-tick path)
$(DEMO_04): $(DEMO_04).c $(CJINJA_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $< $(CJINJA_DIR)/cjinja.c -I$(CJINJA_DIR)

# Demo 5: Performance comparison (both paths)
$(DEMO_05): $(DEMO_05).c $(CJINJA_DIR)/cjinja.c
	$(CC) $(CFLAGS) -o $@ $< $(CJINJA_DIR)/cjinja.c -I$(CJINJA_DIR)

# Run all demos
run-all: all
	@echo "Running all CJinja 7-Tick vs 49-Tick demos..."
	@echo "============================================="
	@echo ""
	@echo "Demo 1: High-Throughput Logging (7-Tick Path)"
	@echo "---------------------------------------------"
	@./$(DEMO_01)
	@echo ""
	@echo "Demo 2: Complex Web Templates (49-Tick Path)"
	@echo "--------------------------------------------"
	@./$(DEMO_02)
	@echo ""
	@echo "Demo 3: SPARQL Result Formatting (Both Paths)"
	@echo "---------------------------------------------"
	@./$(DEMO_03)
	@echo ""
	@echo "Demo 4: Configuration Generation (49-Tick Path)"
	@echo "----------------------------------------------"
	@./$(DEMO_04)
	@echo ""
	@echo "Demo 5: Performance Comparison (Both Paths)"
	@echo "------------------------------------------"
	@./$(DEMO_05)
	@echo ""
	@echo "All demos completed!"

# Run individual demos
run-01: $(DEMO_01)
	@echo "Demo 1: High-Throughput Logging (7-Tick Path)"
	@echo "============================================="
	@./$(DEMO_01)

run-02: $(DEMO_02)
	@echo "Demo 2: Complex Web Templates (49-Tick Path)"
	@echo "============================================"
	@./$(DEMO_02)

run-03: $(DEMO_03)
	@echo "Demo 3: SPARQL Result Formatting (Both Paths)"
	@echo "============================================="
	@./$(DEMO_03)

run-04: $(DEMO_04)
	@echo "Demo 4: Configuration Generation (49-Tick Path)"
	@echo "=============================================="
	@./$(DEMO_04)

run-05: $(DEMO_05)
	@echo "Demo 5: Performance Comparison (Both Paths)"
	@echo "=========================================="
	@./$(DEMO_05)

# Clean build artifacts
clean:
	rm -f $(DEMOS)

# Help
help:
	@echo "CJinja 7-Tick vs 49-Tick Demo Use Cases"
	@echo "======================================"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build all demo executables"
	@echo "  run-all    - Build and run all demos"
	@echo "  run-01     - Run Demo 1: High-throughput logging (7-tick)"
	@echo "  run-02     - Run Demo 2: Complex web templates (49-tick)"
	@echo "  run-03     - Run Demo 3: SPARQL result formatting (both)"
	@echo "  run-04     - Run Demo 4: Configuration generation (49-tick)"
	@echo "  run-05     - Run Demo 5: Performance comparison (both)"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Demo descriptions:"
	@echo "  Demo 1: Performance-critical logging with 7-tick path"
	@echo "  Demo 2: Feature-rich web templates with 49-tick path"
	@echo "  Demo 3: SPARQL integration using both paths"
	@echo "  Demo 4: Configuration generation with inheritance"
	@echo "  Demo 5: Comprehensive performance analysis" 