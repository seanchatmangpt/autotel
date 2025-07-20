/**
 * @file cjinja_ultra_fast.c
 * @brief Ultra-Fast CJinja Template Engine Implementation
 * @version 3.0.0 (Ultra-Fast Optimization)
 * 
 * Optimized implementation targeting <100ns variable substitution.
 * 
 * @author 7T Engine Team
 * @date 2024-01-15
 */

#include "cjinja_ultra_fast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#ifdef __x86_64__
#include <cpuid.h>
#include <immintrin.h>
#endif

// =============================================================================
// SIMD FEATURE DETECTION
// =============================================================================

CJinjaSIMDFeatures cjinja_ultra_detect_simd(void) {
    CJinjaSIMDFeatures features = {0};
    
    #ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;
    
    // Check CPUID support
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        features.sse2_available = (edx & (1 << 26)) != 0;
        features.sse4_available = (ecx & (1 << 19)) != 0;
        features.avx_available = (ecx & (1 << 28)) != 0;
    }
    
    if (__get_cpuid(7, &eax, &ebx, &ecx, &edx)) {
        features.avx2_available = (ebx & (1 << 5)) != 0;
        features.avx512_available = (ebx & (1 << 16)) != 0;
    }
    #endif
    
    return features;
}

// =============================================================================
// ULTRA-FAST CONTEXT MANAGEMENT
// =============================================================================

CJinjaUltraContext* cjinja_ultra_create_context(void) {
    CJinjaUltraContext* ctx = calloc(1, sizeof(CJinjaUltraContext));
    if (!ctx) return NULL;
    
    // Allocate memory pool for hash entries
    ctx->pool = malloc(sizeof(CJinjaHashEntry) * 1024); // Pre-allocate 1024 entries
    if (!ctx->pool) {
        free(ctx);
        return NULL;
    }
    
    ctx->pool_used = 0;
    ctx->total_variables = 0;
    ctx->lookup_count = 0;
    ctx->collision_count = 0;
    
    return ctx;
}

void cjinja_ultra_destroy_context(CJinjaUltraContext* ctx) {
    if (!ctx) return;
    
    free(ctx->pool);
    free(ctx);
}

// =============================================================================
// ULTRA-FAST ENGINE MANAGEMENT
// =============================================================================

CJinjaUltraEngine* cjinja_ultra_create_engine(void) {
    CJinjaUltraEngine* engine = calloc(1, sizeof(CJinjaUltraEngine));
    if (!engine) return NULL;
    
    // Allocate template cache
    engine->template_cache = calloc(1, sizeof(CJinjaTemplateCache));
    if (!engine->template_cache) {
        free(engine);
        return NULL;
    }
    
    // Allocate memory pool
    engine->memory_pool = malloc(MEMORY_POOL_SIZE);
    if (!engine->memory_pool) {
        free(engine->template_cache);
        free(engine);
        return NULL;
    }
    
    engine->memory_pool_used = 0;
    engine->simd_enabled = true; // Enable by default
    
    return engine;
}

void cjinja_ultra_destroy_engine(CJinjaUltraEngine* engine) {
    if (!engine) return;
    
    // Cleanup template cache
    if (engine->template_cache) {
        for (size_t i = 0; i < engine->template_cache->cache_count; i++) {
            CJinjaCompiledTemplate* template = &engine->template_cache->templates[i];
            free(template->segments);
            free(template->original_template);
        }
        free(engine->template_cache);
    }
    
    free(engine->memory_pool);
    free(engine);
}

void cjinja_ultra_set_simd_enabled(CJinjaUltraEngine* engine, bool enabled) {
    if (engine) {
        engine->simd_enabled = enabled;
    }
}

// =============================================================================
// ULTRA-FAST VARIABLE MANAGEMENT
// =============================================================================

