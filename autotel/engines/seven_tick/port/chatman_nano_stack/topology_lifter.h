#ifndef TOPOLOGY_LIFTER_H
#define TOPOLOGY_LIFTER_H

#include <stdint.h> // For uint64_t, etc.

// Initialize the L8 telemetry system
void l8_telemetry_init();

// Record the start of a tick for a given actor
void l8_record_tick_start(uint64_t actor_id, uint64_t current_tick);

// Record the end of a tick for a given actor
void l8_record_tick_end(uint64_t actor_id, uint64_t current_tick);

// Record an opcode span within a tick
void l8_record_opcode_span(uint64_t actor_id, uint64_t opcode_id, uint64_t start_tick, uint64_t end_tick);

// Check if an actor has exceeded its time budget
// Returns 1 if budget exceeded, 0 otherwise
int l8_check_time_budget(uint64_t actor_id, uint64_t elapsed_ticks, uint64_t budget_ticks);

// Convert collected traces into TTL format (placeholder)
char* l8_traces_to_ttl();

// Shutdown the L8 telemetry system
void l8_telemetry_shutdown();

#endif // TOPOLOGY_LIFTER_H
