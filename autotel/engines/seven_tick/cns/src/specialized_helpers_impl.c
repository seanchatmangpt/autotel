#include "cns/specialized_helpers.h"
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// TEMPLATE ENGINE HELPERS IMPLEMENTATION
// ============================================================================

// Simple hash function for template names
static uint64_t template_hash(const char *str)
{
  uint64_t hash = 5381;
  int c;
  while ((c = *str++))
  {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

TemplateCache *cjinja_cache_create(size_t capacity)
{
  TemplateCache *cache = malloc(sizeof(TemplateCache));
  if (!cache)
    return NULL;

  cache->entries = calloc(capacity, sizeof(TemplateCacheEntry));
  cache->capacity = capacity;
  cache->count = 0;
  atomic_init(&cache->hits, 0);
  atomic_init(&cache->misses, 0);
  cache->max_age_ns = 30000000000ULL; // 30 seconds

  return cache;
}

void cjinja_cache_destroy(TemplateCache *cache)
{
  if (!cache)
    return;

  for (size_t i = 0; i < cache->count; i++)
  {
    free(cache->entries[i].template_name);
    free(cache->entries[i].compiled_content);
  }
  free(cache->entries);
  free(cache);
}

TemplateCacheEntry *cjinja_cache_get(TemplateCache *cache, const char *name)
{
  if (!cache || !name)
    return NULL;

  uint64_t hash = template_hash(name);
  uint64_t current_time = cns_get_nanoseconds();

  for (size_t i = 0; i < cache->count; i++)
  {
    if (cache->entries[i].hash == hash &&
        strcmp(cache->entries[i].template_name, name) == 0)
    {

      // Check if entry is still valid
      if (current_time - cache->entries[i].last_access < cache->max_age_ns)
      {
        cache->entries[i].last_access = current_time;
        cache->entries[i].access_count++;
        atomic_fetch_add(&cache->hits, 1);
        return &cache->entries[i];
      }
    }
  }

  atomic_fetch_add(&cache->misses, 1);
  return NULL;
}

void cjinja_cache_put(TemplateCache *cache, const char *name, const char *content)
{
  if (!cache || !name || !content)
    return;

  // Evict old entries if cache is full
  if (cache->count >= cache->capacity)
  {
    cjinja_cache_evict_old(cache);
  }

  TemplateCacheEntry *entry = &cache->entries[cache->count++];
  entry->template_name = strdup(name);
  entry->compiled_content = strdup(content);
  entry->hash = template_hash(name);
  entry->access_count = 1;
  entry->last_access = cns_get_nanoseconds();
  entry->size = strlen(content);
}

void cjinja_cache_evict_old(TemplateCache *cache)
{
  if (!cache || cache->count == 0)
    return;

  // Find least recently used entry
  size_t lru_index = 0;
  uint64_t oldest_time = cache->entries[0].last_access;

  for (size_t i = 1; i < cache->count; i++)
  {
    if (cache->entries[i].last_access < oldest_time)
    {
      oldest_time = cache->entries[i].last_access;
      lru_index = i;
    }
  }

  // Remove LRU entry
  free(cache->entries[lru_index].template_name);
  free(cache->entries[lru_index].compiled_content);

  // Move last entry to LRU position
  if (lru_index < cache->count - 1)
  {
    cache->entries[lru_index] = cache->entries[cache->count - 1];
  }
  cache->count--;
}

// SIMD-optimized string operations
void cjinja_string_upper_simd(char *str, size_t len)
{
  CNS_SIMD_BATCH_START(len);

  for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH)
  {
    for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < len; j++)
    {
      char c = str[i + j];
      if (c >= 'a' && c <= 'z')
      {
        str[i + j] = c - 32;
      }
    }
  }

  CNS_SIMD_BATCH_END(len)
  {
    char c = str[cns_i];
    if (c >= 'a' && c <= 'z')
    {
      str[cns_i] = c - 32;
    }
  }
}