void cjinja_ultra_set_var_fast(CJinjaUltraContext* ctx, const char* key, const char* value, uint32_t key_hash) {
    if (!ctx || !key || !value) return;
    
    size_t key_len = strlen(key);
    size_t value_len = strlen(value);
    
    if (key_len >= MAX_VARIABLE_NAME_LEN || value_len >= MAX_VARIABLE_VALUE_LEN) {
        return; // Skip oversized variables
    }
    
    uint32_t bucket = key_hash & HASH_TABLE_MASK;
    
    // Check if variable already exists
    CJinjaHashEntry* entry = ctx->buckets[bucket];
    while (entry) {
        if (entry->key_hash == key_hash && entry->key_len == key_len && 
            memcmp(entry->key, key, key_len) == 0) {
            // Update existing variable
            memcpy(entry->value, value, value_len);
            entry->value[value_len] = '\0';
            entry->value_len = value_len;
            return;
        }
        entry = entry->next;
    }
    
    // Add new variable
    if (ctx->pool_used >= 1024) return; // Pool exhausted
    
    entry = &ctx->pool[ctx->pool_used++];
    memcpy(entry->key, key, key_len);
    entry->key[key_len] = '\0';
    memcpy(entry->value, value, value_len);
    entry->value[value_len] = '\0';
    entry->key_hash = key_hash;
    entry->key_len = key_len;
    entry->value_len = value_len;
    entry->type = 0; // string
    
    // Insert at head of bucket
    entry->next = ctx->buckets[bucket];
    if (ctx->buckets[bucket]) {
        ctx->collision_count++;
    }
    ctx->buckets[bucket] = entry;
    ctx->total_variables++;
}

void cjinja_ultra_set_var(CJinjaUltraContext* ctx, const char* key, const char* value) {
    if (!ctx || !key) return;
    
    uint32_t key_hash = cjinja_ultra_hash(key, strlen(key));
    cjinja_ultra_set_var_fast(ctx, key, value, key_hash);
}

const char* cjinja_ultra_get_var_fast(CJinjaUltraContext* ctx, const char* key, uint32_t key_hash, size_t key_len) {
    if (!ctx || !key) return NULL;
    
    ctx->lookup_count++;
    
    uint32_t bucket = key_hash & HASH_TABLE_MASK;
    CJinjaHashEntry* entry = ctx->buckets[bucket];
    
    while (entry) {
        if (entry->key_hash == key_hash && entry->key_len == key_len) {
            // Fast memcmp for short keys (most variables are short)
            if (key_len <= 8) {
                if (*(uint64_t*)entry->key == *(uint64_t*)key) {
                    return entry->value;
                }
            } else if (memcmp(entry->key, key, key_len) == 0) {
                return entry->value;
            }
        }
        entry = entry->next;
    }
    
    return NULL;
}

const char* cjinja_ultra_get_var(CJinjaUltraContext* ctx, const char* key) {
    if (!ctx || !key) return NULL;
    
    size_t key_len = strlen(key);
    uint32_t key_hash = cjinja_ultra_hash(key, key_len);
    return cjinja_ultra_get_var_fast(ctx, key, key_hash, key_len);
}

// =============================================================================
// SIMD-OPTIMIZED STRING OPERATIONS
// =============================================================================

void cjinja_ultra_memcpy_simd(char* dest, const char* src, size_t len) {
    #ifdef __AVX2__
    if (len >= 32) {
        size_t simd_len = len & ~31; // Round down to 32-byte boundary
        
        for (size_t i = 0; i < simd_len; i += 32) {
            __m256i data = _mm256_loadu_si256((__m256i*)(src + i));
            _mm256_storeu_si256((__m256i*)(dest + i), data);
        }
        
        // Copy remaining bytes
        memcpy(dest + simd_len, src + simd_len, len - simd_len);
    } else
    #endif
    {
        memcpy(dest, src, len);
    }
}

// =============================================================================
// ULTRA-FAST TEMPLATE PARSING
// =============================================================================

/**
 * @brief Branchless character classification lookup table
 */
static const uint8_t char_class_table[256] = {
    ['{'] = 1, ['}'] = 2, ['%'] = 4, [' '] = 8, ['\t'] = 8, ['\n'] = 8, ['\r'] = 8
};

#define IS_BRACE_OPEN(c) (char_class_table[(uint8_t)(c)] & 1)
#define IS_BRACE_CLOSE(c) (char_class_table[(uint8_t)(c)] & 2)
#define IS_PERCENT(c) (char_class_table[(uint8_t)(c)] & 4)
#define IS_WHITESPACE(c) (char_class_table[(uint8_t)(c)] & 8)

