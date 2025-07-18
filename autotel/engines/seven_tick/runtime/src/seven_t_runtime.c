#include "seven_t_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define INITIAL_CAPACITY 1024
#define BITVEC_WORD_BITS 64
#define HASH_TABLE_SIZE 16384
#define STRING_HASH_SIZE 8192

// Track allocated sizes
typedef struct
{
    size_t predicate_vectors_size;
    size_t object_vectors_size;
    size_t node_counts_size;
    size_t ps_index_size;
} AllocSizes;

// String hash table entry
typedef struct StringHashEntry
{
    char *str;
    uint32_t id;
    struct StringHashEntry *next;
} StringHashEntry;

// String hash table
typedef struct
{
    StringHashEntry *entries[STRING_HASH_SIZE];
} StringHashTable;

// Simple hash table entry for PS->O mapping
typedef struct
{
    uint32_t subject;
    uint32_t predicate;
    uint32_t *objects;
    size_t count;
    size_t capacity;
} PSOEntry;

// Hash table for PS->O lookups
typedef struct
{
    PSOEntry *entries;
    size_t size;
    size_t count;
} PSOHashTable;

// Helper to ensure array capacity
static void ensure_capacity(void **array, size_t *current_size, size_t required_size, size_t elem_size)
{
    if (required_size > *current_size)
    {
        size_t new_size = required_size * 2; // Double for amortized growth
        void *new_array = realloc(*array, new_size * elem_size);
        if (!new_array)
            abort(); // Out of memory

        // Zero new memory
        memset((char *)new_array + (*current_size * elem_size), 0,
               (new_size - *current_size) * elem_size);

        *array = new_array;
        *current_size = new_size;
    }
}

// Simple hash function for strings
static uint32_t hash_string(const char *str)
{
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % STRING_HASH_SIZE;
}

// Simple hash function for PS pairs
static uint32_t hash_ps(uint32_t predicate, uint32_t subject)
{
    return (predicate * 31 + subject) % HASH_TABLE_SIZE;
}

// Find or create PS entry in hash table
static PSOEntry *find_or_create_ps_entry(PSOHashTable *table, uint32_t predicate, uint32_t subject)
{
    uint32_t hash = hash_ps(predicate, subject);
    uint32_t index = hash;

    // Linear probing
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        index = (hash + i) % HASH_TABLE_SIZE;

        if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
        {
            // Empty slot - create new entry
            table->entries[index].predicate = predicate;
            table->entries[index].subject = subject;
            table->entries[index].objects = NULL;
            table->entries[index].count = 0;
            table->entries[index].capacity = 0;
            table->count++;
            return &table->entries[index];
        }

        if (table->entries[index].predicate == predicate && table->entries[index].subject == subject)
        {
            // Found existing entry
            return &table->entries[index];
        }
    }

    // Table is full (shouldn't happen with our size)
    abort();
}

// Bit vector implementation
BitVector *bitvec_create(size_t capacity)
{
    BitVector *bv = malloc(sizeof(BitVector));
    bv->capacity = (capacity + BITVEC_WORD_BITS - 1) / BITVEC_WORD_BITS;
    bv->bits = calloc(bv->capacity, sizeof(uint64_t));
    bv->count = 0;
    return bv;
}

void bitvec_destroy(BitVector *bv)
{
    free(bv->bits);
    free(bv);
}

S7T_HOT void bitvec_set(BitVector *bv, size_t index)
{
    size_t word = index / BITVEC_WORD_BITS;
    size_t bit = index % BITVEC_WORD_BITS;

    if (S7T_UNLIKELY(word >= bv->capacity))
    {
        size_t old_capacity = bv->capacity;
        size_t new_capacity = word * 2 + 1;
        bv->bits = realloc(bv->bits, new_capacity * sizeof(uint64_t));
        memset(&bv->bits[old_capacity], 0, (new_capacity - old_capacity) * sizeof(uint64_t));
        bv->capacity = new_capacity;
    }

    uint64_t mask = 1ULL << bit;
    if (!(bv->bits[word] & mask))
    {
        bv->bits[word] |= mask;
        bv->count++;
    }
}

S7T_HOT S7T_PURE int bitvec_test(BitVector *bv, size_t index)
{
    size_t word = index / BITVEC_WORD_BITS;
    size_t bit = index % BITVEC_WORD_BITS;

    if (S7T_UNLIKELY(word >= bv->capacity))
        return 0;
    return (bv->bits[word] & (1ULL << bit)) != 0;
}

