/*  ─────────────────────────────────────────────────────────────
    sql_execute.c  –  7-Tick SQL Execution Engine
    SIMD-accelerated, branch-free SQL execution with OTel spans
    ───────────────────────────────────────────────────────────── */

#include "../../include/cns/sql.h"
#include <stdio.h>
#include <string.h>

// OpenTelemetry span tracking
typedef struct {
    const char* name;
    uint64_t start;
    uint64_t end;
    uint32_t rows_in;
    uint32_t rows_out;
    uint64_t bytes_processed;
} otel_span_t;

// Span stack for nested operations
static struct {
    otel_span_t spans[32];
    uint32_t depth;
} g_span_stack = {0};

/*═══════════════════════════════════════════════════════════════
  OpenTelemetry Helpers
  ═══════════════════════════════════════════════════════════════*/

S7T_ALWAYS_INLINE void otel_span_begin(const char* name) {
    if (g_span_stack.depth < 32) {
        otel_span_t* span = &g_span_stack.spans[g_span_stack.depth++];
        span->name = name;
        span->start = s7t_cycles();
        span->rows_in = 0;
        span->rows_out = 0;
        span->bytes_processed = 0;
    }
}

S7T_ALWAYS_INLINE void otel_span_end(void) {
    if (g_span_stack.depth > 0) {
        otel_span_t* span = &g_span_stack.spans[--g_span_stack.depth];
        span->end = s7t_cycles();
        
        // Log span (in production, send to OTel collector)
        uint64_t duration = span->end - span->start;
        printf("[OTEL] %s: %lu cycles (%.2f ns), rows: %u->%u, bytes: %lu\n",
               span->name, duration, duration * S7T_NS_PER_CYCLE,
               span->rows_in, span->rows_out, span->bytes_processed);
    }
}

S7T_ALWAYS_INLINE void otel_span_set_rows(uint32_t in, uint32_t out) {
    if (g_span_stack.depth > 0) {
        otel_span_t* span = &g_span_stack.spans[g_span_stack.depth - 1];
        span->rows_in = in;
        span->rows_out = out;
    }
}

S7T_ALWAYS_INLINE void otel_span_add_bytes(uint64_t bytes) {
    if (g_span_stack.depth > 0) {
        otel_span_t* span = &g_span_stack.spans[g_span_stack.depth - 1];
        span->bytes_processed += bytes;
    }
}

/*═══════════════════════════════════════════════════════════════
  SIMD Filter Operations
  ═══════════════════════════════════════════════════════════════*/

// SIMD filter for different operators
S7T_ALWAYS_INLINE uint32_t s7t_simd_filter_op_i32(
    const int32_t* data, 
    s7t_sql_op_t op,
    int32_t value, 
    uint32_t count, 
    uint32_t* out_indices
) {
    otel_span_begin("simd_filter_i32");
    uint32_t matches = 0;
    
#ifdef __AVX2__
    __m256i vval = _mm256_set1_epi32(value);
    
    for (uint32_t i = 0; i < count; i += 8) {
        __m256i vdata = _mm256_loadu_si256((__m256i*)(data + i));
        __m256i vcmp;
        
        // Branch-free operator selection using jump table
        switch (op) {
            case S7T_OP_EQ:
                vcmp = _mm256_cmpeq_epi32(vdata, vval);
                break;
            case S7T_OP_GT:
                vcmp = _mm256_cmpgt_epi32(vdata, vval);
                break;
            case S7T_OP_LT:
                vcmp = _mm256_cmpgt_epi32(vval, vdata);
                break;
            case S7T_OP_GE:
                vcmp = _mm256_or_si256(
                    _mm256_cmpeq_epi32(vdata, vval),
                    _mm256_cmpgt_epi32(vdata, vval)
                );
                break;
            case S7T_OP_LE:
                vcmp = _mm256_or_si256(
                    _mm256_cmpeq_epi32(vdata, vval),
                    _mm256_cmpgt_epi32(vval, vdata)
                );
                break;
            case S7T_OP_NE:
                vcmp = _mm256_xor_si256(
                    _mm256_cmpeq_epi32(vdata, vval),
                    _mm256_set1_epi32(-1)
                );
                break;
            default:
                vcmp = _mm256_setzero_si256();
        }
        
        uint32_t mask = _mm256_movemask_ps(_mm256_castsi256_ps(vcmp));
        
        // Extract matching indices
        while (mask) {
            uint32_t idx = __builtin_ctz(mask);
            out_indices[matches++] = i + idx;
            mask &= mask - 1;
        }
    }
    
    // Handle remaining elements
    for (uint32_t i = (count & ~7); i < count; i++) {
        bool match = false;
        switch (op) {
            case S7T_OP_EQ: match = (data[i] == value); break;
            case S7T_OP_NE: match = (data[i] != value); break;
            case S7T_OP_LT: match = (data[i] < value); break;
            case S7T_OP_LE: match = (data[i] <= value); break;
            case S7T_OP_GT: match = (data[i] > value); break;
            case S7T_OP_GE: match = (data[i] >= value); break;
        }
        if (match) {
            out_indices[matches++] = i;
        }
    }
#else
    // Scalar fallback
    for (uint32_t i = 0; i < count; i++) {
        bool match = false;
        switch (op) {
            case S7T_OP_EQ: match = (data[i] == value); break;
            case S7T_OP_NE: match = (data[i] != value); break;
            case S7T_OP_LT: match = (data[i] < value); break;
            case S7T_OP_LE: match = (data[i] <= value); break;
            case S7T_OP_GT: match = (data[i] > value); break;
            case S7T_OP_GE: match = (data[i] >= value); break;
        }
        if (match) {
            out_indices[matches++] = i;
        }
    }
#endif
    
    otel_span_set_rows(count, matches);
    otel_span_add_bytes(count * sizeof(int32_t));
    otel_span_end();
    
    return matches;
}

