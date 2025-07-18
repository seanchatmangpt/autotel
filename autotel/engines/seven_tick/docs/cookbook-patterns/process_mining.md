# Pattern: Process Mining with 7T Engine

## Description
This pattern demonstrates how to use the 7T Engine for process mining applications, equivalent to pm4py functionality. It covers event log management, process discovery, conformance checking, and performance analysis for business process optimization.

## Code Example
```c
#include "c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Process mining workflow example
int main() {
    printf("=== 7T Process Mining Example ===\n");
    
    // Create event log
    EventLog* event_log = pm7t_create_event_log(1000);
    if (!event_log) {
        fprintf(stderr, "Failed to create event log\n");
        return 1;
    }
    
    // Add sample events (simulating a loan application process)
    uint64_t base_time = time(NULL) * 1000000000ULL; // Nanoseconds
    
    // Case 1: Successful loan application
    pm7t_add_event(event_log, 1, 1, base_time + 1000, 101, 50);      // Submit application
    pm7t_add_event(event_log, 1, 2, base_time + 2000, 102, 100);     // Review application
    pm7t_add_event(event_log, 1, 3, base_time + 3000, 103, 75);      // Check credit
    pm7t_add_event(event_log, 1, 4, base_time + 4000, 104, 200);     // Approve loan
    pm7t_add_event(event_log, 1, 5, base_time + 5000, 105, 150);     // Disburse funds
    
    // Case 2: Rejected loan application
    pm7t_add_event(event_log, 2, 1, base_time + 6000, 101, 50);      // Submit application
    pm7t_add_event(event_log, 2, 2, base_time + 7000, 102, 100);     // Review application
    pm7t_add_event(event_log, 2, 3, base_time + 8000, 103, 75);      // Check credit
    pm7t_add_event(event_log, 2, 6, base_time + 9000, 106, 25);      // Reject application
    
    // Case 3: Application requiring additional documents
    pm7t_add_event(event_log, 3, 1, base_time + 10000, 101, 50);     // Submit application
    pm7t_add_event(event_log, 3, 2, base_time + 11000, 102, 100);    // Review application
    pm7t_add_event(event_log, 3, 7, base_time + 12000, 107, 50);     // Request documents
    pm7t_add_event(event_log, 3, 8, base_time + 13000, 108, 30);     // Receive documents
    pm7t_add_event(event_log, 3, 2, base_time + 14000, 102, 100);    // Review application
    pm7t_add_event(event_log, 3, 3, base_time + 15000, 103, 75);     // Check credit
    pm7t_add_event(event_log, 3, 4, base_time + 16000, 104, 200);    // Approve loan
    pm7t_add_event(event_log, 3, 5, base_time + 17000, 105, 150);    // Disburse funds
    
    printf("Added %zu events to event log\n", pm7t_get_event_count(event_log));
    
    // Extract traces from event log
    TraceLog* trace_log = pm7t_extract_traces(event_log);
    if (!trace_log) {
        fprintf(stderr, "Failed to extract traces\n");
        pm7t_destroy_event_log(event_log);
        return 1;
    }
    
    printf("Extracted %zu traces\n", pm7t_get_trace_count(trace_log));
    
    // Display traces
    for (size_t i = 0; i < pm7t_get_trace_count(trace_log); i++) {
        Trace* trace = pm7t_get_trace(trace_log, i);
        printf("Trace %zu: ", i + 1);
        for (size_t j = 0; j < trace->size; j++) {
            printf("%u ", trace->activities[j]);
        }
        printf("\n");
    }
    
    // Process discovery using Alpha algorithm
    printf("\n=== Process Discovery (Alpha Algorithm) ===\n");
    ProcessModel* alpha_model = pm7t_discover_alpha_algorithm(trace_log);
    if (alpha_model) {
        printf("Discovered process model with %zu transitions:\n", alpha_model->size);
        for (size_t i = 0; i < alpha_model->size; i++) {
            printf("  %u -> %u (freq: %u, prob: %.3f)\n",
                   alpha_model->transitions[i].from_activity,
                   alpha_model->transitions[i].to_activity,
                   alpha_model->transitions[i].frequency,
                   alpha_model->transitions[i].probability);
        }
    }
    
    // Process discovery using Heuristic miner
    printf("\n=== Process Discovery (Heuristic Miner) ===\n");
    ProcessModel* heuristic_model = pm7t_discover_heuristic_miner(trace_log, 0.5);
    if (heuristic_model) {
        printf("Discovered process model with %zu transitions:\n", heuristic_model->size);
        for (size_t i = 0; i < heuristic_model->size; i++) {
            printf("  %u -> %u (freq: %u, prob: %.3f)\n",
                   heuristic_model->transitions[i].from_activity,
                   heuristic_model->transitions[i].to_activity,
                   heuristic_model->transitions[i].frequency,
                   heuristic_model->transitions[i].probability);
        }
    }
    
    // Process analysis
    printf("\n=== Process Analysis ===\n");
    ProcessStats* process_stats = pm7t_analyze_process(event_log);
    if (process_stats) {
        printf("Process statistics:\n");
        for (size_t i = 0; i < process_stats->size; i++) {
            ActivityStats* activity = &process_stats->activities[i];
            if (activity->frequency > 0) {
                printf("  Activity %u: freq=%u, avg_duration=%.2f, min=%.0f, max=%.0f\n",
                       activity->activity_id,
                       activity->frequency,
                       activity->avg_duration,
                       activity->min_duration,
                       activity->max_duration);
            }
        }
    }
    
    // Conformance checking
    printf("\n=== Conformance Checking ===\n");
    if (alpha_model) {
        ConformanceResult conformance = pm7t_check_conformance(alpha_model, trace_log);
        printf("Conformance results:\n");
        printf("  Fitness: %.3f\n", conformance.fitness);
        printf("  Precision: %.3f\n", conformance.precision);
        printf("  Generalization: %.3f\n", conformance.generalization);
        printf("  Simplicity: %.3f\n", conformance.simplicity);
    }
    
    // Performance analysis
    printf("\n=== Performance Analysis ===\n");
    PerformanceAnalysis* performance = pm7t_analyze_performance(event_log);
    if (performance) {
        printf("Performance analysis:\n");
        printf("  Total cases: %zu\n", performance->size);
        printf("  Average duration: %.2f ns\n", performance->avg_duration);
        printf("  Min duration: %.2f ns\n", performance->min_duration);
        printf("  Max duration: %.2f ns\n", performance->max_duration);
        printf("  Throughput: %.2f cases/second\n", performance->throughput);
        
        printf("  Individual case performance:\n");
        for (size_t i = 0; i < performance->size; i++) {
            CasePerformance* case_perf = &performance->cases[i];
            printf("    Case %u: duration=%.2f ns, activities=%u\n",
                   case_perf->case_id,
                   (double)case_perf->duration,
                   case_perf->num_activities);
        }
    }
    
    // Utility functions
    printf("\n=== Utility Information ===\n");
    printf("Unique cases: %u\n", pm7t_get_unique_cases(event_log));
    printf("Unique activities: %u\n", pm7t_get_unique_activities(event_log));
    printf("Unique resources: %u\n", pm7t_get_unique_resources(event_log));
    printf("Memory usage: %zu bytes\n", pm7t_get_memory_usage());
    
    // Cleanup
    if (performance) pm7t_destroy_performance_analysis(performance);
    if (process_stats) pm7t_destroy_process_stats(process_stats);
    if (heuristic_model) pm7t_destroy_process_model(heuristic_model);
    if (alpha_model) pm7t_destroy_process_model(alpha_model);
    pm7t_destroy_trace_log(trace_log);
    pm7t_destroy_event_log(event_log);
    
    printf("\nProcess mining analysis completed successfully!\n");
    return 0;
}
```

