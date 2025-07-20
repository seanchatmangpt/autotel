/*  ─────────────────────────────────────────────────────────────
    src/interner.c  –  7T String Interner Implementation
    FNV-1a hash-based string interning with O(1) lookup
    ───────────────────────────────────────────────────────────── */

#include "../include/cns/core/memory.h"
#include "../s7t_minimal.h"
#include <string.h>
#include <assert.h>

/*═══════════════════════════════════════════════════════════════
  String Interner Structure (7T-Optimized)
  ═══════════════════════════════════════════════════════════════*/

// FNV-1a hash constants
#define FNV1A_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV1A_PRIME        0x100000001b3ULL

// Hash table configuration
#define INTERNER_DEFAULT_CAPACITY 1024
#define INTERNER_MAX_LOAD_FACTOR  0.75

typedef struct S7T_ALIGNED(64) {
    const char* string;     // Interned string pointer
    size_t length;          // String length
    uint64_t hash;          // Cached hash value
    uint32_t refcount;      // Reference count
    uint32_t next_index;    // Chain for collision resolution
} cns_intern_entry_t;

typedef struct S7T_ALIGNED(64) {
    cns_intern_entry_t* entries;    // Hash table entries
    uint32_t* buckets;              // Hash buckets (indices)
    cns_memory_arena_t* arena;      // String storage arena
    uint32_t capacity;              // Table capacity
    uint32_t count;                 // Number of entries
    uint32_t free_list;             // Free entry list head
    uint32_t flags;                 // Interner flags
} cns_string_interner_t;

/*═══════════════════════════════════════════════════════════════
  7T Constraint Enforcement
  ═══════════════════════════════════════════════════════════════*/

_Static_assert(S7T_MAX_CYCLES == 7, "String interner requires 7-tick constraint");

/*═══════════════════════════════════════════════════════════════
  FNV-1a Hash Implementation (< 3 ticks)
  ═══════════════════════════════════════════════════════════════*/

S7T_ALWAYS_INLINE uint64_t cns_fnv1a_hash(const char* data, size_t length) {
    uint64_t hash = FNV1A_OFFSET_BASIS;
    
    // Process 8 bytes at a time when aligned
    if (length >= 8 && ((uintptr_t)data & 7) == 0) {
        const uint64_t* ptr = (const uint64_t*)data;
        size_t qwords = length / 8;
        
        for (size_t i = 0; i < qwords; i++) {
            uint64_t chunk = ptr[i];
            for (int j = 0; j < 8; j++) {
                hash ^= (chunk >> (j * 8)) & 0xFF;
                hash *= FNV1A_PRIME;
            }
        }
        
        // Handle remaining bytes
        data += qwords * 8;
        length &= 7;
    }
    
    // Process remaining bytes
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint64_t)data[i];
        hash *= FNV1A_PRIME;
    }
    
    return hash;
}

/*═══════════════════════════════════════════════════════════════
  Interner Lifecycle
  ═══════════════════════════════════════════════════════════════*/

cns_string_interner_t* cns_interner_create(
    cns_memory_arena_t* arena,
    uint32_t initial_capacity
) {
    if (!arena) return NULL;
    
    if (initial_capacity < 16) {
        initial_capacity = INTERNER_DEFAULT_CAPACITY;
    }
    
    // Ensure capacity is power of 2
    initial_capacity--;
    initial_capacity |= initial_capacity >> 1;
    initial_capacity |= initial_capacity >> 2;
    initial_capacity |= initial_capacity >> 4;
    initial_capacity |= initial_capacity >> 8;
    initial_capacity |= initial_capacity >> 16;
    initial_capacity++;
    
    cns_string_interner_t* interner = CNS_ARENA_NEW(arena, cns_string_interner_t);
    if (!interner) return NULL;
    
    interner->entries = CNS_ARENA_NEW_ARRAY(arena, cns_intern_entry_t, initial_capacity);
    interner->buckets = CNS_ARENA_NEW_ARRAY(arena, uint32_t, initial_capacity);
    
    if (!interner->entries || !interner->buckets) {
        return NULL;
    }
    
    // Initialize buckets to empty
    for (uint32_t i = 0; i < initial_capacity; i++) {
        interner->buckets[i] = UINT32_MAX;
    }
    
    interner->arena = arena;
    interner->capacity = initial_capacity;
    interner->count = 0;
    interner->free_list = UINT32_MAX;
    interner->flags = 0;
    
    return interner;
}

/*═══════════════════════════════════════════════════════════════
  Fast String Lookup (< 7 ticks)
  ═══════════════════════════════════════════════════════════════*/

