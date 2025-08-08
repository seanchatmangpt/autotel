#include "conductor_manifest.h"
#include "meta_probe.h" // For L4 connection
#include <stdio.h>      // For printf

// Example bytecode (dummy data)
static const uint8_t dummy_bytecode_1[] = {0x01, 0x02, 0x03, 0x04};
static const uint8_t dummy_bytecode_2[] = {0x05, 0x06, 0x07, 0x08};

// Example behavior callbacks (dummy functions)
void behavior_callback_1(void* actor_state, void* message, meta_probe_collector_t* collector) {
    printf("Conductor Manifest: Executing behavior callback 1.\n");
    bitactor_fiber_t dummy_fiber = { .fiber_id = 1, .state_ptr = actor_state };
    uint64_t span_id = meta_probe_span_start(collector, &dummy_fiber, 0x1234ULL);
    // Simulate some work
    // ...
    meta_probe_span_end(collector, span_id, 0x1234ULL);
}

void behavior_callback_2(void* actor_state, void* message, meta_probe_collector_t* collector) {
    printf("Conductor Manifest: Executing behavior callback 2.\n");
    bitactor_fiber_t dummy_fiber = { .fiber_id = 2, .state_ptr = actor_state };
    uint64_t span_id = meta_probe_span_start(collector, &dummy_fiber, 0xFEDCULL);
    // Simulate some work
    // ...
    meta_probe_span_end(collector, span_id, 0xFEDCULL);
}

static behavior_callback_t callbacks_1[] = {behavior_callback_1};
static behavior_callback_t callbacks_2[] = {behavior_callback_2};

// The main conductor manifest array
// In a real system, this would be populated by an AOT compiler based on TTL specs.
const conductor_manifest_entry_t conductor_manifest[] = {
    {
        .spec_hash = 0x123456789ABCDEF0ULL, // Example hash
        .bytecode = dummy_bytecode_1,
        .bytecode_size = sizeof(dummy_bytecode_1),
        .behavior_callbacks = callbacks_1,
        .num_callbacks = sizeof(callbacks_1) / sizeof(behavior_callback_t),
        .supervision_strategy = SUPERVISION_STRATEGY_ONE_FOR_ONE
    },
    {
        .spec_hash = 0xFEDCBA9876543210ULL, // Example hash
        .bytecode = dummy_bytecode_2,
        .bytecode_size = sizeof(dummy_bytecode_2),
        .behavior_callbacks = callbacks_2,
        .num_callbacks = sizeof(callbacks_2) / sizeof(behavior_callback_t),
        .supervision_strategy = SUPERVISION_STRATEGY_ALL_FOR_ONE
    }
    // Add more entries as needed
};

// The size of the conductor manifest array
const uint32_t conductor_manifest_size = sizeof(conductor_manifest) / sizeof(conductor_manifest_entry_t);

