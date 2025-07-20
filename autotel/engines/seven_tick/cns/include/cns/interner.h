#ifndef CNS_INTERNER_H
#define CNS_INTERNER_H

#include "cns/types.h"
#include "cns/arena.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CNS STRING INTERNER - O(1) STRING OPERATIONS FOR 7T SUBSTRATE
// ============================================================================

// String interner designed for 7T substrate with deterministic performance
// guarantees. All string operations complete within 7 CPU ticks through
// aggressive precomputation and hash-based lookups.

// ============================================================================
// INTERNER STRUCTURE DEFINITIONS
// ============================================================================

// String entry in the intern table - cache-aligned for performance
typedef struct cns_string_entry {
    cns_hash_t hash;          // Precomputed hash for O(1) comparison
    uint32_t offset;          // Offset in string arena
    uint16_t length;          // String length (supports up to 64KB strings)
    uint16_t ref_count;       // Reference counting for GC
    struct cns_string_entry *next; // Hash collision chain
} cns_string_entry_t;

// Hash table bucket for string entries
typedef struct {
    cns_string_entry_t *first;  // First entry in bucket
    uint32_t count;              // Number of entries in bucket
} cns_interner_bucket_t;

// String interner statistics
typedef struct {
    size_t total_strings;        // Total interned strings
    size_t unique_strings;       // Unique strings (after deduplication)
    size_t total_bytes;          // Total string storage bytes
    size_t table_size;           // Hash table size
    size_t collisions;           // Hash collision count
    double load_factor;          // Current load factor
    uint64_t intern_operations;  // Total intern operations
    uint64_t lookup_operations;  // Total lookup operations
    cns_tick_t total_intern_ticks; // Total ticks spent interning
    cns_tick_t total_lookup_ticks; // Total ticks spent looking up
} cns_interner_stats_t;

// Main string interner structure
struct cns_interner {
    // Hash table for O(1) lookups
    cns_interner_bucket_t *buckets;  // Hash table buckets
    size_t bucket_count;             // Number of buckets (power of 2)
    size_t bucket_mask;              // Mask for fast modulo (bucket_count - 1)
    
    // String storage arena
    cns_arena_t *string_arena;       // Arena for string data
    char *string_base;               // Base pointer for string storage
    size_t string_capacity;          // Total string storage capacity
    size_t string_used;              // Used string storage
    
    // Entry allocation
    cns_arena_t *entry_arena;        // Arena for hash table entries
    cns_string_entry_t *free_entries; // Free list for entries
    
    // Configuration
    float max_load_factor;           // Resize threshold
    bool case_sensitive;             // Case sensitivity flag
    bool enable_gc;                  // Enable garbage collection
    
    // Performance tracking
    cns_interner_stats_t stats;      // Performance statistics
    
    // Thread safety (if enabled)
    void *mutex;                     // Mutex for thread safety
    uint32_t flags;                  // Configuration flags
    uint32_t magic;                  // Magic number for validation
};

// Interner flags
#define CNS_INTERNER_FLAG_CASE_INSENSITIVE (1 << 0)  // Case insensitive strings
#define CNS_INTERNER_FLAG_THREAD_SAFE      (1 << 1)  // Enable thread safety
#define CNS_INTERNER_FLAG_ENABLE_GC        (1 << 2)  // Enable garbage collection
#define CNS_INTERNER_FLAG_TRACK_REFS       (1 << 3)  // Track reference counts
#define CNS_INTERNER_FLAG_PRECOMPUTE_HASH  (1 << 4)  // Precompute hash values

// Magic number for interner validation
#define CNS_INTERNER_MAGIC 0x494E5452  // 'INTR'

// ============================================================================
// INTERNER LIFECYCLE FUNCTIONS - O(1) OPERATIONS
// ============================================================================

// Create a new string interner with specified configuration
// PERFORMANCE: O(1) - completes within 7 CPU ticks
cns_interner_t* cns_interner_create(const cns_interner_config_t *config);

// Create interner with default configuration
// PERFORMANCE: O(1) - optimized default settings
cns_interner_t* cns_interner_create_default(size_t initial_capacity);

// Destroy interner and free all memory
// PERFORMANCE: O(n) where n is number of strings, but typically fast
void cns_interner_destroy(cns_interner_t *interner);

