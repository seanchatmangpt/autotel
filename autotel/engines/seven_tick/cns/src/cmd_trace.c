/*  ─────────────────────────────────────────────────────────────
    cmd_trace.c  –  Telemetry Commands for CNS
    Telemetry7T integration for observability operations
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/cns_commands.h"
#include "../../c_src/telemetry7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  Telemetry State Management
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    Telemetry7TContext* t7t_ctx;
    Telemetry7TSpan** span_history;
    size_t history_size;
    size_t history_capacity;
    bool json_export;
} trace_state_t;

// Get trace state from context
static trace_state_t* trace_get_state(void* context) {
    cns_context_t* ctx = (cns_context_t*)context;
    if (!ctx->user_data) {
        trace_state_t* state = calloc(1, sizeof(trace_state_t));
        state->t7t_ctx = telemetry7t_get_global_context();
        state->history_capacity = 100;
        state->span_history = calloc(state->history_capacity, sizeof(Telemetry7TSpan*));
        ctx->user_data = state;
    }
    return (trace_state_t*)ctx->user_data;
}

/*═══════════════════════════════════════════════════════════════
  Telemetry Control Commands
  ═══════════════════════════════════════════════════════════════*/

// Enable command - enable telemetry
CNS_HANDLER(cns_cmd_trace_enable) {
    trace_state_t* state = trace_get_state(context);
    
    // Initialize global telemetry if needed
    telemetry7t_global_init();
    telemetry7t_enable();
    
    // Enable in context
    telemetry7t_set_enabled(state->t7t_ctx, 1);
    
    printf("Telemetry enabled (7-tick optimized)\n");
    return CNS_OK;
}

