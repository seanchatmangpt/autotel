/*  ─────────────────────────────────────────────────────────────
    otel_integration.c  –  OpenTelemetry Integration Example
    Shows how to use CNS with distributed tracing
    ───────────────────────────────────────────────────────────── */

#include "cns/core/engine.h"
#include "cns/core/registry.h"
#include "cns/commands/base.h"
#include "cns/telemetry/otel.h"
#include "cns/bench/framework.h"
#include <stdio.h>
#include <stdlib.h>

/*═══════════════════════════════════════════════════════════════
  Example Commands with Telemetry
  ═══════════════════════════════════════════════════════════════*/

// Database query command (simulated)
CNS_HANDLER(db_query) {
    cns_telemetry_t* telemetry = (cns_telemetry_t*)ctx->telemetry;
    
    // Start span for database operation
    cns_span_t* span = cns_span_start(telemetry, "database.query", ctx->span);
    
    // Set span attributes
    cns_attribute_t attrs[] = {
        {.key = "db.system", .type = CNS_ATTR_STRING, .string_value = "postgresql"},
        {.key = "db.operation", .type = CNS_ATTR_STRING, .string_value = "SELECT"},
        {.key = "db.statement", .type = CNS_ATTR_STRING, 
         .string_value = cmd->argc > 0 ? cmd->args[0] : "SELECT 1"},
    };
    cns_span_set_attributes(span, attrs, 3);
    
    // Simulate database query (normally would be actual DB call)
    uint64_t start = s7t_cycles();
    
    // Quick validation (< 7 ticks)
    if (cmd->argc == 0) {
        cns_span_end(span, CNS_SPAN_STATUS_ERROR);
        return CNS_ERR_INVALID_ARG;
    }
    
    // Record query event
    cns_span_add_event(span, "query.validated", NULL, 0);
    
    // Simulate query execution
    printf("Executing query: %s\n", cmd->args[0]);
    
    // Record metrics
    uint64_t cycles = s7t_cycles() - start;
    cns_metric_record_latency(telemetry, "db_query", cycles);
    cns_metric_inc_counter(telemetry, "db_query", CNS_OK);
    
    // End span
    cns_span_end(span, CNS_SPAN_STATUS_OK);
    
    return CNS_OK;
}

// API call command with distributed tracing
CNS_HANDLER(api_call) {
    cns_telemetry_t* telemetry = (cns_telemetry_t*)ctx->telemetry;
    
    // Start span
    cns_span_t* span = cns_span_start(telemetry, "api.call", ctx->span);
    
    // Extract trace context from headers (if provided)
    if (cmd->argc >= 2) {
        cns_span_t* parent_span;
        if (cns_context_extract(cmd->args[0], cmd->args[1], &parent_span)) {
            // Link to parent span from upstream service
            cns_span_add_event(span, "context.extracted", NULL, 0);
        }
    }
    
    // Set HTTP attributes
    cns_attribute_t attrs[] = {
        {.key = "http.method", .type = CNS_ATTR_STRING, .string_value = "GET"},
        {.key = "http.url", .type = CNS_ATTR_STRING, 
         .string_value = cmd->argc > 0 ? cmd->args[0] : "/api/v1/status"},
        {.key = "http.status_code", .type = CNS_ATTR_INT64, .int64_value = 200},
    };
    cns_span_set_attributes(span, attrs, 3);
    
    // Inject trace context for downstream
    char traceparent[128], tracestate[256];
    if (cns_context_inject(span, traceparent, sizeof(traceparent), 
                          tracestate, sizeof(tracestate))) {
        printf("Trace context: %s\n", traceparent);
    }
    
    // Simulate API call
    printf("Calling API: %s\n", cmd->argc > 0 ? cmd->args[0] : "/api/v1/status");
    
    // End span
    cns_span_end(span, CNS_SPAN_STATUS_OK);
    
    return CNS_OK;
}