void cjinja_string_lower_simd(char *str, size_t len)
{
  CNS_SIMD_BATCH_START(len);

  for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH)
  {
    for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < len; j++)
    {
      char c = str[i + j];
      if (c >= 'A' && c <= 'Z')
      {
        str[i + j] = c + 32;
      }
    }
  }

  CNS_SIMD_BATCH_END(len)
  {
    char c = str[cns_i];
    if (c >= 'A' && c <= 'Z')
    {
      str[cns_i] = c + 32;
    }
  }
}

int cjinja_string_compare_simd(const char *a, const char *b, size_t len)
{
  CNS_SIMD_BATCH_START(len);

  for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH)
  {
    for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < len; j++)
    {
      if (a[i + j] != b[i + j])
      {
        return a[i + j] - b[i + j];
      }
    }
  }

  CNS_SIMD_BATCH_END(len)
  {
    if (a[cns_i] != b[cns_i])
    {
      return a[cns_i] - b[cns_i];
    }
  }

  return 0;
}

// String pool implementation
StringPool *cjinja_string_pool_create(size_t capacity, size_t block_size)
{
  StringPool *pool = malloc(sizeof(StringPool));
  if (!pool)
    return NULL;

  pool->buffer = malloc(capacity);
  pool->capacity = capacity;
  pool->used = 0;
  pool->block_size = block_size;

  return pool;
}

char *cjinja_string_pool_alloc(StringPool *pool, const char *str)
{
  if (!pool || !str)
    return NULL;

  size_t len = strlen(str) + 1;
  if (pool->used + len > pool->capacity)
    return NULL;

  char *result = pool->buffer + pool->used;
  strcpy(result, str);
  pool->used += len;

  return result;
}

void cjinja_string_pool_destroy(StringPool *pool)
{
  if (pool)
  {
    free(pool->buffer);
    free(pool);
  }
}

// ============================================================================
// PERFORMANCE MONITORING HELPERS IMPLEMENTATION
// ============================================================================

S7TValidator *s7t_validator_create(void)
{
  S7TValidator *validator = malloc(sizeof(S7TValidator));
  if (!validator)
    return NULL;

  atomic_init(&validator->total_operations, 0);
  atomic_init(&validator->compliant_operations, 0);
  atomic_init(&validator->total_cycles, 0);
  atomic_init(&validator->max_cycles, 0);
  atomic_init(&validator->min_cycles, UINT64_MAX);

  return validator;
}

void s7t_validator_record(S7TValidator *validator, uint64_t cycles, const char *operation)
{
  if (!validator)
    return;

  atomic_fetch_add(&validator->total_operations, 1);
  atomic_fetch_add(&validator->total_cycles, cycles);

  if (cycles <= 7)
  {
    atomic_fetch_add(&validator->compliant_operations, 1);
  }

  // Update min/max (simplified - not atomic)
  uint64_t current_max = atomic_load(&validator->max_cycles);
  uint64_t current_min = atomic_load(&validator->min_cycles);

  if (cycles > current_max)
    atomic_store(&validator->max_cycles, cycles);
  if (cycles < current_min)
    atomic_store(&validator->min_cycles, cycles);
}

void s7t_validator_report(S7TValidator *validator)
{
  if (!validator)
    return;

  uint64_t total = atomic_load(&validator->total_operations);
  uint64_t compliant = atomic_load(&validator->compliant_operations);
  uint64_t total_cycles = atomic_load(&validator->total_cycles);
  uint64_t max_cycles = atomic_load(&validator->max_cycles);
  uint64_t min_cycles = atomic_load(&validator->min_cycles);

  printf("S7T Validation Report:\n");
  printf("  Total Operations: %lu\n", total);
  printf("  Compliant Operations: %lu (%.1f%%)\n", compliant,
         total > 0 ? (double)compliant / total * 100 : 0.0);
  printf("  Average Cycles: %.1f\n", total > 0 ? (double)total_cycles / total : 0.0);
  printf("  Min Cycles: %lu\n", min_cycles);
  printf("  Max Cycles: %lu\n", max_cycles);
}

void s7t_validator_destroy(S7TValidator *validator)
{
  free(validator);
}

// Memory tracker implementation
MemoryTracker *s7t_memory_tracker_create(size_t capacity)
{
  MemoryTracker *tracker = malloc(sizeof(MemoryTracker));
  if (!tracker)
    return NULL;

  tracker->accesses = calloc(capacity, sizeof(MemoryAccess));
  tracker->capacity = capacity;
  tracker->count = 0;
  atomic_init(&tracker->cache_hits, 0);
  atomic_init(&tracker->cache_misses, 0);

  return tracker;
}