// Disable command - disable telemetry
CNS_HANDLER(cns_cmd_trace_disable) {
    trace_state_t* state = trace_get_state(context);
    
    telemetry7t_disable();
    telemetry7t_set_enabled(state->t7t_ctx, 0);
    
    printf("Telemetry disabled\n");
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Span Management Commands
  ═══════════════════════════════════════════════════════════════*/

// Start command - start a new span
CNS_HANDLER(cns_cmd_trace_start) {
    trace_state_t* state = trace_get_state(context);
    
    if (!telemetry7t_is_enabled()) {
        printf("Warning: Telemetry is disabled. Enable with 'cns trace.enable'\n");
    }
    
    // Parse span parameters
    const char* name = "unnamed";
    const char* operation = "generic";
    uint8_t kind = TELEMETRY7T_KIND_INTERNAL;
    
    for (uint8_t i = 0; i < cmd->argc - 1; i++) {
        if (strcmp(cmd->args[i], "-n") == 0) {
            name = cmd->args[i + 1];
        } else if (strcmp(cmd->args[i], "-o") == 0) {
            operation = cmd->args[i + 1];
        } else if (strcmp(cmd->args[i], "-k") == 0) {
            const char* kind_str = cmd->args[i + 1];
            if (strcmp(kind_str, "server") == 0) {
                kind = TELEMETRY7T_KIND_SERVER;
            } else if (strcmp(kind_str, "client") == 0) {
                kind = TELEMETRY7T_KIND_CLIENT;
            } else if (strcmp(kind_str, "producer") == 0) {
                kind = TELEMETRY7T_KIND_PRODUCER;
            } else if (strcmp(kind_str, "consumer") == 0) {
                kind = TELEMETRY7T_KIND_CONSUMER;
            }
        }
    }
    
    // Tick 1-3: Create span
    Telemetry7TSpan* span = telemetry7t_span_begin(name, operation, kind);
    
    if (!span) {
        printf("Error: Failed to create span\n");
        return CNS_ERR_RESOURCE;
    }
    
    // Tick 4-5: Add to history
    if (state->history_size < state->history_capacity) {
        state->span_history[state->history_size++] = span;
    }
    
    // Tick 6-7: Report
    printf("Span started: %s (id: %lu, trace: %lu)\n",
           name, span->span_id, span->trace_id);
    
    // Add some attributes for demonstration
    telemetry7t_add_attribute_string(span, "command", "cns");
    telemetry7t_add_attribute_int(span, "tick_budget", 7);
    
    return CNS_OK;
}

// End command - end current span
CNS_HANDLER(cns_cmd_trace_end) {
    trace_state_t* state = trace_get_state(context);
    
    // Get current span
    Telemetry7TSpan* current = telemetry7t_get_current_span(state->t7t_ctx);
    
    if (!current) {
        printf("Error: No active span to end\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    // Parse status
    uint8_t status = TELEMETRY7T_STATUS_OK;
    
    if (cmd->argc > 0) {
        if (strcmp(cmd->args[0], "error") == 0) {
            status = TELEMETRY7T_STATUS_ERROR;
        } else if (strcmp(cmd->args[0], "ok") == 0) {
            status = TELEMETRY7T_STATUS_OK;
        }
    }
    
    // Get duration before ending
    uint64_t duration_ns = telemetry7t_get_span_duration_ns(current);
    const char* name = current->name;
    
    // End the span
    telemetry7t_span_finish(current, status);
    
    printf("Span ended: %s (duration: %lu ns, status: %s)\n",
           name, duration_ns,
           status == TELEMETRY7T_STATUS_OK ? "OK" : "ERROR");
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Telemetry View Commands
  ═══════════════════════════════════════════════════════════════*/

// View command - view trace information
CNS_HANDLER(cns_cmd_trace_view) {
    trace_state_t* state = trace_get_state(context);
    
    // Parse view options
    const char* format = "text";
    bool current_only = false;
    
    for (uint8_t i = 0; i < cmd->argc - 1; i++) {
        if (strcmp(cmd->args[i], "-f") == 0) {
            format = cmd->args[i + 1];
        } else if (strcmp(cmd->args[i], "-c") == 0) {
            current_only = true;
        }
    }
    
    if (current_only) {
        // Show current span
        Telemetry7TSpan* current = telemetry7t_get_current_span(state->t7t_ctx);
        if (!current) {
            printf("No active span\n");
            return CNS_OK;
        }
        
        if (strcmp(format, "json") == 0) {
            char buffer[1024];
            telemetry7t_span_export_json(current, buffer, sizeof(buffer));
            printf("%s\n", buffer);
        } else {
            telemetry7t_span_print(current);
        }
    } else {
        // Show span history
        printf("Span History (%zu spans):\n", state->history_size);
        printf("%-8s %-20s %-12s %-8s %-12s\n",
               "ID", "Name", "Operation", "Status", "Duration");
        printf("------------------------------------------------------------\n");
        
        for (size_t i = 0; i < state->history_size; i++) {
            Telemetry7TSpan* span = state->span_history[i];
            uint64_t duration = telemetry7t_get_span_duration_ns(span);
            const char* status_str = "ACTIVE";
            
            if (span->end_time_ns > 0) {
                status_str = span->status == TELEMETRY7T_STATUS_OK ? "OK" : 
                            span->status == TELEMETRY7T_STATUS_ERROR ? "ERROR" : "UNSET";
            }
            
            printf("%-8lu %-20s %-12s %-8s %lu ns\n",
                   span->span_id, span->name, span->operation,
                   status_str, duration);
        }
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Telemetry Export Commands
  ═══════════════════════════════════════════════════════════════*/

// Export command - export traces
CNS_HANDLER(cns_cmd_trace_export) {
    trace_state_t* state = trace_get_state(context);
    
    if (state->history_size == 0) {
        printf("No spans to export\n");
        return CNS_OK;
    }
    
    // Parse export options
    const char* format = "json";
    const char* output = "traces.json";
    
    for (uint8_t i = 0; i < cmd->argc - 1; i++) {
        if (strcmp(cmd->args[i], "-f") == 0) {
            format = cmd->args[i + 1];
        } else if (strcmp(cmd->args[i], "-o") == 0) {
            output = cmd->args[i + 1];
        }
    }
    
    if (strcmp(format, "json") == 0) {
        FILE* f = fopen(output, "w");
        if (!f) {
            printf("Error: Cannot open file '%s'\n", output);
            return CNS_ERR_RESOURCE;
        }
        
        fprintf(f, "{\n  \"spans\": [\n");
        
        for (size_t i = 0; i < state->history_size; i++) {
            char buffer[1024];
            telemetry7t_span_export_json(state->span_history[i], 
                                       buffer, sizeof(buffer));
            fprintf(f, "    %s%s\n", buffer,
                   i < state->history_size - 1 ? "," : "");
        }
        
        fprintf(f, "  ]\n}\n");
        fclose(f);
        
        printf("Exported %zu spans to '%s'\n", state->history_size, output);
    } else {
        printf("Unknown format: %s (only 'json' supported)\n", format);
        return CNS_ERR_INVALID_ARG;
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Telemetry Benchmark Commands
  ═══════════════════════════════════════════════════════════════*/

// Bench command - benchmark telemetry performance
CNS_HANDLER(cns_cmd_trace_bench) {
    printf("Running telemetry benchmark...\n\n");
    
    // Enable telemetry for benchmark
    telemetry7t_global_init();
    telemetry7t_enable();
    
    // Run the benchmark
    telemetry7t_benchmark();
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Telemetry Statistics Commands
  ═══════════════════════════════════════════════════════════════*/

// Stats command - show telemetry statistics
CNS_HANDLER(cns_cmd_trace_stats) {
    trace_state_t* state = trace_get_state(context);
    
    printf("Telemetry Statistics:\n");
    printf("  Status: %s\n", telemetry7t_is_enabled() ? "Enabled" : "Disabled");
    printf("  Total spans: %zu\n", state->history_size);
    
    if (state->history_size > 0) {
        // Calculate statistics
        uint64_t total_duration = 0;
        uint64_t min_duration = UINT64_MAX;
        uint64_t max_duration = 0;
        size_t active_count = 0;
        size_t ok_count = 0;
        size_t error_count = 0;
        
        for (size_t i = 0; i < state->history_size; i++) {
            Telemetry7TSpan* span = state->span_history[i];
            
            if (telemetry7t_is_span_active(span)) {
                active_count++;
            } else {
                uint64_t duration = telemetry7t_get_span_duration_ns(span);
                total_duration += duration;
                
                if (duration < min_duration) min_duration = duration;
                if (duration > max_duration) max_duration = duration;
                
                if (span->status == TELEMETRY7T_STATUS_OK) ok_count++;
                else if (span->status == TELEMETRY7T_STATUS_ERROR) error_count++;
            }
        }
        
        size_t completed = state->history_size - active_count;
        
        printf("\nSpan Status:\n");
        printf("  Active: %zu\n", active_count);
        printf("  Completed: %zu (OK: %zu, ERROR: %zu)\n", 
               completed, ok_count, error_count);
        
        if (completed > 0) {
            printf("\nDuration Statistics:\n");
            printf("  Average: %lu ns\n", total_duration / completed);
            printf("  Min: %lu ns\n", min_duration);
            printf("  Max: %lu ns\n", max_duration);
        }
    }
    
    // Show current span info
    Telemetry7TSpan* current = telemetry7t_get_current_span(state->t7t_ctx);
    if (current) {
        printf("\nCurrent Span:\n");
        printf("  Name: %s\n", current->name);
        printf("  ID: %lu\n", current->span_id);
        printf("  Duration: %lu ns\n", telemetry7t_get_span_duration_ns(current));
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Telemetry Command Registration
  ═══════════════════════════════════════════════════════════════*/

// Register all telemetry commands
void cns_register_trace_commands(cns_engine_t* engine) {
    // Control commands
    CNS_REGISTER_CMD(engine, "trace.enable", cns_cmd_trace_enable, 0, 0,
        "Enable telemetry tracking");
    CNS_REGISTER_CMD(engine, "trace.disable", cns_cmd_trace_disable, 0, 0,
        "Disable telemetry tracking");
    
    // Span management
    CNS_REGISTER_CMD(engine, "trace.start", cns_cmd_trace_start, 0, 6,
        "Start span [-n name] [-o operation] [-k kind]");
    CNS_REGISTER_CMD(engine, "trace.end", cns_cmd_trace_end, 0, 1,
        "End current span [ok|error]");
    
    // View and export
    CNS_REGISTER_CMD(engine, "trace.view", cns_cmd_trace_view, 0, 3,
        "View traces [-f format] [-c current]");
    CNS_REGISTER_CMD(engine, "trace.export", cns_cmd_trace_export, 0, 4,
        "Export traces [-f format] [-o output]");
    
    // Statistics and benchmarking
    CNS_REGISTER_CMD(engine, "trace.stats", cns_cmd_trace_stats, 0, 0,
        "Show telemetry statistics");
    CNS_REGISTER_CMD(engine, "trace.bench", cns_cmd_trace_bench, 0, 0,
        "Run telemetry benchmark");
    
    // Short aliases
    CNS_REGISTER_CMD(engine, "te", cns_cmd_trace_enable, 0, 0,
        "Alias for trace.enable");
    CNS_REGISTER_CMD(engine, "td", cns_cmd_trace_disable, 0, 0,
        "Alias for trace.disable");
    CNS_REGISTER_CMD(engine, "ts", cns_cmd_trace_start, 0, 6,
        "Alias for trace.start");
    CNS_REGISTER_CMD(engine, "tend", cns_cmd_trace_end, 0, 1,
        "Alias for trace.end");
}