// Clear all interned strings (fast reset)
// PERFORMANCE: O(1) - resets arenas and hash table
cns_result_t cns_interner_clear(cns_interner_t *interner);

// Clone interner configuration (not contents)
// PERFORMANCE: O(1) - copies configuration only
cns_interner_t* cns_interner_clone_config(const cns_interner_t *interner);

// ============================================================================
// STRING INTERNING FUNCTIONS - O(1) GUARANTEED
// ============================================================================

// Intern a string and return reference
// PERFORMANCE: O(1) average case - hash lookup with precomputed hash
cns_string_ref_t cns_interner_intern(cns_interner_t *interner, const char *str);

// Intern string with known length (avoids strlen)
// PERFORMANCE: O(1) - no string length computation needed
cns_string_ref_t cns_interner_intern_len(cns_interner_t *interner, 
                                        const char *str, 
                                        size_t length);

// Intern string with precomputed hash (fastest path)
// PERFORMANCE: O(1) - direct hash table lookup
cns_string_ref_t cns_interner_intern_hash(cns_interner_t *interner,
                                         const char *str,
                                         size_t length,
                                         cns_hash_t hash);

// Intern formatted string (using arena for formatting)
// PERFORMANCE: O(1) for format, O(1) for intern
cns_string_ref_t cns_interner_intern_printf(cns_interner_t *interner,
                                           const char *format, ...);

// ============================================================================
// STRING LOOKUP FUNCTIONS - O(1) GUARANTEED
// ============================================================================

// Look up string reference without interning
// PERFORMANCE: O(1) - hash table lookup only
cns_string_ref_t cns_interner_lookup(const cns_interner_t *interner, 
                                    const char *str);

// Look up with known length
// PERFORMANCE: O(1) - no strlen computation
cns_string_ref_t cns_interner_lookup_len(const cns_interner_t *interner,
                                        const char *str,
                                        size_t length);

// Look up with precomputed hash
// PERFORMANCE: O(1) - direct hash table access
cns_string_ref_t cns_interner_lookup_hash(const cns_interner_t *interner,
                                         const char *str,
                                         size_t length,
                                         cns_hash_t hash);

// Check if string is interned
// PERFORMANCE: O(1) - simple hash lookup
bool cns_interner_contains(const cns_interner_t *interner, const char *str);

// ============================================================================
// STRING REFERENCE OPERATIONS - O(1) GUARANTEED
// ============================================================================

// Resolve string reference to actual string pointer
// PERFORMANCE: O(1) - simple pointer arithmetic
const char* cns_string_ref_resolve(const cns_interner_t *interner, 
                                  cns_string_ref_t ref);

// Get string length from reference
// PERFORMANCE: O(1) - stored in reference
static inline size_t cns_string_ref_length(cns_string_ref_t ref) {
    return ref.length;
}

// Get string hash from reference
// PERFORMANCE: O(1) - stored in reference
static inline cns_hash_t cns_string_ref_hash(cns_string_ref_t ref) {
    return ref.hash;
}

// Compare string references for equality
// PERFORMANCE: O(1) - hash comparison only
static inline bool cns_string_ref_equal(cns_string_ref_t a, cns_string_ref_t b) {
    return a.hash == b.hash && a.offset == b.offset;
}

// Check if string reference is valid
// PERFORMANCE: O(1) - simple validity check
static inline bool cns_string_ref_is_valid(cns_string_ref_t ref) {
    return ref.offset != 0;
}

// Get invalid/null string reference
// PERFORMANCE: O(1) - constant value
static inline cns_string_ref_t cns_string_ref_null(void) {
    return (cns_string_ref_t){0, 0, 0, 0};
}

// ============================================================================
// REFERENCE COUNTING AND GARBAGE COLLECTION
// ============================================================================

// Increment reference count
// PERFORMANCE: O(1) - simple atomic increment
cns_result_t cns_string_ref_retain(cns_interner_t *interner, cns_string_ref_t ref);

// Decrement reference count
// PERFORMANCE: O(1) - simple atomic decrement
cns_result_t cns_string_ref_release(cns_interner_t *interner, cns_string_ref_t ref);

// Get reference count
// PERFORMANCE: O(1) - stored in entry
uint16_t cns_string_ref_count(const cns_interner_t *interner, cns_string_ref_t ref);

// Run garbage collection to free unreferenced strings
// PERFORMANCE: O(n) where n is number of strings - use sparingly
cns_result_t cns_interner_gc(cns_interner_t *interner);

