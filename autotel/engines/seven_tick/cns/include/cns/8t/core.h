#ifndef CNS_8T_CORE_H
#define CNS_8T_CORE_H

#include "types.h"

// 8T Core API - 80/20 approach with essential functions only

// Initialize 8T engine
int t8_init(void);

// Shutdown 8T engine
void t8_shutdown(void);

// Process a single tick
int t8_process_tick(tick_t* tick);

// Batch process multiple ticks
int t8_process_batch(tick_t* ticks, size_t count);

// Get current metrics
void t8_get_metrics(metrics_t* metrics);

// Reset metrics
void t8_reset_metrics(void);

// Set configuration option
int t8_set_option(const char* key, const char* value);

// Get configuration option
const char* t8_get_option(const char* key);

#endif // CNS_8T_CORE_H