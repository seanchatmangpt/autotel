#ifndef PM7T_H
#define PM7T_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


typedef struct
{
  uint32_t case_id;
  uint32_t activity_id;
  uint64_t timestamp;
  uint32_t resource_id;
  uint32_t cost;
} Event;

typedef struct
{
  Event *events;
  size_t capacity;
  size_t size;
} EventLog;

typedef struct
{
  uint32_t *activities;
  size_t size;
} Trace;

typedef struct
{
  Trace *traces;
  size_t capacity;
  size_t size;
} TraceLog;

typedef struct
{
  uint32_t from_activity;
  uint32_t to_activity;
  uint32_t frequency;
  double probability;
} Transition;

typedef struct
{
  Transition *transitions;
  size_t capacity;
  size_t size;
  uint32_t num_activities;
} ProcessModel;

typedef struct
{
  uint32_t activity_id;
  uint32_t frequency;
  double avg_duration;
  double min_duration;
  double max_duration;
} ActivityStats;

typedef struct
{
  ActivityStats *activities;
  size_t capacity;
  size_t size;
} ProcessStats;

// Event log management
EventLog *pm7t_create_event_log(size_t initial_capacity);
void pm7t_destroy_event_log(EventLog *log);
int pm7t_add_event(EventLog *log, uint32_t case_id, uint32_t activity_id,
                   uint64_t timestamp, uint32_t resource_id, uint32_t cost);
Event *pm7t_get_event(EventLog *log, size_t index);
size_t pm7t_get_event_count(EventLog *log);

// Trace extraction
TraceLog *pm7t_extract_traces(EventLog *event_log);
void pm7t_destroy_trace_log(TraceLog *trace_log);
Trace *pm7t_get_trace(TraceLog *trace_log, size_t index);
size_t pm7t_get_trace_count(TraceLog *trace_log);

// Process discovery algorithms
ProcessModel *pm7t_discover_alpha_algorithm(TraceLog *trace_log);
ProcessModel *pm7t_discover_heuristic_miner(TraceLog *trace_log, double dependency_threshold);
ProcessModel *pm7t_discover_inductive_miner(TraceLog *trace_log);
void pm7t_destroy_process_model(ProcessModel *model);

// Process analysis
ProcessStats *pm7t_analyze_process(EventLog *event_log);
void pm7t_destroy_process_stats(ProcessStats *stats);
double pm7t_calculate_fitness(ProcessModel *model, TraceLog *trace_log);
double pm7t_calculate_precision(ProcessModel *model, TraceLog *trace_log);
double pm7t_calculate_generalization(ProcessModel *model, TraceLog *trace_log);
double pm7t_calculate_simplicity(ProcessModel *model);

// Conformance checking
typedef struct
{
  double fitness;
  double precision;
  double generalization;
  double simplicity;
} ConformanceResult;

ConformanceResult pm7t_check_conformance(ProcessModel *model, TraceLog *trace_log);

// Performance analysis
typedef struct
{
  uint32_t case_id;
  uint64_t start_time;
  uint64_t end_time;
  uint64_t duration;
  uint32_t num_activities;
} CasePerformance;

typedef struct
{
  CasePerformance *cases;
  size_t capacity;
  size_t size;
  double avg_duration;
  double min_duration;
  double max_duration;
  double throughput;
} PerformanceAnalysis;

PerformanceAnalysis *pm7t_analyze_performance(EventLog *event_log);
void pm7t_destroy_performance_analysis(PerformanceAnalysis *analysis);

// Bottleneck analysis
typedef struct
{
  uint32_t activity_id;
  double avg_waiting_time;
  double avg_processing_time;
  double utilization;
  uint32_t queue_length;
} BottleneckInfo;

typedef struct
{
  BottleneckInfo *bottlenecks;
  size_t capacity;
  size_t size;
} BottleneckAnalysis;

BottleneckAnalysis *pm7t_analyze_bottlenecks(EventLog *event_log);
void pm7t_destroy_bottleneck_analysis(BottleneckAnalysis *analysis);

// Variant analysis
typedef struct
{
  Trace *trace;
  uint32_t frequency;
  double percentage;
} Variant;

typedef struct
{
  Variant *variants;
  size_t capacity;
  size_t size;
  uint32_t total_cases;
} VariantAnalysis;

VariantAnalysis *pm7t_analyze_variants(TraceLog *trace_log);
void pm7t_destroy_variant_analysis(VariantAnalysis *analysis);

// Social network analysis
typedef struct
{
  uint32_t resource_id;
  uint32_t handover_count;
  double centrality;
} ResourceNode;

typedef struct
{
  ResourceNode *resources;
  size_t capacity;
  size_t size;
} SocialNetwork;

SocialNetwork *pm7t_analyze_social_network(EventLog *event_log);
void pm7t_destroy_social_network(SocialNetwork *network);

// Data filtering
EventLog *pm7t_filter_by_case(EventLog *event_log, uint32_t case_id);
EventLog *pm7t_filter_by_activity(EventLog *event_log, uint32_t activity_id);
EventLog *pm7t_filter_by_time_range(EventLog *event_log, uint64_t start_time, uint64_t end_time);
EventLog *pm7t_filter_by_resource(EventLog *event_log, uint32_t resource_id);

// Data export/import
int pm7t_export_csv(EventLog *event_log, const char *filename);
EventLog *pm7t_import_csv(const char *filename);
int pm7t_export_xes(EventLog *event_log, const char *filename);
EventLog *pm7t_import_xes(const char *filename);

// Utility functions
void pm7t_sort_events_by_timestamp(EventLog *event_log);
void pm7t_sort_events_by_case(EventLog *event_log);
uint32_t pm7t_get_unique_cases(EventLog *event_log);
uint32_t pm7t_get_unique_activities(EventLog *event_log);
uint32_t pm7t_get_unique_resources(EventLog *event_log);

// Memory management
void pm7t_set_memory_limit(size_t bytes);
size_t pm7t_get_memory_usage(void);

#endif // PM7T_H