char* cjinja_ultra_render_variables(const char* template_str, CJinjaUltraContext* ctx) {
    if (!template_str || !ctx) return NULL;
    
    uint64_t start_time = __builtin_ia32_rdtsc(); // Use RDTSC for precise timing
    
    size_t template_len = strlen(template_str);
    size_t buffer_size = template_len * 2; // Estimate
    char* buffer = malloc(buffer_size);
    if (!buffer) return NULL;
    
    size_t buffer_pos = 0;
    const char* pos = template_str;
    const char* end = template_str + template_len;
    
    while (pos < end) {
        // Branchless variable detection
        uint8_t c1 = *pos;
        uint8_t c2 = (pos + 1 < end) ? *(pos + 1) : 0;
        
        if (c1 == '{' && c2 == '{') {
            // Variable substitution found
            pos += 2;
            const char* var_start = pos;
            
            // Fast variable name scanning using SIMD when possible
            #ifdef __SSE2__
            if (pos + 16 <= end) {
                __m128i brace_pattern = _mm_set1_epi8('}');
                
                while (pos + 16 <= end) {
                    __m128i chunk = _mm_loadu_si128((__m128i*)pos);
                    __m128i cmp = _mm_cmpeq_epi8(chunk, brace_pattern);
                    int mask = _mm_movemask_epi8(cmp);
                    
                    if (mask) {
                        int offset = __builtin_ctz(mask); // Count trailing zeros
                        pos += offset;
                        break;
                    }
                    pos += 16;
                }
            }
            #endif
            
            // Fallback to scalar scanning
            while (pos < end && *pos != '}') pos++;
            
            if (pos + 1 < end && *pos == '}' && *(pos + 1) == '}') {
                // Extract variable name
                size_t var_len = pos - var_start;
                if (var_len < MAX_VARIABLE_NAME_LEN) {
                    char var_name[MAX_VARIABLE_NAME_LEN];
                    memcpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';
                    
                    // Ultra-fast variable lookup
                    uint32_t var_hash = cjinja_ultra_hash(var_name, var_len);
                    const char* var_value = cjinja_ultra_get_var_fast(ctx, var_name, var_hash, var_len);
                    
                    if (var_value) {
                        size_t value_len = strlen(var_value);
                        
                        // Ensure buffer capacity
                        if (buffer_pos + value_len >= buffer_size) {
                            buffer_size = (buffer_pos + value_len) * 2;
                            buffer = realloc(buffer, buffer_size);
                            if (!buffer) return NULL;
                        }
                        
                        // SIMD-optimized copy
                        cjinja_ultra_memcpy_simd(buffer + buffer_pos, var_value, value_len);
                        buffer_pos += value_len;
                    }
                }
                pos += 2; // Skip }}
            }
        } else {
            // Regular character - use vectorized copy for long text runs
            const char* text_start = pos;
            
            // Find next variable or end of template
            #ifdef __SSE2__
            if (pos + 16 <= end) {
                __m128i brace_pattern = _mm_set1_epi8('{');
                
                while (pos + 16 <= end) {
                    __m128i chunk = _mm_loadu_si128((__m128i*)pos);
                    __m128i cmp = _mm_cmpeq_epi8(chunk, brace_pattern);
                    int mask = _mm_movemask_epi8(cmp);
                    
                    if (mask) {
                        int offset = __builtin_ctz(mask);
                        pos += offset;
                        break;
                    }
                    pos += 16;
                }
            }
            #endif
            
            // Fallback to scalar scanning
            while (pos < end && *pos != '{') pos++;
            
            size_t text_len = pos - text_start;
            if (text_len > 0) {
                // Ensure buffer capacity
                if (buffer_pos + text_len >= buffer_size) {
                    buffer_size = (buffer_pos + text_len) * 2;
                    buffer = realloc(buffer, buffer_size);
                    if (!buffer) return NULL;
                }
                
                // SIMD-optimized copy
                cjinja_ultra_memcpy_simd(buffer + buffer_pos, text_start, text_len);
                buffer_pos += text_len;
            }
        }
    }
    
    buffer[buffer_pos] = '\0';
    
    uint64_t end_time = __builtin_ia32_rdtsc();
    uint64_t cycles = end_time - start_time;
    
    // Update performance stats (simplified)
    ctx->lookup_count++;
    
    return buffer;
}

