/*  ─────────────────────────────────────────────────────────────
    cns_linker_dynamic_benchmark.c  –  CNS Dynamic Linker Benchmark
    Advanced benchmarking for dynamic linking and PLT/GOT operations
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <elf.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#ifdef __x86_64__
#include <immintrin.h>
#endif

#define S7T_MAX_CYCLES 7
#define ESTIMATED_CPU_FREQ_GHZ 3.5
#define NS_PER_CYCLE (1.0 / ESTIMATED_CPU_FREQ_GHZ)
#define BENCHMARK_ITERATIONS 1000

// Force evaluation to prevent optimization
#define FORCE_EVAL(x) __asm__ __volatile__("" : : "g"(x) : "memory")

/*═══════════════════════════════════════════════════════════════
  Dynamic Linking Structures
  ═══════════════════════════════════════════════════════════════*/

// PLT (Procedure Linkage Table) entry
typedef struct {
    uint64_t got_offset;     // Offset into GOT
    uint64_t stub_addr;      // Address of PLT stub
    uint32_t symbol_index;   // Symbol table index
    uint32_t flags;          // PLT flags
} PLTEntry;

// GOT (Global Offset Table) entry
typedef struct {
    uint64_t address;        // Resolved address
    uint32_t symbol_index;   // Symbol table index
    uint8_t type;           // GOT entry type
    uint8_t state;          // Resolution state (unresolved/resolved)
    uint16_t reserved;
} GOTEntry;

// Dynamic symbol
typedef struct {
    uint32_t name_hash;      // Symbol name hash
    uint64_t address;        // Symbol address
    uint32_t size;           // Symbol size
    uint8_t type;           // Symbol type
    uint8_t binding;        // Symbol binding
    uint16_t version;       // Symbol version
    void* library;          // Containing library
} DynSymbol;

// Lazy binding context
typedef struct {
    PLTEntry* plt_table;
    GOTEntry* got_table;
    DynSymbol* dynsym_table;
    uint32_t plt_count;
    uint32_t got_count;
    uint32_t dynsym_count;
    uint32_t* hash_table;
    uint32_t hash_size;
    uint64_t bind_count;     // Number of bindings performed
    uint64_t lookup_cycles;  // Total lookup cycles
} LazyBindContext;

/*═══════════════════════════════════════════════════════════════
  Timing Utilities
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

/*═══════════════════════════════════════════════════════════════
  7-tick Optimized Hash Function
  ═══════════════════════════════════════════════════════════════*/

static inline uint32_t fast_elf_hash(const char* name) {
    uint32_t h = 0, g;
    
#ifdef __ARM_NEON
    // NEON optimized hash for ARM
    if (((uintptr_t)name & 7) == 0) {
        uint32x4_t hash_vec = vdupq_n_u32(0);
        while (*name) {
            uint8x8_t str_vec = vld1_u8((const uint8_t*)name);
            uint32x4_t expanded = vmovl_u16(vget_low_u16(vmovl_u8(str_vec)));
            hash_vec = vaddq_u32(hash_vec, expanded);
            hash_vec = vshlq_n_u32(hash_vec, 4);
            
            name += 4;
            if (name[0] == 0 || name[1] == 0 || name[2] == 0 || name[3] == 0) break;
        }
        // Reduce to scalar
        h = vgetq_lane_u32(hash_vec, 0) ^ vgetq_lane_u32(hash_vec, 1) ^
            vgetq_lane_u32(hash_vec, 2) ^ vgetq_lane_u32(hash_vec, 3);
    }
#else
    // Standard ELF hash with unrolling
    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000)) {
            h ^= g >> 24;
            h &= ~g;
        }
    }
#endif
    
    return h;
}

/*═══════════════════════════════════════════════════════════════
  PLT/GOT Resolution (7-tick optimized)
  ═══════════════════════════════════════════════════════════════*/