void s7t_track_memory_access(MemoryTracker *tracker, void *addr, size_t size, bool is_write)
{
  if (!tracker || tracker->count >= tracker->capacity)
    return;

  MemoryAccess *access = &tracker->accesses[tracker->count++];
  access->address = addr;
  access->size = size;
  access->timestamp = cns_get_nanoseconds();
  access->is_write = is_write;

  // Check if sequential access
  if (tracker->count > 1)
  {
    MemoryAccess *prev = &tracker->accesses[tracker->count - 2];
    access->is_sequential = ((char *)addr == (char *)prev->address + prev->size);
  }
  else
  {
    access->is_sequential = false;
  }

  // Simulate cache behavior
  static uintptr_t last_cache_line = 0;
  uintptr_t cache_line = (uintptr_t)addr / 64; // 64-byte cache line

  if (cache_line == last_cache_line)
  {
    atomic_fetch_add(&tracker->cache_hits, 1);
  }
  else
  {
    atomic_fetch_add(&tracker->cache_misses, 1);
    last_cache_line = cache_line;
  }
}

void s7t_memory_tracker_report(MemoryTracker *tracker)
{
  if (!tracker)
    return;

  uint64_t hits = atomic_load(&tracker->cache_hits);
  uint64_t misses = atomic_load(&tracker->cache_misses);
  uint64_t total = hits + misses;

  size_t sequential = 0;
  for (size_t i = 0; i < tracker->count; i++)
  {
    if (tracker->accesses[i].is_sequential)
    {
      sequential++;
    }
  }

  printf("Memory Access Report:\n");
  printf("  Total Accesses: %zu\n", tracker->count);
  printf("  Cache Hit Rate: %.1f%%\n",
         total > 0 ? (double)hits / total * 100 : 0.0);
  printf("  Sequential Accesses: %zu (%.1f%%)\n", sequential,
         tracker->count > 0 ? (double)sequential / tracker->count * 100 : 0.0);
}

void s7t_memory_tracker_destroy(MemoryTracker *tracker)
{
  if (tracker)
  {
    free(tracker->accesses);
    free(tracker);
  }
}

// ============================================================================
// PROCESS MINING HELPERS IMPLEMENTATION
// ============================================================================

// SIMD matrix operations
void pm_matrix_multiply_simd(const double *a, const double *b, double *result,
                             size_t rows_a, size_t cols_a, size_t cols_b)
{
  CNS_SIMD_BATCH_START(rows_a * cols_b);

  for (size_t i = 0; i < rows_a; i++)
  {
    for (size_t j = 0; j < cols_b; j++)
    {
      double sum = 0.0;
      for (size_t k = 0; k < cols_a; k++)
      {
        sum += a[i * cols_a + k] * b[k * cols_b + j];
      }
      result[i * cols_b + j] = sum;
    }
  }
}

void pm_vector_add_simd(const double *a, const double *b, double *result, size_t size)
{
  CNS_SIMD_BATCH_START(size);

  for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH)
  {
    for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < size; j++)
    {
      result[i + j] = a[i + j] + b[i + j];
    }
  }

  CNS_SIMD_BATCH_END(size)
  {
    result[cns_i] = a[cns_i] + b[cns_i];
  }
}

// Event set operations
EventSet *pm_event_set_create(size_t max_events)
{
  EventSet *set = malloc(sizeof(EventSet));
  if (!set)
    return NULL;

  set->max_events = max_events;
  set->word_count = (max_events + 63) / 64;
  set->bits = calloc(set->word_count, sizeof(uint64_t));

  return set;
}

void pm_event_set_add(EventSet *set, uint32_t event_id)
{
  if (!set || event_id >= set->max_events)
    return;

  size_t word = event_id / 64;
  size_t bit = event_id % 64;
  set->bits[word] |= (1ULL << bit);
}