char* cjinja_ultra_render_branchless(const char* template_str, CJinjaUltraContext* ctx) {
    if (!template_str || !ctx) return NULL;
    
    size_t template_len = strlen(template_str);
    char* buffer = malloc(template_len * 2);
    if (!buffer) return NULL;
    
    size_t buffer_pos = 0;
    const char* pos = template_str;
    
    // Branchless parsing using lookup tables and bit manipulation
    while (*pos) {
        // Use lookup table for character classification
        uint8_t char_type = char_class_table[(uint8_t)*pos];
        
        // Branchless variable detection
        int is_var_start = (pos[0] == '{' && pos[1] == '{') ? 1 : 0;
        
        if (is_var_start) {
            pos += 2;
            const char* var_start = pos;
            
            // Find variable end
            while (*pos && !(*pos == '}' && pos[1] == '}')) pos++;
            
            if (*pos == '}' && pos[1] == '}') {
                size_t var_len = pos - var_start;
                char var_name[64];
                memcpy(var_name, var_start, var_len);
                var_name[var_len] = '\0';
                
                const char* value = cjinja_ultra_get_var(ctx, var_name);
                if (value) {
                    size_t value_len = strlen(value);
                    memcpy(buffer + buffer_pos, value, value_len);
                    buffer_pos += value_len;
                }
                pos += 2;
            }
        } else {
            buffer[buffer_pos++] = *pos++;
        }
    }
    
    buffer[buffer_pos] = '\0';
    return buffer;
}

// =============================================================================
// TEMPLATE COMPILATION AND CACHING
// =============================================================================

CJinjaCompiledTemplate* cjinja_ultra_compile_template(const char* template_str) {
    if (!template_str) return NULL;
    
    CJinjaCompiledTemplate* template = calloc(1, sizeof(CJinjaCompiledTemplate));
    if (!template) return NULL;
    
    template->original_template = strdup(template_str);
    template->template_hash = cjinja_ultra_hash(template_str, strlen(template_str));
    template->compile_time_ns = __builtin_ia32_rdtsc(); // Simplified timing
    
    // Pre-allocate segments
    template->allocated_segments = 32;
    template->segments = malloc(sizeof(CJinjaTemplateSegment) * template->allocated_segments);
    if (!template->segments) {
        free(template->original_template);
        free(template);
        return NULL;
    }
    
    // Parse template into segments
    const char* pos = template_str;
    size_t segment_count = 0;
    
    while (*pos) {
        if (segment_count >= template->allocated_segments) {
            template->allocated_segments *= 2;
            template->segments = realloc(template->segments, 
                sizeof(CJinjaTemplateSegment) * template->allocated_segments);
            if (!template->segments) break;
        }
        
        CJinjaTemplateSegment* seg = &template->segments[segment_count];
        
        if (pos[0] == '{' && pos[1] == '{') {
            // Variable segment
            seg->type = CJINJA_SEG_VARIABLE;
            pos += 2;
            
            const char* var_start = pos;
            while (*pos && !(*pos == '}' && pos[1] == '}')) pos++;
            
            if (*pos == '}' && pos[1] == '}') {
                size_t var_len = pos - var_start;
                if (var_len < MAX_VARIABLE_NAME_LEN) {
                    memcpy(seg->data.var_seg.var_name, var_start, var_len);
                    seg->data.var_seg.var_name[var_len] = '\0';
                    seg->data.var_seg.var_len = var_len;
                    seg->data.var_seg.var_hash = cjinja_ultra_hash(var_start, var_len);
                    seg->data.var_seg.has_filter = false;
                    
                    segment_count++;
                }
                pos += 2;
            }
        } else {
            // Text segment
            seg->type = CJINJA_SEG_TEXT;
            const char* text_start = pos;
            
            while (*pos && !(*pos == '{' && pos[1] == '{')) pos++;
            
            seg->data.text_seg.text = text_start;
            seg->data.text_seg.length = pos - text_start;
            segment_count++;
        }
    }
    
    template->segment_count = segment_count;
    template->compile_time_ns = __builtin_ia32_rdtsc() - template->compile_time_ns;
    
    return template;
}

