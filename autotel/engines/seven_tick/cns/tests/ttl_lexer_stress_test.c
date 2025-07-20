/*
 * TTL LEXER STRESS TEST AND PERFORMANCE BENCHMARK
 * Extreme testing for production readiness
 */

#include "../lean_sigma_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

/*═══════════════════════════════════════════════════════════════
  Stress Test Configuration
  ═══════════════════════════════════════════════════════════════*/

#define THREAD_COUNT 8
#define ITERATIONS_PER_THREAD 10000
#define MAX_SOURCE_SIZE (1024 * 1024) // 1MB source files

// Test scenarios
typedef enum {
    SCENARIO_NORMAL_CODE,
    SCENARIO_KEYWORD_HEAVY,
    SCENARIO_NUMBER_HEAVY,
    SCENARIO_WHITESPACE_HEAVY,
    SCENARIO_MIXED_COMPLEXITY,
    SCENARIO_ERROR_PRONE,
    SCENARIO_MAX
} TestScenario;

// Thread test context
typedef struct {
    int thread_id;
    TestScenario scenario;
    uint64_t total_cycles;
    uint64_t tokens_parsed;
    uint64_t errors_found;
    double dpmo;
    bool success;
} ThreadContext;

/*═══════════════════════════════════════════════════════════════
  Source Code Generators
  ═══════════════════════════════════════════════════════════════*/

// Generate normal C-like code
char* generate_normal_code(size_t size) {
    char* source = malloc(size + 1);
    size_t pos = 0;
    
    const char* templates[] = {
        "int var%d = %d;\n",
        "float f%d = %d.%d;\n",
        "if (x%d > %d) { return %d; }\n",
        "for (int i%d = 0; i%d < %d; i%d++) { sum += i%d; }\n",
        "while (n%d > 0) { n%d = n%d - 1; }\n"
    };
    
    int var_counter = 0;
    while (pos < size - 100) {
        int template_idx = rand() % 5;
        int written = snprintf(&source[pos], size - pos, templates[template_idx],
                              var_counter, rand() % 1000, rand() % 100,
                              var_counter, var_counter, rand() % 100, var_counter, var_counter);
        pos += written;
        var_counter++;
    }
    
    source[pos] = '\0';
    return source;
}

// Generate keyword-heavy code
char* generate_keyword_heavy(size_t size) {
    char* source = malloc(size + 1);
    size_t pos = 0;
    
    const char* keywords[] = {"int", "if", "for", "while", "return", "char", "float", "void"};
    
    while (pos < size - 20) {
        int kw_idx = rand() % 8;
        int written = snprintf(&source[pos], size - pos, "%s ", keywords[kw_idx]);
        pos += written;
    }
    
    source[pos] = '\0';
    return source;
}

// Generate number-heavy code
char* generate_number_heavy(size_t size) {
    char* source = malloc(size + 1);
    size_t pos = 0;
    
    while (pos < size - 20) {
        if (rand() % 2) {
            // Integer
            int written = snprintf(&source[pos], size - pos, "%d ", rand() % 1000000);
            pos += written;
        } else {
            // Float
            int written = snprintf(&source[pos], size - pos, "%d.%d ", 
                                 rand() % 1000, rand() % 1000);
            pos += written;
        }
    }
    
    source[pos] = '\0';
    return source;
}

// Generate whitespace-heavy code (tests SIMD optimization)
char* generate_whitespace_heavy(size_t size) {
    char* source = malloc(size + 1);
    
    for (size_t i = 0; i < size; i++) {
        if (i % 10 < 7) {
            // 70% whitespace
            const char ws[] = {' ', '\t', '\n', '\r'};
            source[i] = ws[rand() % 4];
        } else {
            // 30% actual content
            source[i] = 'a' + (rand() % 26);
        }
    }
    
    source[size] = '\0';
    return source;
}