bool pm_event_set_contains(EventSet *set, uint32_t event_id)
{
  if (!set || event_id >= set->max_events)
    return false;

  size_t word = event_id / 64;
  size_t bit = event_id % 64;
  return (set->bits[word] & (1ULL << bit)) != 0;
}

size_t pm_event_set_popcount(EventSet *set)
{
  if (!set)
    return 0;

  size_t count = 0;
  for (size_t i = 0; i < set->word_count; i++)
  {
    count += __builtin_popcountll(set->bits[i]);
  }
  return count;
}

void pm_event_set_destroy(EventSet *set)
{
  if (set)
  {
    free(set->bits);
    free(set);
  }
}

// ============================================================================
// SQL DOMAIN HELPERS IMPLEMENTATION
// ============================================================================

S7TTable *sql_table_create(const char *name, uint32_t max_rows)
{
  S7TTable *table = aligned_alloc(64, sizeof(S7TTable));
  if (!table)
    return NULL;

  strncpy(table->name, name, sizeof(table->name) - 1);
  table->name[sizeof(table->name) - 1] = '\0';
  table->id = 0;
  table->column_count = 0;
  table->row_count = 0;
  table->max_rows = max_rows;

  // Initialize column data pointers
  for (int i = 0; i < S7T_SQL_MAX_COLUMNS; i++)
  {
    table->column_data[i] = NULL;
    table->column_masks[i] = NULL;
  }

  // Allocate row masks
  size_t mask_size = (max_rows + 63) / 64 * sizeof(uint64_t);
  table->row_masks = aligned_alloc(64, mask_size);
  memset(table->row_masks, 0, mask_size);

  return table;
}

void sql_table_destroy(S7TTable *table)
{
  if (!table)
    return;

  for (int i = 0; i < table->column_count; i++)
  {
    free(table->column_data[i]);
    free(table->column_masks[i]);
  }
  free(table->row_masks);
  free(table);
}

S7TColumn *sql_column_create(S7TTable *table, const char *name, uint32_t type)
{
  if (!table || table->column_count >= S7T_SQL_MAX_COLUMNS)
    return NULL;

  S7TColumn *column = &table->columns[table->column_count];
  strncpy(column->name, name, sizeof(column->name) - 1);
  column->name[sizeof(column->name) - 1] = '\0';
  column->type = type;
  column->offset = 0;

  // Calculate size based on type
  switch (type)
  {
  case S7T_TYPE_INT32:
    column->size = sizeof(int32_t);
    break;
  case S7T_TYPE_INT64:
    column->size = sizeof(int64_t);
    break;
  case S7T_TYPE_FLOAT32:
    column->size = sizeof(float);
    break;
  case S7T_TYPE_FLOAT64:
    column->size = sizeof(double);
    break;
  default:
    column->size = sizeof(int32_t);
    break;
  }

  // Allocate column data
  size_t data_size = table->max_rows * column->size;
  column->data = aligned_alloc(64, data_size);
  memset(column->data, 0, data_size);

  // Allocate bit mask
  size_t mask_size = (table->max_rows + 63) / 64 * sizeof(uint64_t);
  column->bit_mask = aligned_alloc(64, mask_size);
  memset(column->bit_mask, 0, mask_size);

  table->column_data[table->column_count] = column->data;
  table->column_masks[table->column_count] = column->bit_mask;
  table->column_count++;

  return column;
}

void sql_column_filter_int32_simd(S7TColumn *column, int32_t value, uint64_t *result_mask)
{
  if (!column || column->type != S7T_TYPE_INT32)
    return;

  int32_t *data = (int32_t *)column->data;
  size_t word_count = (column->size + 63) / 64;

  CNS_SIMD_BATCH_START(word_count);

  for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH)
  {
    for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < word_count; j++)
    {
      uint64_t mask = 0;
      for (int k = 0; k < 64 && (i + j) * 64 + k < column->size; k++)
      {
        if (data[(i + j) * 64 + k] == value)
        {
          mask |= (1ULL << k);
        }
      }
      result_mask[i + j] = mask;
    }
  }

  CNS_SIMD_BATCH_END(word_count)
  {
    uint64_t mask = 0;
    for (int k = 0; k < 64 && cns_i * 64 + k < column->size; k++)
    {
      if (data[cns_i * 64 + k] == value)
      {
        mask |= (1ULL << k);
      }
    }
    result_mask[cns_i] = mask;
  }
}

