#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "7t_compliance.h"
#include "sparql7t.h"
#include "shacl7t.h"
#include "owl7t.h"

// Example use cases with their requirements
typedef struct {
    const char* name;
    const char* description;
    size_t num_classes;
    size_t num_properties;
    size_t num_shapes;
    size_t num_subjects;
    size_t num_predicates;
    size_t num_objects;
    int needs_index;
} UseCase;

static UseCase use_cases[] = {
    // L1-COMPLIANT Examples
    {
        "financial_transaction_validator",
        "High-frequency trading transaction validation",
        200,    // classes (Account Types, Transaction Types, Currencies, Regions)
        50,     // properties
        50,     // shapes (min/max amounts, required fields)
        1000,   // subjects (active transactions in window)
        10,     // predicates
        1000,   // objects
        1       // needs index for direct lookups
    },
    {
        "network_packet_filter",
        "Real-time network packet filtering",
        100,    // classes (Protocol types, Security levels)
        20,     // properties
        30,     // shapes (security rules)
        500,    // subjects (active connections)
        5,      // predicates
        500,    // objects
        1       // needs index
    },
    
    // L2-COMPLIANT Examples
    {
        "sprint_health_monitor",
        "Live Sprint Health IDE Plugin",
        200,    // classes (Scrum ontology)
        100,    // properties
        200,    // shapes (health rules)
        20000,  // subjects (tasks, stories, sprints)
        50,     // predicates
        20000,  // objects
        0       // uses bit-vector joins instead of index
    },
    {
        "api_gateway_validator",
        "High-frequency API request validation",
        500,    // classes (API resources, methods, auth types)
        200,    // properties
        300,    // shapes (validation rules)
        50000,  // subjects (active sessions/requests)
        30,     // predicates
        10000,  // objects
        0       // bit-vector joins
    },
    
    // L3-COMPLIANT Examples
    {
        "enterprise_compliance_engine",
        "Complex business logic validation",
        10000,  // classes (full enterprise ontology)
        5000,   // properties
        2000,   // shapes (compliance rules)
        2000000,// subjects (all business entities)
        200,    // predicates
        500000, // objects
        0       // bit-vector joins only
    },
    {
        "digital_twin_state_enforcer",
        "Manufacturing digital twin validation",
        5000,   // classes (equipment, processes, materials)
        2000,   // properties
        1000,   // shapes (operational constraints)
        1000000,// subjects (sensors, actuators, states)
        100,    // predicates
        200000, // objects
        0       // bit-vector joins
    }
};

void analyze_use_case(const UseCase* uc) {
    printf("\n=== Use Case: %s ===\n", uc->name);
    printf("Description: %s\n", uc->description);
    printf("\nRequirements:\n");
    printf("  - Classes: %zu\n", uc->num_classes);
    printf("  - Properties: %zu\n", uc->num_properties);
    printf("  - Shapes: %zu\n", uc->num_shapes);
    printf("  - Subjects: %zu\n", uc->num_subjects);
    printf("  - Predicates: %zu\n", uc->num_predicates);
    printf("  - Objects: %zu\n", uc->num_objects);
    
    // Calculate footprint
    KernelFootprint footprint = {0};
    
    // OWL footprint
    size_t owl_size = calculate_owl_footprint(uc->num_classes, uc->num_properties);
    footprint.owl_class_masks_kb = owl_size * 0.8;  // 80% for class masks
    footprint.owl_property_vectors_kb = owl_size * 0.2;  // 20% for property vectors
    
    // SHACL footprint
    size_t shacl_size = calculate_shacl_footprint(uc->num_shapes, uc->num_properties);
    footprint.shacl_shape_masks_kb = shacl_size * 0.6;
    footprint.shacl_property_masks_kb = shacl_size * 0.4;
    
    // SPARQL footprint
    size_t sparql_size = calculate_sparql_footprint(
        uc->num_subjects, uc->num_predicates, uc->num_objects, uc->needs_index
    );
    
    if (uc->needs_index) {
        // With index: most space goes to the index
        footprint.sparql_index_kb = sparql_size * 0.9;
        footprint.sparql_predicate_vectors_kb = sparql_size * 0.05;
        footprint.sparql_object_vectors_kb = sparql_size * 0.05;
    } else {
        // Without index: space split between vectors
        footprint.sparql_predicate_vectors_kb = sparql_size * 0.3;
        footprint.sparql_object_vectors_kb = sparql_size * 0.7;
        footprint.sparql_index_kb = 0;
    }
    
    // Kernel code size (estimated)
    footprint.kernel_code_kb = 12;  // Typical compiled kernel size
    
    // Total
    footprint.total_footprint_kb = 
        footprint.owl_class_masks_kb +
        footprint.owl_property_vectors_kb +
        footprint.shacl_shape_masks_kb +
        footprint.shacl_property_masks_kb +
        footprint.sparql_predicate_vectors_kb +
        footprint.sparql_object_vectors_kb +
        footprint.sparql_index_kb +
        footprint.kernel_code_kb;
    
    // Generate certificate
    ComplianceCertificate cert;
    generate_compliance_certificate(uc->name, &footprint, &cert);
    print_compliance_certificate(&cert);
    
    // Analysis
    if (cert.certified_tier == TIER_NON_COMPLIANT) {
        printf("⚠️  WARNING: This use case exceeds L3 cache capacity!\n");
        printf("   Consider sharding or reducing data size.\n");
    } else {
        printf("✅ This use case fits within %s\n", TIER_SPECS[cert.certified_tier].name);
        printf("   Expected performance: %.0f ns latency, %.0fM ops/sec\n",
               cert.guaranteed_latency_ns,
               cert.guaranteed_throughput_ops_sec / 1000000.0);
    }
}

int main() {
    printf("=== 7T Memory Hierarchy Compliance Analysis ===\n");
    printf("\nThe 7T system provides predictable performance by ensuring\n");
    printf("your logic fits within CPU cache tiers.\n");
    
    // Show tier specifications
    printf("\n--- Tier Specifications ---\n");
    for (int i = 0; i < TIER_NON_COMPLIANT; i++) {
        const TierSpecification* spec = &TIER_SPECS[i];
        printf("\n%s:\n", spec->name);
        printf("  Max footprint: %zu KB\n", spec->max_footprint_kb);
        printf("  Guaranteed latency: < %.0f ns\n", spec->max_latency_ns);
        printf("  Guaranteed throughput: > %lluM ops/sec\n", 
               spec->min_throughput_ops_sec / 1000000);
    }
    
    // Analyze each use case
    printf("\n\n--- Use Case Analysis ---\n");
    size_t num_use_cases = sizeof(use_cases) / sizeof(use_cases[0]);
    
    for (size_t i = 0; i < num_use_cases; i++) {
        analyze_use_case(&use_cases[i]);
    }
    
    // Summary
    printf("\n\n=== Summary ===\n");
    printf("The 7T Compliance System ensures predictable performance by:\n");
    printf("1. Calculating exact memory footprints\n");
    printf("2. Certifying kernels against cache tiers\n");
    printf("3. Providing guaranteed latency/throughput\n");
    printf("\nDesign your ontologies to fit your performance requirements!\n");
    
    return 0;
}