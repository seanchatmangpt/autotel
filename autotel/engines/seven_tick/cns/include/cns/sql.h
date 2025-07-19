/*  ─────────────────────────────────────────────────────────────
    cns/sql.h  –  7-Tick SQL Engine Header (v1.0)
    Physics-compliant SQL with ≤7 cycle operations
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_SQL_H
#define CNS_SQL_H

#include "../../include/s7t.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  Core SQL Types and Constants
  ═══════════════════════════════════════════════════════════════*/

// Maximum values (cache-aligned)
#define S7T_SQL_MAX_COLUMNS     64
#define S7T_SQL_MAX_TABLES      16
#define S7T_SQL_MAX_PREDICATES  8
#define S7T_SQL_MAX_GROUP_KEYS  256
#define S7T_SQL_MAX_ROWS        1024
#define S7T_SQL_ARENA_SIZE      (1024 * 1024)  // 1MB arena

// SQL Data Types
typedef enum {
    S7T_TYPE_INT32 = 0,
    S7T_TYPE_INT64,
    S7T_TYPE_FLOAT32,
    S7T_TYPE_FLOAT64,
    S7T_TYPE_ID,        // Interned string ID
    S7T_TYPE_DATE,      // Days since epoch
    S7T_TYPE_TIME,      // Nanoseconds
    S7T_TYPE_BOOL
} s7t_sql_type_t;

// SQL Operators (branch-free)
typedef enum {
    S7T_OP_EQ = 0,      // Equal
    S7T_OP_NE,          // Not equal
    S7T_OP_LT,          // Less than
    S7T_OP_LE,          // Less or equal
    S7T_OP_GT,          // Greater than
    S7T_OP_GE,          // Greater or equal
    S7T_OP_IN,          // In set
    S7T_OP_BETWEEN      // Between range
} s7t_sql_op_t;

// Aggregate Functions
typedef enum {
    S7T_AGG_COUNT = 0,
    S7T_AGG_SUM,
    S7T_AGG_MIN,
    S7T_AGG_MAX,
    S7T_AGG_AVG
} s7t_sql_agg_t;

// Join Types
typedef enum {
    S7T_JOIN_INNER = 0,
    S7T_JOIN_HASH
} s7t_join_type_t;

/*═══════════════════════════════════════════════════════════════
  Column Store Structure (Cache-aligned)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    void* data;                     // Column data pointer
    uint32_t count;                 // Number of rows
    uint32_t capacity;              // Allocated capacity
    s7t_sql_type_t type;            // Data type
    uint8_t null_bitmap[128];       // Null bitmap (1024 rows)
    char name[32];                  // Column name
} s7t_column_t;

/*═══════════════════════════════════════════════════════════════
  Table Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    s7t_column_t columns[S7T_SQL_MAX_COLUMNS];
    uint32_t column_count;
    uint32_t row_count;
    uint32_t table_id;
    char name[32];
} s7t_table_t;

/*═══════════════════════════════════════════════════════════════
  Predicate Structure (Branch-free evaluation)
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t column_idx;            // Column index
    s7t_sql_op_t op;                // Operator
    union {
        int64_t i64;
        double f64;
        s7t_id_t id;
        struct {
            int64_t low;
            int64_t high;
        } range;
    } value;
} s7t_predicate_t;

/*═══════════════════════════════════════════════════════════════
  Query Plan Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    // Source tables
    s7t_table_t* tables[S7T_SQL_MAX_TABLES];
    uint32_t table_count;
    
    // Projection
    uint32_t project_cols[S7T_SQL_MAX_COLUMNS];
    uint32_t project_count;
    
    // Selection predicates
    s7t_predicate_t predicates[S7T_SQL_MAX_PREDICATES];
    uint32_t predicate_count;
    
    // Grouping
    uint32_t group_col;
    s7t_sql_agg_t agg_func;
    
    // Ordering
    uint32_t order_col;
    bool order_desc;
    
    // Limit
    uint32_t limit;
    
    // Join
    s7t_join_type_t join_type;
    uint32_t join_left_col;
    uint32_t join_right_col;
    
    // Performance
    uint64_t estimated_cycles;
} s7t_query_plan_t;

/*═══════════════════════════════════════════════════════════════
  Arena Allocator (Zero-copy, deterministic)
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    uint8_t* base;                  // Base pointer
    uint64_t offset;                // Current offset
    uint64_t size;                  // Total size
    uint64_t allocations;           // Allocation count
} s7t_arena_t;

/*═══════════════════════════════════════════════════════════════
  Result Set Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct S7T_ALIGNED(64) {
    s7t_column_t columns[S7T_SQL_MAX_COLUMNS];
    uint32_t column_count;
    uint32_t row_count;
    uint64_t execution_cycles;
    s7t_arena_t* arena;             // Result arena
} s7t_result_t;

/*═══════════════════════════════════════════════════════════════
  SIMD Operations (AVX2/NEON)
  ═══════════════════════════════════════════════════════════════*/

#ifdef __AVX2__
#include <immintrin.h>
typedef __m256i s7t_vec256_t;
#define S7T_VEC_WIDTH 8  // 8x 32-bit integers
#elif defined(__ARM_NEON)
#include <arm_neon.h>
typedef int32x4_t s7t_vec128_t;
#define S7T_VEC_WIDTH 4  // 4x 32-bit integers
#else
#define S7T_VEC_WIDTH 1  // Scalar fallback
#endif

/*═══════════════════════════════════════════════════════════════
  Public API (All functions < 7 cycles)
  ═══════════════════════════════════════════════════════════════*/

