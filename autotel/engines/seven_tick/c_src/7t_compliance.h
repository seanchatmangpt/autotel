#ifndef SEVEN_TICK_COMPLIANCE_H
#define SEVEN_TICK_COMPLIANCE_H

#include <stdint.h>
#include <stddef.h>

// 7T Memory Hierarchy Compliance Tiers
typedef enum {
    TIER_L1_COMPLIANT,  // Hot Path: < 10ns, > 100M ops/sec
    TIER_L2_COMPLIANT,  // Warm Path: < 30ns, > 30M ops/sec  
    TIER_L3_COMPLIANT,  // Cold Path: < 100ns, > 10M ops/sec
    TIER_NON_COMPLIANT  // Does not fit in cache
} ComplianceTier;

// Cache size constants for tier validation
#define L1_CACHE_SIZE_KB    64      // Conservative L1 size
#define L2_CACHE_SIZE_KB    512     // Conservative L2 size
#define L3_CACHE_SIZE_KB    16384   // Conservative L3 size (16MB)

// Performance guarantees by tier
typedef struct {
    ComplianceTier tier;
    const char* name;
    size_t max_footprint_kb;
    double max_latency_ns;
    uint64_t min_throughput_ops_sec;
} TierSpecification;

static const TierSpecification TIER_SPECS[] = {
    {TIER_L1_COMPLIANT, "L1-COMPLIANT (Hot Path)",   L1_CACHE_SIZE_KB, 10.0,  100000000},
    {TIER_L2_COMPLIANT, "L2-COMPLIANT (Warm Path)",  L2_CACHE_SIZE_KB, 30.0,  30000000},
    {TIER_L3_COMPLIANT, "L3-COMPLIANT (Cold Path)",  L3_CACHE_SIZE_KB, 100.0, 10000000},
    {TIER_NON_COMPLIANT, "NON-COMPLIANT",            SIZE_MAX,         1000.0, 1000000}
};

// Kernel footprint breakdown
typedef struct {
    size_t owl_class_masks_kb;
    size_t owl_property_vectors_kb;
    size_t shacl_shape_masks_kb;
    size_t shacl_property_masks_kb;
    size_t sparql_predicate_vectors_kb;
    size_t sparql_object_vectors_kb;
    size_t sparql_index_kb;
    size_t kernel_code_kb;
    size_t total_footprint_kb;
} KernelFootprint;

// Compliance certificate
typedef struct {
    char kernel_name[256];
    KernelFootprint footprint;
    ComplianceTier certified_tier;
    double guaranteed_latency_ns;
    uint64_t guaranteed_throughput_ops_sec;
    char timestamp[64];
} ComplianceCertificate;

// Tier calculation functions
ComplianceTier calculate_compliance_tier(const KernelFootprint* footprint);
void generate_compliance_certificate(const char* kernel_name, 
                                   const KernelFootprint* footprint,
                                   ComplianceCertificate* cert);
void print_compliance_certificate(const ComplianceCertificate* cert);

// Footprint calculation helpers
size_t calculate_owl_footprint(size_t num_classes, size_t num_properties);
size_t calculate_shacl_footprint(size_t num_shapes, size_t num_properties);
size_t calculate_sparql_footprint(size_t num_subjects, size_t num_predicates, 
                                 size_t num_objects, int use_index);

#endif