// Generate mixed complexity code
char* generate_mixed_complexity(size_t size) {
    char* source = malloc(size + 1);
    size_t pos = 0;
    
    while (pos < size - 100) {
        int choice = rand() % 5;
        
        switch (choice) {
            case 0: // Variable declaration
                pos += snprintf(&source[pos], size - pos, 
                               "int complex_var_%d = %d + %d * %d;\n",
                               rand() % 1000, rand() % 100, rand() % 100, rand() % 100);
                break;
            case 1: // Function call
                pos += snprintf(&source[pos], size - pos,
                               "result = compute_%d(%d, %d, %d);\n",
                               rand() % 100, rand() % 100, rand() % 100, rand() % 100);
                break;
            case 2: // Complex expression
                pos += snprintf(&source[pos], size - pos,
                               "if ((a_%d > %d && b_%d < %d) || c_%d == %d) { }\n",
                               rand() % 100, rand() % 100, rand() % 100,
                               rand() % 100, rand() % 100, rand() % 100);
                break;
            case 3: // Whitespace block
                for (int i = 0; i < 10 && pos < size; i++) {
                    source[pos++] = (i % 3 == 0) ? '\n' : ' ';
                }
                break;
            case 4: // Comment-like structure
                pos += snprintf(&source[pos], size - pos, "// comment %d\n", rand() % 1000);
                break;
        }
    }
    
    source[pos] = '\0';
    return source;
}

// Generate error-prone code
char* generate_error_prone(size_t size) {
    char* source = malloc(size + 1);
    size_t pos = 0;
    
    while (pos < size - 50) {
        int choice = rand() % 4;
        
        switch (choice) {
            case 0: // Valid code
                pos += snprintf(&source[pos], size - pos, "int x = %d; ", rand() % 100);
                break;
            case 1: // Invalid characters
                source[pos++] = '@' + (rand() % 10);
                source[pos++] = ' ';
                break;
            case 2: // Malformed numbers
                pos += snprintf(&source[pos], size - pos, "%d.%d.%d ", 
                               rand() % 100, rand() % 100, rand() % 100);
                break;
            case 3: // Mixed valid/invalid
                pos += snprintf(&source[pos], size - pos, "var#%d = $%d; ",
                               rand() % 100, rand() % 100);
                break;
        }
    }
    
    source[pos] = '\0';
    return source;
}

/*═══════════════════════════════════════════════════════════════
  Thread Worker Function
  ═══════════════════════════════════════════════════════════════*/

void* stress_test_worker(void* arg) {
    ThreadContext* ctx = (ThreadContext*)arg;
    ctx->success = true;
    
    printf("Thread %d: Starting scenario %d\n", ctx->thread_id, ctx->scenario);
    
    for (int iter = 0; iter < ITERATIONS_PER_THREAD; iter++) {
        // Generate test source based on scenario
        size_t source_size = 1000 + (rand() % 10000); // 1KB-10KB files
        char* source = NULL;
        
        switch (ctx->scenario) {
            case SCENARIO_NORMAL_CODE:
                source = generate_normal_code(source_size);
                break;
            case SCENARIO_KEYWORD_HEAVY:
                source = generate_keyword_heavy(source_size);
                break;
            case SCENARIO_NUMBER_HEAVY:
                source = generate_number_heavy(source_size);
                break;
            case SCENARIO_WHITESPACE_HEAVY:
                source = generate_whitespace_heavy(source_size);
                break;
            case SCENARIO_MIXED_COMPLEXITY:
                source = generate_mixed_complexity(source_size);
                break;
            case SCENARIO_ERROR_PRONE:
                source = generate_error_prone(source_size);
                break;
            default:
                source = generate_normal_code(source_size);
        }
        
        // Parse with lexer
        LeanLexer lexer;
        lean_lexer_init(&lexer, source);
        
        uint64_t start_cycles = get_cycles();
        int token_count = 0;
        
        while (lexer.current_token.type != TOK_EOF && token_count < 100000) {
            int result = lean_lexer_next_token(&lexer);
            if (result < 0) {
                ctx->errors_found++;
            }
            token_count++;
            ctx->tokens_parsed++;
        }
        
        ctx->total_cycles += get_cycles() - start_cycles;
        
        // Track quality metrics
        if (lexer.quality.defects > 0) {
            ctx->errors_found += lexer.quality.defects;
        }
        
        lean_lexer_destroy(&lexer);
        free(source);
        
        // Progress indicator
        if (iter % 1000 == 0) {
            printf("Thread %d: %d/%d iterations\n", 
                   ctx->thread_id, iter, ITERATIONS_PER_THREAD);
        }
    }
    
    // Calculate DPMO for this thread
    if (ctx->tokens_parsed > 0) {
        ctx->dpmo = ((double)ctx->errors_found / ctx->tokens_parsed) * 1000000.0;
    }
    
    printf("Thread %d: Completed. Tokens: %llu, Errors: %llu, DPMO: %.2f\n",
           ctx->thread_id, ctx->tokens_parsed, ctx->errors_found, ctx->dpmo);
    
    return NULL;
}