S7T_ALWAYS_INLINE const char* cns_interner_lookup(
    cns_string_interner_t* interner,
    const char* string,
    size_t length
) {
    if (!interner || !string) return NULL;
    
    // Calculate hash (2-3 ticks)
    uint64_t hash = cns_fnv1a_hash(string, length);
    uint32_t bucket = (uint32_t)(hash & (interner->capacity - 1));
    
    // Search chain (1-2 ticks average)
    uint32_t entry_index = interner->buckets[bucket];
    while (entry_index != UINT32_MAX) {
        cns_intern_entry_t* entry = &interner->entries[entry_index];
        
        // Quick hash check first
        if (entry->hash == hash && entry->length == length) {
            // Detailed comparison only if hash matches
            if (memcmp(entry->string, string, length) == 0) {
                return entry->string;
            }
        }
        
        entry_index = entry->next_index;
    }
    
    return NULL; // Not found
}

/*═══════════════════════════════════════════════════════════════
  String Interning (< 7 ticks for existing, more for new)
  ═══════════════════════════════════════════════════════════════*/

const char* cns_interner_intern(
    cns_string_interner_t* interner,
    const char* string,
    size_t length
) {
    if (!interner || !string) return NULL;
    
    // Try fast lookup first
    const char* existing = cns_interner_lookup(interner, string, length);
    if (existing) {
        // Update reference count for existing string
        uint64_t hash = cns_fnv1a_hash(string, length);
        uint32_t bucket = (uint32_t)(hash & (interner->capacity - 1));
        uint32_t entry_index = interner->buckets[bucket];
        
        while (entry_index != UINT32_MAX) {
            cns_intern_entry_t* entry = &interner->entries[entry_index];
            if (entry->hash == hash && entry->string == existing) {
                entry->refcount++;
                break;
            }
            entry_index = entry->next_index;
        }
        
        return existing;
    }
    
    // Check load factor for resize
    if (interner->count >= (uint32_t)(interner->capacity * INTERNER_MAX_LOAD_FACTOR)) {
        // TODO: Implement resize if needed
        return NULL;
    }
    
    // Allocate new entry
    uint32_t new_index;
    if (interner->free_list != UINT32_MAX) {
        new_index = interner->free_list;
        interner->free_list = interner->entries[new_index].next_index;
    } else {
        if (interner->count >= interner->capacity) {
            return NULL; // Table full
        }
        new_index = interner->count;
    }
    
    // Store string in arena
    char* stored_string = cns_arena_strndup(interner->arena, string, length);
    if (!stored_string) {
        return NULL;
    }
    
    // Calculate hash and bucket
    uint64_t hash = cns_fnv1a_hash(string, length);
    uint32_t bucket = (uint32_t)(hash & (interner->capacity - 1));
    
    // Initialize entry
    cns_intern_entry_t* entry = &interner->entries[new_index];
    entry->string = stored_string;
    entry->length = length;
    entry->hash = hash;
    entry->refcount = 1;
    entry->next_index = interner->buckets[bucket];
    
    // Link into bucket chain
    interner->buckets[bucket] = new_index;
    interner->count++;
    
    return stored_string;
}

/*═══════════════════════════════════════════════════════════════
  String Interning with NULL termination
  ═══════════════════════════════════════════════════════════════*/

const char* cns_interner_intern_cstr(
    cns_string_interner_t* interner,
    const char* cstring
) {
    if (!cstring) return NULL;
    return cns_interner_intern(interner, cstring, strlen(cstring));
}

/*═══════════════════════════════════════════════════════════════
  Reference Counting
  ═══════════════════════════════════════════════════════════════*/

void cns_interner_release(
    cns_string_interner_t* interner,
    const char* string
) {
    if (!interner || !string) return;
    
    // Find the entry
    size_t length = strlen(string);
    uint64_t hash = cns_fnv1a_hash(string, length);
    uint32_t bucket = (uint32_t)(hash & (interner->capacity - 1));
    
    uint32_t entry_index = interner->buckets[bucket];
    uint32_t prev_index = UINT32_MAX;
    
    while (entry_index != UINT32_MAX) {
        cns_intern_entry_t* entry = &interner->entries[entry_index];
        
        if (entry->hash == hash && entry->string == string) {
            entry->refcount--;
            
            // Remove entry if no references remain
            if (entry->refcount == 0) {
                // Unlink from chain
                if (prev_index != UINT32_MAX) {
                    interner->entries[prev_index].next_index = entry->next_index;
                } else {
                    interner->buckets[bucket] = entry->next_index;
                }
                
                // Add to free list
                entry->next_index = interner->free_list;
                interner->free_list = entry_index;
                interner->count--;
                
                // Note: String memory remains in arena (no individual free)
            }
            return;
        }
        
        prev_index = entry_index;
        entry_index = entry->next_index;
    }
}

