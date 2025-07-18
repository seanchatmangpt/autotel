# Pattern: Efficient Data Loading

## Description
This pattern demonstrates how to efficiently load large datasets into the 7T Engine, including bulk triple insertion, data validation, and progress monitoring for production data pipelines.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Data loading statistics
typedef struct {
    size_t total_triples;
    size_t loaded_triples;
    size_t failed_triples;
    size_t duplicate_triples;
    time_t start_time;
    time_t last_progress;
} LoadingStats;

// Initialize loading statistics
void init_loading_stats(LoadingStats* stats) {
    stats->total_triples = 0;
    stats->loaded_triples = 0;
    stats->failed_triples = 0;
    stats->duplicate_triples = 0;
    stats->start_time = time(NULL);
    stats->last_progress = stats->start_time;
}

// Progress reporting
void report_progress(LoadingStats* stats) {
    time_t now = time(NULL);
    if (now - stats->last_progress >= 5) {  // Report every 5 seconds
        double progress = (double)stats->loaded_triples / stats->total_triples * 100.0;
        double rate = (double)stats->loaded_triples / (now - stats->start_time);
        printf("Progress: %.1f%% (%zu/%zu) - Rate: %.0f triples/sec\n", 
               progress, stats->loaded_triples, stats->total_triples, rate);
        stats->last_progress = now;
    }
}

// Bulk triple loading with validation
int load_triples_bulk(S7TEngine* engine, const char* filename, LoadingStats* stats) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return -1;
    }
    
    char line[1024];
    uint32_t s, p, o;
    int line_count = 0;
    
    // First pass: count lines
    while (fgets(line, sizeof(line), file)) {
        stats->total_triples++;
    }
    rewind(file);
    
    printf("Loading %zu triples from %s\n", stats->total_triples, filename);
    
    // Second pass: load data
    while (fgets(line, sizeof(line), file)) {
        line_count++;
        
        // Parse triple (format: subject predicate object)
        if (sscanf(line, "%u %u %u", &s, &p, &o) != 3) {
            fprintf(stderr, "Invalid triple format at line %d: %s", line_count, line);
            stats->failed_triples++;
            continue;
        }
        
        // Validate bounds
        if (s >= engine->max_subjects || p >= engine->max_predicates || o >= engine->max_objects) {
            fprintf(stderr, "Triple out of bounds at line %d: %u %u %u\n", line_count, s, p, o);
            stats->failed_triples++;
            continue;
        }
        
        // Add triple (duplicates are handled automatically)
        s7t_add_triple(engine, s, p, o);
        stats->loaded_triples++;
        
        // Report progress
        report_progress(stats);
    }
    
    fclose(file);
    
    // Final report
    time_t total_time = time(NULL) - stats->start_time;
    printf("Loading completed in %ld seconds\n", total_time);
    printf("Loaded: %zu, Failed: %zu, Duplicates: %zu\n", 
           stats->loaded_triples, stats->failed_triples, stats->duplicate_triples);
    
    return 0;
}

// Memory-efficient batch loading
int load_triples_batch(S7TEngine* engine, const char* filename, size_t batch_size) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    uint32_t* subjects = malloc(batch_size * sizeof(uint32_t));
    uint32_t* predicates = malloc(batch_size * sizeof(uint32_t));
    uint32_t* objects = malloc(batch_size * sizeof(uint32_t));
    
    if (!subjects || !predicates || !objects) {
        free(subjects);
        free(predicates);
        free(objects);
        fclose(file);
        return -1;
    }
    
    char line[1024];
    size_t batch_count = 0;
    size_t total_loaded = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%u %u %u", &subjects[batch_count], 
                   &predicates[batch_count], &objects[batch_count]) == 3) {
            batch_count++;
            
            if (batch_count >= batch_size) {
                // Process batch
                for (size_t i = 0; i < batch_count; i++) {
                    s7t_add_triple(engine, subjects[i], predicates[i], objects[i]);
                }
                total_loaded += batch_count;
                batch_count = 0;
                printf("Loaded batch: %zu total\n", total_loaded);
            }
        }
    }
    
    // Process remaining triples
    if (batch_count > 0) {
        for (size_t i = 0; i < batch_count; i++) {
            s7t_add_triple(engine, subjects[i], predicates[i], objects[i]);
        }
        total_loaded += batch_count;
    }
    
    free(subjects);
    free(predicates);
    free(objects);
    fclose(file);
    
    printf("Total loaded: %zu triples\n", total_loaded);
    return 0;
}

int main() {
    S7TEngine* engine = s7t_create(100000, 1000, 100000);
    if (!engine) {
        fprintf(stderr, "Failed to create engine\n");
        return 1;
    }
    
    LoadingStats stats;
    init_loading_stats(&stats);
    
    // Load data with progress monitoring
    if (load_triples_bulk(engine, "data.nt", &stats) != 0) {
        fprintf(stderr, "Failed to load data\n");
        s7t_destroy(engine);
        return 1;
    }
    
    // Alternative: batch loading for very large datasets
    // load_triples_batch(engine, "large_data.nt", 10000);
    
    s7t_destroy(engine);
    return 0;
}
```

## Tips
- Use batch loading for datasets larger than available memory.
- Monitor progress for long-running loads.
- Validate data format before loading.
- Consider parallel loading for very large datasets. 