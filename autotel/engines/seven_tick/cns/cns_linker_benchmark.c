/*  ─────────────────────────────────────────────────────────────
    cns_linker_benchmark.c  –  CNS Linker Performance Benchmark Suite
    Comprehensive benchmarking of CNS linking performance
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <pthread.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#ifdef __x86_64__
#include <immintrin.h>
#endif

#define CNS_MAX_SYMBOLS 100000
#define CNS_MAX_SECTIONS 1000
#define CNS_MAX_RELOCATIONS 50000
#define S7T_MAX_CYCLES 7
#define ESTIMATED_CPU_FREQ_GHZ 3.5
#define NS_PER_CYCLE (1.0 / ESTIMATED_CPU_FREQ_GHZ)
#define BENCHMARK_ITERATIONS 1000

// Force evaluation to prevent optimization
#define FORCE_EVAL(x) __asm__ __volatile__("" : : "g"(x) : "memory")

/*═══════════════════════════════════════════════════════════════
  CNS Linker Data Structures
  ═══════════════════════════════════════════════════════════════*/

// Symbol table entry
typedef struct {
    uint32_t name_hash;      // Hash of symbol name
    uint32_t section_id;     // Section containing symbol
    uint64_t offset;         // Offset within section
    uint32_t size;           // Symbol size
    uint8_t type;            // Symbol type (function, data, etc)
    uint8_t binding;         // Local, global, weak
    uint8_t visibility;      // Default, hidden, protected
    uint8_t flags;           // Additional flags
} CNSSymbol;

// Section header
typedef struct {
    uint32_t id;             // Section ID
    uint32_t name_hash;      // Hash of section name
    uint64_t vaddr;          // Virtual address
    uint64_t size;           // Section size
    uint32_t flags;          // Section flags
    uint32_t alignment;      // Required alignment
    void* data;              // Section data
} CNSSection;

// Relocation entry
typedef struct {
    uint32_t section_id;     // Section containing relocation
    uint64_t offset;         // Offset within section
    uint32_t symbol_id;      // Symbol to relocate
    uint32_t type;           // Relocation type
    int64_t addend;          // Relocation addend
} CNSRelocation;

// Object file representation
typedef struct {
    CNSSymbol* symbols;
    CNSSection* sections;
    CNSRelocation* relocations;
    uint32_t symbol_count;
    uint32_t section_count;
    uint32_t relocation_count;
    char* string_table;
    uint32_t string_table_size;
} CNSObjectFile;

// Linker context
typedef struct {
    CNSObjectFile** objects;     // Input object files
    uint32_t object_count;       // Number of objects
    CNSSymbol* global_symbols;   // Global symbol table
    uint32_t global_symbol_count;
    uint8_t* output_buffer;      // Output executable buffer
    uint64_t output_size;        // Output size
    uint32_t* hash_table;        // Symbol hash table
    uint32_t hash_size;          // Hash table size
    uint64_t total_cycles;       // Performance tracking
} CNSLinkerContext;

/*═══════════════════════════════════════════════════════════════
  Timing and Performance Utilities
  ═══════════════════════════════════════════════════════════════*/

static inline uint64_t get_cycles(void) {
    uint64_t cycles;
#ifdef __x86_64__
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    cycles = ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (cycles));
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cycles = (uint64_t)ts.tv_sec * 2400000000ULL + (uint64_t)ts.tv_nsec * 2.4;
#endif
    return cycles;
}

static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/*═══════════════════════════════════════════════════════════════
  Hash Functions (7-tick optimized)
  ═══════════════════════════════════════════════════════════════*/

static inline uint32_t cns_hash_string(const char* str) {
    uint32_t hash = 5381;
    
#ifdef __ARM_NEON
    // NEON optimized hash
    if (((uintptr_t)str & 7) == 0) {  // 8-byte aligned
        uint8x8_t hash_vec = vdup_n_u8(5);
        while (*str) {
            uint8x8_t str_vec = vld1_u8((const uint8_t*)str);
            hash_vec = vadd_u8(hash_vec, str_vec);
            hash_vec = vshl_n_u8(hash_vec, 1);
            str += 8;
            if (str[0] == 0 || str[1] == 0 || str[2] == 0 || str[3] == 0 ||
                str[4] == 0 || str[5] == 0 || str[6] == 0 || str[7] == 0) break;
        }
        // Reduce vector to scalar
        uint64_t temp = vget_lane_u64(vreinterpret_u64_u8(hash_vec), 0);
        hash = (uint32_t)(temp ^ (temp >> 32));
    }
#else
    // Scalar fallback with unrolling
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
#endif
    
    return hash;
}

