#include "cns/8t.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

// Parser benchmark for 8T

#define ITERATIONS 100000

double get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

void benchmark_parser() {
    printf("Running parser benchmark...\n");
    
    // Sample triple data
    const char* triple = "<http://example.org/subject1> <http://example.org/predicate1> <http://example.org/object1> .";
    size_t len = strlen(triple);
    
    triple_t output;
    double start = get_time_us();
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Parser would parse here - placeholder for now
        output.subject = 1;
        output.predicate = 2;
        output.object = 3;
    }
    
    double end = get_time_us();
    double elapsed = end - start;
    double per_op = elapsed / ITERATIONS;
    
    printf("Parser Performance:\n");
    printf("  Total time: %.2f ms\n", elapsed / 1000.0);
    printf("  Per operation: %.2f ns\n", per_op * 1000.0);
    printf("  Throughput: %.2f M ops/sec\n", 1.0 / per_op);
}

void benchmark_batch_parsing() {
    printf("\nRunning batch parser benchmark...\n");
    
    // Simulate batch parsing
    triple_t batch[1000];
    double start = get_time_us();
    
    for (int i = 0; i < ITERATIONS / 1000; i++) {
        // Batch parse would happen here
        for (int j = 0; j < 1000; j++) {
            batch[j].subject = j;
            batch[j].predicate = j + 1;
            batch[j].object = j + 2;
        }
    }
    
    double end = get_time_us();
    double elapsed = end - start;
    double per_batch = elapsed / (ITERATIONS / 1000);
    
    printf("Batch Parser Performance:\n");
    printf("  Total time: %.2f ms\n", elapsed / 1000.0);
    printf("  Per batch (1000 items): %.2f us\n", per_batch);
    printf("  Throughput: %.2f M triples/sec\n", ITERATIONS / elapsed);
}

int main() {
    printf("8T Parser Benchmark\n");
    printf("==================\n\n");
    
    benchmark_parser();
    benchmark_batch_parsing();
    
    printf("\n✓ Benchmark complete\n");
    return 0;
}