// ============================================================================
// RUNTIME SYSTEM HELPERS IMPLEMENTATION
// ============================================================================

ArenaAllocator *runtime_arena_create(size_t size, size_t alignment)
{
  ArenaAllocator *arena = malloc(sizeof(ArenaAllocator));
  if (!arena)
    return NULL;

  arena->buffer = aligned_alloc(alignment, size);
  arena->size = size;
  arena->used = 0;
  arena->alignment = alignment;

  return arena;
}

void *runtime_arena_alloc(ArenaAllocator *arena, size_t size)
{
  if (!arena || arena->used + size > arena->size)
    return NULL;

  void *ptr = arena->buffer + arena->used;
  arena->used += size;

  // Align to next boundary
  arena->used = (arena->used + arena->alignment - 1) & ~(arena->alignment - 1);

  return ptr;
}

void runtime_arena_reset(ArenaAllocator *arena)
{
  if (arena)
  {
    arena->used = 0;
  }
}

void runtime_arena_destroy(ArenaAllocator *arena)
{
  if (arena)
  {
    free(arena->buffer);
    free(arena);
  }
}

// Object pool implementation
ObjectPool *runtime_object_pool_create(size_t capacity,
                                       void *(*create)(void),
                                       void (*destroy)(void *))
{
  ObjectPool *pool = malloc(sizeof(ObjectPool));
  if (!pool)
    return NULL;

  pool->objects = calloc(capacity, sizeof(void *));
  pool->capacity = capacity;
  atomic_init(&pool->head, 0);
  atomic_init(&pool->tail, 0);
  pool->create_func = create;
  pool->destroy_func = destroy;

  // Pre-populate pool
  for (size_t i = 0; i < capacity; i++)
  {
    pool->objects[i] = create ? create() : NULL;
  }

  return pool;
}

void *runtime_object_pool_acquire(ObjectPool *pool)
{
  if (!pool)
    return NULL;

  uint64_t head = atomic_load(&pool->head);
  uint64_t tail = atomic_load(&pool->tail);

  if (head == tail)
    return NULL; // Pool is empty

  void *obj = pool->objects[head % pool->capacity];
  atomic_store(&pool->head, (head + 1) % pool->capacity);

  return obj;
}

void runtime_object_pool_release(ObjectPool *pool, void *obj)
{
  if (!pool || !obj)
    return;

  uint64_t tail = atomic_load(&pool->tail);
  pool->objects[tail % pool->capacity] = obj;
  atomic_store(&pool->tail, (tail + 1) % pool->capacity);
}

void runtime_object_pool_destroy(ObjectPool *pool)
{
  if (!pool)
    return;

  if (pool->destroy_func)
  {
    for (size_t i = 0; i < pool->capacity; i++)
    {
      if (pool->objects[i])
      {
        pool->destroy_func(pool->objects[i]);
      }
    }
  }

  free(pool->objects);
  free(pool);
}

// ============================================================================
// ML COMMANDS HELPERS IMPLEMENTATION
// ============================================================================

void ml_matrix_multiply_simd(const double *a, const double *b, double *result,
                             size_t rows_a, size_t cols_a, size_t cols_b)
{
  CNS_SIMD_BATCH_START(rows_a * cols_b);

  for (size_t i = 0; i < rows_a; i++)
  {
    for (size_t j = 0; j < cols_b; j++)
    {
      double sum = 0.0;
      for (size_t k = 0; k < cols_a; k++)
      {
        sum += a[i * cols_a + k] * b[k * cols_b + j];
      }
      result[i * cols_b + j] = sum;
    }
  }
}

void ml_vector_normalize_simd(double *vector, size_t size)
{
  // Calculate magnitude
  double magnitude = 0.0;
  CNS_SIMD_BATCH_START(size);

  for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH)
  {
    for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < size; j++)
    {
      magnitude += vector[i + j] * vector[i + j];
    }
  }

  CNS_SIMD_BATCH_END(size)
  {
    magnitude += vector[cns_i] * vector[cns_i];
  }

  magnitude = sqrt(magnitude);

  if (magnitude > 0)
  {
    // Normalize
    CNS_SIMD_BATCH_START(size);

    for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH)
    {
      for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < size; j++)
      {
        vector[i + j] /= magnitude;
      }
    }

    CNS_SIMD_BATCH_END(size)
    {
      vector[cns_i] /= magnitude;
    }
  }
}