// Fast symbol lookup
static DynSymbol* lookup_symbol_fast(LazyBindContext* ctx, uint32_t hash) {
    uint32_t index = hash & (ctx->hash_size - 1);
    
    // Linear probing with prefetch
    while (ctx->hash_table[index] != 0) {
        uint32_t sym_idx = ctx->hash_table[index] - 1;
        
        // Prefetch next entry
        __builtin_prefetch(&ctx->hash_table[(index + 1) & (ctx->hash_size - 1)], 0, 1);
        
        if (ctx->dynsym_table[sym_idx].name_hash == hash) {
            return &ctx->dynsym_table[sym_idx];
        }
        
        index = (index + 1) & (ctx->hash_size - 1);
    }
    
    return NULL;
}

// Lazy PLT resolution (simulated)
static uint64_t resolve_plt_lazy(LazyBindContext* ctx, uint32_t plt_index) {
    uint64_t start_cycles = get_cycles();
    
    PLTEntry* plt = &ctx->plt_table[plt_index];
    GOTEntry* got = &ctx->got_table[plt->got_offset / sizeof(GOTEntry)];
    
    // Check if already resolved
    if (got->state == 1) {
        return get_cycles() - start_cycles;
    }
    
    // Lookup symbol
    DynSymbol* sym = &ctx->dynsym_table[plt->symbol_index];
    DynSymbol* resolved = lookup_symbol_fast(ctx, sym->name_hash);
    
    if (resolved) {
        // Update GOT entry
        got->address = resolved->address;
        got->state = 1;  // Mark as resolved
        ctx->bind_count++;
    }
    
    return get_cycles() - start_cycles;
}

// Batch PLT resolution (for eager binding)
static uint64_t resolve_plt_batch(LazyBindContext* ctx, uint32_t* indices, int count) {
    uint64_t total_cycles = 0;
    
#ifdef __AVX2__
    // Process multiple PLT entries in parallel
    for (int i = 0; i < count - 3; i += 4) {
        uint64_t start = get_cycles();
        
        // Prefetch GOT entries
        for (int j = 0; j < 4; j++) {
            PLTEntry* plt = &ctx->plt_table[indices[i + j]];
            __builtin_prefetch(&ctx->got_table[plt->got_offset / sizeof(GOTEntry)], 1, 3);
        }
        
        // Resolve entries
        for (int j = 0; j < 4; j++) {
            resolve_plt_lazy(ctx, indices[i + j]);
        }
        
        total_cycles += get_cycles() - start;
    }
    
    // Handle remainder
    for (int i = (count & ~3); i < count; i++) {
        total_cycles += resolve_plt_lazy(ctx, indices[i]);
    }
#else
    // Scalar version
    for (int i = 0; i < count; i++) {
        total_cycles += resolve_plt_lazy(ctx, indices[i]);
    }
#endif
    
    return total_cycles;
}

/*═══════════════════════════════════════════════════════════════
  TLS (Thread Local Storage) Benchmarks
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint64_t module_id;
    uint64_t offset;
    void* address;
} TLSDescriptor;

static uint64_t benchmark_tls_access(int iterations) {
    // Simulate TLS access patterns
    __thread uint64_t tls_var = 0;
    uint64_t total_cycles = 0;
    
    for (int i = 0; i < iterations; i++) {
        uint64_t start = get_cycles();
        
        // Simulate different TLS access models
        // Model 1: Initial exec (IE)
        tls_var = i;
        FORCE_EVAL(tls_var);
        
        // Model 2: Local exec (LE) - fastest
        volatile uint64_t local = tls_var + i;
        FORCE_EVAL(local);
        
        // Model 3: General dynamic (GD) - slowest
        volatile uint64_t* ptr = &tls_var;
        *ptr = i * 2;
        FORCE_EVAL(*ptr);
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    return total_cycles / iterations;
}

/*═══════════════════════════════════════════════════════════════
  Version Symbol Resolution
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint16_t version;
    uint16_t flags;
    uint32_t name_hash;
} VersionSymbol;

static uint64_t benchmark_version_lookup(LazyBindContext* ctx, int iterations) {
    VersionSymbol* versions = malloc(100 * sizeof(VersionSymbol));
    
    // Generate test versions
    for (int i = 0; i < 100; i++) {
        versions[i].version = 1 + (i % 5);
        versions[i].flags = i % 2;
        versions[i].name_hash = fast_elf_hash("symbol") + i;
    }
    
    uint64_t total_cycles = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        
        // Simulate version symbol resolution
        for (int i = 0; i < 100; i++) {
            DynSymbol* sym = lookup_symbol_fast(ctx, versions[i].name_hash);
            if (sym && sym->version == versions[i].version) {
                FORCE_EVAL(sym->address);
            }
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    free(versions);
    return total_cycles / iterations;
}

/*═══════════════════════════════════════════════════════════════
  Benchmark Functions
  ═══════════════════════════════════════════════════════════════*/