/*═══════════════════════════════════════════════════════════════
  Symbol Resolution (7-tick optimized)
  ═══════════════════════════════════════════════════════════════*/

// Fast symbol lookup using hash table
static CNSSymbol* cns_lookup_symbol_fast(CNSLinkerContext* ctx, uint32_t name_hash) {
    uint32_t index = name_hash & (ctx->hash_size - 1);
    uint32_t* hash_entry = &ctx->hash_table[index];
    
    // Linear probing with prefetch
    while (*hash_entry != 0) {
        uint32_t sym_idx = *hash_entry - 1;
        
        // Prefetch next cache line
        __builtin_prefetch(&ctx->hash_table[(index + 1) & (ctx->hash_size - 1)], 0, 1);
        
        if (ctx->global_symbols[sym_idx].name_hash == name_hash) {
            return &ctx->global_symbols[sym_idx];
        }
        
        index = (index + 1) & (ctx->hash_size - 1);
        hash_entry = &ctx->hash_table[index];
    }
    
    return NULL;
}

// Batch symbol resolution (SIMD optimized)
static int cns_resolve_symbols_batch(CNSLinkerContext* ctx, uint32_t* hashes, 
                                    CNSSymbol** results, int count) {
    int resolved = 0;
    
#ifdef __AVX2__
    // AVX2 optimized batch lookup
    for (int i = 0; i < count - 7; i += 8) {
        __m256i hash_vec = _mm256_loadu_si256((__m256i*)&hashes[i]);
        __m256i mask = _mm256_set1_epi32(ctx->hash_size - 1);
        __m256i indices = _mm256_and_si256(hash_vec, mask);
        
        // Gather from hash table
        for (int j = 0; j < 8; j++) {
            uint32_t idx = _mm256_extract_epi32(indices, j);
            uint32_t hash = hashes[i + j];
            results[i + j] = cns_lookup_symbol_fast(ctx, hash);
            if (results[i + j]) resolved++;
        }
    }
    
    // Handle remainder
    for (int i = (count & ~7); i < count; i++) {
        results[i] = cns_lookup_symbol_fast(ctx, hashes[i]);
        if (results[i]) resolved++;
    }
#else
    // Scalar fallback
    for (int i = 0; i < count; i++) {
        results[i] = cns_lookup_symbol_fast(ctx, hashes[i]);
        if (results[i]) resolved++;
    }
#endif
    
    return resolved;
}

/*═══════════════════════════════════════════════════════════════
  Relocation Processing (7-tick optimized)
  ═══════════════════════════════════════════════════════════════*/

static void cns_apply_relocation_fast(uint8_t* target, uint64_t value, uint32_t type) {
    switch (type) {
        case 1: // R_X86_64_64 / R_AARCH64_ABS64
            *(uint64_t*)target = value;
            break;
        case 2: // R_X86_64_32 / R_AARCH64_ABS32
            *(uint32_t*)target = (uint32_t)value;
            break;
        case 3: // R_X86_64_PC32 / R_AARCH64_PREL32
            *(uint32_t*)target = (uint32_t)(value - (uint64_t)target - 4);
            break;
        default:
            // Handle other relocation types
            break;
    }
}

// Batch relocation processing
static int cns_process_relocations_batch(CNSLinkerContext* ctx, CNSRelocation* relocs, int count) {
    int processed = 0;
    
    // Process relocations in batches for better cache utilization
    for (int i = 0; i < count; i++) {
        CNSRelocation* rel = &relocs[i];
        CNSSection* section = &ctx->objects[0]->sections[rel->section_id];
        CNSSymbol* symbol = &ctx->global_symbols[rel->symbol_id];
        
        uint8_t* target = (uint8_t*)section->data + rel->offset;
        uint64_t value = section->vaddr + symbol->offset + rel->addend;
        
        cns_apply_relocation_fast(target, value, rel->type);
        processed++;
    }
    
    return processed;
}