char* cjinja_ultra_render_precompiled(CJinjaCompiledTemplate* template, CJinjaUltraContext* ctx) {
    if (!template || !ctx) return NULL;
    
    template->usage_count++;
    
    size_t buffer_size = 4096;
    char* buffer = malloc(buffer_size);
    if (!buffer) return NULL;
    
    size_t buffer_pos = 0;
    
    for (size_t i = 0; i < template->segment_count; i++) {
        CJinjaTemplateSegment* seg = &template->segments[i];
        
        if (seg->type == CJINJA_SEG_TEXT) {
            // Copy text segment
            size_t text_len = seg->data.text_seg.length;
            if (buffer_pos + text_len >= buffer_size) {
                buffer_size = (buffer_pos + text_len) * 2;
                buffer = realloc(buffer, buffer_size);
                if (!buffer) return NULL;
            }
            
            cjinja_ultra_memcpy_simd(buffer + buffer_pos, seg->data.text_seg.text, text_len);
            buffer_pos += text_len;
            
        } else if (seg->type == CJINJA_SEG_VARIABLE) {
            // Substitute variable
            const char* value = cjinja_ultra_get_var_fast(ctx, 
                seg->data.var_seg.var_name,
                seg->data.var_seg.var_hash,
                seg->data.var_seg.var_len);
            
            if (value) {
                size_t value_len = strlen(value);
                if (buffer_pos + value_len >= buffer_size) {
                    buffer_size = (buffer_pos + value_len) * 2;
                    buffer = realloc(buffer, buffer_size);
                    if (!buffer) return NULL;
                }
                
                cjinja_ultra_memcpy_simd(buffer + buffer_pos, value, value_len);
                buffer_pos += value_len;
            }
        }
    }
    
    buffer[buffer_pos] = '\0';
    return buffer;
}

char* cjinja_ultra_render_compiled(CJinjaUltraEngine* engine, const char* template_str, CJinjaUltraContext* ctx) {
    if (!engine || !template_str || !ctx) return NULL;
    
    uint32_t template_hash = cjinja_ultra_hash(template_str, strlen(template_str));
    
    // Look for cached template
    for (size_t i = 0; i < engine->template_cache->cache_count; i++) {
        CJinjaCompiledTemplate* template = &engine->template_cache->templates[i];
        if (template->template_hash == template_hash && 
            strcmp(template->original_template, template_str) == 0) {
            engine->template_cache->cache_hits++;
            return cjinja_ultra_render_precompiled(template, ctx);
        }
    }
    
    // Template not cached, compile it
    engine->template_cache->cache_misses++;
    
    if (engine->template_cache->cache_count >= TEMPLATE_CACHE_SIZE) {
        // Cache full, use fallback rendering
        return cjinja_ultra_render_variables(template_str, ctx);
    }
    
    // Compile and cache template
    CJinjaCompiledTemplate* new_template = cjinja_ultra_compile_template(template_str);
    if (!new_template) {
        return cjinja_ultra_render_variables(template_str, ctx);
    }
    
    // Add to cache
    size_t cache_index = engine->template_cache->cache_count++;
    engine->template_cache->templates[cache_index] = *new_template;
    free(new_template); // Move ownership to cache
    
    return cjinja_ultra_render_precompiled(&engine->template_cache->templates[cache_index], ctx);
}

void cjinja_ultra_destroy_template(CJinjaCompiledTemplate* template) {
    if (!template) return;
    
    free(template->segments);
    free(template->original_template);
    free(template);
}

// =============================================================================
// PERFORMANCE MONITORING
// =============================================================================