S7T_HOT BitVector *bitvec_and(BitVector *S7T_RESTRICT a, BitVector *S7T_RESTRICT b)
{
    size_t min_capacity = a->capacity < b->capacity ? a->capacity : b->capacity;
    BitVector *result = bitvec_create(min_capacity * BITVEC_WORD_BITS);

    result->count = 0;
    for (size_t i = 0; i < min_capacity; i++)
    {
        result->bits[i] = a->bits[i] & b->bits[i];
        result->count += __builtin_popcountll(result->bits[i]);
    }

    return result;
}

BitVector *bitvec_or(BitVector *S7T_RESTRICT a, BitVector *S7T_RESTRICT b)
{
    size_t max_capacity = a->capacity > b->capacity ? a->capacity : b->capacity;
    BitVector *result = bitvec_create(max_capacity * BITVEC_WORD_BITS);

    result->count = 0;
    for (size_t i = 0; i < a->capacity && i < max_capacity; i++)
    {
        result->bits[i] |= a->bits[i];
    }
    for (size_t i = 0; i < b->capacity && i < max_capacity; i++)
    {
        result->bits[i] |= b->bits[i];
    }

    for (size_t i = 0; i < max_capacity; i++)
    {
        result->count += __builtin_popcountll(result->bits[i]);
    }

    return result;
}

S7T_PURE size_t bitvec_popcount(BitVector *bv)
{
    return bv->count;
}

// Engine creation and management
EngineState *s7t_create_engine(void)
{
    EngineState *engine = calloc(1, sizeof(EngineState));

    // Allocate extension tracking
    AllocSizes *sizes = calloc(1, sizeof(AllocSizes));
    sizes->predicate_vectors_size = INITIAL_CAPACITY;
    sizes->object_vectors_size = INITIAL_CAPACITY;
    sizes->node_counts_size = INITIAL_CAPACITY;
    sizes->ps_index_size = INITIAL_CAPACITY;

    // Initialize string hash table
    StringHashTable *string_hash = calloc(1, sizeof(StringHashTable));

    // Store string hash table in first slot, sizes in second slot
    engine->string_table = calloc(INITIAL_CAPACITY, sizeof(char *));
    engine->string_table[0] = (char *)string_hash;
    engine->string_table[1] = (char *)sizes;
    engine->string_capacity = INITIAL_CAPACITY;
    engine->string_count = 2; // Start at 2 to skip hash table and sizes

    // Pre-allocate arrays
    engine->predicate_vectors = calloc(INITIAL_CAPACITY, sizeof(BitVector *));
    engine->object_vectors = calloc(INITIAL_CAPACITY, sizeof(BitVector *));
    engine->node_property_counts = calloc(INITIAL_CAPACITY, sizeof(uint32_t));
    engine->object_type_ids = calloc(INITIAL_CAPACITY, sizeof(uint32_t));

    // Initialize PS->O hash table
    engine->ps_to_o_index = calloc(1, sizeof(PSOHashTable));
    PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
    table->entries = calloc(HASH_TABLE_SIZE, sizeof(PSOEntry));
    table->size = HASH_TABLE_SIZE;
    table->count = 0;

    // Initialize counts array (not used with hash table)
    engine->ps_to_o_counts = NULL;

    return engine;
}

void s7t_destroy_engine(EngineState *engine)
{
    // Get sizes and string hash table
    StringHashTable *string_hash = (StringHashTable *)engine->string_table[0];
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    // Free string hash table entries
    for (int i = 0; i < STRING_HASH_SIZE; i++)
    {
        StringHashEntry *entry = string_hash->entries[i];
        while (entry)
        {
            StringHashEntry *next = entry->next;
            free(entry->str);
            free(entry);
            entry = next;
        }
    }
    free(string_hash);

    // Free string table (skip first two entries - strings are freed via hash table)
    // The strings are already freed when we freed the hash table entries above
    free(sizes);
    free(engine->string_table);

    // Free bit vectors
    for (size_t i = 0; i <= engine->max_predicate_id && i < sizes->predicate_vectors_size; i++)
    {
        if (engine->predicate_vectors[i])
        {
            bitvec_destroy(engine->predicate_vectors[i]);
        }
    }
    free(engine->predicate_vectors);

    for (size_t i = 0; i <= engine->max_object_id && i < sizes->object_vectors_size; i++)
    {
        if (engine->object_vectors[i])
        {
            bitvec_destroy(engine->object_vectors[i]);
        }
    }
    free(engine->object_vectors);

    // Free PS->O hash table
    if (engine->ps_to_o_index)
    {
        PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
        for (size_t i = 0; i < table->size; i++)
        {
            if (table->entries[i].objects)
            {
                free(table->entries[i].objects);
            }
        }
        free(table->entries);
        free(table);
    }

    free(engine->node_property_counts);
    free(engine->object_type_ids);
    free(engine);
}