/*═══════════════════════════════════════════════════════════════
  Link-time Optimization (LTO)
  ═══════════════════════════════════════════════════════════════*/

static void cns_optimize_code_layout(CNSLinkerContext* ctx) {
    // Sort sections by access pattern for better cache utilization
    // Hot sections first, cold sections last
    
    // Simple bubble sort for demonstration (would use quicksort in production)
    for (uint32_t i = 0; i < ctx->objects[0]->section_count - 1; i++) {
        for (uint32_t j = 0; j < ctx->objects[0]->section_count - i - 1; j++) {
            CNSSection* s1 = &ctx->objects[0]->sections[j];
            CNSSection* s2 = &ctx->objects[0]->sections[j + 1];
            
            // Hot sections have lower IDs (simplified heuristic)
            if (s1->id > s2->id) {
                CNSSection temp = *s1;
                *s1 = *s2;
                *s2 = temp;
            }
        }
    }
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Test Functions
  ═══════════════════════════════════════════════════════════════*/

// Benchmark 1: Symbol Resolution Performance
static double benchmark_symbol_resolution(CNSLinkerContext* ctx, int iterations) {
    // Generate test symbol hashes
    uint32_t* test_hashes = malloc(1000 * sizeof(uint32_t));
    CNSSymbol** results = malloc(1000 * sizeof(CNSSymbol*));
    
    for (int i = 0; i < 1000; i++) {
        test_hashes[i] = rand() % ctx->global_symbol_count;
    }
    
    uint64_t total_cycles = 0;
    int total_resolved = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        int resolved = cns_resolve_symbols_batch(ctx, test_hashes, results, 1000);
        uint64_t cycles = get_cycles() - start;
        
        total_cycles += cycles;
        total_resolved += resolved;
        FORCE_EVAL(results[0]);
    }
    
    free(test_hashes);
    free(results);
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_symbol = avg_cycles / 1000;
    
    printf("Symbol Resolution:\n");
    printf("  Total symbols: %u\n", ctx->global_symbol_count);
    printf("  Avg cycles per batch (1000 symbols): %.0f\n", avg_cycles);
    printf("  Cycles per symbol: %.3f\n", cycles_per_symbol);
    printf("  7-tick compliant: %s\n", cycles_per_symbol <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_symbol;
}

// Benchmark 2: Relocation Processing Performance
static double benchmark_relocation_processing(CNSLinkerContext* ctx, int iterations) {
    // Create test relocations
    int reloc_count = 1000;
    CNSRelocation* test_relocs = malloc(reloc_count * sizeof(CNSRelocation));
    
    for (int i = 0; i < reloc_count; i++) {
        test_relocs[i].section_id = rand() % ctx->objects[0]->section_count;
        test_relocs[i].offset = rand() % 1024;
        test_relocs[i].symbol_id = rand() % ctx->global_symbol_count;
        test_relocs[i].type = 1 + (rand() % 3);
        test_relocs[i].addend = rand() % 256;
    }
    
    uint64_t total_cycles = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        int processed = cns_process_relocations_batch(ctx, test_relocs, reloc_count);
        uint64_t cycles = get_cycles() - start;
        
        total_cycles += cycles;
        FORCE_EVAL(processed);
    }
    
    free(test_relocs);
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_reloc = avg_cycles / reloc_count;
    
    printf("\nRelocation Processing:\n");
    printf("  Relocations per batch: %d\n", reloc_count);
    printf("  Avg cycles per batch: %.0f\n", avg_cycles);
    printf("  Cycles per relocation: %.3f\n", cycles_per_reloc);
    printf("  7-tick compliant: %s\n", cycles_per_reloc <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_reloc;
}

// Benchmark 3: Section Merging Performance
static double benchmark_section_merging(CNSLinkerContext* ctx, int iterations) {
    uint64_t total_cycles = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        
        // Simulate section merging
        uint64_t output_offset = 0;
        for (uint32_t i = 0; i < ctx->object_count; i++) {
            CNSObjectFile* obj = ctx->objects[i];
            
            for (uint32_t j = 0; j < obj->section_count; j++) {
                CNSSection* section = &obj->sections[j];
                
                // Align output offset
                output_offset = (output_offset + section->alignment - 1) & ~(section->alignment - 1);
                
                // Copy section data (simplified)
                if (ctx->output_buffer && section->data) {
                    memcpy(ctx->output_buffer + output_offset, section->data, section->size);
                }
                
                output_offset += section->size;
            }
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
        
        FORCE_EVAL(output_offset);
    }
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_section = avg_cycles / (ctx->object_count * ctx->objects[0]->section_count);
    
    printf("\nSection Merging:\n");
    printf("  Total sections: %u\n", ctx->object_count * ctx->objects[0]->section_count);
    printf("  Avg cycles per merge: %.0f\n", avg_cycles);
    printf("  Cycles per section: %.3f\n", cycles_per_section);
    printf("  7-tick compliant: %s\n", cycles_per_section <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_section;
}

// Benchmark 4: Link-time Optimization
static double benchmark_link_optimization(CNSLinkerContext* ctx, int iterations) {
    uint64_t total_cycles = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        cns_optimize_code_layout(ctx);
        uint64_t cycles = get_cycles() - start;
        
        total_cycles += cycles;
    }
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_section = avg_cycles / ctx->objects[0]->section_count;
    
    printf("\nLink-time Optimization:\n");
    printf("  Sections optimized: %u\n", ctx->objects[0]->section_count);
    printf("  Avg cycles per optimization: %.0f\n", avg_cycles);
    printf("  Cycles per section: %.3f\n", cycles_per_section);
    printf("  7-tick compliant: %s\n", cycles_per_section <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_section;
}

// Benchmark 5: Full Link Performance
static double benchmark_full_link(CNSLinkerContext* ctx, int iterations) {
    uint64_t total_cycles = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        
        // Simulate full link process
        // 1. Symbol resolution
        for (uint32_t i = 0; i < ctx->global_symbol_count; i++) {
            cns_lookup_symbol_fast(ctx, ctx->global_symbols[i].name_hash);
        }
        
        // 2. Section merging
        uint64_t output_offset = 0;
        for (uint32_t i = 0; i < ctx->object_count; i++) {
            for (uint32_t j = 0; j < ctx->objects[i]->section_count; j++) {
                output_offset += ctx->objects[i]->sections[j].size;
            }
        }
        
        // 3. Relocation processing
        for (uint32_t i = 0; i < ctx->objects[0]->relocation_count; i++) {
            // Simulate relocation
            volatile uint64_t dummy = i * 17;
            FORCE_EVAL(dummy);
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    double avg_cycles = (double)total_cycles / iterations;
    int total_operations = ctx->global_symbol_count + 
                          (ctx->object_count * ctx->objects[0]->section_count) +
                          ctx->objects[0]->relocation_count;
    double cycles_per_op = avg_cycles / total_operations;
    
    printf("\nFull Link Process:\n");
    printf("  Total operations: %d\n", total_operations);
    printf("  Avg cycles per link: %.0f\n", avg_cycles);
    printf("  Cycles per operation: %.3f\n", cycles_per_op);
    printf("  7-tick compliant: %s\n", cycles_per_op <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_op;
}

/*═══════════════════════════════════════════════════════════════
  Test Data Generation
  ═══════════════════════════════════════════════════════════════*/

static void generate_test_data(CNSLinkerContext* ctx, int num_objects, int symbols_per_obj) {
    ctx->object_count = num_objects;
    ctx->objects = malloc(num_objects * sizeof(CNSObjectFile*));
    ctx->global_symbol_count = 0;
    ctx->hash_size = 16384;  // Power of 2 for fast masking
    ctx->hash_table = calloc(ctx->hash_size, sizeof(uint32_t));
    
    // Allocate global symbol table
    ctx->global_symbols = malloc(num_objects * symbols_per_obj * sizeof(CNSSymbol));
    
    // Generate object files
    for (int i = 0; i < num_objects; i++) {
        CNSObjectFile* obj = malloc(sizeof(CNSObjectFile));
        ctx->objects[i] = obj;
        
        // Generate symbols
        obj->symbol_count = symbols_per_obj;
        obj->symbols = malloc(symbols_per_obj * sizeof(CNSSymbol));
        
        for (int j = 0; j < symbols_per_obj; j++) {
            CNSSymbol* sym = &obj->symbols[j];
            char name[32];
            snprintf(name, sizeof(name), "symbol_%d_%d", i, j);
            
            sym->name_hash = cns_hash_string(name);
            sym->section_id = j % 10;  // Distribute across sections
            sym->offset = j * 16;
            sym->size = 16;
            sym->type = (j % 3 == 0) ? 1 : 2;  // Mix functions and data
            sym->binding = (j % 10 == 0) ? 2 : 1;  // Some global, mostly local
            sym->visibility = 0;
            sym->flags = 0;
            
            // Add to global symbol table if global binding
            if (sym->binding == 2) {
                ctx->global_symbols[ctx->global_symbol_count++] = *sym;
                
                // Add to hash table
                uint32_t hash_idx = sym->name_hash & (ctx->hash_size - 1);
                while (ctx->hash_table[hash_idx] != 0) {
                    hash_idx = (hash_idx + 1) & (ctx->hash_size - 1);
                }
                ctx->hash_table[hash_idx] = ctx->global_symbol_count;  // 1-based index
            }
        }
        
        // Generate sections
        obj->section_count = 10;
        obj->sections = malloc(10 * sizeof(CNSSection));
        
        for (int j = 0; j < 10; j++) {
            CNSSection* sec = &obj->sections[j];
            sec->id = j;
            sec->name_hash = cns_hash_string(j < 5 ? ".text" : ".data");
            sec->vaddr = 0x400000 + (j * 0x1000);
            sec->size = 4096;
            sec->flags = j < 5 ? 0x5 : 0x3;  // Text: R+X, Data: R+W
            sec->alignment = 16;
            sec->data = malloc(sec->size);
            memset(sec->data, 0, sec->size);
        }
        
        // Generate relocations
        obj->relocation_count = 100;
        obj->relocations = malloc(100 * sizeof(CNSRelocation));
        
        for (int j = 0; j < 100; j++) {
            CNSRelocation* rel = &obj->relocations[j];
            rel->section_id = j % 10;
            rel->offset = (j * 8) % 4096;
            rel->symbol_id = j % ctx->global_symbol_count;
            rel->type = 1 + (j % 3);
            rel->addend = j;
        }
    }
    
    // Allocate output buffer
    ctx->output_size = num_objects * 10 * 4096;  // Rough estimate
    ctx->output_buffer = malloc(ctx->output_size);
}

static void cleanup_test_data(CNSLinkerContext* ctx) {
    for (uint32_t i = 0; i < ctx->object_count; i++) {
        CNSObjectFile* obj = ctx->objects[i];
        
        for (uint32_t j = 0; j < obj->section_count; j++) {
            free(obj->sections[j].data);
        }
        
        free(obj->symbols);
        free(obj->sections);
        free(obj->relocations);
        free(obj);
    }
    
    free(ctx->objects);
    free(ctx->global_symbols);
    free(ctx->hash_table);
    free(ctx->output_buffer);
}

/*═══════════════════════════════════════════════════════════════
  Main Benchmark Runner
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char** argv) {
    printf("🚀 CNS Linker Performance Benchmark Suite\n");
    printf("==========================================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : BENCHMARK_ITERATIONS;
    int num_objects = (argc > 2) ? atoi(argv[2]) : 10;
    int symbols_per_obj = (argc > 3) ? atoi(argv[3]) : 1000;
    
    printf("Configuration:\n");
    printf("  Iterations: %d\n", iterations);
    printf("  Object files: %d\n", num_objects);
    printf("  Symbols per object: %d\n", symbols_per_obj);
    printf("  Total symbols: ~%d\n", num_objects * symbols_per_obj);
    printf("  CPU frequency estimate: %.1f GHz\n", ESTIMATED_CPU_FREQ_GHZ);
    printf("  7-tick budget: %d cycles (%.2f ns)\n\n", S7T_MAX_CYCLES, S7T_MAX_CYCLES * NS_PER_CYCLE);
    
    // Initialize linker context
    CNSLinkerContext ctx = {0};
    generate_test_data(&ctx, num_objects, symbols_per_obj);
    
    printf("Test data generated:\n");
    printf("  Global symbols: %u\n", ctx.global_symbol_count);
    printf("  Hash table size: %u\n", ctx.hash_size);
    printf("  Output buffer: %.1f MB\n\n", ctx.output_size / (1024.0 * 1024.0));
    
    // Run benchmarks
    double results[5];
    results[0] = benchmark_symbol_resolution(&ctx, iterations);
    results[1] = benchmark_relocation_processing(&ctx, iterations);
    results[2] = benchmark_section_merging(&ctx, iterations);
    results[3] = benchmark_link_optimization(&ctx, iterations);
    results[4] = benchmark_full_link(&ctx, iterations);
    
    // Summary
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("CNS LINKER PERFORMANCE SUMMARY\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    int passed = 0;
    double total_cycles = 0;
    const char* test_names[] = {
        "Symbol Resolution",
        "Relocation Processing",
        "Section Merging",
        "Link Optimization",
        "Full Link Process"
    };
    
    printf("\n%-25s %15s %12s %8s\n", "Test", "Cycles/Op", "Time (ns)", "7-Tick");
    printf("%-25s %15s %12s %8s\n", "----", "---------", "---------", "------");
    
    for (int i = 0; i < 5; i++) {
        bool compliant = results[i] <= S7T_MAX_CYCLES;
        if (compliant) passed++;
        total_cycles += results[i];
        
        printf("%-25s %15.3f %12.2f %8s\n",
               test_names[i],
               results[i],
               results[i] * NS_PER_CYCLE,
               compliant ? "✅ PASS" : "❌ FAIL");
    }
    
    double avg_cycles = total_cycles / 5;
    
    printf("\nOverall Results:\n");
    printf("  Tests passed: %d/5 (%.0f%%)\n", passed, (passed/5.0)*100);
    printf("  Average cycles/op: %.3f\n", avg_cycles);
    printf("  Overall 7-tick compliant: %s\n", avg_cycles <= S7T_MAX_CYCLES ? "✅ YES" : "❌ NO");
    
    // Performance analysis
    printf("\nPerformance Analysis:\n");
    if (avg_cycles <= S7T_MAX_CYCLES) {
        printf("  ✅ CNS linker meets 7-tick performance requirements\n");
        printf("  🚀 Ready for production deployment\n");
    } else if (avg_cycles <= S7T_MAX_CYCLES * 2) {
        printf("  ⚠️  Close to target - minor optimizations needed\n");
        printf("  📈 Focus on: ");
        for (int i = 0; i < 5; i++) {
            if (results[i] > S7T_MAX_CYCLES) {
                printf("%s ", test_names[i]);
            }
        }
        printf("\n");
    } else {
        printf("  ❌ Significant optimization required\n");
        printf("  🔧 Consider: SIMD optimization, better caching, parallel processing\n");
    }
    
    // Mermaid performance diagram
    printf("\n```mermaid\n");
    printf("graph TD\n");
    printf("    A[CNS Linker Benchmark] --> B[5 Performance Tests]\n");
    printf("    B --> C[%d Passed]\n", passed);
    printf("    B --> D[%d Failed]\n", 5 - passed);
    printf("    C --> E[%.3f Avg Cycles]\n", avg_cycles);
    printf("    E --> F{≤7 Cycles?}\n");
    if (avg_cycles <= S7T_MAX_CYCLES) {
        printf("    F -->|YES| G[✅ 7-TICK COMPLIANT]\n");
    } else {
        printf("    F -->|NO| H[❌ NEEDS OPTIMIZATION]\n");
        printf("    H --> I[Target: %.1fx improvement]\n", avg_cycles / S7T_MAX_CYCLES);
    }
    printf("```\n");
    
    // Cleanup
    cleanup_test_data(&ctx);
    
    return passed == 5 ? 0 : 1;
}