void cjinja_ultra_get_stats(CJinjaUltraEngine* engine, CJinjaUltraContext* ctx, CJinjaUltraStats* stats) {
    if (!stats) return;
    
    memset(stats, 0, sizeof(CJinjaUltraStats));
    
    if (engine) {
        stats->total_renders = engine->render_count;
        stats->avg_render_time_ns = engine->render_count > 0 ? 
            engine->total_render_time_ns / engine->render_count : 0;
        stats->memory_pool_used = engine->memory_pool_used;
        stats->memory_efficiency = (double)engine->memory_pool_used / MEMORY_POOL_SIZE;
        
        if (engine->template_cache) {
            stats->cache_hits = engine->template_cache->cache_hits;
            stats->cache_misses = engine->template_cache->cache_misses;
            uint64_t total_cache_ops = stats->cache_hits + stats->cache_misses;
            stats->cache_hit_rate = total_cache_ops > 0 ? 
                (double)stats->cache_hits / total_cache_ops : 0.0;
        }
    }
    
    if (ctx) {
        stats->hash_lookups = ctx->lookup_count;
        stats->hash_collisions = ctx->collision_count;
        stats->collision_rate = stats->hash_lookups > 0 ? 
            (double)stats->hash_collisions / stats->hash_lookups : 0.0;
    }
}

void cjinja_ultra_reset_stats(CJinjaUltraEngine* engine, CJinjaUltraContext* ctx) {
    if (engine) {
        engine->render_count = 0;
        engine->total_render_time_ns = 0;
        
        if (engine->template_cache) {
            engine->template_cache->cache_hits = 0;
            engine->template_cache->cache_misses = 0;
        }
    }
    
    if (ctx) {
        ctx->lookup_count = 0;
        ctx->collision_count = 0;
    }
}

// =============================================================================
// BENCHMARKING
// =============================================================================

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

CJinjaBenchmarkResult cjinja_ultra_benchmark_variables(size_t iterations) {
    CJinjaBenchmarkResult result = {0};
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    if (!ctx) return result;
    
    // Setup test variables
    cjinja_ultra_set_var(ctx, "name", "John");
    cjinja_ultra_set_var(ctx, "company", "Acme Corp");
    cjinja_ultra_set_var(ctx, "title", "Engineer");
    
    const char* template = "Hello {{name}} from {{company}}, you are a {{title}}!";
    
    result.iterations = iterations;
    result.min_time_ns = UINT64_MAX;
    result.max_time_ns = 0;
    
    uint64_t start_total = get_time_ns();
    
    for (size_t i = 0; i < iterations; i++) {
        uint64_t start = __builtin_ia32_rdtsc();
        char* rendered = cjinja_ultra_render_variables(template, ctx);
        uint64_t end = __builtin_ia32_rdtsc();
        
        uint64_t cycles = end - start;
        uint64_t time_ns = cycles * 1000000000ULL / 3000000000ULL; // Assume 3GHz CPU
        
        if (time_ns < result.min_time_ns) result.min_time_ns = time_ns;
        if (time_ns > result.max_time_ns) result.max_time_ns = time_ns;
        result.total_time_ns += time_ns;
        
        free(rendered);
    }
    
    uint64_t end_total = get_time_ns();
    result.total_time_ns = end_total - start_total;
    result.avg_time_ns = result.total_time_ns / iterations;
    result.ops_per_second = 1000000000.0 / result.avg_time_ns;
    
    cjinja_ultra_destroy_context(ctx);
    return result;
}

void cjinja_ultra_benchmark_comparison(void) {
    printf("=== CJinja Ultra-Fast Performance Benchmark ===\n\n");
    
    CJinjaBenchmarkResult ultra_result = cjinja_ultra_benchmark_variables(10000);
    
    printf("Ultra-Fast Implementation Results:\n");
    printf("  Iterations: %lu\n", ultra_result.iterations);
    printf("  Average time: %lu ns\n", ultra_result.avg_time_ns);
    printf("  Min time: %lu ns\n", ultra_result.min_time_ns);
    printf("  Max time: %lu ns\n", ultra_result.max_time_ns);
    printf("  Operations/second: %.0f\n", ultra_result.ops_per_second);
    
    if (ultra_result.avg_time_ns < 100) {
        printf("  ✅ TARGET ACHIEVED: Sub-100ns variable substitution!\n");
    } else {
        printf("  ⚠️  Target missed: %lu ns (target: <100ns)\n", ultra_result.avg_time_ns);
    }
    
    printf("\nPerformance improvement over original 206ns:\n");
    printf("  Speedup: %.2fx\n", 206.0 / ultra_result.avg_time_ns);
    printf("  Time reduction: %.1f%%\n", (206.0 - ultra_result.avg_time_ns) / 206.0 * 100);
}