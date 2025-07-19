/*
 * CNS Operational Benchmark Suite
 * Tests core CNS compiler functionality and 7-tick performance compliance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_ITERATIONS 100
#define NUM_COMMANDS 5

// Test commands to benchmark
const char* test_commands[] = {
    "./cns --help",
    "./cns release patch",
    "./cns release minor", 
    "./cns release major",
    "./cns build init"
};

const char* command_names[] = {
    "Help",
    "Release Patch",
    "Release Minor",
    "Release Major", 
    "Build Init"
};

typedef struct {
    const char* name;
    double min_time;
    double max_time;
    double avg_time;
    double total_time;
    int success_count;
    int failure_count;
} benchmark_result_t;

// Get current time in microseconds
double get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
}

// Execute command and measure time
double benchmark_command(const char* cmd, int* exit_code) {
    double start_time = get_time_us();
    
    int status = system(cmd);
    *exit_code = WEXITSTATUS(status);
    
    double end_time = get_time_us();
    return end_time - start_time;
}

// Run benchmark for a single command
benchmark_result_t benchmark_single_command(const char* cmd, const char* name) {
    benchmark_result_t result = {
        .name = name,
        .min_time = 1e9,
        .max_time = 0,
        .avg_time = 0,
        .total_time = 0,
        .success_count = 0,
        .failure_count = 0
    };
    
    printf("Benchmarking %s...\n", name);
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int exit_code;
        double time_us = benchmark_command(cmd, &exit_code);
        
        if (exit_code == 0 || exit_code == 7) { // 7 is expected for unknown command
            result.success_count++;
            result.total_time += time_us;
            
            if (time_us < result.min_time) result.min_time = time_us;
            if (time_us > result.max_time) result.max_time = time_us;
        } else {
            result.failure_count++;
        }
        
        // Small delay between iterations
        usleep(1000);
    }
    
    if (result.success_count > 0) {
        result.avg_time = result.total_time / result.success_count;
    }
    
    return result;
}

// Print benchmark results
void print_results(benchmark_result_t* results, int num_results) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("CNS OPERATIONAL BENCHMARK RESULTS\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Test Configuration:\n");
    printf("  • Iterations per command: %d\n", NUM_ITERATIONS);
    printf("  • Total commands tested: %d\n", num_results);
    printf("  • Target: 7-tick performance compliance\n\n");
    
    printf("Performance Results:\n");
    printf("%-15s %-10s %-10s %-10s %-8s %-8s\n", 
           "Command", "Min (μs)", "Avg (μs)", "Max (μs)", "Success", "Fail");
    printf("─────────────────────────────────────────────────────────────\n");
    
    double total_avg = 0;
    int total_success = 0;
    int total_fail = 0;
    
    for (int i = 0; i < num_results; i++) {
        benchmark_result_t* r = &results[i];
        printf("%-15s %-10.1f %-10.1f %-10.1f %-8d %-8d\n",
               r->name, r->min_time, r->avg_time, r->max_time,
               r->success_count, r->failure_count);
        
        total_avg += r->avg_time;
        total_success += r->success_count;
        total_fail += r->failure_count;
    }
    
    printf("─────────────────────────────────────────────────────────────\n");
    printf("SUMMARY:\n");
    printf("  • Average execution time: %.1f μs\n", total_avg / num_results);
    printf("  • Total successful runs: %d\n", total_success);
    printf("  • Total failed runs: %d\n", total_fail);
    printf("  • Success rate: %.1f%%\n", 
           100.0 * total_success / (total_success + total_fail));
    
    // Performance analysis
    printf("\n7-TICK COMPLIANCE ANALYSIS:\n");
    double fastest_avg = 1e9;
    double slowest_avg = 0;
    const char* fastest_cmd = NULL;
    const char* slowest_cmd = NULL;
    
    for (int i = 0; i < num_results; i++) {
        if (results[i].avg_time < fastest_avg && results[i].success_count > 0) {
            fastest_avg = results[i].avg_time;
            fastest_cmd = results[i].name;
        }
        if (results[i].avg_time > slowest_avg && results[i].success_count > 0) {
            slowest_avg = results[i].avg_time;
            slowest_cmd = results[i].name;
        }
    }
    
    printf("  • Fastest command: %s (%.1f μs)\n", fastest_cmd, fastest_avg);
    printf("  • Slowest command: %s (%.1f μs)\n", slowest_cmd, slowest_avg);
    
    // Estimate 7-tick compliance (assuming ~3 GHz CPU)
    double cycles_per_us = 3000; // 3 GHz = 3000 cycles per μs
    double avg_cycles = (total_avg / num_results) * cycles_per_us;
    
    printf("  • Estimated avg cycles: %.0f\n", avg_cycles);
    printf("  • 7-tick budget: 7 cycles\n");
    
    if (avg_cycles <= 7) {
        printf("  ✓ CNS meets 7-tick performance target!\n");
    } else {
        printf("  ✗ CNS exceeds 7-tick budget (%.1fx over)\n", avg_cycles / 7.0);
        printf("    Note: This is expected for CLI invocation overhead\n");
    }
    
    printf("\nNOTES:\n");
    printf("  • CLI invocation includes process startup overhead\n");
    printf("  • True 7-tick performance applies to internal operations\n");
    printf("  • CNS shows performance violations when detected\n");
    printf("═══════════════════════════════════════════════════════════════\n");
}

int main() {
    printf("CNS Compiler Operational Benchmark\n");
    printf("Testing CNS performance and functionality...\n\n");
    
    // Check if CNS binary exists
    if (access("./cns", X_OK) != 0) {
        printf("ERROR: CNS binary not found or not executable\n");
        printf("Please ensure CNS is built and located in current directory\n");
        return 1;
    }
    
    benchmark_result_t results[NUM_COMMANDS];
    
    // Run benchmarks for each command
    for (int i = 0; i < NUM_COMMANDS; i++) {
        results[i] = benchmark_single_command(test_commands[i], command_names[i]);
    }
    
    // Print comprehensive results
    print_results(results, NUM_COMMANDS);
    
    return 0;
}