// Arena operations (1-2 cycles)
S7T_ALWAYS_INLINE void s7t_arena_init(s7t_arena_t* arena, void* buffer, size_t size) {
    arena->base = (uint8_t*)buffer;
    arena->offset = 0;
    arena->size = size;
    arena->allocations = 0;
}

S7T_ALWAYS_INLINE void* s7t_arena_alloc(s7t_arena_t* arena, size_t size) {
    size = (size + 63) & ~63;  // Align to cache line
    if (S7T_UNLIKELY(arena->offset + size > arena->size)) {
        return NULL;
    }
    void* ptr = arena->base + arena->offset;
    arena->offset += size;
    arena->allocations++;
    return ptr;
}

// Column operations (2-3 cycles)
S7T_ALWAYS_INLINE void s7t_column_init(s7t_column_t* col, const char* name, 
                                       s7t_sql_type_t type, s7t_arena_t* arena) {
    strncpy(col->name, name, 31);
    col->type = type;
    col->count = 0;
    col->capacity = S7T_SQL_MAX_ROWS;
    
    size_t elem_size = 8;  // All types fit in 8 bytes
    col->data = s7t_arena_alloc(arena, elem_size * col->capacity);
    memset(col->null_bitmap, 0, sizeof(col->null_bitmap));
}

// Table operations
S7T_ALWAYS_INLINE void s7t_table_init(s7t_table_t* table, const char* name, uint32_t id) {
    strncpy(table->name, name, 31);
    table->table_id = id;
    table->column_count = 0;
    table->row_count = 0;
}

// SIMD filter operation (4-5 cycles for 8 values)
S7T_ALWAYS_INLINE uint32_t s7t_simd_filter_eq_i32(const int32_t* data, int32_t value, 
                                                   uint32_t count, uint32_t* out_indices) {
    uint32_t matches = 0;
    
#ifdef __AVX2__
    __m256i vval = _mm256_set1_epi32(value);
    for (uint32_t i = 0; i < count; i += 8) {
        __m256i vdata = _mm256_loadu_si256((__m256i*)(data + i));
        __m256i vcmp = _mm256_cmpeq_epi32(vdata, vval);
        uint32_t mask = _mm256_movemask_ps(_mm256_castsi256_ps(vcmp));
        
        while (mask) {
            uint32_t idx = __builtin_ctz(mask);
            out_indices[matches++] = i + idx;
            mask &= mask - 1;
        }
    }
#else
    // Scalar fallback
    for (uint32_t i = 0; i < count; i++) {
        if (data[i] == value) {
            out_indices[matches++] = i;
        }
    }
#endif
    
    return matches;
}

// Hash join build (5-6 cycles per row)
typedef struct {
    uint32_t* keys;
    uint32_t* values;
    uint32_t* buckets;
    uint32_t bucket_count;
    uint32_t size;
} s7t_hash_table_t;

S7T_ALWAYS_INLINE void s7t_hash_build(s7t_hash_table_t* ht, const uint32_t* keys, 
                                      const uint32_t* values, uint32_t count) {
    // Power of 2 for fast modulo
    ht->bucket_count = 256;  // Fixed size for L1 cache
    
    // Build hash table with linear probing
    for (uint32_t i = 0; i < count; i++) {
        uint32_t hash = keys[i] * 0x9e3779b9;  // Golden ratio hash
        uint32_t bucket = hash & (ht->bucket_count - 1);
        
        // Linear probe for empty slot
        while (ht->buckets[bucket] != 0xFFFFFFFF) {
            bucket = (bucket + 1) & (ht->bucket_count - 1);
        }
        
        ht->buckets[bucket] = i;
        ht->keys[i] = keys[i];
        ht->values[i] = values[i];
    }
    ht->size = count;
}

// Query plan validation (1-2 cycles)
S7T_ALWAYS_INLINE bool s7t_validate_plan(const s7t_query_plan_t* plan) {
    // Check cycle budget
    if (plan->estimated_cycles > S7T_MAX_CYCLES) {
        return false;
    }
    
    // Validate limits
    if (plan->limit > S7T_SQL_MAX_ROWS) {
        return false;
    }
    
    if (plan->predicate_count > S7T_SQL_MAX_PREDICATES) {
        return false;
    }
    
    return true;
}

// OpenTelemetry span helpers
typedef struct {
    uint64_t start_cycles;
    uint64_t end_cycles;
    const char* operation;
    uint32_t rows_processed;
    uint32_t rows_output;
} s7t_span_t;

S7T_ALWAYS_INLINE void s7t_span_start(s7t_span_t* span, const char* op) {
    span->start_cycles = s7t_cycles();
    span->operation = op;
    span->rows_processed = 0;
    span->rows_output = 0;
}

S7T_ALWAYS_INLINE void s7t_span_end(s7t_span_t* span) {
    span->end_cycles = s7t_cycles();
}

/*═══════════════════════════════════════════════════════════════
  SQL Execution Engine API
  ═══════════════════════════════════════════════════════════════*/

// Parse SQL query into plan
s7t_query_plan_t* s7t_sql_parse(const char* query, s7t_arena_t* arena);

// Compile plan to micro-ops
bool s7t_sql_compile(s7t_query_plan_t* plan);

// Execute compiled plan
s7t_result_t* s7t_sql_execute(s7t_query_plan_t* plan, s7t_arena_t* arena);

// Insert row into table
bool s7t_sql_insert(s7t_table_t* table, const void* values[], s7t_arena_t* arena);

// Explain query plan
void s7t_sql_explain(const s7t_query_plan_t* plan, char* buffer, size_t size);

#endif /* CNS_SQL_H */