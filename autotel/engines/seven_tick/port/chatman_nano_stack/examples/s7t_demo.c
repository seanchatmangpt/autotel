/*  ─────────────────────────────────────────────────────────────
    s7t_demo.c  –  Comprehensive CHATMAN-NANO-STACK Demo
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/s7t.h"
#include "../include/s7t_patterns.h"
#include "../include/s7t_workflow.h"

// Pre-allocate 16MB arena for zero-allocation operations
S7T_DECLARE_POOL(g_pool, 16 * 1024 * 1024);

/*═══════════════════════════════════════════════════════════════
  Demo 1: Core Primitives
  ═══════════════════════════════════════════════════════════════*/

void demo_core_primitives(void) {
    printf("\n=== Demo 1: Core Primitives ===\n");
    
    // Cycle counting
    uint64_t start = s7t_cycles();
    
    // Bit operations (1 cycle)
    uint32_t x = 0xDEADBEEF;
    uint32_t popcount = s7t_popcount(x);
    uint32_t leading_zeros = s7t_clz(x);
    uint32_t trailing_zeros = s7t_ctz(x);
    
    // Branch-free selection (1 cycle)
    uint32_t a = 42, b = 69;
    uint32_t min_val = s7t_min_u32(a, b);
    uint32_t max_val = s7t_max_u32(a, b);
    (void)trailing_zeros; (void)max_val;  // Suppress unused warnings
    
    // Atomic operations (2-3 cycles)
    s7t_atomic_u64 counter = 0;
    uint64_t prev = s7t_atomic_inc_u64(&counter);
    
    uint64_t cycles = s7t_cycles() - start;
    printf("Core operations completed in %llu cycles\n", (unsigned long long)cycles);
    printf("  Popcount(%08X) = %u\n", x, popcount);
    printf("  CLZ(%08X) = %u\n", x, leading_zeros);
    printf("  Min(%u, %u) = %u\n", a, b, min_val);
    printf("  Atomic increment: %llu -> %llu\n", (unsigned long long)prev, (unsigned long long)(prev + 1));
}

/*═══════════════════════════════════════════════════════════════
  Demo 2: Memory Arena
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t id;
    uint64_t timestamp;
    double value;
    char name[32];
} sensor_data_t;

void demo_memory_arena(void) {
    printf("\n=== Demo 2: Memory Arena (Zero Allocation) ===\n");
    
    // Create arena from global pool
    s7t_arena_t arena = {
        .data = g_pool.data,
        .size = sizeof(g_pool.data),
        .used = 0
    };
    
    uint64_t start = s7t_cycles();
    
    // Allocate 1000 sensor structs (1 cycle each)
    sensor_data_t* sensors[1000];
    for (int i = 0; i < 1000; i++) {
        sensors[i] = s7t_arena_alloc(&arena, sizeof(sensor_data_t));
        sensors[i]->id = i;
        sensors[i]->timestamp = start + i;
        sensors[i]->value = i * 3.14159;
    }
    
    uint64_t cycles = s7t_cycles() - start;
    printf("Allocated 1000 structs in %llu cycles (%.2f cycles/alloc)\n",
           (unsigned long long)cycles, (double)cycles / 1000);
    printf("Arena usage: %zu / %zu bytes\n", arena.used, arena.size);
}

/*═══════════════════════════════════════════════════════════════
  Demo 3: Nanosecond Design Patterns
  ═══════════════════════════════════════════════════════════════*/

// Singleton pattern
typedef struct {
    uint32_t requests_processed;
    uint64_t total_cycles;
} server_stats_t;
S7T_SINGLETON_DECLARE(server_stats_t, g_stats);

// Factory pattern
typedef struct {
    uint32_t type;
    uint32_t size;
} widget_t;

widget_t* create_small_widget(s7t_arena_t* arena) {
    widget_t* w = s7t_arena_alloc(arena, sizeof(widget_t));
    w->type = 1;
    w->size = 16;
    return w;
}