// ML algorithm registry
MLAlgorithmRegistry *ml_algorithm_registry_create(size_t capacity)
{
  MLAlgorithmRegistry *registry = malloc(sizeof(MLAlgorithmRegistry));
  if (!registry)
    return NULL;

  registry->algorithms = calloc(capacity, sizeof(MLAlgorithm));
  registry->capacity = capacity;
  registry->count = 0;

  return registry;
}

void ml_algorithm_register(MLAlgorithmRegistry *registry, const char *name,
                           void *(*fit)(const double *, const uint32_t *, size_t, size_t),
                           double (*predict)(void *, const double *, size_t),
                           void (*destroy)(void *))
{
  if (!registry || registry->count >= registry->capacity)
    return;

  MLAlgorithm *alg = &registry->algorithms[registry->count++];
  alg->name = strdup(name);
  alg->id = registry->count;
  alg->fit_func = fit;
  alg->predict_func = predict;
  alg->destroy_func = destroy;
}

MLAlgorithm *ml_algorithm_get(MLAlgorithmRegistry *registry, const char *name)
{
  if (!registry || !name)
    return NULL;

  for (size_t i = 0; i < registry->count; i++)
  {
    if (strcmp(registry->algorithms[i].name, name) == 0)
    {
      return &registry->algorithms[i];
    }
  }
  return NULL;
}

void ml_algorithm_registry_destroy(MLAlgorithmRegistry *registry)
{
  if (!registry)
    return;

  for (size_t i = 0; i < registry->count; i++)
  {
    free(registry->algorithms[i].name);
  }
  free(registry->algorithms);
  free(registry);
}

// ============================================================================
// PERFORMANCE ANALYSIS HELPERS IMPLEMENTATION
// ============================================================================

CacheSimulator *perf_cache_simulator_create(size_t size_kb, size_t associativity)
{
  CacheSimulator *cache = malloc(sizeof(CacheSimulator));
  if (!cache)
    return NULL;

  cache->size_kb = size_kb;
  cache->associativity = associativity;
  cache->line_size = 64;

  size_t num_sets = (size_kb * 1024) / (associativity * cache->line_size);
  cache->tags = calloc(num_sets * associativity, sizeof(uint64_t));
  cache->lru_counters = calloc(num_sets * associativity, sizeof(uint64_t));

  atomic_init(&cache->hits, 0);
  atomic_init(&cache->misses, 0);

  return cache;
}

bool perf_cache_access(CacheSimulator *cache, void *address)
{
  if (!cache)
    return false;

  uintptr_t addr = (uintptr_t)address;
  uintptr_t tag = addr / cache->line_size;
  size_t num_sets = (cache->size_kb * 1024) / (cache->associativity * cache->line_size);
  size_t set_index = tag % num_sets;

  // Check if tag exists in set
  for (size_t i = 0; i < cache->associativity; i++)
  {
    size_t index = set_index * cache->associativity + i;
    if (cache->tags[index] == tag)
    {
      // Hit - update LRU
      cache->lru_counters[index] = cns_get_nanoseconds();
      atomic_fetch_add(&cache->hits, 1);
      return true;
    }
  }

  // Miss - find LRU entry and replace
  size_t lru_index = set_index * cache->associativity;
  uint64_t oldest_time = cache->lru_counters[lru_index];

  for (size_t i = 1; i < cache->associativity; i++)
  {
    size_t index = set_index * cache->associativity + i;
    if (cache->lru_counters[index] < oldest_time)
    {
      oldest_time = cache->lru_counters[index];
      lru_index = index;
    }
  }

  cache->tags[lru_index] = tag;
  cache->lru_counters[lru_index] = cns_get_nanoseconds();
  atomic_fetch_add(&cache->misses, 1);

  return false;
}