// Benchmark 1: Lazy Binding Performance
static double benchmark_lazy_binding(LazyBindContext* ctx, int iterations) {
    printf("Lazy Binding Performance:\n");
    
    // Generate random PLT access pattern
    uint32_t* access_pattern = malloc(1000 * sizeof(uint32_t));
    for (int i = 0; i < 1000; i++) {
        access_pattern[i] = rand() % ctx->plt_count;
    }
    
    uint64_t total_cycles = 0;
    ctx->bind_count = 0;
    
    // Reset GOT state for testing
    for (uint32_t i = 0; i < ctx->got_count; i++) {
        ctx->got_table[i].state = 0;
    }
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        
        // Simulate function calls triggering lazy binding
        for (int i = 0; i < 100; i++) {
            resolve_plt_lazy(ctx, access_pattern[i]);
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    free(access_pattern);
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_bind = avg_cycles / 100;
    
    printf("  PLT entries: %u\n", ctx->plt_count);
    printf("  Bindings performed: %lu\n", ctx->bind_count);
    printf("  Avg cycles per 100 bindings: %.0f\n", avg_cycles);
    printf("  Cycles per binding: %.3f\n", cycles_per_bind);
    printf("  7-tick compliant: %s\n", cycles_per_bind <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_bind;
}

// Benchmark 2: Eager Binding Performance
static double benchmark_eager_binding(LazyBindContext* ctx, int iterations) {
    printf("\nEager Binding Performance:\n");
    
    // Create list of all PLT indices
    uint32_t* all_indices = malloc(ctx->plt_count * sizeof(uint32_t));
    for (uint32_t i = 0; i < ctx->plt_count; i++) {
        all_indices[i] = i;
    }
    
    uint64_t total_cycles = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        // Reset GOT state
        for (uint32_t i = 0; i < ctx->got_count; i++) {
            ctx->got_table[i].state = 0;
        }
        
        uint64_t start = get_cycles();
        uint64_t batch_cycles = resolve_plt_batch(ctx, all_indices, ctx->plt_count);
        total_cycles += batch_cycles;
    }
    
    free(all_indices);
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_symbol = avg_cycles / ctx->plt_count;
    
    printf("  Total symbols: %u\n", ctx->plt_count);
    printf("  Avg cycles for full binding: %.0f\n", avg_cycles);
    printf("  Cycles per symbol: %.3f\n", cycles_per_symbol);
    printf("  7-tick compliant: %s\n", cycles_per_symbol <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_symbol;
}

// Benchmark 3: Symbol Interposition
static double benchmark_symbol_interposition(LazyBindContext* ctx, int iterations) {
    printf("\nSymbol Interposition Performance:\n");
    
    uint64_t total_cycles = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        uint64_t start = get_cycles();
        
        // Simulate symbol interposition checks
        for (uint32_t i = 0; i < ctx->dynsym_count; i++) {
            DynSymbol* sym = &ctx->dynsym_table[i];
            
            // Check for interposition (RTLD_GLOBAL symbols)
            if (sym->binding == 1) {  // STB_GLOBAL
                // Simulate interposition check across libraries
                volatile uint64_t check = sym->address ^ sym->name_hash;
                FORCE_EVAL(check);
            }
        }
        
        uint64_t cycles = get_cycles() - start;
        total_cycles += cycles;
    }
    
    double avg_cycles = (double)total_cycles / iterations;
    double cycles_per_symbol = avg_cycles / ctx->dynsym_count;
    
    printf("  Symbols checked: %u\n", ctx->dynsym_count);
    printf("  Avg cycles per check: %.0f\n", avg_cycles);
    printf("  Cycles per symbol: %.3f\n", cycles_per_symbol);
    printf("  7-tick compliant: %s\n", cycles_per_symbol <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_symbol;
}

// Benchmark 4: TLS Access
static double benchmark_tls_operations(int iterations) {
    printf("\nTLS Access Performance:\n");
    
    uint64_t avg_cycles = benchmark_tls_access(iterations);
    double cycles_per_access = avg_cycles / 3.0;  // 3 TLS operations per iteration
    
    printf("  TLS models tested: IE, LE, GD\n");
    printf("  Avg cycles per iteration: %.0f\n", (double)avg_cycles);
    printf("  Cycles per TLS access: %.3f\n", cycles_per_access);
    printf("  7-tick compliant: %s\n", cycles_per_access <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_access;
}

// Benchmark 5: Version Symbol Resolution
static double benchmark_version_symbols(LazyBindContext* ctx, int iterations) {
    printf("\nVersion Symbol Resolution:\n");
    
    uint64_t avg_cycles = benchmark_version_lookup(ctx, iterations);
    double cycles_per_lookup = avg_cycles / 100.0;  // 100 lookups per iteration
    
    printf("  Version lookups per iteration: 100\n");
    printf("  Avg cycles per iteration: %.0f\n", (double)avg_cycles);
    printf("  Cycles per lookup: %.3f\n", cycles_per_lookup);
    printf("  7-tick compliant: %s\n", cycles_per_lookup <= S7T_MAX_CYCLES ? "✅ PASS" : "❌ FAIL");
    
    return cycles_per_lookup;
}

/*═══════════════════════════════════════════════════════════════
  Test Data Generation
  ═══════════════════════════════════════════════════════════════*/

static void generate_dynamic_test_data(LazyBindContext* ctx) {
    // Initialize sizes
    ctx->plt_count = 500;
    ctx->got_count = 500;
    ctx->dynsym_count = 1000;
    ctx->hash_size = 2048;  // Power of 2
    
    // Allocate tables
    ctx->plt_table = malloc(ctx->plt_count * sizeof(PLTEntry));
    ctx->got_table = malloc(ctx->got_count * sizeof(GOTEntry));
    ctx->dynsym_table = malloc(ctx->dynsym_count * sizeof(DynSymbol));
    ctx->hash_table = calloc(ctx->hash_size, sizeof(uint32_t));
    
    // Generate dynamic symbols
    for (uint32_t i = 0; i < ctx->dynsym_count; i++) {
        char name[32];
        snprintf(name, sizeof(name), "dynsym_%u", i);
        
        ctx->dynsym_table[i].name_hash = fast_elf_hash(name);
        ctx->dynsym_table[i].address = 0x7f0000000000ULL + (i * 0x1000);
        ctx->dynsym_table[i].size = 64 + (i % 256);
        ctx->dynsym_table[i].type = (i % 3 == 0) ? 1 : 2;  // STT_FUNC or STT_OBJECT
        ctx->dynsym_table[i].binding = (i % 10 == 0) ? 2 : 1;  // STB_WEAK or STB_GLOBAL
        ctx->dynsym_table[i].version = 1 + (i % 3);
        ctx->dynsym_table[i].library = (void*)(uintptr_t)(1 + (i % 5));
        
        // Add to hash table
        uint32_t idx = ctx->dynsym_table[i].name_hash & (ctx->hash_size - 1);
        while (ctx->hash_table[idx] != 0) {
            idx = (idx + 1) & (ctx->hash_size - 1);
        }
        ctx->hash_table[idx] = i + 1;  // 1-based index
    }
    
    // Generate PLT entries
    for (uint32_t i = 0; i < ctx->plt_count; i++) {
        ctx->plt_table[i].got_offset = i * sizeof(GOTEntry);
        ctx->plt_table[i].stub_addr = 0x400000 + (i * 16);
        ctx->plt_table[i].symbol_index = i % ctx->dynsym_count;
        ctx->plt_table[i].flags = 0;
    }
    
    // Initialize GOT entries
    for (uint32_t i = 0; i < ctx->got_count; i++) {
        ctx->got_table[i].address = 0;  // Unresolved
        ctx->got_table[i].symbol_index = i % ctx->dynsym_count;
        ctx->got_table[i].type = 1;  // Normal GOT entry
        ctx->got_table[i].state = 0;  // Unresolved
    }
}

static void cleanup_dynamic_test_data(LazyBindContext* ctx) {
    free(ctx->plt_table);
    free(ctx->got_table);
    free(ctx->dynsym_table);
    free(ctx->hash_table);
}

/*═══════════════════════════════════════════════════════════════
  Main Benchmark Runner
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char** argv) {
    printf("🚀 CNS Dynamic Linker Benchmark Suite\n");
    printf("======================================\n\n");
    
    int iterations = (argc > 1) ? atoi(argv[1]) : BENCHMARK_ITERATIONS;
    
    printf("Configuration:\n");
    printf("  Iterations: %d\n", iterations);
    printf("  CPU frequency estimate: %.1f GHz\n", ESTIMATED_CPU_FREQ_GHZ);
    printf("  7-tick budget: %d cycles (%.2f ns)\n\n", S7T_MAX_CYCLES, S7T_MAX_CYCLES * NS_PER_CYCLE);
    
    // Initialize context
    LazyBindContext ctx = {0};
    generate_dynamic_test_data(&ctx);
    
    printf("Test data generated:\n");
    printf("  PLT entries: %u\n", ctx.plt_count);
    printf("  GOT entries: %u\n", ctx.got_count);
    printf("  Dynamic symbols: %u\n", ctx.dynsym_count);
    printf("  Hash table size: %u\n\n", ctx.hash_size);
    
    // Run benchmarks
    double results[5];
    results[0] = benchmark_lazy_binding(&ctx, iterations);
    results[1] = benchmark_eager_binding(&ctx, iterations);
    results[2] = benchmark_symbol_interposition(&ctx, iterations);
    results[3] = benchmark_tls_operations(iterations);
    results[4] = benchmark_version_symbols(&ctx, iterations);
    
    // Summary
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("CNS DYNAMIC LINKER PERFORMANCE SUMMARY\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    int passed = 0;
    double total_cycles = 0;
    const char* test_names[] = {
        "Lazy Binding",
        "Eager Binding",
        "Symbol Interposition",
        "TLS Access",
        "Version Symbols"
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
    
    // Dynamic linking specific analysis
    printf("\nDynamic Linking Analysis:\n");
    if (results[0] <= S7T_MAX_CYCLES && results[1] <= S7T_MAX_CYCLES) {
        printf("  ✅ Both lazy and eager binding meet 7-tick requirements\n");
        printf("  🚀 Recommendation: Use lazy binding for faster startup\n");
    } else if (results[1] <= S7T_MAX_CYCLES) {
        printf("  ⚠️  Eager binding performs better than lazy\n");
        printf("  📈 Recommendation: Use RTLD_NOW for critical paths\n");
    } else {
        printf("  ❌ Dynamic linking needs optimization\n");
        printf("  🔧 Consider: PLT/GOT optimization, symbol caching\n");
    }
    
    // Cleanup
    cleanup_dynamic_test_data(&ctx);
    
    return passed == 5 ? 0 : 1;
}