/*═══════════════════════════════════════════════════════════════
  Aggregation Operations
  ═══════════════════════════════════════════════════════════════*/

// SIMD sum aggregation
S7T_ALWAYS_INLINE int64_t s7t_simd_sum_i32(
    const int32_t* data,
    const uint32_t* indices,
    uint32_t count
) {
    otel_span_begin("simd_sum_i32");
    int64_t sum = 0;
    
#ifdef __AVX2__
    __m256i vsum = _mm256_setzero_si256();
    
    // Process 8 values at a time
    uint32_t i;
    for (i = 0; i + 7 < count; i += 8) {
        // Gather 8 values using indices
        int32_t values[8];
        for (int j = 0; j < 8; j++) {
            values[j] = data[indices[i + j]];
        }
        
        __m256i vdata = _mm256_loadu_si256((__m256i*)values);
        vsum = _mm256_add_epi32(vsum, vdata);
    }
    
    // Horizontal sum
    __m128i vsum_low = _mm256_castsi256_si128(vsum);
    __m128i vsum_high = _mm256_extracti128_si256(vsum, 1);
    __m128i vsum_128 = _mm_add_epi32(vsum_low, vsum_high);
    
    // Further reduction
    vsum_128 = _mm_hadd_epi32(vsum_128, vsum_128);
    vsum_128 = _mm_hadd_epi32(vsum_128, vsum_128);
    sum = _mm_cvtsi128_si32(vsum_128);
    
    // Handle remaining elements
    for (; i < count; i++) {
        sum += data[indices[i]];
    }
#else
    // Scalar fallback
    for (uint32_t i = 0; i < count; i++) {
        sum += data[indices[i]];
    }
#endif
    
    otel_span_set_rows(count, 1);
    otel_span_add_bytes(count * sizeof(int32_t));
    otel_span_end();
    
    return sum;
}

// Grouped aggregation with bounded cardinality
typedef struct {
    uint32_t groups[S7T_SQL_MAX_GROUP_KEYS];
    int64_t values[S7T_SQL_MAX_GROUP_KEYS];
    uint32_t counts[S7T_SQL_MAX_GROUP_KEYS];
    uint32_t group_count;
} s7t_group_state_t;

S7T_ALWAYS_INLINE void s7t_group_aggregate(
    s7t_group_state_t* state,
    const uint32_t* group_keys,
    const int32_t* values,
    const uint32_t* indices,
    uint32_t count,
    s7t_sql_agg_t agg_func
) {
    otel_span_begin("group_aggregate");
    
    // Initialize state
    state->group_count = 0;
    
    // Process each row
    for (uint32_t i = 0; i < count; i++) {
        uint32_t row_idx = indices[i];
        uint32_t group_key = group_keys[row_idx];
        int32_t value = values[row_idx];
        
        // Find or create group (linear search for simplicity)
        uint32_t group_idx = 0;
        bool found = false;
        
        for (uint32_t j = 0; j < state->group_count; j++) {
            if (state->groups[j] == group_key) {
                group_idx = j;
                found = true;
                break;
            }
        }
        
        if (!found && state->group_count < S7T_SQL_MAX_GROUP_KEYS) {
            group_idx = state->group_count++;
            state->groups[group_idx] = group_key;
            state->values[group_idx] = 0;
            state->counts[group_idx] = 0;
        }
        
        // Update aggregate
        switch (agg_func) {
            case S7T_AGG_COUNT:
                state->values[group_idx]++;
                break;
            case S7T_AGG_SUM:
                state->values[group_idx] += value;
                break;
            case S7T_AGG_MIN:
                if (state->counts[group_idx] == 0 || value < state->values[group_idx]) {
                    state->values[group_idx] = value;
                }
                break;
            case S7T_AGG_MAX:
                if (state->counts[group_idx] == 0 || value > state->values[group_idx]) {
                    state->values[group_idx] = value;
                }
                break;
            case S7T_AGG_AVG:
                state->values[group_idx] += value;
                break;
        }
        state->counts[group_idx]++;
    }
    
    // Finalize averages
    if (agg_func == S7T_AGG_AVG) {
        for (uint32_t i = 0; i < state->group_count; i++) {
            if (state->counts[i] > 0) {
                state->values[i] /= state->counts[i];
            }
        }
    }
    
    otel_span_set_rows(count, state->group_count);
    otel_span_add_bytes(count * (sizeof(uint32_t) + sizeof(int32_t)));
    otel_span_end();
}