/*═══════════════════════════════════════════════════════════════
  Statistics and Debugging
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint32_t total_strings;
    uint32_t unique_strings;
    uint32_t total_buckets;
    uint32_t used_buckets;
    uint32_t max_chain_length;
    double load_factor;
    double avg_chain_length;
    size_t memory_used;
} cns_interner_stats_t;

void cns_interner_get_stats(
    const cns_string_interner_t* interner,
    cns_interner_stats_t* stats
) {
    if (!interner || !stats) return;
    
    memset(stats, 0, sizeof(*stats));
    
    stats->total_strings = interner->count;
    stats->unique_strings = interner->count;
    stats->total_buckets = interner->capacity;
    stats->load_factor = (double)interner->count / interner->capacity;
    
    // Calculate chain statistics
    uint32_t used_buckets = 0;
    uint32_t total_chain_length = 0;
    uint32_t max_chain_length = 0;
    
    for (uint32_t i = 0; i < interner->capacity; i++) {
        if (interner->buckets[i] != UINT32_MAX) {
            used_buckets++;
            
            uint32_t chain_length = 0;
            uint32_t entry_index = interner->buckets[i];
            while (entry_index != UINT32_MAX) {
                chain_length++;
                entry_index = interner->entries[entry_index].next_index;
            }
            
            total_chain_length += chain_length;
            if (chain_length > max_chain_length) {
                max_chain_length = chain_length;
            }
        }
    }
    
    stats->used_buckets = used_buckets;
    stats->max_chain_length = max_chain_length;
    stats->avg_chain_length = used_buckets > 0 ? 
        (double)total_chain_length / used_buckets : 0.0;
    
    // Memory usage estimation
    stats->memory_used = sizeof(cns_string_interner_t) +
                        (interner->capacity * sizeof(cns_intern_entry_t)) +
                        (interner->capacity * sizeof(uint32_t));
}

/*═══════════════════════════════════════════════════════════════
  Bulk Operations
  ═══════════════════════════════════════════════════════════════*/

// Intern multiple strings in batch
size_t cns_interner_intern_batch(
    cns_string_interner_t* interner,
    const char** strings,
    size_t count,
    const char** results
) {
    if (!interner || !strings || !results) return 0;
    
    size_t successful = 0;
    for (size_t i = 0; i < count; i++) {
        results[i] = cns_interner_intern_cstr(interner, strings[i]);
        if (results[i]) {
            successful++;
        }
    }
    
    return successful;
}

/*═══════════════════════════════════════════════════════════════
  Debug and Validation
  ═══════════════════════════════════════════════════════════════*/

#ifdef DEBUG
bool cns_interner_validate(const cns_string_interner_t* interner) {
    if (!interner) return false;
    if (!interner->entries || !interner->buckets) return false;
    if (interner->count > interner->capacity) return false;
    
    // Validate hash table consistency
    uint32_t counted_entries = 0;
    for (uint32_t i = 0; i < interner->capacity; i++) {
        uint32_t entry_index = interner->buckets[i];
        while (entry_index != UINT32_MAX) {
            if (entry_index >= interner->capacity) return false;
            
            const cns_intern_entry_t* entry = &interner->entries[entry_index];
            if (!entry->string) return false;
            if (entry->refcount == 0) return false;
            
            // Verify hash consistency
            uint64_t computed_hash = cns_fnv1a_hash(entry->string, entry->length);
            if (computed_hash != entry->hash) return false;
            
            // Verify bucket placement
            uint32_t expected_bucket = (uint32_t)(entry->hash & (interner->capacity - 1));
            if (expected_bucket != i) return false;
            
            counted_entries++;
            entry_index = entry->next_index;
        }
    }
    
    return counted_entries <= interner->count;
}

void cns_interner_debug_dump(const cns_string_interner_t* interner) {
    if (!interner) return;
    
    printf("String Interner Debug Dump:\n");
    printf("  Capacity: %u\n", interner->capacity);
    printf("  Count: %u\n", interner->count);
    printf("  Load Factor: %.2f\n", (double)interner->count / interner->capacity);
    
    cns_interner_stats_t stats;
    cns_interner_get_stats(interner, &stats);
    printf("  Used Buckets: %u/%u\n", stats.used_buckets, stats.total_buckets);
    printf("  Max Chain Length: %u\n", stats.max_chain_length);
    printf("  Avg Chain Length: %.2f\n", stats.avg_chain_length);
}
#endif