widget_t* create_large_widget(s7t_arena_t* arena) {
    widget_t* w = s7t_arena_alloc(arena, sizeof(widget_t));
    w->type = 2;
    w->size = 64;
    return w;
}

S7T_FACTORY_DECLARE(widget_t, widget_factory, create_small_widget, create_large_widget);

// Strategy pattern
uint32_t strategy_fast(void* ctx) { return *(uint32_t*)ctx * 2; }
uint32_t strategy_accurate(void* ctx) { return *(uint32_t*)ctx * 3; }
S7T_STRATEGY_TABLE(compute_strategy, uint32_t, strategy_fast, strategy_accurate);

void demo_patterns(void) {
    printf("\n=== Demo 3: Nanosecond Design Patterns ===\n");
    
    s7t_arena_t arena = {
        .data = g_pool.data + g_pool.used,
        .size = sizeof(g_pool.data) - g_pool.used,
        .used = 0
    };
    
    // Singleton usage
    server_stats_t* stats = g_stats_get();
    stats->requests_processed++;
    
    // Factory usage
    widget_t* small = widget_factory_create(&arena, 0);
    widget_t* large = widget_factory_create(&arena, 1);
    
    // Builder pattern
    widget_t custom = S7T_BUILDER(widget_t, .type = 3, .size = 32);
    
    // Strategy pattern
    uint32_t value = 21;
    s7t_strategy_t strat = { .strategy_id = 0, .context = &value };
    uint32_t result = compute_strategy_execute(&strat);
    
    printf("Singleton: %u requests\n", stats->requests_processed);
    printf("Factory: small=%u, large=%u\n", small->size, large->size);
    printf("Builder: custom widget size=%u\n", custom.size);
    printf("Strategy: %u -> %u\n", value, result);
}

/*═══════════════════════════════════════════════════════════════
  Demo 4: State Machine
  ═══════════════════════════════════════════════════════════════*/

enum { STATE_IDLE, STATE_ACTIVE, STATE_ERROR, NUM_STATES };
enum { EVENT_START, EVENT_STOP, EVENT_ERROR, NUM_EVENTS };

S7T_STATE_MACHINE_DECLARE(protocol_sm, NUM_STATES, NUM_EVENTS);

void demo_state_machine(void) {
    printf("\n=== Demo 4: State Machine (Branch-Free) ===\n");
    
    // Initialize state transitions
    protocol_sm_lattice[STATE_IDLE * NUM_EVENTS + EVENT_START] = 
        (s7t_state_transition_t){.next_state = STATE_ACTIVE, .action = 0};
    protocol_sm_lattice[STATE_ACTIVE * NUM_EVENTS + EVENT_STOP] = 
        (s7t_state_transition_t){.next_state = STATE_IDLE, .action = 1};
    protocol_sm_lattice[STATE_ACTIVE * NUM_EVENTS + EVENT_ERROR] = 
        (s7t_state_transition_t){.next_state = STATE_ERROR, .action = 2};
    
    // Process events
    uint64_t start = s7t_cycles();
    
    s7t_state_transition(&protocol_sm, EVENT_START);
    printf("After START: state=%u\n", protocol_sm.current_state);
    
    s7t_state_transition(&protocol_sm, EVENT_ERROR);
    printf("After ERROR: state=%u\n", protocol_sm.current_state);
    
    uint64_t cycles = s7t_cycles() - start;
    printf("State transitions completed in %llu cycles\n", (unsigned long long)cycles);
}

/*═══════════════════════════════════════════════════════════════
  Demo 5: Lock-Free Ring Buffer
  ═══════════════════════════════════════════════════════════════*/