// String interning
S7T_HOT uint32_t s7t_intern_string(EngineState *engine, const char *str)
{
    // Use a hash table for O(1) lookups
    StringHashTable *string_hash = (StringHashTable *)engine->string_table[0];
    uint32_t hash = hash_string(str);
    StringHashEntry *entry = string_hash->entries[hash];

    while (entry)
    {
        if (strcmp(entry->str, str) == 0)
        {
            return entry->id;
        }
        entry = entry->next;
    }

    // Add new string
    if (S7T_UNLIKELY(engine->string_count >= engine->string_capacity))
    {
        engine->string_capacity *= 2;
        engine->string_table = realloc(engine->string_table,
                                       engine->string_capacity * sizeof(char *));
    }

    // Allocate memory for the new string and its hash entry
    size_t str_len = strlen(str) + 1; // +1 for null terminator
    char *new_str = malloc(str_len);
    if (!new_str)
        abort(); // Out of memory
    strcpy(new_str, str);

    // Allocate memory for the new hash entry
    StringHashEntry *new_entry = malloc(sizeof(StringHashEntry));
    if (!new_entry)
        abort(); // Out of memory

    // Initialize new entry
    new_entry->str = new_str;
    new_entry->id = engine->string_count;
    new_entry->next = NULL;

    // Insert new entry at the head of the linked list
    new_entry->next = string_hash->entries[hash];
    string_hash->entries[hash] = new_entry;

    // Store the new string in the string table
    engine->string_table[engine->string_count] = new_str;
    return engine->string_count++;
}

// Triple addition - hot path optimized
S7T_HOT void s7t_add_triple(EngineState *engine, uint32_t s, uint32_t p, uint32_t o)
{
    // Get sizes from the second slot (first slot is now string hash table)
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    // Update max IDs
    if (S7T_UNLIKELY(s > engine->max_subject_id))
        engine->max_subject_id = s;
    if (S7T_UNLIKELY(p > engine->max_predicate_id))
        engine->max_predicate_id = p;
    if (S7T_UNLIKELY(o > engine->max_object_id))
        engine->max_object_id = o;

    // Ensure arrays are large enough
    ensure_capacity((void **)&engine->predicate_vectors, &sizes->predicate_vectors_size,
                    p + 1, sizeof(BitVector *));
    ensure_capacity((void **)&engine->object_vectors, &sizes->object_vectors_size,
                    o + 1, sizeof(BitVector *));
    ensure_capacity((void **)&engine->node_property_counts, &sizes->node_counts_size,
                    s + 1, sizeof(uint32_t));
    ensure_capacity((void **)&engine->object_type_ids, &sizes->object_vectors_size,
                    o + 1, sizeof(uint32_t));

    // Ensure predicate vector exists
    if (S7T_UNLIKELY(!engine->predicate_vectors[p]))
    {
        engine->predicate_vectors[p] = bitvec_create(INITIAL_CAPACITY);
    }
    bitvec_set(engine->predicate_vectors[p], s);

    // Ensure object vector exists
    if (S7T_UNLIKELY(!engine->object_vectors[o]))
    {
        engine->object_vectors[o] = bitvec_create(INITIAL_CAPACITY);
    }
    bitvec_set(engine->object_vectors[o], s);

    // Update PS->O hash table
    PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
    PSOEntry *entry = find_or_create_ps_entry(table, p, s);

    // Add object to entry
    if (entry->count >= entry->capacity)
    {
        size_t new_capacity = entry->capacity == 0 ? 4 : entry->capacity * 2;
        entry->objects = realloc(entry->objects, new_capacity * sizeof(uint32_t));
        entry->capacity = new_capacity;
    }

    entry->objects[entry->count++] = o;

    // Update node property count
    engine->node_property_counts[s]++;

    engine->triple_count++;
}

