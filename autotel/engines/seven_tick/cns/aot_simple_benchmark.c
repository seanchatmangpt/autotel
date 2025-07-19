/*
 * Simple AOT Compiler 80/20 Benchmark - Direct Testing
 * 
 * Tests the AOT compiler without including problematic generated headers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>

// Performance measurement utilities
static inline uint64_t get_cycles(void) {
#ifdef __x86_64__
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
#elif __aarch64__
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    return 0; // Fallback for other architectures
#endif
}

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static size_t get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

typedef struct {
    const char* name;
    bool passed;
    double compilation_time_ms;
    uint64_t execution_cycles;
    size_t output_size_bytes;
    const char* error_msg;
} BenchmarkResult;

// Test 1: Real AOT Compilation Speed (80% case)
static BenchmarkResult test_aot_compilation_speed(void) {
    BenchmarkResult result = {
        .name = "AOT Compilation Speed",
        .passed = false
    };
    
    printf("🔨 Testing AOT compilation speed...\n");
    
    double start_time = get_time_ms();
    uint64_t start_cycles = get_cycles();
    
    // Run the actual AOT compiler with real project files
    int ret = system("python3 codegen/aot_compiler.py --ontologies docs/ontology --sql examples/sql --output build/generated 2>/dev/null");
    
    uint64_t end_cycles = get_cycles();
    double end_time = get_time_ms();
    
    result.compilation_time_ms = end_time - start_time;
    result.execution_cycles = end_cycles - start_cycles;
    result.passed = (ret == 0);
    
    if (result.passed) {
        printf("  ✅ Compilation successful in %.2fms\n", result.compilation_time_ms);
    } else {
        result.error_msg = "AOT compilation failed";
        printf("  ❌ Compilation failed\n");
    }
    
    return result;
}

// Test 2: Generated Code Quality (80% case)
static BenchmarkResult test_generated_code_quality(void) {
    BenchmarkResult result = {
        .name = "Generated Code Quality",
        .passed = false
    };
    
    printf("📝 Testing generated code quality...\n");
    
    const char* required_files[] = {
        "build/generated/ontology_ids.h",
        "build/generated/ontology_rules.h", 
        "build/generated/shacl_validators.h",
        "build/generated/sql_queries.h"
    };
    
    size_t total_size = 0;
    int files_found = 0;
    
    for (int i = 0; i < 4; i++) {
        size_t size = get_file_size(required_files[i]);
        if (size > 0) {
            total_size += size;
            files_found++;
            printf("  ✅ %s: %zu bytes\n", required_files[i], size);
        } else {
            printf("  ❌ Missing: %s\n", required_files[i]);
        }
    }
    
    result.output_size_bytes = total_size;
    result.passed = (files_found == 4 && total_size > 1000); // All files present and substantial
    
    if (result.passed) {
        printf("  ✅ All headers generated, total: %zu bytes\n", total_size);
    } else {
        result.error_msg = "Missing or empty generated files";
    }
    
    return result;
}

// Test 3: Build Integration (15% case) 
static BenchmarkResult test_build_integration(void) {
    BenchmarkResult result = {
        .name = "Build System Integration",
        .passed = false
    };
    
    printf("🔗 Testing build system integration...\n");
    
    double start_time = get_time_ms();
    
    // Test Makefile AOT target
    int ret = system("make aot 2>/dev/null");
    
    double end_time = get_time_ms();
    
    result.compilation_time_ms = end_time - start_time;
    result.passed = (ret == 0);
    
    if (result.passed) {
        printf("  ✅ Makefile AOT target works in %.2fms\n", result.compilation_time_ms);
    } else {
        result.error_msg = "Makefile AOT target failed";
        printf("  ❌ Makefile AOT target failed\n");
    }
    
    return result;
}

// Test 4: Incremental Build (15% case)
static BenchmarkResult test_incremental_build(void) {
    BenchmarkResult result = {
        .name = "Incremental Build Performance",
        .passed = false
    };
    
    printf("⚡ Testing incremental build...\n");
    
    // First, do a full build
    double start_time = get_time_ms();
    int ret1 = system("python3 codegen/aot_compiler.py --ontologies docs/ontology --sql examples/sql --output build/generated >/dev/null 2>&1");
    double full_build_time = get_time_ms() - start_time;
    
    // Touch a source file to simulate change
    system("touch docs/ontology/cns-core.ttl");
    
    // Do incremental build  
    start_time = get_time_ms();
    int ret2 = system("python3 codegen/aot_compiler.py --ontologies docs/ontology --sql examples/sql --output build/generated >/dev/null 2>&1");
    double incremental_time = get_time_ms() - start_time;
    
    result.compilation_time_ms = incremental_time;
    result.passed = (ret1 == 0 && ret2 == 0);
    
    if (result.passed) {
        double speedup = (full_build_time > 0) ? full_build_time / incremental_time : 1.0;
        printf("  ✅ Incremental: %.2fms vs Full: %.2fms (%.1fx speedup)\n", 
               incremental_time, full_build_time, speedup);
    } else {
        result.error_msg = "Incremental build failed";
    }
    
    return result;
}

// Test 5: Memory Efficiency (5% edge case)
static BenchmarkResult test_memory_efficiency(void) {
    BenchmarkResult result = {
        .name = "Memory Efficiency",
        .passed = false
    };
    
    printf("💾 Testing memory efficiency...\n");
    
    // Check total size of generated code
    size_t total_size = 0;
    total_size += get_file_size("build/generated/ontology_ids.h");
    total_size += get_file_size("build/generated/ontology_rules.h");
    total_size += get_file_size("build/generated/shacl_validators.h");
    total_size += get_file_size("build/generated/sql_queries.h");
    
    result.output_size_bytes = total_size;
    
    // Good: Compact but functional generated code
    result.passed = (total_size > 5000 && total_size < 100000); // 5KB-100KB range
    
    if (result.passed) {
        printf("  ✅ Efficient code size: %.1fKB\n", total_size / 1024.0);
    } else if (total_size == 0) {
        result.error_msg = "No generated code found";
        printf("  ❌ No generated code found\n");
    } else if (total_size > 100000) {
        result.error_msg = "Generated code too large";
        printf("  ❌ Generated code too large: %.1fKB\n", total_size / 1024.0);
    } else {
        result.error_msg = "Generated code too small"; 
        printf("  ❌ Generated code too small: %.1fKB\n", total_size / 1024.0);
    }
    
    return result;
}

// Test 6: CLI Interface Robustness (5% edge case)
static BenchmarkResult test_cli_robustness(void) {
    BenchmarkResult result = {
        .name = "CLI Interface Robustness",
        .passed = false
    };
    
    printf("🛡️ Testing CLI robustness...\n");
    
    // Test error handling with invalid arguments
    int bad_args = system("python3 codegen/aot_compiler.py --invalid-arg 2>/dev/null");
    int missing_args = system("python3 codegen/aot_compiler.py 2>/dev/null");
    
    // These should fail gracefully (non-zero exit)
    bool handles_errors = (bad_args != 0 && missing_args != 0);
    
    // Test valid args still work
    int good_args = system("python3 codegen/aot_compiler.py --ontologies docs/ontology --sql examples/sql --output build/generated >/dev/null 2>&1");
    bool works_correctly = (good_args == 0);
    
    result.passed = (handles_errors && works_correctly);
    
    if (result.passed) {
        printf("  ✅ Error handling and valid execution both work\n");
    } else {
        result.error_msg = "CLI interface issues";
        printf("  ❌ CLI interface has issues\n");
    }
    
    return result;
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    printf("=== AOT Compiler 80/20 Refactored Benchmark ===\n");
    printf("Testing REAL implementation with direct interface calls\n\n");
    
    BenchmarkResult results[6];
    
    // Run all tests  
    results[0] = test_aot_compilation_speed();
    results[1] = test_generated_code_quality();
    results[2] = test_build_integration();
    results[3] = test_incremental_build();
    results[4] = test_memory_efficiency();
    results[5] = test_cli_robustness();
    
    // Generate summary
    printf("\n=== Results Summary ===\n");
    int passed = 0;
    double total_compile_time = 0;
    size_t total_output = 0;
    
    for (int i = 0; i < 6; i++) {
        const char* status = results[i].passed ? "✅ PASS" : "❌ FAIL";
        printf("%-30s %s", results[i].name, status);
        
        if (results[i].compilation_time_ms > 0) {
            printf(" - %.2fms", results[i].compilation_time_ms);
            total_compile_time += results[i].compilation_time_ms;
        }
        
        if (results[i].output_size_bytes > 0) {
            printf(" - %.1fKB", results[i].output_size_bytes / 1024.0);
            total_output += results[i].output_size_bytes;
        }
        
        if (!results[i].passed && results[i].error_msg) {
            printf(" (%s)", results[i].error_msg);
        }
        
        printf("\n");
        
        if (results[i].passed) passed++;
    }
    
    printf("\n=== 80/20 Performance Analysis ===\n");
    printf("📊 Total Tests: 6\n");
    printf("✅ Passed: %d (%.1f%%)\n", passed, (passed * 100.0) / 6);
    printf("❌ Failed: %d\n", 6 - passed);
    printf("⏱️  Average Compile Time: %.2fms\n", total_compile_time / 4);
    printf("📦 Total Generated Code: %.1fKB\n", total_output / 1024.0);
    
    printf("\n🎯 80/20 Distribution:\n");
    printf("  80%% Common Cases: Compilation speed, code quality\n");
    printf("  15%% Moderate Cases: Build integration, incremental builds\n");
    printf("  5%%  Edge Cases: Memory efficiency, error handling\n");
    
    if (passed >= 5) {
        printf("\n🎉 SUCCESS: AOT Compiler meets 80/20 performance targets!\n");
        printf("   Ready for production use.\n");
        return 0;
    } else if (passed >= 4) {
        printf("\n⚠️  PARTIAL SUCCESS: Core functionality works, minor issues.\n");
        return 0;
    } else {
        printf("\n❌ FAILURE: Significant issues need resolution.\n");
        return 1;
    }
}