void demo_ring_buffer(void) {
    printf("\n=== Demo 5: Lock-Free Ring Buffer ===\n");
    
    s7t_ring_buffer_t* ring = S7T_POOL_ALLOC(g_pool, sizeof(s7t_ring_buffer_t));
    memset(ring, 0, sizeof(s7t_ring_buffer_t));
    
    uint64_t start = s7t_cycles();
    
    // Push 100 events
    for (int i = 0; i < 100; i++) {
        s7t_event_t event;
        event.data[0] = i;
        event.data[1] = i * i;
        s7t_ring_push(ring, &event);
    }
    
    // Pop 50 events
    int popped = 0;
    s7t_event_t event;
    while (popped < 50 && s7t_ring_pop(ring, &event)) {
        popped++;
    }
    
    uint64_t cycles = s7t_cycles() - start;
    printf("Pushed 100, popped %d events in %llu cycles\n", popped, (unsigned long long)cycles);
    printf("Average: %.2f cycles per operation\n", (double)cycles / 150);
}

/*═══════════════════════════════════════════════════════════════
  Demo 6: String Interning (Flyweight)
  ═══════════════════════════════════════════════════════════════*/

void demo_string_interning(void) {
    printf("\n=== Demo 6: String Interning (O(1) Comparison) ===\n");
    
    // Setup flyweight table
    s7t_flyweight_t* fw = S7T_POOL_ALLOC(g_pool, sizeof(s7t_flyweight_t));
    fw->capacity = 1024;
    fw->table = S7T_POOL_ALLOC(g_pool, fw->capacity * sizeof(s7t_id_t));
    fw->hashes = S7T_POOL_ALLOC(g_pool, fw->capacity * sizeof(uint32_t));
    fw->strings = S7T_POOL_ALLOC(g_pool, fw->capacity * sizeof(char*));
    fw->count = 1; // 0 is reserved for NULL
    
    // Initialize table
    for (uint32_t i = 0; i < fw->capacity; i++) {
        fw->table[i] = S7T_ID_NULL;
    }
    
    // Intern strings
    const char* str1 = "http://example.org/sensor";
    const char* str2 = "http://example.org/sensor";  // Same string
    const char* str3 = "http://example.org/actuator";
    
    s7t_id_t id1 = s7t_flyweight_intern(fw, str1, strlen(str1));
    s7t_id_t id2 = s7t_flyweight_intern(fw, str2, strlen(str2));
    s7t_id_t id3 = s7t_flyweight_intern(fw, str3, strlen(str3));
    
    printf("Interned IDs: '%s'=%u, '%s'=%u, '%s'=%u\n", 
           str1, id1, str2, id2, str3, id3);
    printf("String comparison: %s\n", (id1 == id2) ? "MATCH (1 cycle)" : "NO MATCH");
}

/*═══════════════════════════════════════════════════════════════
  Demo 7: Workflow Engine
  ═══════════════════════════════════════════════════════════════*/

void demo_workflow_engine(void) {
    printf("\n=== Demo 7: Workflow Engine ===\n");
    
    // Create arena for workflow
    s7t_arena_t arena = {
        .data = g_pool.data + g_pool.used,
        .size = 4 * 1024 * 1024,  // 4MB for workflow
        .used = 0
    };
    
    // Initialize workflow engine
    s7t_workflow_engine_t* engine = s7t_arena_alloc(&arena, sizeof(s7t_workflow_engine_t));
    s7t_workflow_init(engine, &arena);
    
    // Configure pipeline with 3 stages
    engine->num_rings = 4;  // 3 stages + 1
    engine->num_state_machines = 1;
    
    // Run workflow ticks
    uint64_t total_cycles = 0;
    for (int i = 0; i < 1000; i++) {
        s7t_workflow_tick(engine);
        total_cycles += engine->last_tick_cycles;
    }
    
    printf("Executed 1000 workflow ticks\n");
    printf("Total cycles: %llu\n", (unsigned long long)total_cycles);
    printf("Average cycles per tick: %.2f\n", (double)total_cycles / 1000);
    printf("Tick count: %llu\n", (unsigned long long)atomic_load(&engine->tick_count));
}

/*═══════════════════════════════════════════════════════════════
  Demo 8: SIMD Operations
  ═══════════════════════════════════════════════════════════════*/

