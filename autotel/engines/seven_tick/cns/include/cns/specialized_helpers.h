#ifndef CNS_SPECIALIZED_HELPERS_H
#define CNS_SPECIALIZED_HELPERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <immintrin.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// TEMPLATE ENGINE HELPERS (for cjinja.c)
// ============================================================================

// Template cache optimization
typedef struct
{
  char *template_name;
  char *compiled_content;
  uint64_t hash;
  uint64_t access_count;
  uint64_t last_access;
  size_t size;
} TemplateCacheEntry;

typedef struct
{
  TemplateCacheEntry *entries;
  size_t capacity;
  size_t count;
  atomic_uint64_t hits;
  atomic_uint64_t misses;
  uint64_t max_age_ns;
} TemplateCache;

// High-performance template cache
TemplateCache *cjinja_cache_create(size_t capacity);
void cjinja_cache_destroy(TemplateCache *cache);
TemplateCacheEntry *cjinja_cache_get(TemplateCache *cache, const char *name);
void cjinja_cache_put(TemplateCache *cache, const char *name, const char *content);
void cjinja_cache_evict_old(TemplateCache *cache);

// SIMD-optimized string operations
void cjinja_string_upper_simd(char *str, size_t len);
void cjinja_string_lower_simd(char *str, size_t len);
int cjinja_string_compare_simd(const char *a, const char *b, size_t len);
void cjinja_string_replace_simd(char *str, char old_char, char new_char, size_t len);

// String pool for template variables
typedef struct
{
  char *buffer;
  size_t used;
  size_t capacity;
  size_t block_size;
} StringPool;

StringPool *cjinja_string_pool_create(size_t capacity, size_t block_size);
char *cjinja_string_pool_alloc(StringPool *pool, const char *str);
void cjinja_string_pool_destroy(StringPool *pool);

// ============================================================================
// PERFORMANCE MONITORING HELPERS (for 7t_performance.c)
// ============================================================================

// High-precision cycle counting
static inline uint64_t cns_rdtsc(void)
{
  return __builtin_readcyclecounter();
}