// Query primitives - optimized for L1 cache
S7T_HOT BitVector *s7t_get_subject_vector(EngineState *engine, uint32_t predicate_id, uint32_t object_id)
{
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    if (S7T_UNLIKELY(predicate_id >= sizes->predicate_vectors_size ||
                     object_id >= sizes->object_vectors_size))
    {
        return bitvec_create(0);
    }

    BitVector *pred_vec = engine->predicate_vectors[predicate_id];
    BitVector *obj_vec = engine->object_vectors[object_id];

    if (S7T_UNLIKELY(!pred_vec || !obj_vec))
    {
        return bitvec_create(0);
    }

    return bitvec_and(pred_vec, obj_vec);
}

S7T_HOT BitVector *s7t_get_object_vector(EngineState *engine, uint32_t predicate_id, uint32_t subject_id)
{
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size ||
                     subject_id > engine->max_subject_id))
    {
        return bitvec_create(0);
    }

    // Use hash table lookup
    PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
    uint32_t hash = hash_ps(predicate_id, subject_id);
    uint32_t index = hash;

    // Linear probing
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        index = (hash + i) % HASH_TABLE_SIZE;

        if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
        {
            // Not found
            return bitvec_create(0);
        }

        if (table->entries[index].predicate == predicate_id && table->entries[index].subject == subject_id)
        {
            // Found - create bit vector from objects
            BitVector *result = bitvec_create(engine->max_object_id + 1);
            for (size_t j = 0; j < table->entries[index].count; j++)
            {
                bitvec_set(result, table->entries[index].objects[j]);
            }
            return result;
        }
    }

    return bitvec_create(0);
}

S7T_HOT S7T_PURE uint32_t *s7t_get_objects(EngineState *engine, uint32_t predicate_id,
                                           uint32_t subject_id, size_t *count)
{
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size ||
                     subject_id > engine->max_subject_id))
    {
        *count = 0;
        return NULL;
    }

    // Use hash table lookup
    PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
    uint32_t hash = hash_ps(predicate_id, subject_id);
    uint32_t index = hash;

    // Linear probing
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        index = (hash + i) % HASH_TABLE_SIZE;

        if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
        {
            // Not found
            *count = 0;
            return NULL;
        }

        if (table->entries[index].predicate == predicate_id && table->entries[index].subject == subject_id)
        {
            // Found
            *count = table->entries[index].count;
            return table->entries[index].objects;
        }
    }

    *count = 0;
    return NULL;
}

// SHACL validation primitives
S7T_HOT S7T_PURE int shacl_check_min_count(EngineState *engine, uint32_t subject_id,
                                           uint32_t predicate_id, uint32_t min_count)
{
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size ||
                     subject_id > engine->max_subject_id))
    {
        return min_count == 0;
    }

    // Use hash table lookup
    PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
    uint32_t hash = hash_ps(predicate_id, subject_id);
    uint32_t index = hash;

    // Linear probing
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        index = (hash + i) % HASH_TABLE_SIZE;

        if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
        {
            // Not found
            return min_count == 0;
        }

        if (table->entries[index].predicate == predicate_id && table->entries[index].subject == subject_id)
        {
            // Found
            return table->entries[index].count >= min_count;
        }
    }

    return min_count == 0;
}

S7T_HOT S7T_PURE int shacl_check_max_count(EngineState *engine, uint32_t subject_id,
                                           uint32_t predicate_id, uint32_t max_count)
{
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    if (S7T_UNLIKELY(predicate_id >= sizes->ps_index_size ||
                     subject_id > engine->max_subject_id))
    {
        return 1;
    }

    // Use hash table lookup
    PSOHashTable *table = (PSOHashTable *)engine->ps_to_o_index;
    uint32_t hash = hash_ps(predicate_id, subject_id);
    uint32_t index = hash;

    // Linear probing
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        index = (hash + i) % HASH_TABLE_SIZE;

        if (table->entries[index].predicate == 0 && table->entries[index].subject == 0)
        {
            // Not found
            return 1;
        }

        if (table->entries[index].predicate == predicate_id && table->entries[index].subject == subject_id)
        {
            // Found
            return table->entries[index].count <= max_count;
        }
    }

    return 1;
}

S7T_HOT S7T_PURE int shacl_check_class(EngineState *engine, uint32_t subject_id, uint32_t class_id)
{
    AllocSizes *sizes = (AllocSizes *)engine->string_table[1];

    if (S7T_UNLIKELY(subject_id >= sizes->node_counts_size))
    {
        return 0;
    }

    return engine->object_type_ids[subject_id] == class_id;
}