/*═══════════════════════════════════════════════════════════════
  Sort Operations (Bitonic sort for small datasets)
  ═══════════════════════════════════════════════════════════════*/

// Bitonic merge step
S7T_ALWAYS_INLINE void bitonic_merge(
    uint32_t* indices,
    const int32_t* values,
    uint32_t low,
    uint32_t count,
    bool ascending
) {
    if (count <= 1) return;
    
    uint32_t k = 1;
    while (k < count) k <<= 1;
    k >>= 1;
    
    for (uint32_t i = low; i < low + count - k; i++) {
        bool should_swap = ascending ? 
            (values[indices[i]] > values[indices[i + k]]) :
            (values[indices[i]] < values[indices[i + k]]);
            
        if (should_swap) {
            uint32_t temp = indices[i];
            indices[i] = indices[i + k];
            indices[i + k] = temp;
        }
    }
    
    bitonic_merge(indices, values, low, k, ascending);
    bitonic_merge(indices, values, low + k, count - k, ascending);
}

// Bitonic sort
S7T_ALWAYS_INLINE void s7t_bitonic_sort(
    uint32_t* indices,
    const int32_t* values,
    uint32_t count,
    bool ascending
) {
    otel_span_begin("bitonic_sort");
    
    // Initialize indices
    for (uint32_t i = 0; i < count; i++) {
        indices[i] = i;
    }
    
    // Bitonic sort stages
    for (uint32_t k = 2; k <= count; k <<= 1) {
        for (uint32_t j = k >> 1; j > 0; j >>= 1) {
            for (uint32_t i = 0; i < count; i++) {
                uint32_t ij = i ^ j;
                if (ij > i && ij < count) {
                    bool dir = ((i & k) == 0) ? ascending : !ascending;
                    bool should_swap = dir ?
                        (values[indices[i]] > values[indices[ij]]) :
                        (values[indices[i]] < values[indices[ij]]);
                        
                    if (should_swap) {
                        uint32_t temp = indices[i];
                        indices[i] = indices[ij];
                        indices[ij] = temp;
                    }
                }
            }
        }
    }
    
    otel_span_set_rows(count, count);
    otel_span_add_bytes(count * sizeof(int32_t));
    otel_span_end();
}

/*═══════════════════════════════════════════════════════════════
  Main Execution Function
  ═══════════════════════════════════════════════════════════════*/