/*═══════════════════════════════════════════════════════════════
  Performance Analysis
  ═══════════════════════════════════════════════════════════════*/

void analyze_performance(ThreadContext* contexts, int num_threads) {
    uint64_t total_cycles = 0;
    uint64_t total_tokens = 0;
    uint64_t total_errors = 0;
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║              STRESS TEST PERFORMANCE ANALYSIS                  ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    // Per-scenario analysis
    for (int scenario = 0; scenario < SCENARIO_MAX; scenario++) {
        uint64_t scenario_cycles = 0;
        uint64_t scenario_tokens = 0;
        uint64_t scenario_errors = 0;
        int scenario_threads = 0;
        
        for (int i = 0; i < num_threads; i++) {
            if (contexts[i].scenario == scenario) {
                scenario_cycles += contexts[i].total_cycles;
                scenario_tokens += contexts[i].tokens_parsed;
                scenario_errors += contexts[i].errors_found;
                scenario_threads++;
            }
        }
        
        if (scenario_threads > 0) {
            double avg_cycles_per_token = (double)scenario_cycles / scenario_tokens;
            double scenario_dpmo = ((double)scenario_errors / scenario_tokens) * 1000000.0;
            
            const char* scenario_names[] = {
                "Normal Code", "Keyword Heavy", "Number Heavy",
                "Whitespace Heavy", "Mixed Complexity", "Error Prone"
            };
            
            printf("📊 Scenario: %s\n", scenario_names[scenario]);
            printf("   Threads: %d\n", scenario_threads);
            printf("   Total tokens: %llu\n", scenario_tokens);
            printf("   Avg cycles/token: %.2f\n", avg_cycles_per_token);
            printf("   7-tick compliant: %s\n", avg_cycles_per_token <= 7.0 ? "YES ✓" : "NO ✗");
            printf("   DPMO: %.2f\n", scenario_dpmo);
            printf("   Sigma level: %.2f\n\n", 
                   scenario_dpmo <= 3.4 ? 6.0 : (scenario_dpmo <= 233 ? 5.0 : 4.0));
            
            total_cycles += scenario_cycles;
            total_tokens += scenario_tokens;
            total_errors += scenario_errors;
        }
    }
    
    // Overall analysis
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("📈 OVERALL METRICS\n");
    printf("   Total threads: %d\n", num_threads);
    printf("   Total tokens parsed: %llu\n", total_tokens);
    printf("   Total errors found: %llu\n", total_errors);
    printf("   Average cycles/token: %.2f\n", (double)total_cycles / total_tokens);
    printf("   Overall DPMO: %.2f\n", ((double)total_errors / total_tokens) * 1000000.0);
    printf("   Tokens per second: %.2f million\n", 
           (double)total_tokens / (total_cycles / 2400000000.0) / 1000000.0); // Assuming 2.4GHz
}

/*═══════════════════════════════════════════════════════════════
  Main Stress Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char* argv[]) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║         TTL LEXER STRESS TEST & PERFORMANCE BENCHMARK          ║\n");
    printf("║            Multi-threaded • High-volume • Production Ready     ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    // Determine thread count
    int num_threads = THREAD_COUNT;
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads < 1 || num_threads > 64) {
            num_threads = THREAD_COUNT;
        }
    }
    
    printf("🚀 Configuration:\n");
    printf("   Threads: %d\n", num_threads);
    printf("   Iterations per thread: %d\n", ITERATIONS_PER_THREAD);
    printf("   Total iterations: %d\n", num_threads * ITERATIONS_PER_THREAD);
    printf("\n");
    
    // Create thread contexts
    ThreadContext* contexts = calloc(num_threads, sizeof(ThreadContext));
    pthread_t* threads = calloc(num_threads, sizeof(pthread_t));
    
    // Assign scenarios to threads
    for (int i = 0; i < num_threads; i++) {
        contexts[i].thread_id = i;
        contexts[i].scenario = i % SCENARIO_MAX;
    }
    
    // Start timing
    time_t start_time = time(NULL);
    
    // Launch threads
    printf("🔥 Launching stress test threads...\n\n");
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, stress_test_worker, &contexts[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    time_t end_time = time(NULL);
    double duration = difftime(end_time, start_time);
    
    // Analyze results
    analyze_performance(contexts, num_threads);
    
    printf("\n⏱️  Total execution time: %.2f seconds\n", duration);
    printf("✅ Stress test completed successfully!\n\n");
    
    // Cleanup
    free(contexts);
    free(threads);
    
    return 0;
}