// Nanosecond precision timing
static inline uint64_t cns_get_nanoseconds(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Performance validation for 7-tick compliance
typedef struct
{
  atomic_uint64_t total_operations;
  atomic_uint64_t compliant_operations;
  atomic_uint64_t total_cycles;
  atomic_uint64_t max_cycles;
  atomic_uint64_t min_cycles;
} S7TValidator;

S7TValidator *s7t_validator_create(void);
void s7t_validator_record(S7TValidator *validator, uint64_t cycles, const char *operation);
void s7t_validator_report(S7TValidator *validator);
void s7t_validator_destroy(S7TValidator *validator);

// Memory access pattern tracking
typedef struct
{
  void *address;
  size_t size;
  uint64_t timestamp;
  bool is_write;
  bool is_sequential;
} MemoryAccess;

typedef struct
{
  MemoryAccess *accesses;
  size_t capacity;
  size_t count;
  atomic_uint64_t cache_hits;
  atomic_uint64_t cache_misses;
} MemoryTracker;

MemoryTracker *s7t_memory_tracker_create(size_t capacity);
void s7t_track_memory_access(MemoryTracker *tracker, void *addr, size_t size, bool is_write);
void s7t_memory_tracker_report(MemoryTracker *tracker);
void s7t_memory_tracker_destroy(MemoryTracker *tracker);

// ============================================================================
// PROCESS MINING HELPERS (for 7t_process_mining.c)
// ============================================================================

// SIMD-optimized matrix operations
void pm_matrix_multiply_simd(const double *a, const double *b, double *result,
                             size_t rows_a, size_t cols_a, size_t cols_b);
void pm_matrix_transpose_simd(const double *input, double *output, size_t rows, size_t cols);
void pm_vector_add_simd(const double *a, const double *b, double *result, size_t size);
void pm_vector_multiply_simd(const double *a, const double *b, double *result, size_t size);

// Event set operations with bit vectors
typedef struct
{
  uint64_t *bits;
  size_t word_count;
  size_t max_events;
} EventSet;

EventSet *pm_event_set_create(size_t max_events);
void pm_event_set_add(EventSet *set, uint32_t event_id);
void pm_event_set_remove(EventSet *set, uint32_t event_id);
bool pm_event_set_contains(EventSet *set, uint32_t event_id);
EventSet *pm_event_set_intersection(EventSet *a, EventSet *b);
EventSet *pm_event_set_union(EventSet *a, EventSet *b);
size_t pm_event_set_popcount(EventSet *set);
void pm_event_set_destroy(EventSet *set);

// Process mining algorithm cache
typedef struct
{
  char *algorithm_name;
  void *parameters;
  double *result_cache;
  uint64_t last_used;
  size_t cache_size;
} AlgorithmCache;

typedef struct
{
  AlgorithmCache *entries;
  size_t capacity;
  size_t count;
} AlgorithmCacheManager;

AlgorithmCacheManager *pm_algorithm_cache_create(size_t capacity);
void pm_algorithm_cache_put(AlgorithmCacheManager *cache, const char *name,
                            void *params, double *results, size_t size);
double *pm_algorithm_cache_get(AlgorithmCacheManager *cache, const char *name, void *params);
void pm_algorithm_cache_destroy(AlgorithmCacheManager *cache);

// ============================================================================
// TPOT PROCESS MINING HELPERS (for pm7t.c)
// ============================================================================

// Dataset management with SIMD optimization
typedef struct
{
  double *data;
  uint32_t *labels;
  uint32_t num_samples;
  uint32_t num_features;
  uint64_t *feature_mask;
  uint64_t *sample_mask;
  bool is_aligned;
} Dataset7T;

Dataset7T *tpot_dataset_create(uint32_t samples, uint32_t features);
void tpot_dataset_destroy(Dataset7T *dataset);
void tpot_dataset_normalize_simd(Dataset7T *dataset);
void tpot_dataset_standardize_simd(Dataset7T *dataset);

// Pipeline optimization with lock-free operations
typedef struct
{
  uint32_t pipeline_id;
  uint32_t num_steps;
  void **steps;
  double fitness_score;
  uint64_t evaluation_time_ns;
  atomic_uint32_t ref_count;
} Pipeline7T;

typedef struct
{
  Pipeline7T **pipelines;
  size_t capacity;
  atomic_uint64_t head;
  atomic_uint64_t tail;
} PipelineQueue;

PipelineQueue *tpot_pipeline_queue_create(size_t capacity);
int tpot_pipeline_queue_push(PipelineQueue *queue, Pipeline7T *pipeline);
Pipeline7T *tpot_pipeline_queue_pop(PipelineQueue *queue);
void tpot_pipeline_queue_destroy(PipelineQueue *queue);

// Genetic algorithm optimization
typedef struct
{
  Pipeline7T **population;
  uint32_t population_size;
  uint32_t generation;
  uint32_t best_pipeline_id;
  double best_fitness;
  atomic_uint64_t evaluations;
} OptimizationEngine7T;

OptimizationEngine7T *tpot_optimization_engine_create(uint32_t population_size);
void tpot_optimization_engine_destroy(OptimizationEngine7T *engine);
void tpot_optimization_engine_evolve(OptimizationEngine7T *engine, Dataset7T *dataset);

// ============================================================================
// SQL DOMAIN HELPERS (for sql_domain.c)
// ============================================================================

// Cache-aligned table structures
typedef struct
{
  char name[64];
  uint32_t id;
  uint32_t column_count;
  uint32_t row_count;
  uint32_t max_rows;
  void *column_data[S7T_SQL_MAX_COLUMNS];
  uint64_t *column_masks[S7T_SQL_MAX_COLUMNS];
  uint64_t *row_masks;
} S7TTable;

typedef struct
{
  char name[32];
  uint32_t type;
  uint32_t offset;
  uint32_t size;
  void *data;
  uint64_t *bit_mask;
} S7TColumn;

// Table management with 7-tick optimization
S7TTable *sql_table_create(const char *name, uint32_t max_rows);
void sql_table_destroy(S7TTable *table);
S7TColumn *sql_column_create(S7TTable *table, const char *name, uint32_t type);
void sql_column_destroy(S7TColumn *column);

// SIMD-optimized column operations
void sql_column_filter_int32_simd(S7TColumn *column, int32_t value, uint64_t *result_mask);
void sql_column_filter_float32_simd(S7TColumn *column, float value, uint64_t *result_mask);
void sql_column_sort_int32_simd(S7TColumn *column);
void sql_column_sort_float32_simd(S7TColumn *column);

// Query execution optimization
typedef struct
{
  S7TTable *table;
  uint64_t *row_mask;
  uint32_t result_count;
  uint64_t execution_time_ns;
} QueryResult;

QueryResult *sql_query_execute_simd(S7TTable *table, const char *query);
void sql_query_result_destroy(QueryResult *result);

// ============================================================================
// RUNTIME SYSTEM HELPERS (for seven_t_runtime.c)
// ============================================================================

// Arena allocator for runtime allocations
typedef struct
{
  char *buffer;
  size_t size;
  size_t used;
  size_t alignment;
} ArenaAllocator;

ArenaAllocator *runtime_arena_create(size_t size, size_t alignment);
void *runtime_arena_alloc(ArenaAllocator *arena, size_t size);
void runtime_arena_reset(ArenaAllocator *arena);
void runtime_arena_destroy(ArenaAllocator *arena);

// Lock-free object pool
typedef struct
{
  void **objects;
  size_t capacity;
  atomic_uint64_t head;
  atomic_uint64_t tail;
  void *(*create_func)(void);
  void (*destroy_func)(void *);
} ObjectPool;

ObjectPool *runtime_object_pool_create(size_t capacity,
                                       void *(*create)(void),
                                       void (*destroy)(void *));
void *runtime_object_pool_acquire(ObjectPool *pool);
void runtime_object_pool_release(ObjectPool *pool, void *obj);
void runtime_object_pool_destroy(ObjectPool *pool);

// Runtime performance monitoring
typedef struct
{
  atomic_uint64_t allocations;
  atomic_uint64_t deallocations;
  atomic_uint64_t total_memory;
  atomic_uint64_t peak_memory;
  atomic_uint64_t cache_hits;
  atomic_uint64_t cache_misses;
} RuntimeStats;

RuntimeStats *runtime_stats_create(void);
void runtime_stats_record_allocation(RuntimeStats *stats, size_t size);
void runtime_stats_record_deallocation(RuntimeStats *stats, size_t size);
void runtime_stats_record_cache_access(RuntimeStats *stats, bool hit);
void runtime_stats_report(RuntimeStats *stats);
void runtime_stats_destroy(RuntimeStats *stats);

// ============================================================================
// ML COMMANDS HELPERS (for cmd_ml.c)
// ============================================================================

// SIMD-optimized matrix operations for ML
void ml_matrix_multiply_simd(const double *a, const double *b, double *result,
                             size_t rows_a, size_t cols_a, size_t cols_b);
void ml_matrix_inverse_simd(double *matrix, size_t size);
void ml_vector_dot_product_simd(const double *a, const double *b, double *result, size_t size);
void ml_vector_normalize_simd(double *vector, size_t size);

// ML algorithm registry with caching
typedef struct
{
  char *name;
  uint32_t id;
  void *(*fit_func)(const double *data, const uint32_t *labels, size_t samples, size_t features);
  double (*predict_func)(void *model, const double *features, size_t feature_count);
  void (*destroy_func)(void *model);
} MLAlgorithm;

typedef struct
{
  MLAlgorithm *algorithms;
  size_t capacity;
  size_t count;
} MLAlgorithmRegistry;

MLAlgorithmRegistry *ml_algorithm_registry_create(size_t capacity);
void ml_algorithm_register(MLAlgorithmRegistry *registry, const char *name,
                           void *(*fit)(const double *, const uint32_t *, size_t, size_t),
                           double (*predict)(void *, const double *, size_t),
                           void (*destroy)(void *));
MLAlgorithm *ml_algorithm_get(MLAlgorithmRegistry *registry, const char *name);
void ml_algorithm_registry_destroy(MLAlgorithmRegistry *registry);

// ML model cache
typedef struct
{
  char *model_key;
  void *model;
  uint64_t last_used;
  size_t memory_usage;
} MLModelCache;

typedef struct
{
  MLModelCache *entries;
  size_t capacity;
  size_t count;
  size_t max_memory;
  size_t current_memory;
} MLModelCacheManager;

MLModelCacheManager *ml_model_cache_create(size_t capacity, size_t max_memory);
void ml_model_cache_put(MLModelCacheManager *cache, const char *key, void *model, size_t memory);
void *ml_model_cache_get(MLModelCacheManager *cache, const char *key);
void ml_model_cache_evict_lru(MLModelCacheManager *cache);
void ml_model_cache_destroy(MLModelCacheManager *cache);

// ============================================================================
// PERFORMANCE ANALYSIS HELPERS (for s7t_perf.c)
// ============================================================================

// Cache simulation for performance analysis
typedef struct
{
  size_t size_kb;
  size_t associativity;
  size_t line_size;
  uint64_t *tags;
  uint64_t *lru_counters;
  atomic_uint64_t hits;
  atomic_uint64_t misses;
} CacheSimulator;

CacheSimulator *perf_cache_simulator_create(size_t size_kb, size_t associativity);
bool perf_cache_access(CacheSimulator *cache, void *address);
void perf_cache_get_stats(CacheSimulator *cache, uint64_t *hits, uint64_t *misses);
void perf_cache_simulator_destroy(CacheSimulator *cache);

// Branch prediction analysis
typedef struct
{
  char *branch_name;
  uint64_t total_branches;
  uint64_t taken_branches;
  uint64_t mispredictions;
  double prediction_rate;
} BranchInfo;

typedef struct
{
  BranchInfo *branches;
  size_t capacity;
  size_t count;
} BranchAnalyzer;

BranchAnalyzer *perf_branch_analyzer_create(size_t capacity);
void perf_branch_record(BranchAnalyzer *analyzer, const char *name, bool taken, bool predicted);
void perf_branch_analyzer_report(BranchAnalyzer *analyzer);
void perf_branch_analyzer_destroy(BranchAnalyzer *analyzer);

// Performance validation suite
typedef struct
{
  char *test_name;
  bool (*test_func)(void *context);
  void *context;
  uint64_t max_cycles;
  uint64_t max_memory;
} PerformanceTest;

typedef struct
{
  PerformanceTest *tests;
  size_t capacity;
  size_t count;
} PerformanceTestSuite;

PerformanceTestSuite *perf_test_suite_create(size_t capacity);
void perf_test_add(PerformanceTestSuite *suite, const char *name,
                   bool (*test)(void *), void *context, uint64_t max_cycles);
bool perf_test_suite_run(PerformanceTestSuite *suite);
void perf_test_suite_destroy(PerformanceTestSuite *suite);

// ============================================================================
// TELEMETRY TESTING HELPERS (for test_telemetry7t.c)
// ============================================================================

// High-performance test data generation
typedef struct
{
  char *trace_id;
  char *span_id;
  char *operation_name;
  uint64_t start_time;
  uint64_t end_time;
  uint32_t attribute_count;
  char **attribute_keys;
  char **attribute_values;
} TestSpan;

typedef struct
{
  TestSpan *spans;
  size_t capacity;
  size_t count;
  uint64_t generation_time_ns;
} TestDataGenerator;

TestDataGenerator *telemetry_test_data_generator_create(size_t capacity);
TestSpan *telemetry_generate_test_span(TestDataGenerator *generator, const char *operation);
void telemetry_test_data_generator_destroy(TestDataGenerator *generator);

// Test performance validation
typedef struct
{
  uint64_t min_latency_ns;
  uint64_t max_latency_ns;
  uint64_t avg_latency_ns;
  uint64_t total_operations;
  uint64_t successful_operations;
  double success_rate;
} TestPerformanceMetrics;

TestPerformanceMetrics *telemetry_test_performance_create(void);
void telemetry_test_performance_record(TestPerformanceMetrics *metrics,
                                       uint64_t latency_ns, bool success);
void telemetry_test_performance_report(TestPerformanceMetrics *metrics);
void telemetry_test_performance_destroy(TestPerformanceMetrics *metrics);

// Stress testing utilities
typedef struct
{
  uint32_t num_threads;
  uint32_t operations_per_thread;
  uint64_t max_duration_ns;
  atomic_uint64_t completed_operations;
  atomic_uint64_t failed_operations;
} StressTestConfig;

StressTestConfig *telemetry_stress_test_create(uint32_t threads, uint32_t ops_per_thread, uint64_t max_duration);
void telemetry_stress_test_run(StressTestConfig *config, void (*operation)(void *), void *context);
void telemetry_stress_test_report(StressTestConfig *config);
void telemetry_stress_test_destroy(StressTestConfig *config);

// ============================================================================
// UTILITY MACROS FOR ALL COMPONENTS
// ============================================================================

// Branch prediction for all components
#define CNS_LIKELY(x) __builtin_expect(!!(x), 1)
#define CNS_UNLIKELY(x) __builtin_expect(!!(x), 0)

// Function attributes for optimization
#define CNS_HOT __attribute__((hot))
#define CNS_COLD __attribute__((cold))
#define CNS_PURE __attribute__((pure))
#define CNS_CONST __attribute__((const))
#define CNS_INLINE inline __attribute__((always_inline))

// Memory alignment
#define CNS_CACHE_ALIGNED __attribute__((aligned(64)))
#define CNS_VECTOR_ALIGNED __attribute__((aligned(32)))

// Performance measurement macros
#define CNS_PERFORMANCE_START(counters) \
  uint64_t cns_start_time = cns_get_nanoseconds()

#define CNS_PERFORMANCE_END(counters, cache_hit)               \
  do                                                           \
  {                                                            \
    uint64_t cns_end_time = cns_get_nanoseconds();             \
    uint64_t cns_duration = cns_end_time - cns_start_time;     \
    if (counters)                                              \
    {                                                          \
      atomic_fetch_add(&counters->total_operations, 1);        \
      atomic_fetch_add(&counters->total_cycles, cns_duration); \
      if (cache_hit)                                           \
        atomic_fetch_add(&counters->cache_hits, 1);            \
      else                                                     \
        atomic_fetch_add(&counters->cache_misses, 1);          \
    }                                                          \
  } while (0)

// SIMD batch processing
#ifdef __AVX512F__
#define CNS_VECTOR_WIDTH 8
#define CNS_VECTOR_TYPE __m512i
#elif defined(__AVX2__)
#define CNS_VECTOR_WIDTH 4
#define CNS_VECTOR_TYPE __m256i
#elif defined(__ARM_NEON)
#define CNS_VECTOR_WIDTH 4
#define CNS_VECTOR_TYPE uint32x4_t
#else
#define CNS_VECTOR_WIDTH 1
#define CNS_VECTOR_TYPE uint32_t
#endif

#define CNS_SIMD_BATCH_START(count) \
  size_t cns_simd_count = (count) & ~(CNS_VECTOR_WIDTH - 1)

#define CNS_SIMD_BATCH_END(count) \
  for (size_t cns_i = cns_simd_count; cns_i < (count); cns_i++)

#endif // CNS_SPECIALIZED_HELPERS_H