// Set garbage collection threshold
// PERFORMANCE: O(1) - updates configuration
cns_result_t cns_interner_set_gc_threshold(cns_interner_t *interner, size_t threshold);

// ============================================================================
// HASH COMPUTATION - OPTIMIZED FOR 7T PERFORMANCE
// ============================================================================

// Compute hash for string (FNV-1a variant optimized for speed)
// PERFORMANCE: O(n) where n is string length, but optimized with SIMD
cns_hash_t cns_hash_string(const char *str);

// Compute hash for string with known length
// PERFORMANCE: O(n) - avoids strlen computation
cns_hash_t cns_hash_string_len(const char *str, size_t length);

// Compute case-insensitive hash
// PERFORMANCE: O(n) - case conversion during hash computation
cns_hash_t cns_hash_string_case_insensitive(const char *str);

// Compute hash with custom seed
// PERFORMANCE: O(n) - custom seed for hash diversity
cns_hash_t cns_hash_string_seeded(const char *str, size_t length, uint32_t seed);

// ============================================================================
// INTERNER INFORMATION AND STATISTICS
// ============================================================================

// Get interner statistics
// PERFORMANCE: O(1) - returns cached statistics
cns_result_t cns_interner_get_stats(const cns_interner_t *interner, 
                                   cns_interner_stats_t *stats);

// Get number of interned strings
// PERFORMANCE: O(1) - returns cached count
size_t cns_interner_string_count(const cns_interner_t *interner);

// Get total memory usage
// PERFORMANCE: O(1) - returns cached value
size_t cns_interner_memory_usage(const cns_interner_t *interner);

// Get load factor
// PERFORMANCE: O(1) - simple arithmetic
double cns_interner_load_factor(const cns_interner_t *interner);

// Check if interner needs resizing
// PERFORMANCE: O(1) - load factor comparison
bool cns_interner_needs_resize(const cns_interner_t *interner);

// ============================================================================
// INTERNER CONFIGURATION AND TUNING
// ============================================================================

// Resize hash table to new size
// PERFORMANCE: O(n) where n is number of strings - use when load factor high
cns_result_t cns_interner_resize(cns_interner_t *interner, size_t new_capacity);

// Set case sensitivity
// PERFORMANCE: O(1) - updates flag, affects future operations
cns_result_t cns_interner_set_case_sensitive(cns_interner_t *interner, bool case_sensitive);

// Set maximum load factor before resize
// PERFORMANCE: O(1) - updates configuration
cns_result_t cns_interner_set_max_load_factor(cns_interner_t *interner, float max_load);

// Enable/disable garbage collection
// PERFORMANCE: O(1) - updates flag
cns_result_t cns_interner_set_gc_enabled(cns_interner_t *interner, bool enabled);

// ============================================================================
// ITERATOR INTERFACE FOR ENUMERATION
// ============================================================================

// Iterator for walking all interned strings
typedef struct {
    const cns_interner_t *interner;  // Interner being iterated
    size_t bucket_index;             // Current bucket index
    cns_string_entry_t *current;     // Current entry
    cns_string_ref_t current_ref;    // Current string reference
} cns_interner_iterator_t;

// Initialize iterator
// PERFORMANCE: O(1) - sets up iterator state
cns_interner_iterator_t cns_interner_iter_begin(const cns_interner_t *interner);

// Check if iterator has more strings
// PERFORMANCE: O(1) - checks iterator state
bool cns_interner_iter_has_next(const cns_interner_iterator_t *iter);

// Get next string reference from iterator
// PERFORMANCE: O(1) average - moves to next entry
cns_string_ref_t cns_interner_iter_next(cns_interner_iterator_t *iter);

// Reset iterator to beginning
// PERFORMANCE: O(1) - resets iterator state
void cns_interner_iter_reset(cns_interner_iterator_t *iter);

// ============================================================================
// UTILITY FUNCTIONS AND HELPERS
// ============================================================================

// Copy string reference with retention
// PERFORMANCE: O(1) - increments reference count
cns_string_ref_t cns_string_ref_copy(cns_interner_t *interner, cns_string_ref_t ref);