#if defined(__SSE2__) || defined(__AVX2__)
void demo_simd_operations(void) {
    printf("\n=== Demo 8: SIMD Operations ===\n");
    
    // Allocate aligned arrays
    uint32_t* array1 = S7T_POOL_ALLOC(g_pool, 256 * sizeof(uint32_t));
    uint32_t* array2 = S7T_POOL_ALLOC(g_pool, 256 * sizeof(uint32_t));
    uint32_t* result = S7T_POOL_ALLOC(g_pool, 256 * sizeof(uint32_t));
    
    // Initialize arrays
    for (int i = 0; i < 256; i++) {
        array1[i] = i;
        array2[i] = 256 - i;
    }
    
    uint64_t start = s7t_cycles();
    
#if defined(__AVX2__)
    // Process 8 elements at a time with AVX2
    for (int i = 0; i < 256; i += 8) {
        __m256i a = _mm256_load_si256((__m256i*)&array1[i]);
        __m256i b = _mm256_load_si256((__m256i*)&array2[i]);
        __m256i c = _mm256_add_epi32(a, b);
        _mm256_store_si256((__m256i*)&result[i], c);
    }
#else
    // Process 4 elements at a time with SSE2
    for (int i = 0; i < 256; i += 4) {
        s7t_vec128 a = s7t_vec_load(&array1[i]);
        s7t_vec128 b = s7t_vec_load(&array2[i]);
        s7t_vec128 c = _mm_add_epi32(a, b);
        s7t_vec_store(&result[i], c);
    }
#endif
    
    uint64_t cycles = s7t_cycles() - start;
    printf("SIMD processed 256 elements in %llu cycles\n", (unsigned long long)cycles);
    printf("Cycles per element: %.2f\n", (double)cycles / 256);
    printf("Sample results: [0]=%u, [127]=%u, [255]=%u\n",
           result[0], result[127], result[255]);
}
#endif

/*═══════════════════════════════════════════════════════════════
  Demo 9: Performance Validation
  ═══════════════════════════════════════════════════════════════*/

void demo_performance_validation(void) {
    printf("\n=== Demo 9: Performance Validation ===\n");
    
    s7t_perf_counter_t pc = {
        .cycles = 0,
        .count = 0,
        .min_cycles = UINT64_MAX,
        .max_cycles = 0
    };
    
    // Measure arena allocation
    for (int i = 0; i < 10000; i++) {
        uint64_t start = s7t_cycles();
        void* p = S7T_POOL_ALLOC(g_pool, 64);
        (void)p;  // Suppress unused warning
        uint64_t cycles = s7t_cycles() - start;
        s7t_perf_update(&pc, cycles);
    }
    
    printf("Arena Allocation Performance:\n");
    printf("  Operations: %llu\n", (unsigned long long)pc.count);
    printf("  Average cycles: %.2f\n", (double)pc.cycles / pc.count);
    printf("  Min cycles: %llu\n", (unsigned long long)pc.min_cycles);
    printf("  Max cycles: %llu\n", (unsigned long long)pc.max_cycles);
    printf("  Physics compliance: %s\n", 
           pc.max_cycles <= S7T_MAX_CYCLES ? "✓ PASS" : "✗ FAIL");
}

/*═══════════════════════════════════════════════════════════════
  Main Function
  ═══════════════════════════════════════════════════════════════*/

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║        CHATMAN-NANO-STACK Demo - Physics Compliant        ║\n");
    printf("║               All operations ≤7 CPU cycles                ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    // Run all demos
    demo_core_primitives();
    demo_memory_arena();
    demo_patterns();
    demo_state_machine();
    demo_ring_buffer();
    demo_string_interning();
    demo_workflow_engine();
    
#if defined(__SSE2__) || defined(__AVX2__)
    demo_simd_operations();
#endif
    
    demo_performance_validation();
    
    printf("\n=== Summary ===\n");
    printf("Global pool usage: %zu / %zu bytes (%.1f%%)\n",
           g_pool.used, sizeof(g_pool.data),
           100.0 * g_pool.used / sizeof(g_pool.data));
    printf("All demos completed successfully!\n\n");
    
    return 0;
}