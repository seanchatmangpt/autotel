/*  ─────────────────────────────────────────────────────────────
    cmd_pm.c  –  Process Mining Commands for CNS
    PM7T integration for process analysis operations
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/cns_commands.h"
#include "../../c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  Process Mining State Management
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    EventLog* current_log;
    TraceLog* current_traces;
    ProcessModel* current_model;
    char log_name[64];
} pm_state_t;

// Get PM state from context
static pm_state_t* pm_get_state(void* context) {
    cns_context_t* ctx = (cns_context_t*)context;
    if (!ctx->user_data) {
        ctx->user_data = calloc(1, sizeof(pm_state_t));
    }
    return (pm_state_t*)ctx->user_data;
}

/*═══════════════════════════════════════════════════════════════
  Event Log Management Commands
  ═══════════════════════════════════════════════════════════════*/

// Import command - import event logs
CNS_HANDLER(cns_cmd_pm_import) {
    if (cmd->argc < 1) {
        printf("Usage: cns pm.import <filename> [-t type]\n");
        printf("Types: csv, xes (default: auto-detect)\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    const char* filename = cmd->args[0];
    const char* type = "auto";
    
    // Parse type if specified
    for (uint8_t i = 1; i < cmd->argc - 1; i++) {
        if (strcmp(cmd->args[i], "-t") == 0) {
            type = cmd->args[i + 1];
        }
    }
    
    // Get state
    pm_state_t* state = pm_get_state(context);
    
    // Clean up previous log if exists
    if (state->current_log) {
        pm7t_destroy_event_log(state->current_log);
        state->current_log = NULL;
    }
    
    // Tick 1-3: Detect file type and import
    EventLog* log = NULL;
    if (strcmp(type, "xes") == 0 || strstr(filename, ".xes")) {
        log = pm7t_import_xes(filename);
    } else if (strcmp(type, "csv") == 0 || strstr(filename, ".csv")) {
        log = pm7t_import_csv(filename);
    } else {
        // Auto-detect based on extension
        if (strstr(filename, ".xes")) {
            log = pm7t_import_xes(filename);
        } else {
            log = pm7t_import_csv(filename);
        }
    }
    
    // Tick 4-5: Validate import
    if (!log) {
        printf("Error: Failed to import event log from '%s'\n", filename);
        return CNS_ERR_RESOURCE;
    }
    
    // Tick 6-7: Store and report
    state->current_log = log;
    strncpy(state->log_name, filename, sizeof(state->log_name) - 1);
    
    size_t event_count = pm7t_get_event_count(log);
    uint32_t case_count = pm7t_get_unique_cases(log);
    uint32_t activity_count = pm7t_get_unique_activities(log);
    
    printf("Event log imported successfully:\n");
    printf("  File: %s\n", filename);
    printf("  Events: %zu\n", event_count);
    printf("  Cases: %u\n", case_count);
    printf("  Activities: %u\n", activity_count);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Process Discovery Commands
  ═══════════════════════════════════════════════════════════════*/

// Discover command - discover process model
CNS_HANDLER(cns_cmd_pm_discover) {
    pm_state_t* state = pm_get_state(context);
    
    if (!state->current_log) {
        printf("Error: No event log loaded. Use 'cns pm.import' first.\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    // Parse algorithm type
    const char* algorithm = "alpha"; // default
    double threshold = 0.5; // for heuristic miner
    
    for (uint8_t i = 0; i < cmd->argc - 1; i++) {
        if (strcmp(cmd->args[i], "-a") == 0) {
            algorithm = cmd->args[i + 1];
        } else if (strcmp(cmd->args[i], "-t") == 0) {
            threshold = atof(cmd->args[i + 1]);
        }
    }
    
    // Extract traces if not already done
    if (!state->current_traces) {
        state->current_traces = pm7t_extract_traces(state->current_log);
        if (!state->current_traces) {
            return CNS_ERR_RESOURCE;
        }
    }
    
    // Clean up previous model
    if (state->current_model) {
        pm7t_destroy_process_model(state->current_model);
        state->current_model = NULL;
    }
    
    // Discover process model
    ProcessModel* model = NULL;
    uint64_t start = s7t_cycles();
    
    if (strcmp(algorithm, "alpha") == 0) {
        model = pm7t_discover_alpha_algorithm(state->current_traces);
    } else if (strcmp(algorithm, "heuristic") == 0) {
        model = pm7t_discover_heuristic_miner(state->current_traces, threshold);
    } else if (strcmp(algorithm, "inductive") == 0) {
        model = pm7t_discover_inductive_miner(state->current_traces);
    } else {
        printf("Unknown algorithm: %s\n", algorithm);
        printf("Available: alpha, heuristic, inductive\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    uint64_t cycles = s7t_cycles() - start;
    
    if (!model) {
        printf("Error: Process discovery failed\n");
        return CNS_ERR_INTERNAL;
    }
    
    state->current_model = model;
    
    printf("Process model discovered:\n");
    printf("  Algorithm: %s\n", algorithm);
    printf("  Activities: %u\n", model->num_activities);
    printf("  Transitions: %zu\n", model->size);
    printf("  Discovery time: %lu cycles\n", cycles);
    
    // Show top transitions
    printf("\nTop transitions:\n");
    size_t show_count = model->size < 5 ? model->size : 5;
    for (size_t i = 0; i < show_count; i++) {
        Transition* t = &model->transitions[i];
        printf("  Activity %u → %u (freq: %u, prob: %.2f)\n",
               t->from_activity, t->to_activity, 
               t->frequency, t->probability);
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Process Analysis Commands
  ═══════════════════════════════════════════════════════════════*/

// Analyze command - analyze process performance
CNS_HANDLER(cns_cmd_pm_analyze) {
    pm_state_t* state = pm_get_state(context);
    
    if (!state->current_log) {
        printf("Error: No event log loaded. Use 'cns pm.import' first.\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    // Parse analysis type
    const char* type = "performance"; // default
    
    if (cmd->argc > 0) {
        type = cmd->args[0];
    }
    
    if (strcmp(type, "performance") == 0) {
        // Analyze performance
        PerformanceAnalysis* perf = pm7t_analyze_performance(state->current_log);
        if (!perf) {
            return CNS_ERR_INTERNAL;
        }
        
        printf("Performance Analysis:\n");
        printf("  Cases analyzed: %zu\n", perf->size);
        printf("  Average duration: %.2f hours\n", perf->avg_duration / 3600000000.0);
        printf("  Min duration: %.2f hours\n", perf->min_duration / 3600000000.0);
        printf("  Max duration: %.2f hours\n", perf->max_duration / 3600000000.0);
        printf("  Throughput: %.2f cases/day\n", perf->throughput * 86400.0);
        
        pm7t_destroy_performance_analysis(perf);
        
    } else if (strcmp(type, "bottleneck") == 0) {
        // Analyze bottlenecks
        BottleneckAnalysis* bottlenecks = pm7t_analyze_bottlenecks(state->current_log);
        if (!bottlenecks) {
            return CNS_ERR_INTERNAL;
        }
        
        printf("Bottleneck Analysis:\n");
        printf("  Activities analyzed: %zu\n", bottlenecks->size);
        printf("\nTop bottlenecks:\n");
        
        size_t show_count = bottlenecks->size < 5 ? bottlenecks->size : 5;
        for (size_t i = 0; i < show_count; i++) {
            BottleneckInfo* b = &bottlenecks->bottlenecks[i];
            printf("  Activity %u:\n", b->activity_id);
            printf("    Avg waiting time: %.2f min\n", b->avg_waiting_time / 60000000.0);
            printf("    Avg processing time: %.2f min\n", b->avg_processing_time / 60000000.0);
            printf("    Utilization: %.2f%%\n", b->utilization * 100.0);
            printf("    Queue length: %u\n", b->queue_length);
        }
        
        pm7t_destroy_bottleneck_analysis(bottlenecks);
        
    } else if (strcmp(type, "variants") == 0) {
        // Analyze process variants
        if (!state->current_traces) {
            state->current_traces = pm7t_extract_traces(state->current_log);
        }
        
        VariantAnalysis* variants = pm7t_analyze_variants(state->current_traces);
        if (!variants) {
            return CNS_ERR_INTERNAL;
        }
        
        printf("Variant Analysis:\n");
        printf("  Total cases: %u\n", variants->total_cases);
        printf("  Unique variants: %zu\n", variants->size);
        printf("\nTop variants:\n");
        
        size_t show_count = variants->size < 5 ? variants->size : 5;
        for (size_t i = 0; i < show_count; i++) {
            Variant* v = &variants->variants[i];
            printf("  Variant %zu: %u cases (%.1f%%)\n", 
                   i + 1, v->frequency, v->percentage * 100.0);
            printf("    Path: ");
            for (size_t j = 0; j < v->trace->size && j < 5; j++) {
                printf("%u", v->trace->activities[j]);
                if (j < v->trace->size - 1) printf(" → ");
            }
            if (v->trace->size > 5) printf(" → ...");
            printf("\n");
        }
        
        pm7t_destroy_variant_analysis(variants);
        
    } else {
        printf("Unknown analysis type: %s\n", type);
        printf("Available: performance, bottleneck, variants\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Conformance Checking Commands
  ═══════════════════════════════════════════════════════════════*/

// Conform command - check conformance
CNS_HANDLER(cns_cmd_pm_conform) {
    pm_state_t* state = pm_get_state(context);
    
    if (!state->current_log || !state->current_model) {
        printf("Error: Need both event log and process model.\n");
        printf("Use 'cns pm.import' and 'cns pm.discover' first.\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    if (!state->current_traces) {
        state->current_traces = pm7t_extract_traces(state->current_log);
    }
    
    // Check conformance
    ConformanceResult result = pm7t_check_conformance(
        state->current_model, state->current_traces);
    
    printf("Conformance Checking Results:\n");
    printf("  Fitness: %.2f%% (how well log fits model)\n", result.fitness * 100.0);
    printf("  Precision: %.2f%% (model specificity)\n", result.precision * 100.0);
    printf("  Generalization: %.2f%% (future behavior prediction)\n", 
           result.generalization * 100.0);
    printf("  Simplicity: %.2f%% (model complexity)\n", result.simplicity * 100.0);
    
    // Overall score
    double overall = (result.fitness + result.precision + 
                     result.generalization + result.simplicity) / 4.0;
    printf("\nOverall conformance score: %.2f%%\n", overall * 100.0);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Process Mining Utilities
  ═══════════════════════════════════════════════════════════════*/

// Stats command - show PM statistics
CNS_HANDLER(cns_cmd_pm_stats) {
    pm_state_t* state = pm_get_state(context);
    
    if (!state->current_log) {
        printf("No event log loaded.\n");
        return CNS_OK;
    }
    
    ProcessStats* stats = pm7t_analyze_process(state->current_log);
    if (!stats) {
        return CNS_ERR_INTERNAL;
    }
    
    printf("Process Statistics:\n");
    printf("  Log: %s\n", state->log_name);
    printf("  Events: %zu\n", pm7t_get_event_count(state->current_log));
    printf("  Cases: %u\n", pm7t_get_unique_cases(state->current_log));
    printf("  Activities: %u\n", pm7t_get_unique_activities(state->current_log));
    printf("  Resources: %u\n", pm7t_get_unique_resources(state->current_log));
    
    printf("\nActivity Statistics:\n");
    size_t show_count = stats->size < 10 ? stats->size : 10;
    for (size_t i = 0; i < show_count; i++) {
        ActivityStats* a = &stats->activities[i];
        printf("  Activity %u:\n", a->activity_id);
        printf("    Frequency: %u\n", a->frequency);
        printf("    Avg duration: %.2f min\n", a->avg_duration / 60000000.0);
        printf("    Duration range: %.2f - %.2f min\n",
               a->min_duration / 60000000.0,
               a->max_duration / 60000000.0);
    }
    
    pm7t_destroy_process_stats(stats);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Process Mining Command Registration
  ═══════════════════════════════════════════════════════════════*/

// Register all PM commands
void cns_register_pm_commands(cns_engine_t* engine) {
    // Event log management
    CNS_REGISTER_CMD(engine, "pm.import", cns_cmd_pm_import, 1, 3,
        "Import event log [-t type]");
    
    // Process discovery
    CNS_REGISTER_CMD(engine, "pm.discover", cns_cmd_pm_discover, 0, 4,
        "Discover process model [-a algorithm] [-t threshold]");
    
    // Analysis
    CNS_REGISTER_CMD(engine, "pm.analyze", cns_cmd_pm_analyze, 0, 1,
        "Analyze process [performance|bottleneck|variants]");
    
    // Conformance
    CNS_REGISTER_CMD(engine, "pm.conform", cns_cmd_pm_conform, 0, 0,
        "Check conformance between log and model");
    
    // Statistics
    CNS_REGISTER_CMD(engine, "pm.stats", cns_cmd_pm_stats, 0, 0,
        "Show process mining statistics");
    
    // Short aliases
    CNS_REGISTER_CMD(engine, "pmi", cns_cmd_pm_import, 1, 3,
        "Alias for pm.import");
    CNS_REGISTER_CMD(engine, "pmd", cns_cmd_pm_discover, 0, 4,
        "Alias for pm.discover");
    CNS_REGISTER_CMD(engine, "pma", cns_cmd_pm_analyze, 0, 1,
        "Alias for pm.analyze");
}