s7t_result_t* s7t_sql_execute(s7t_query_plan_t* plan, s7t_arena_t* arena) {
    otel_span_begin("sql_execute");
    
    // Allocate result set
    s7t_result_t* result = (s7t_result_t*)s7t_arena_alloc(arena, sizeof(s7t_result_t));
    if (!result) {
        otel_span_end();
        return NULL;
    }
    
    result->arena = arena;
    result->column_count = 0;
    result->row_count = 0;
    result->execution_cycles = s7t_cycles();
    
    // For demo, assume single table
    if (plan->table_count == 0) {
        otel_span_end();
        return result;
    }
    
    s7t_table_t* table = plan->tables[0];
    uint32_t* row_indices = (uint32_t*)s7t_arena_alloc(arena, 
                                                       table->row_count * sizeof(uint32_t));
    if (!row_indices) {
        otel_span_end();
        return NULL;
    }
    
    // Step 1: Apply filters
    uint32_t matched_rows = table->row_count;
    
    if (plan->predicate_count > 0) {
        otel_span_begin("apply_filters");
        
        // Initialize with all rows
        for (uint32_t i = 0; i < table->row_count; i++) {
            row_indices[i] = i;
        }
        
        // Apply each predicate
        for (uint32_t p = 0; p < plan->predicate_count; p++) {
            s7t_predicate_t* pred = &plan->predicates[p];
            s7t_column_t* col = &table->columns[pred->column_idx];
            
            if (col->type == S7T_TYPE_INT32) {
                uint32_t* temp_indices = (uint32_t*)s7t_arena_alloc(arena,
                                                                    matched_rows * sizeof(uint32_t));
                
                uint32_t new_matched = s7t_simd_filter_op_i32(
                    (int32_t*)col->data,
                    pred->op,
                    (int32_t)pred->value.i64,
                    matched_rows,
                    temp_indices
                );
                
                // Update indices with filtered results
                for (uint32_t i = 0; i < new_matched; i++) {
                    row_indices[i] = row_indices[temp_indices[i]];
                }
                matched_rows = new_matched;
            }
        }
        
        otel_span_set_rows(table->row_count, matched_rows);
        otel_span_end();
    } else {
        // No filter, use all rows
        for (uint32_t i = 0; i < table->row_count; i++) {
            row_indices[i] = i;
        }
    }
    
    // Step 2: Apply grouping if needed
    if (plan->group_col > 0 && plan->agg_func != S7T_AGG_COUNT) {
        otel_span_begin("apply_grouping");
        
        s7t_group_state_t* group_state = (s7t_group_state_t*)s7t_arena_alloc(
            arena, sizeof(s7t_group_state_t));
            
        s7t_column_t* group_col = &table->columns[plan->group_col];
        s7t_column_t* value_col = &table->columns[0];  // Assume first column for aggregation
        
        s7t_group_aggregate(
            group_state,
            (uint32_t*)group_col->data,
            (int32_t*)value_col->data,
            row_indices,
            matched_rows,
            plan->agg_func
        );
        
        // Create result columns for groups
        s7t_column_init(&result->columns[0], "group", S7T_TYPE_INT32, arena);
        s7t_column_init(&result->columns[1], "value", S7T_TYPE_INT64, arena);
        result->column_count = 2;
        
        // Copy group results
        int32_t* group_data = (int32_t*)result->columns[0].data;
        int64_t* value_data = (int64_t*)result->columns[1].data;
        
        for (uint32_t i = 0; i < group_state->group_count; i++) {
            group_data[i] = group_state->groups[i];
            value_data[i] = group_state->values[i];
        }
        
        result->row_count = group_state->group_count;
        result->columns[0].count = group_state->group_count;
        result->columns[1].count = group_state->group_count;
        
        otel_span_end();
    } else {
        // Step 3: Apply sorting if needed
        if (plan->order_col > 0) {
            otel_span_begin("apply_sorting");
            
            s7t_column_t* sort_col = &table->columns[plan->order_col];
            if (sort_col->type == S7T_TYPE_INT32) {
                s7t_bitonic_sort(row_indices, (int32_t*)sort_col->data, 
                                matched_rows, !plan->order_desc);
            }
            
            otel_span_end();
        }
        
        // Step 4: Apply limit
        if (plan->limit > 0 && matched_rows > plan->limit) {
            matched_rows = plan->limit;
        }
        
        // Step 5: Project columns
        otel_span_begin("project_columns");
        
        if (plan->project_count == 0) {
            // Select all columns
            result->column_count = table->column_count;
            for (uint32_t i = 0; i < table->column_count; i++) {
                memcpy(&result->columns[i], &table->columns[i], sizeof(s7t_column_t));
                
                // Allocate new data buffer
                size_t elem_size = 8;
                result->columns[i].data = s7t_arena_alloc(arena, elem_size * matched_rows);
                
                // Copy selected rows
                for (uint32_t j = 0; j < matched_rows; j++) {
                    memcpy((uint8_t*)result->columns[i].data + j * elem_size,
                           (uint8_t*)table->columns[i].data + row_indices[j] * elem_size,
                           elem_size);
                }
                result->columns[i].count = matched_rows;
            }
        } else {
            // Select specific columns
            result->column_count = plan->project_count;
            for (uint32_t i = 0; i < plan->project_count; i++) {
                uint32_t col_idx = plan->project_cols[i];
                memcpy(&result->columns[i], &table->columns[col_idx], sizeof(s7t_column_t));
                
                // Allocate new data buffer
                size_t elem_size = 8;
                result->columns[i].data = s7t_arena_alloc(arena, elem_size * matched_rows);
                
                // Copy selected rows
                for (uint32_t j = 0; j < matched_rows; j++) {
                    memcpy((uint8_t*)result->columns[i].data + j * elem_size,
                           (uint8_t*)table->columns[col_idx].data + row_indices[j] * elem_size,
                           elem_size);
                }
                result->columns[i].count = matched_rows;
            }
        }
        
        result->row_count = matched_rows;
        
        otel_span_end();
    }
    
    // Calculate execution time
    result->execution_cycles = s7t_cycles() - result->execution_cycles;
    
    otel_span_set_rows(table->row_count, result->row_count);
    otel_span_end();
    
    return result;
}