void perf_cache_get_stats(CacheSimulator *cache, uint64_t *hits, uint64_t *misses)
{
  if (!cache)
    return;

  *hits = atomic_load(&cache->hits);
  *misses = atomic_load(&cache->misses);
}

void perf_cache_simulator_destroy(CacheSimulator *cache)
{
  if (cache)
  {
    free(cache->tags);
    free(cache->lru_counters);
    free(cache);
  }
}

// ============================================================================
// TELEMETRY TESTING HELPERS IMPLEMENTATION
// ============================================================================

TestDataGenerator *telemetry_test_data_generator_create(size_t capacity)
{
  TestDataGenerator *generator = malloc(sizeof(TestDataGenerator));
  if (!generator)
    return NULL;

  generator->spans = calloc(capacity, sizeof(TestSpan));
  generator->capacity = capacity;
  generator->count = 0;
  generator->generation_time_ns = 0;

  return generator;
}

TestSpan *telemetry_generate_test_span(TestDataGenerator *generator, const char *operation)
{
  if (!generator || generator->count >= generator->capacity)
    return NULL;

  TestSpan *span = &generator->spans[generator->count++];

  // Generate random IDs
  uint64_t trace_id = (uint64_t)rand() << 32 | rand();
  uint64_t span_id = (uint64_t)rand() << 32 | rand();

  span->trace_id = malloc(33);
  span->span_id = malloc(17);
  span->operation_name = strdup(operation);

  snprintf(span->trace_id, 33, "%016llx", trace_id);
  snprintf(span->span_id, 17, "%016llx", span_id);

  span->start_time = cns_get_nanoseconds();
  span->end_time = 0;
  span->attribute_count = 0;
  span->attribute_keys = NULL;
  span->attribute_values = NULL;

  return span;
}

void telemetry_test_data_generator_destroy(TestDataGenerator *generator)
{
  if (!generator)
    return;

  for (size_t i = 0; i < generator->count; i++)
  {
    TestSpan *span = &generator->spans[i];
    free(span->trace_id);
    free(span->span_id);
    free(span->operation_name);

    for (uint32_t j = 0; j < span->attribute_count; j++)
    {
      free(span->attribute_keys[j]);
      free(span->attribute_values[j]);
    }
    free(span->attribute_keys);
    free(span->attribute_values);
  }

  free(generator->spans);
  free(generator);
}

// Performance metrics
TestPerformanceMetrics *telemetry_test_performance_create(void)
{
  TestPerformanceMetrics *metrics = malloc(sizeof(TestPerformanceMetrics));
  if (!metrics)
    return NULL;

  metrics->min_latency_ns = UINT64_MAX;
  metrics->max_latency_ns = 0;
  metrics->avg_latency_ns = 0;
  metrics->total_operations = 0;
  metrics->successful_operations = 0;
  metrics->success_rate = 0.0;

  return metrics;
}

void telemetry_test_performance_record(TestPerformanceMetrics *metrics,
                                       uint64_t latency_ns, bool success)
{
  if (!metrics)
    return;

  metrics->total_operations++;
  if (success)
    metrics->successful_operations++;

  if (latency_ns < metrics->min_latency_ns)
    metrics->min_latency_ns = latency_ns;
  if (latency_ns > metrics->max_latency_ns)
    metrics->max_latency_ns = latency_ns;

  // Update average
  metrics->avg_latency_ns = ((metrics->avg_latency_ns * (metrics->total_operations - 1)) + latency_ns) / metrics->total_operations;

  metrics->success_rate = (double)metrics->successful_operations / metrics->total_operations;
}

void telemetry_test_performance_report(TestPerformanceMetrics *metrics)
{
  if (!metrics)
    return;

  printf("Test Performance Report:\n");
  printf("  Total Operations: %lu\n", metrics->total_operations);
  printf("  Successful Operations: %lu (%.1f%%)\n",
         metrics->successful_operations, metrics->success_rate * 100);
  printf("  Min Latency: %lu ns\n", metrics->min_latency_ns);
  printf("  Max Latency: %lu ns\n", metrics->max_latency_ns);
  printf("  Avg Latency: %lu ns\n", metrics->avg_latency_ns);
}

void telemetry_test_performance_destroy(TestPerformanceMetrics *metrics)
{
  free(metrics);
}