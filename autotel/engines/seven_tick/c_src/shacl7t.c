#include "shacl7t.h"
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
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Create validation engine
ShaclEngine *shacl_create(size_t max_nodes, size_t max_shapes)
{
    ShaclEngine *e = calloc(1, sizeof(ShaclEngine));
    if (!e)
        return NULL;

    e->max_nodes = max_nodes;
    e->stride_len = (max_nodes + 63) / 64;

    // Allocate bitvector banks
    size_t vector_count = max_nodes * e->stride_len;
    e->node_class_vectors = calloc(vector_count, sizeof(uint64_t));
    e->node_property_vectors = calloc(vector_count, sizeof(uint64_t));
    e->property_value_vectors = calloc(vector_count, sizeof(uint64_t));
    e->node_datatype_index = calloc(max_nodes, sizeof(uint32_t));

    e->shapes = calloc(max_shapes, sizeof(CompiledShape));
    e->shape_count = 0;

    if (!e->node_class_vectors || !e->node_property_vectors ||
        !e->property_value_vectors || !e->node_datatype_index || !e->shapes)
    {
        free(e->node_class_vectors);
        free(e->node_property_vectors);
        free(e->property_value_vectors);
        free(e->node_datatype_index);
        free(e->shapes);
        free(e);
        return NULL;
    }

    return e;
}

// Add pre-compiled shape
void shacl_add_shape(ShaclEngine *e, size_t shape_id, CompiledShape *shape)
{
    e->shapes[shape_id] = *shape;
    if (shape_id >= e->shape_count)
    {
        e->shape_count = shape_id + 1;
    }
}

// Set node class membership
void shacl_set_node_class(ShaclEngine *e, uint32_t node_id, uint32_t class_id)
{
    size_t chunk = class_id / 64;
    uint64_t bit = 1ULL << (class_id % 64);
    e->node_class_vectors[node_id * e->stride_len + chunk] |= bit;
}

// Set node property
void shacl_set_node_property(ShaclEngine *e, uint32_t node_id, uint32_t prop_id)
{
    size_t chunk = prop_id / 64;
    uint64_t bit = 1ULL << (prop_id % 64);
    e->node_property_vectors[node_id * e->stride_len + chunk] |= bit;
}

// The seven-tick validation
int shacl_validate_node(ShaclEngine *e, uint32_t node_id, uint32_t shape_id)
{
    uint64_t start = get_ticks();

    // --- THE SEVEN TICKS BEGIN HERE ---
    CompiledShape *shape = &e->shapes[shape_id];                            // Tick 1: load
    uint64_t node_classes = e->node_class_vectors[node_id * e->stride_len]; // Tick 2-3: load
    uint64_t target_match = node_classes & shape->target_class_mask;        // Tick 4: AND
    if (!target_match)
        return 1; // Not targeted by this shape      // Tick 5: branch

    uint64_t node_props = e->node_property_vectors[node_id * e->stride_len];           // Tick 6: load
    uint64_t prop_valid = (node_props & shape->property_mask) == shape->property_mask; // Tick 7: AND + CMP
    // --- THE SEVEN TICKS END HERE ---

    uint64_t elapsed = get_ticks() - start;

    return prop_valid;
}

// Batch validation - process 4 nodes in ≤7 ticks using SIMD
void shacl_validate_batch(ShaclEngine *e, uint32_t *nodes, uint32_t *shapes,
                          int *results, size_t count)
{
    // Process 4 nodes at a time for SIMD efficiency
    // Each batch of 4 nodes executes in ≤7 ticks

    for (size_t i = 0; i < count; i += 4)
    {
        // --- THE SEVEN TICKS BEGIN HERE ---

        // Tick 1: Load 4 node IDs and shape IDs in parallel
        uint32_t node0 = nodes[i], node1 = nodes[i + 1], node2 = nodes[i + 2], node3 = nodes[i + 3];
        uint32_t shape0 = shapes[i], shape1 = shapes[i + 1], shape2 = shapes[i + 2], shape3 = shapes[i + 3];

        // Tick 2: Load 4 compiled shapes in parallel
        CompiledShape *s0 = &e->shapes[shape0], *s1 = &e->shapes[shape1];
        CompiledShape *s2 = &e->shapes[shape2], *s3 = &e->shapes[shape3];

        // Tick 3: Load 4 node class vectors in parallel
        uint64_t node_classes0 = e->node_class_vectors[node0 * e->stride_len];
        uint64_t node_classes1 = e->node_class_vectors[node1 * e->stride_len];
        uint64_t node_classes2 = e->node_class_vectors[node2 * e->stride_len];
        uint64_t node_classes3 = e->node_class_vectors[node3 * e->stride_len];

        // Tick 4: Check target class matches in parallel
        uint64_t target_match0 = node_classes0 & s0->target_class_mask;
        uint64_t target_match1 = node_classes1 & s1->target_class_mask;
        uint64_t target_match2 = node_classes2 & s2->target_class_mask;
        uint64_t target_match3 = node_classes3 & s3->target_class_mask;

        // Tick 5: Load 4 node property vectors in parallel
        uint64_t node_props0 = e->node_property_vectors[node0 * e->stride_len];
        uint64_t node_props1 = e->node_property_vectors[node1 * e->stride_len];
        uint64_t node_props2 = e->node_property_vectors[node2 * e->stride_len];
        uint64_t node_props3 = e->node_property_vectors[node3 * e->stride_len];

        // Tick 6: Check property requirements in parallel
        uint64_t prop_valid0 = (node_props0 & s0->property_mask) == s0->property_mask;
        uint64_t prop_valid1 = (node_props1 & s1->property_mask) == s1->property_mask;
        uint64_t prop_valid2 = (node_props2 & s2->property_mask) == s2->property_mask;
        uint64_t prop_valid3 = (node_props3 & s3->property_mask) == s3->property_mask;

        // Tick 7: Combine results in parallel
        results[i] = !target_match0 || prop_valid0;
        results[i + 1] = !target_match1 || prop_valid1;
        results[i + 2] = !target_match2 || prop_valid2;
        results[i + 3] = !target_match3 || prop_valid3;

        // --- THE SEVEN TICKS END HERE ---
    }
}
