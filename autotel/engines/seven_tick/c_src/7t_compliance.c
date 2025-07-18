#include "7t_compliance.h"
#include "shacl7t.h"  // For CompiledShape
#include <stdio.h>
#include <time.h>
#include <string.h>

// Calculate which tier a kernel qualifies for based on footprint
ComplianceTier calculate_compliance_tier(const KernelFootprint* footprint) {
    if (footprint->total_footprint_kb <= L1_CACHE_SIZE_KB) {
        return TIER_L1_COMPLIANT;
    } else if (footprint->total_footprint_kb <= L2_CACHE_SIZE_KB) {
        return TIER_L2_COMPLIANT;
    } else if (footprint->total_footprint_kb <= L3_CACHE_SIZE_KB) {
        return TIER_L3_COMPLIANT;
    } else {
        return TIER_NON_COMPLIANT;
    }
}

// Generate a compliance certificate for a kernel
void generate_compliance_certificate(const char* kernel_name, 
                                   const KernelFootprint* footprint,
                                   ComplianceCertificate* cert) {
    // Copy kernel name
    strncpy(cert->kernel_name, kernel_name, sizeof(cert->kernel_name) - 1);
    
    // Copy footprint
    cert->footprint = *footprint;
    
    // Determine tier
    cert->certified_tier = calculate_compliance_tier(footprint);
    
    // Set guarantees based on tier
    const TierSpecification* spec = &TIER_SPECS[cert->certified_tier];
    cert->guaranteed_latency_ns = spec->max_latency_ns;
    cert->guaranteed_throughput_ops_sec = spec->min_throughput_ops_sec;
    
    // Set timestamp
    time_t now = time(NULL);
    strftime(cert->timestamp, sizeof(cert->timestamp), 
             "%Y-%m-%d %H:%M:%S", localtime(&now));
}

// Print certificate in formatted output
void print_compliance_certificate(const ComplianceCertificate* cert) {
    const TierSpecification* spec = &TIER_SPECS[cert->certified_tier];
    
    printf("\n");
    printf("======================================\n");
    printf("7T COMPLIANCE CERTIFICATE\n");
    printf("======================================\n");
    printf("Kernel Name:      %s\n", cert->kernel_name);
    printf("Footprint:        %zu KB\n", cert->footprint.total_footprint_kb);
    printf("Certified Tier:   %s\n", spec->name);
    printf("\n");
    printf("Guarantees:\n");
    printf("  - Latency:      < %.0f nanoseconds\n", cert->guaranteed_latency_ns);
    printf("  - Throughput:   > %llu Million Ops/Sec\n", 
           cert->guaranteed_throughput_ops_sec / 1000000);
    printf("\n");
    printf("Breakdown:\n");
    
    if (cert->footprint.owl_class_masks_kb > 0) {
        printf("  - OWL class_masks:         %6zu KB\n", 
               cert->footprint.owl_class_masks_kb);
    }
    if (cert->footprint.owl_property_vectors_kb > 0) {
        printf("  - OWL property_vectors:    %6zu KB\n", 
               cert->footprint.owl_property_vectors_kb);
    }
    if (cert->footprint.shacl_shape_masks_kb > 0) {
        printf("  - SHACL shape_masks:       %6zu KB\n", 
               cert->footprint.shacl_shape_masks_kb);
    }
    if (cert->footprint.shacl_property_masks_kb > 0) {
        printf("  - SHACL property_masks:    %6zu KB\n", 
               cert->footprint.shacl_property_masks_kb);
    }
    if (cert->footprint.sparql_predicate_vectors_kb > 0) {
        printf("  - SPARQL predicate_vectors:%6zu KB\n", 
               cert->footprint.sparql_predicate_vectors_kb);
    }
    if (cert->footprint.sparql_object_vectors_kb > 0) {
        printf("  - SPARQL object_vectors:   %6zu KB\n", 
               cert->footprint.sparql_object_vectors_kb);
    }
    if (cert->footprint.sparql_index_kb > 0) {
        printf("  - SPARQL ps_to_o_index:    %6zu KB\n", 
               cert->footprint.sparql_index_kb);
    }
    printf("  - Kernel Code Size:        %6zu KB\n", cert->footprint.kernel_code_kb);
    printf("======================================\n");
    printf("Generated: %s\n", cert->timestamp);
    printf("======================================\n\n");
}// Footprint calculation helpers

size_t calculate_owl_footprint(size_t num_classes, size_t num_properties) {
    // Subclass closure matrix: num_classes * (num_classes / 8) bytes
    size_t class_chunks = (num_classes + 63) / 64;
    size_t subclass_closure_size = num_classes * class_chunks * sizeof(uint64_t);
    
    // Property vectors for characteristics
    size_t prop_chunks = (num_properties + 63) / 64;
    size_t property_vectors_size = 4 * prop_chunks * sizeof(uint64_t); // trans, sym, func, inv_func
    
    return (subclass_closure_size + property_vectors_size) / 1024; // Convert to KB
}

size_t calculate_shacl_footprint(size_t num_shapes, size_t num_properties) {
    // Shape structures
    size_t shape_size = num_shapes * sizeof(CompiledShape);
    
    // Property mask vectors (assuming average 50% of properties used per shape)
    size_t prop_chunks = (num_properties + 63) / 64;
    size_t property_masks_size = num_shapes * prop_chunks * sizeof(uint64_t) / 2;
    
    return (shape_size + property_masks_size) / 1024; // Convert to KB
}

size_t calculate_sparql_footprint(size_t num_subjects, size_t num_predicates, 
                                 size_t num_objects, int use_index) {
    size_t footprint = 0;
    
    // Predicate vectors: num_predicates * (num_subjects / 64) * 8 bytes
    size_t subject_chunks = (num_subjects + 63) / 64;
    footprint += num_predicates * subject_chunks * sizeof(uint64_t);
    
    // Object vectors: num_objects * (num_subjects / 64) * 8 bytes
    footprint += num_objects * subject_chunks * sizeof(uint64_t);
    
    // PS->O index if used
    if (use_index) {
        footprint += num_predicates * num_subjects * sizeof(uint32_t);
    }
    
    return footprint / 1024; // Convert to KB
}

// Tier recommendation based on use case
const char* recommend_tier_for_use_case(const char* use_case) {
    if (strstr(use_case, "trading") || strstr(use_case, "packet") || 
        strstr(use_case, "real-time control")) {
        return "L1-COMPLIANT";
    } else if (strstr(use_case, "UI") || strstr(use_case, "API") || 
               strstr(use_case, "stream")) {
        return "L2-COMPLIANT";
    } else if (strstr(use_case, "business") || strstr(use_case, "analytics") || 
               strstr(use_case, "twin")) {
        return "L3-COMPLIANT";
    }
    return "L2-COMPLIANT"; // Default
}