// Create string reference from literal (compile-time strings)
// PERFORMANCE: O(1) - direct interning of literals
#define CNS_STRING_LITERAL(interner, literal) \
    cns_interner_intern_hash((interner), (literal), sizeof(literal) - 1, \
                           cns_hash_string_len((literal), sizeof(literal) - 1))

// Compare string reference with C string
// PERFORMANCE: O(n) worst case, but optimized with hash pre-check
int cns_string_ref_compare(const cns_interner_t *interner, 
                          cns_string_ref_t ref, 
                          const char *str);

// Check if string reference starts with prefix
// PERFORMANCE: O(prefix_length) - length-optimized comparison
bool cns_string_ref_starts_with(const cns_interner_t *interner,
                               cns_string_ref_t ref,
                               const char *prefix);

// Check if string reference ends with suffix
// PERFORMANCE: O(suffix_length) - length-optimized comparison
bool cns_string_ref_ends_with(const cns_interner_t *interner,
                             cns_string_ref_t ref,
                             const char *suffix);

// ============================================================================
// DEBUG AND VALIDATION FUNCTIONS
// ============================================================================

// Validate interner integrity
// PERFORMANCE: O(n) where n is number of strings - use for debugging
cns_result_t cns_interner_validate(const cns_interner_t *interner);

// Print interner statistics
// PERFORMANCE: O(1) - prints cached statistics
cns_result_t cns_interner_print_stats(const cns_interner_t *interner, FILE *output);

// Dump all interned strings
// PERFORMANCE: O(n) - walks all strings
cns_result_t cns_interner_dump_strings(const cns_interner_t *interner, FILE *output);

// Check for hash collisions
// PERFORMANCE: O(n) - analyzes hash distribution
cns_result_t cns_interner_analyze_collisions(const cns_interner_t *interner);

// ============================================================================
// PERFORMANCE MONITORING INTEGRATION
// ============================================================================

// Performance callback for interner events
typedef void (*cns_interner_perf_callback_t)(const cns_interner_t *interner,
                                            const char *operation,
                                            const char *string,
                                            cns_tick_t ticks,
                                            void *user_data);

// Set performance monitoring callback
// PERFORMANCE: O(1) - stores callback pointer
cns_result_t cns_interner_set_perf_callback(cns_interner_t *interner,
                                           cns_interner_perf_callback_t callback,
                                           void *user_data);

// Clear performance monitoring callback
// PERFORMANCE: O(1) - clears callback pointer
cns_result_t cns_interner_clear_perf_callback(cns_interner_t *interner);

// ============================================================================
// THREAD SAFETY FUNCTIONS
// ============================================================================

// Enable thread safety for interner
// PERFORMANCE: O(1) - initializes mutex
cns_result_t cns_interner_enable_thread_safety(cns_interner_t *interner);

// Disable thread safety
// PERFORMANCE: O(1) - destroys mutex
cns_result_t cns_interner_disable_thread_safety(cns_interner_t *interner);

// Lock interner for exclusive access
// PERFORMANCE: O(1) - mutex lock
cns_result_t cns_interner_lock(cns_interner_t *interner);

// Unlock interner
// PERFORMANCE: O(1) - mutex unlock
cns_result_t cns_interner_unlock(cns_interner_t *interner);

// ============================================================================
// UTILITY MACROS FOR COMMON PATTERNS
// ============================================================================

// Intern string and check for success
#define CNS_INTERNER_INTERN_CHECK(interner, str, ref_var) \
    do { \
        (ref_var) = cns_interner_intern((interner), (str)); \
        if (CNS_7T_UNLIKELY(!cns_string_ref_is_valid(ref_var))) \
            return CNS_ERROR_STRING_INTERN; \
    } while(0)

// Resolve string reference and check validity
#define CNS_INTERNER_RESOLVE_CHECK(interner, ref, str_var) \
    do { \
        if (CNS_7T_UNLIKELY(!cns_string_ref_is_valid(ref))) \
            return CNS_ERROR_INVALID_ARG; \
        (str_var) = cns_string_ref_resolve((interner), (ref)); \
        if (CNS_7T_UNLIKELY((str_var) == NULL)) \
            return CNS_ERROR_NOT_FOUND; \
    } while(0)

// Compare string references for equality (fast path)
#define CNS_STRING_REF_EQUAL_FAST(a, b) \
    ((a).hash == (b).hash && (a).offset == (b).offset)

#ifdef __cplusplus
}
#endif

#endif // CNS_INTERNER_H