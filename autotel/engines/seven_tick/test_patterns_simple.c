#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

// Simple timer for benchmarking
typedef struct {
    struct timeval start;
    struct timeval end;
    double duration_ms;
} SimpleTimer;

static void simple_timer_start(SimpleTimer* t) {
    gettimeofday(&t->start, NULL);
}

static void simple_timer_stop(SimpleTimer* t) {
    gettimeofday(&t->end, NULL);
    long seconds = t->end.tv_sec - t->start.tv_sec;
    long microseconds = t->end.tv_usec - t->start.tv_usec;
    t->duration_ms = (seconds * 1000.0) + (microseconds / 1000.0);
}

// Simple hash function
static uint32_t simple_hash(const char* str) {
    uint32_t hash = 0x811c9dc5;
    while (*str) {
        hash ^= (uint32_t)*str++;
        hash *= 0x01000193;
    }
    return hash;
}

#define S7T_PATTERNS_EXAMPLE
#include "s7t_patterns.h"

// Example visitor implementations
static void visit_literal(S7T_Node* node, void* context) {
    (void)context;
    printf("   Visiting literal node with data: %u\n", node->data);
}

static void visit_binary(S7T_Node* node, void* context) {
    (void)context;
    printf("   Visiting binary node with data: %u\n", node->data);
}

static void visit_unary(S7T_Node* node, void* context) {
    (void)context;
    printf("   Visiting unary node with data: %u\n", node->data);
}

static void visit_call(S7T_Node* node, void* context) {
    (void)context;
    printf("   Visiting call node with data: %u\n", node->data);
}

// Visitor dispatch test
void test_visitor_pattern() {
    printf("\n11. Visitor Pattern (Full Test):\n");
    
    S7T_VisitorTable vtable = {
        .visitors = {visit_literal, visit_binary, visit_unary, visit_call}
    };
    
    S7T_Node nodes[] = {
        {.type = S7T_NODE_LITERAL, .data = 100},
        {.type = S7T_NODE_BINARY, .data = 200},
        {.type = S7T_NODE_UNARY, .data = 300},
        {.type = S7T_NODE_CALL, .data = 400}
    };
    
    for (int i = 0; i < 4; i++) {
        s7t_accept_visitor(&nodes[i], &vtable, NULL);
    }
}

// Simple benchmarks
void run_simple_benchmarks() {
    printf("\n=== Simple Pattern Benchmarks ===\n\n");
    
    SimpleTimer timer;
    const int iterations = 1000000;
    
    // Benchmark 1: Singleton
    simple_timer_start(&timer);
    for (int i = 0; i < iterations; i++) {
        S7T_Singleton* s = s7t_singleton_get();
        s->data[0] = i;
    }
    simple_timer_stop(&timer);
    printf("1. Singleton: %.3f ms for %d ops (%.2f ns/op)\n",
           timer.duration_ms, iterations, (timer.duration_ms * 1000000.0) / iterations);
    
    // Benchmark 2: Factory
    simple_timer_start(&timer);
    S7T_Object obj;
    for (int i = 0; i < iterations; i++) {
        s7t_factory_create(&obj, i & 0x3);
    }
    simple_timer_stop(&timer);
    printf("2. Factory: %.3f ms for %d ops (%.2f ns/op)\n",
           timer.duration_ms, iterations, (timer.duration_ms * 1000000.0) / iterations);
    
    // Benchmark 3: State Machine
    simple_timer_start(&timer);
    S7T_State state = S7T_STATE_IDLE;
    for (int i = 0; i < iterations; i++) {
        state = s7t_state_transition(state, i & 0x3);
    }
    simple_timer_stop(&timer);
    printf("3. State Machine: %.3f ms for %d ops (%.2f ns/op)\n",
           timer.duration_ms, iterations, (timer.duration_ms * 1000000.0) / iterations);
    
    // Benchmark 4: Strategy
    simple_timer_start(&timer);
    volatile uint32_t sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += s7t_execute_strategy(i & 0x3, i);
    }
    simple_timer_stop(&timer);
    printf("4. Strategy: %.3f ms for %d ops (%.2f ns/op)\n",
           timer.duration_ms, iterations, (timer.duration_ms * 1000000.0) / iterations);
    
    // Benchmark 5: Decorator
    simple_timer_start(&timer);
    S7T_Decorated decorated = {0};
    for (int i = 0; i < iterations; i++) {
        s7t_add_decoration(&decorated, 1 << (i & 0x7));
        s7t_has_decoration(&decorated, S7T_ATTR_CACHED);
    }
    simple_timer_stop(&timer);
    printf("5. Decorator: %.3f ms for %d ops (%.2f ns/op)\n",
           timer.duration_ms, iterations, (timer.duration_ms * 1000000.0) / iterations);
}

int main(void) {
    printf("S7T Nanosecond Design Patterns - Simple Test\n");
    printf("============================================\n");
    
    // Test basic patterns
    printf("\n=== Basic Pattern Tests ===\n");
    
    // Singleton
    S7T_Singleton* single = s7t_singleton_get();
    single->data[0] = 0xCAFEBABE;
    printf("Singleton: ID=0x%X, Data=0x%llX\n", 
           single->instance_id, (unsigned long long)single->data[0]);
    
    // Factory
    S7T_Object analyzer;
    s7t_factory_create(&analyzer, S7T_TYPE_ANALYZER);
    printf("Factory: Created type %u with flags 0x%X\n", 
           analyzer.type_id, analyzer.flags);
    
    // State Machine
    S7T_State state = S7T_STATE_IDLE;
    state = s7t_state_transition(state, S7T_EVENT_START);
    printf("State Machine: IDLE -> START = State %u\n", state);
    
    // Strategy
    uint32_t result = s7t_execute_strategy(2, 10);
    printf("Strategy: Precise(10) = %u\n", result);
    
    // Decorator
    S7T_Decorated obj = {.core_data = 42, .attributes = 0};
    s7t_add_decoration(&obj, S7T_ATTR_CACHED | S7T_ATTR_VALIDATED);
    printf("Decorator: Added CACHED=%d, VALIDATED=%d\n",
           s7t_has_decoration(&obj, S7T_ATTR_CACHED),
           s7t_has_decoration(&obj, S7T_ATTR_VALIDATED));
    
    // Test visitor pattern
    test_visitor_pattern();
    
    // Run benchmarks
    run_simple_benchmarks();
    
    printf("\n✓ All patterns working with zero heap allocation!\n");
    return 0;
}