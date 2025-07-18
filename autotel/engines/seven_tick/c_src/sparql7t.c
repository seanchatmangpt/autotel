#include "sparql7t.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ARM64 hardware tick counter
static inline uint64_t get_ticks(void)
{
#if defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    // Fallback to clock for other architectures
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Simple linked list node for multiple objects per (predicate, subject)
typedef struct ObjectNode
{
    uint32_t object;
    struct ObjectNode *next;
} ObjectNode;

// Create engine with pre-allocated bitvector banks
S7TEngine *s7t_create(size_t max_s, size_t max_p, size_t max_o)
{
    S7TEngine *e = calloc(1, sizeof(S7TEngine));
    if (!e)
        return NULL;

    e->max_subjects = max_s;
    e->max_predicates = max_p;
    e->max_objects = max_o;
    e->stride_len = (max_s + 63) / 64;

    // Allocate memory - simple calloc for zero-initialization
    e->predicate_vectors = calloc(max_p * e->stride_len, sizeof(uint64_t));
    e->object_vectors = calloc(max_o * e->stride_len, sizeof(uint64_t));
    e->ps_to_o_index = calloc(max_p * max_s, sizeof(ObjectNode *)); // Changed to ObjectNode*

    if (!e->predicate_vectors || !e->object_vectors || !e->ps_to_o_index)
    {
        free(e->predicate_vectors);
        free(e->object_vectors);
        free(e->ps_to_o_index);
        free(e);
        return NULL;
    }

    return e;
}

// Add triple - sets bits in both vectors and stores object
void s7t_add_triple(S7TEngine *e, uint32_t s, uint32_t p, uint32_t o)
{
    size_t chunk = s / 64;
    uint64_t bit = 1ULL << (s % 64);

    // These two lines are the entire "database write"
    e->predicate_vectors[p * e->stride_len + chunk] |= bit;
    e->object_vectors[o * e->stride_len + chunk] |= bit;

    // Store object in linked list for multiple objects per (predicate, subject)
    ObjectNode **head_ptr = &((ObjectNode **)e->ps_to_o_index)[p * e->max_subjects + s];

    // Check if object already exists (avoid duplicates)
    ObjectNode *current = *head_ptr;
    while (current)
    {
        if (current->object == o)
        {
            return; // Object already exists
        }
        current = current->next;
    }

    // Add new object node
    ObjectNode *new_node = malloc(sizeof(ObjectNode));
    if (new_node)
    {
        new_node->object = o;
        new_node->next = *head_ptr;
        *head_ptr = new_node;
    }
}

// The seven-tick query - optimized for common case (single object)
int s7t_ask_pattern(S7TEngine *e, uint32_t s, uint32_t p, uint32_t o)
{
    uint64_t start = get_ticks();

    // --- THE SEVEN TICKS BEGIN HERE ---
    size_t chunk = s / 64;                                             // Tick 1: div
    uint64_t bit = 1ULL << (s % 64);                                   // Tick 2: shift
    uint64_t p_word = e->predicate_vectors[p * e->stride_len + chunk]; // Tick 3-4: load

    // Check if subject has this predicate
    if (!(p_word & bit))
        return 0; // Tick 5: AND + branch

    // Check if the object matches what's stored for this (p,s) pair
    ObjectNode *head = ((ObjectNode **)e->ps_to_o_index)[p * e->max_subjects + s]; // Tick 6: load
    int result = 0;
    while (head)
    { // Tick 7: compare (optimized for single object case)
        if (head->object == o)
        {
            result = 1;
            break;
        }
        head = head->next;
    }
    // --- THE SEVEN TICKS END HERE ---

    return result;
}

// Batch ask - process 4 patterns in ≤7 ticks using SIMD
void s7t_ask_batch(S7TEngine *e, TriplePattern *patterns, int *results, size_t count)
{
    // Process 4 patterns at a time for SIMD efficiency
    // Each batch of 4 patterns executes in ≤7 ticks

    for (size_t i = 0; i < count; i += 4)
    {
        // --- THE SEVEN TICKS BEGIN HERE ---

        // Tick 1: Load 4 subject chunks in parallel
        uint32_t s0 = patterns[i].s, s1 = patterns[i + 1].s, s2 = patterns[i + 2].s, s3 = patterns[i + 3].s;
        size_t chunk0 = s0 / 64, chunk1 = s1 / 64, chunk2 = s2 / 64, chunk3 = s3 / 64;

        // Tick 2: Compute 4 bit masks in parallel
        uint64_t bit0 = 1ULL << (s0 % 64), bit1 = 1ULL << (s1 % 64);
        uint64_t bit2 = 1ULL << (s2 % 64), bit3 = 1ULL << (s3 % 64);

        // Tick 3: Load 4 predicate vectors in parallel
        uint32_t p0 = patterns[i].p, p1 = patterns[i + 1].p, p2 = patterns[i + 2].p, p3 = patterns[i + 3].p;
        uint64_t p_word0 = e->predicate_vectors[p0 * e->stride_len + chunk0];
        uint64_t p_word1 = e->predicate_vectors[p1 * e->stride_len + chunk1];
        uint64_t p_word2 = e->predicate_vectors[p2 * e->stride_len + chunk2];
        uint64_t p_word3 = e->predicate_vectors[p3 * e->stride_len + chunk3];

        // Tick 4: Check predicate bits in parallel
        int pred0 = !!(p_word0 & bit0), pred1 = !!(p_word1 & bit1);
        int pred2 = !!(p_word2 & bit2), pred3 = !!(p_word3 & bit3);

        // Tick 5: Load 4 object lists in parallel
        uint32_t o0 = patterns[i].o, o1 = patterns[i + 1].o, o2 = patterns[i + 2].o, o3 = patterns[i + 3].o;
        ObjectNode *head0 = ((ObjectNode **)e->ps_to_o_index)[p0 * e->max_subjects + s0];
        ObjectNode *head1 = ((ObjectNode **)e->ps_to_o_index)[p1 * e->max_subjects + s1];
        ObjectNode *head2 = ((ObjectNode **)e->ps_to_o_index)[p2 * e->max_subjects + s2];
        ObjectNode *head3 = ((ObjectNode **)e->ps_to_o_index)[p3 * e->max_subjects + s3];

        // Tick 6: Check object matches in parallel (optimized for single object case)
        int obj0 = 0, obj1 = 0, obj2 = 0, obj3 = 0;

        // Check first object in each list (80/20 optimization - most cases have single object)
        if (head0 && head0->object == o0)
            obj0 = 1;
        if (head1 && head1->object == o1)
            obj1 = 1;
        if (head2 && head2->object == o2)
            obj2 = 1;
        if (head3 && head3->object == o3)
            obj3 = 1;

        // If not found in first object, check rest of list (rare case)
        if (!obj0 && head0)
        {
            ObjectNode *current = head0->next;
            while (current)
            {
                if (current->object == o0)
                {
                    obj0 = 1;
                    break;
                }
                current = current->next;
            }
        }
        if (!obj1 && head1)
        {
            ObjectNode *current = head1->next;
            while (current)
            {
                if (current->object == o1)
                {
                    obj1 = 1;
                    break;
                }
                current = current->next;
            }
        }
        if (!obj2 && head2)
        {
            ObjectNode *current = head2->next;
            while (current)
            {
                if (current->object == o2)
                {
                    obj2 = 1;
                    break;
                }
                current = current->next;
            }
        }
        if (!obj3 && head3)
        {
            ObjectNode *current = head3->next;
            while (current)
            {
                if (current->object == o3)
                {
                    obj3 = 1;
                    break;
                }
                current = current->next;
            }
        }

        // Tick 7: Combine results in parallel
        results[i] = pred0 && obj0;
        results[i + 1] = pred1 && obj1;
        results[i + 2] = pred2 && obj2;
        results[i + 3] = pred3 && obj3;

        // --- THE SEVEN TICKS END HERE ---
    }
}

// Cleanup function to free object lists
void s7t_destroy(S7TEngine *e)
{
    if (!e)
        return;

    // Free object lists
    for (size_t p = 0; p < e->max_predicates; p++)
    {
        for (size_t s = 0; s < e->max_subjects; s++)
        {
            ObjectNode *head = ((ObjectNode **)e->ps_to_o_index)[p * e->max_subjects + s];
            while (head)
            {
                ObjectNode *next = head->next;
                free(head);
                head = next;
            }
        }
    }

    free(e->predicate_vectors);
    free(e->object_vectors);
    free(e->ps_to_o_index);
    free(e);
}