## Advanced Process Mining Example
```c
// Advanced process mining with filtering and analysis
void advanced_process_mining_example() {
    printf("=== Advanced Process Mining Example ===\n");
    
    // Create a larger event log
    EventLog* event_log = pm7t_create_event_log(10000);
    
    // Generate synthetic process data
    uint64_t base_time = time(NULL) * 1000000000ULL;
    uint32_t case_id = 1;
    
    // Generate multiple process variants
    for (int variant = 0; variant < 5; variant++) {
        for (int instance = 0; instance < 20; instance++) {
            uint64_t case_start = base_time + (case_id * 1000000);
            
            switch (variant) {
                case 0: // Standard process
                    pm7t_add_event(event_log, case_id, 1, case_start + 1000, 101, 50);
                    pm7t_add_event(event_log, case_id, 2, case_start + 2000, 102, 100);
                    pm7t_add_event(event_log, case_id, 3, case_start + 3000, 103, 75);
                    pm7t_add_event(event_log, case_id, 4, case_start + 4000, 104, 200);
                    break;
                    
                case 1: // Process with exception
                    pm7t_add_event(event_log, case_id, 1, case_start + 1000, 101, 50);
                    pm7t_add_event(event_log, case_id, 2, case_start + 2000, 102, 100);
                    pm7t_add_event(event_log, case_id, 5, case_start + 3000, 105, 150);
                    pm7t_add_event(event_log, case_id, 3, case_start + 4000, 103, 75);
                    pm7t_add_event(event_log, case_id, 4, case_start + 5000, 104, 200);
                    break;
                    
                case 2: // Process with loop
                    pm7t_add_event(event_log, case_id, 1, case_start + 1000, 101, 50);
                    pm7t_add_event(event_log, case_id, 2, case_start + 2000, 102, 100);
                    pm7t_add_event(event_log, case_id, 6, case_start + 3000, 106, 80);
                    pm7t_add_event(event_log, case_id, 2, case_start + 4000, 102, 100);
                    pm7t_add_event(event_log, case_id, 3, case_start + 5000, 103, 75);
                    pm7t_add_event(event_log, case_id, 4, case_start + 6000, 104, 200);
                    break;
                    
                case 3: // Process with parallel activities
                    pm7t_add_event(event_log, case_id, 1, case_start + 1000, 101, 50);
                    pm7t_add_event(event_log, case_id, 7, case_start + 2000, 107, 120);
                    pm7t_add_event(event_log, case_id, 8, case_start + 2000, 108, 90);
                    pm7t_add_event(event_log, case_id, 9, case_start + 3000, 109, 60);
                    pm7t_add_event(event_log, case_id, 4, case_start + 4000, 104, 200);
                    break;
                    
                case 4: // Process with cancellation
                    pm7t_add_event(event_log, case_id, 1, case_start + 1000, 101, 50);
                    pm7t_add_event(event_log, case_id, 2, case_start + 2000, 102, 100);
                    pm7t_add_event(event_log, case_id, 10, case_start + 3000, 110, 25);
                    break;
            }
            
            case_id++;
        }
    }
    
    printf("Generated %zu events across %u cases\n", 
           pm7t_get_event_count(event_log), case_id - 1);
    
    // Extract traces
    TraceLog* trace_log = pm7t_extract_traces(event_log);
    
    // Process discovery with different algorithms
    printf("\n=== Process Discovery Comparison ===\n");
    
    // Alpha algorithm
    ProcessModel* alpha_model = pm7t_discover_alpha_algorithm(trace_log);
    if (alpha_model) {
        ConformanceResult alpha_conformance = pm7t_check_conformance(alpha_model, trace_log);
        printf("Alpha Algorithm: %zu transitions, fitness=%.3f, precision=%.3f\n",
               alpha_model->size, alpha_conformance.fitness, alpha_conformance.precision);
    }
    
    // Heuristic miner with different thresholds
    for (double threshold = 0.1; threshold <= 0.9; threshold += 0.2) {
        ProcessModel* heuristic_model = pm7t_discover_heuristic_miner(trace_log, threshold);
        if (heuristic_model) {
            ConformanceResult heuristic_conformance = pm7t_check_conformance(heuristic_model, trace_log);
            printf("Heuristic Miner (%.1f): %zu transitions, fitness=%.3f, precision=%.3f\n",
                   threshold, heuristic_model->size, 
                   heuristic_conformance.fitness, heuristic_conformance.precision);
            pm7t_destroy_process_model(heuristic_model);
        }
    }
    
    // Performance analysis
    PerformanceAnalysis* performance = pm7t_analyze_performance(event_log);
    if (performance) {
        printf("\n=== Performance Insights ===\n");
        printf("Average case duration: %.2f seconds\n", 
               performance->avg_duration / 1000000000.0);
        printf("Process throughput: %.2f cases/hour\n", 
               performance->throughput * 3600);
        
        // Identify slow cases
        printf("Slow cases (>2x average):\n");
        for (size_t i = 0; i < performance->size; i++) {
            double duration_seconds = (double)performance->cases[i].duration / 1000000000.0;
            double avg_seconds = performance->avg_duration / 1000000000.0;
            if (duration_seconds > 2 * avg_seconds) {
                printf("  Case %u: %.2f seconds\n", 
                       performance->cases[i].case_id, duration_seconds);
            }
        }
    }
    
    // Cleanup
    if (performance) pm7t_destroy_performance_analysis(performance);
    if (alpha_model) pm7t_destroy_process_model(alpha_model);
    pm7t_destroy_trace_log(trace_log);
    pm7t_destroy_event_log(event_log);
}
```

## Tips
- Use appropriate process discovery algorithms based on your data characteristics.
- Monitor memory usage for large event logs.
- Filter event logs before analysis to focus on relevant cases.
- Consider temporal aspects when analyzing process performance.
- Use conformance checking to validate discovered process models. 