// Batch processing with telemetry
CNS_HANDLER(batch_process) {
    cns_telemetry_t* telemetry = (cns_telemetry_t*)ctx->telemetry;
    
    // Start parent span
    cns_span_t* parent = cns_span_start(telemetry, "batch.process", ctx->span);
    
    int batch_size = cmd->argc > 0 ? atoi(cmd->args[0]) : 10;
    
    cns_attribute_t attrs[] = {
        {.key = "batch.size", .type = CNS_ATTR_INT64, .int64_value = batch_size},
    };
    cns_span_set_attributes(parent, attrs, 1);
    
    // Process items
    for (int i = 0; i < batch_size; i++) {
        // Child span for each item
        cns_span_t* item_span = cns_span_start(telemetry, "batch.item", parent);
        
        cns_attribute_t item_attrs[] = {
            {.key = "item.index", .type = CNS_ATTR_INT64, .int64_value = i},
        };
        cns_span_set_attributes(item_span, item_attrs, 1);
        
        // Simulate processing
        printf("Processing item %d/%d\n", i + 1, batch_size);
        
        // Record item metrics
        cns_metric_inc_counter(telemetry, "batch.items.processed", CNS_OK);
        
        cns_span_end(item_span, CNS_SPAN_STATUS_OK);
    }
    
    // End parent span
    cns_span_end(parent, CNS_SPAN_STATUS_OK);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Command Definitions
  ═══════════════════════════════════════════════════════════════*/

static cns_command_def_t example_commands[] = {
    CNS_COMMAND_DEF("db_query", cns_handler_db_query, 1, 1, 
                    "Execute database query with tracing"),
    CNS_COMMAND_DEF("api_call", cns_handler_api_call, 0, 2, 
                    "Make API call with distributed tracing"),
    CNS_COMMAND_DEF("batch_process", cns_handler_batch_process, 0, 1, 
                    "Process batch with telemetry"),
};

/*═══════════════════════════════════════════════════════════════
  Benchmark with Telemetry
  ═══════════════════════════════════════════════════════════════*/

void bench_telemetry_overhead(void* context) {
    cns_engine_t* engine = (cns_engine_t*)context;
    cns_command_t cmd = {
        .name = "db_query",
        .hash = s7t_hash_string("db_query", 8),
        .args = (char*[]){"SELECT * FROM users"},
        .argc = 1,
        .type = CNS_CMD_EXEC,
        .flags = CNS_FLAG_TRACED,
    };
    
    cns_context_t ctx = {
        .telemetry = engine->telemetry,
        .span = NULL,
    };
    
    cns_engine_execute(engine, &cmd, &ctx);
}

static cns_bench_def_t telemetry_benchmarks[] = {
    CNS_BENCHMARK_DEF(telemetry_overhead, bench_telemetry_overhead,
                      "Measure telemetry overhead for commands"),
};

/*═══════════════════════════════════════════════════════════════
  Main Example
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char** argv) {
    // Initialize engine with telemetry
    cns_engine_config_t config = CNS_DEFAULT_ENGINE_CONFIG;
    config.telemetry.service_name = "cns-example";
    config.telemetry.otlp_endpoint = getenv("OTEL_EXPORTER_OTLP_ENDPOINT") ?: 
                                    "localhost:4317";
    
    cns_engine_t engine;
    if (cns_engine_init(&engine, &config) != CNS_OK) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }
    
    // Register commands
    cns_registry_register_batch(engine.registry, example_commands, 
                               sizeof(example_commands) / sizeof(example_commands[0]),
                               NULL);
    
    printf("=== CNS OpenTelemetry Integration Example ===\n\n");
    
    // Example 1: Database query with tracing
    printf("1. Database Query with Tracing:\n");
    {
        cns_command_t cmd = {
            .name = "db_query",
            .hash = s7t_hash_string("db_query", 8),
            .args = (char*[]){"SELECT * FROM products WHERE price > 100"},
            .argc = 1,
        };
        
        cns_context_t ctx = {
            .telemetry = engine.telemetry,
        };
        
        cns_result_t result = cns_engine_execute(&engine, &cmd, &ctx);
        printf("Result: %s\n\n", cns_result_str(result));
    }
    
    // Example 2: API call with distributed tracing
    printf("2. API Call with Distributed Tracing:\n");
    {
        cns_command_t cmd = {
            .name = "api_call",
            .hash = s7t_hash_string("api_call", 8),
            .args = (char*[]){"/api/v1/users/123"},
            .argc = 1,
        };
        
        cns_context_t ctx = {
            .telemetry = engine.telemetry,
        };
        
        cns_result_t result = cns_engine_execute(&engine, &cmd, &ctx);
        printf("Result: %s\n\n", cns_result_str(result));
    }
    
    // Example 3: Batch processing
    printf("3. Batch Processing with Child Spans:\n");
    {
        cns_command_t cmd = {
            .name = "batch_process",
            .hash = s7t_hash_string("batch_process", 13),
            .args = (char*[]){"5"},
            .argc = 1,
        };
        
        cns_context_t ctx = {
            .telemetry = engine.telemetry,
        };
        
        cns_result_t result = cns_engine_execute(&engine, &cmd, &ctx);
        printf("Result: %s\n\n", cns_result_str(result));
    }
    
    // Example 4: Performance benchmark
    printf("4. Telemetry Overhead Benchmark:\n");
    {
        cns_bench_config_t bench_config = CNS_DEFAULT_BENCH_CONFIG;
        bench_config.iterations = 1000;
        
        cns_bench_result_t result = cns_bench_run(&telemetry_benchmarks[0], 
                                                  &bench_config);
        
        printf("  Average cycles: %lu\n", result.avg_cycles);
        printf("  Overhead: %.2f ns\n", result.ns_per_op);
        printf("  Ops/sec: %.2f\n", result.ops_per_sec);
    }
    
    // Export metrics
    printf("\n5. Exporting Telemetry Data...\n");
    cns_telemetry_flush(engine.telemetry);
    
    // Get engine statistics
    cns_engine_stats_t stats;
    cns_engine_get_stats(&engine, &stats);
    
    printf("\nEngine Statistics:\n");
    printf("  Total commands: %lu\n", stats.total_commands);
    printf("  Average cycles: %lu\n", stats.avg_cycles);
    printf("  Throughput: %.2f cmd/s\n", stats.throughput);
    
    // Cleanup
    cns_engine_shutdown(&engine);
    
    return 0;
}