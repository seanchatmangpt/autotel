#include "cns/8t.h"
#include <assert.h>
#include <stdio.h>

// Unit tests for 8T core functionality

void test_init_shutdown() {
    printf("Testing init/shutdown...\n");
    
    // Test successful init
    int ret = t8_init();
    assert(ret == 0);
    
    // Test double init fails
    ret = t8_init();
    assert(ret == -1);
    
    // Test shutdown
    t8_shutdown();
    
    // Test init after shutdown
    ret = t8_init();
    assert(ret == 0);
    
    t8_shutdown();
    printf("✓ Init/shutdown tests passed\n");
}

void test_process_tick() {
    printf("Testing tick processing...\n");
    
    t8_init();
    
    tick_t tick = {
        .id = 1,
        .type = 0,
        .data = NULL,
        .size = 0
    };
    
    int ret = t8_process_tick(&tick);
    assert(ret == 0);
    
    // Test null tick
    ret = t8_process_tick(NULL);
    assert(ret == -1);
    
    t8_shutdown();
    printf("✓ Tick processing tests passed\n");
}

void test_batch_processing() {
    printf("Testing batch processing...\n");
    
    t8_init();
    
    tick_t ticks[10];
    for (int i = 0; i < 10; i++) {
        ticks[i].id = i;
        ticks[i].type = 0;
        ticks[i].data = NULL;
        ticks[i].size = 0;
    }
    
    int ret = t8_process_batch(ticks, 10);
    assert(ret == 0);
    
    t8_shutdown();
    printf("✓ Batch processing tests passed\n");
}

void test_metrics() {
    printf("Testing metrics...\n");
    
    t8_init();
    
    // Reset metrics
    t8_reset_metrics();
    
    // Process some ticks
    tick_t tick = {.id = 1, .type = 0, .data = NULL, .size = 0};
    for (int i = 0; i < 5; i++) {
        t8_process_tick(&tick);
    }
    
    // Get metrics
    metrics_t metrics;
    t8_get_metrics(&metrics);
    assert(metrics.ticks_processed == 5);
    
    t8_shutdown();
    printf("✓ Metrics tests passed\n");
}

int main() {
    printf("Running 8T core unit tests...\n\n");
    
    test_init_shutdown();
    test_process_tick();
    test_batch_processing();
    test_metrics();
    
    printf("\n✓ All tests passed!\n");
    return 0;
}