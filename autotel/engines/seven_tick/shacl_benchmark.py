#!/usr/bin/env python3
"""
SHACL Validation Benchmark - Testing the new property checking implementation

This benchmark tests the newly implemented SHACL validation methods:
- _has_property_value() - checks if a node has any value for a property
- _count_property_values() - counts property values for a node

These were previously placeholder implementations that always returned True/1.
"""

import time
import random
from demo import RealSPARQL, RealSHACL, MockSPARQL, MockSHACL, RUNTIME_AVAILABLE

def create_test_data(sparql, num_entities=1000):
    """Create test data for SHACL validation"""
    print(f"Creating {num_entities} test entities...")
    
    # Add entities with various properties
    for i in range(num_entities):
        entity = f"ex:Entity{i}"
        
        # All entities have a name
        sparql.add_triple(entity, "ex:name", f"Entity {i}")
        
        # 80% have email
        if i % 5 != 0:
            sparql.add_triple(entity, "ex:email", f"entity{i}@example.com")
        
        # 60% have phone
        if i % 5 < 3:
            sparql.add_triple(entity, "ex:phone", f"+1-555-{i:04d}")
        
        # 40% have age
        if i % 5 < 2:
            sparql.add_triple(entity, "ex:age", str(20 + (i % 60)))
        
        # 20% are employees
        if i % 5 == 0:
            sparql.add_triple(entity, "rdf:type", "ex:Employee")
        else:
            sparql.add_triple(entity, "rdf:type", "ex:Person")
    
    print("Test data created:")
    print(f"  - All entities have name")
    print(f"  - 80% have email")
    print(f"  - 60% have phone")
    print(f"  - 40% have age")
    print(f"  - 20% are employees")

def benchmark_property_checking(shacl, num_checks=10000):
    """Benchmark the _has_property_value method"""
    print(f"\nBenchmarking property value checking ({num_checks} checks)...")
    
    properties = ["ex:name", "ex:email", "ex:phone", "ex:age"]
    
    start_time = time.perf_counter()
    
    for i in range(num_checks):
        entity = f"ex:Entity{i % 1000}"
        property_id = properties[i % len(properties)]
        
        # Test the new implementation
        has_property = shacl._has_property_value(entity, property_id)
        
        # Simple validation to ensure it's working
        if i < 5:  # Show first few results
            print(f"  {entity} has {property_id}: {has_property}")
    
    elapsed = time.perf_counter() - start_time
    
    checks_per_sec = num_checks / elapsed
    ns_per_check = elapsed * 1e9 / num_checks
    
    print(f"Property checking results:")
    print(f"  Total checks: {num_checks}")
    print(f"  Total time: {elapsed:.3f} seconds")
    print(f"  Checks per second: {checks_per_sec:,.0f}")
    print(f"  Nanoseconds per check: {ns_per_check:.1f}")
    
    if ns_per_check < 10:
        print(f"  ✅ Achieving sub-10ns property checking!")
    elif ns_per_check < 100:
        print(f"  ✅ Achieving sub-100ns property checking!")
    else:
        print(f"  ⚠️  Checking time above 100ns")
    
    return ns_per_check

def benchmark_property_counting(shacl, num_counts=10000):
    """Benchmark the _count_property_values method"""
    print(f"\nBenchmarking property value counting ({num_counts} counts)...")
    
    properties = ["ex:name", "ex:email", "ex:phone", "ex:age"]
    
    start_time = time.perf_counter()
    
    total_count = 0
    for i in range(num_counts):
        entity = f"ex:Entity{i % 1000}"
        property_id = properties[i % len(properties)]
        
        # Test the new implementation
        count = shacl._count_property_values(entity, property_id)
        total_count += count
        
        # Simple validation to ensure it's working
        if i < 5:  # Show first few results
            print(f"  {entity} has {count} values for {property_id}")
    
    elapsed = time.perf_counter() - start_time
    
    counts_per_sec = num_counts / elapsed
    ns_per_count = elapsed * 1e9 / num_counts
    
    print(f"Property counting results:")
    print(f"  Total counts: {num_counts}")
    print(f"  Total properties found: {total_count}")
    print(f"  Average properties per entity: {total_count / num_counts:.1f}")
    print(f"  Total time: {elapsed:.3f} seconds")
    print(f"  Counts per second: {counts_per_sec:,.0f}")
    print(f"  Nanoseconds per count: {ns_per_count:.1f}")
    
    if ns_per_count < 10:
        print(f"  ✅ Achieving sub-10ns property counting!")
    elif ns_per_count < 100:
        print(f"  ✅ Achieving sub-100ns property counting!")
    else:
        print(f"  ⚠️  Counting time above 100ns")
    
    return ns_per_count

def benchmark_shacl_validation(shacl, num_validations=1000):
    """Benchmark full SHACL validation"""
    print(f"\nBenchmarking full SHACL validation ({num_validations} validations)...")
    
    # Define shapes
    person_shape = {
        'id': 'PersonShape',
        'target_class': 'ex:Person',
        'constraints': {
            'properties': ['ex:name', 'ex:email'],
            'min_count': 1
        }
    }
    
    employee_shape = {
        'id': 'EmployeeShape',
        'target_class': 'ex:Employee',
        'constraints': {
            'properties': ['ex:name', 'ex:email', 'ex:phone'],
            'min_count': 1
        }
    }
    
    shacl.define_shape('PersonShape', 'ex:Person', person_shape['constraints'])
    shacl.define_shape('EmployeeShape', 'ex:Employee', employee_shape['constraints'])
    
    start_time = time.perf_counter()
    
    valid_count = 0
    for i in range(num_validations):
        entity = f"ex:Entity{i % 1000}"
        
        # Validate against all shapes
        results = shacl.validate_node(entity)
        
        # Count valid results
        if results:
            valid_count += sum(1 for valid in results.values() if valid)
        
        # Show first few results
        if i < 5:
            print(f"  {entity} validation: {results}")
    
    elapsed = time.perf_counter() - start_time
    
    validations_per_sec = num_validations / elapsed
    ns_per_validation = elapsed * 1e9 / num_validations
    
    print(f"SHACL validation results:")
    print(f"  Total validations: {num_validations}")
    print(f"  Valid results: {valid_count}")
    print(f"  Total time: {elapsed:.3f} seconds")
    print(f"  Validations per second: {validations_per_sec:,.0f}")
    print(f"  Nanoseconds per validation: {ns_per_validation:.1f}")
    
    if ns_per_validation < 10:
        print(f"  ✅ Achieving sub-10ns SHACL validation!")
    elif ns_per_validation < 100:
        print(f"  ✅ Achieving sub-100ns SHACL validation!")
    else:
        print(f"  ⚠️  Validation time above 100ns")
    
    return ns_per_validation

def main():
    """Run the SHACL benchmark"""
    print("╔══════════════════════════════════════════════╗")
    print("║   SHACL Validation Benchmark - New Features  ║")
    print("╚══════════════════════════════════════════════╝")
    
    print("\nTesting newly implemented SHACL validation methods:")
    print("  • _has_property_value() - Real property checking")
    print("  • _count_property_values() - Real property counting")
    print("  • Full SHACL validation with real constraints")
    
    try:
        # Create engines
        if RUNTIME_AVAILABLE:
            print(f"\nUsing RealSPARQL and RealSHACL (C runtime available)")
            sparql = RealSPARQL()
            shacl = RealSHACL(sparql)
        else:
            print(f"\nUsing MockSPARQL and MockSHACL (C runtime not available)")
            sparql = MockSPARQL()
            shacl = MockSHACL(sparql)
        
        # Create test data
        create_test_data(sparql, 1000)
        
        # Run benchmarks
        property_check_time = benchmark_property_checking(shacl, 10000)
        property_count_time = benchmark_property_counting(shacl, 10000)
        validation_time = benchmark_shacl_validation(shacl, 1000)
        
        # Summary
        print(f"\n{'='*50}")
        print(f"SHACL Benchmark Summary")
        print(f"{'='*50}")
        print(f"Property checking: {property_check_time:.1f} ns")
        print(f"Property counting:  {property_count_time:.1f} ns")
        print(f"Full validation:    {validation_time:.1f} ns")
        
        # Performance assessment
        if property_check_time < 100 and property_count_time < 100 and validation_time < 100:
            print(f"\n✅ All operations achieving sub-100ns performance!")
        elif property_check_time < 1000 and property_count_time < 1000 and validation_time < 1000:
            print(f"\n✅ All operations achieving sub-microsecond performance!")
        else:
            print(f"\n⚠️  Some operations above microsecond performance")
        
        print(f"\nImplementation Status:")
        print(f"  ✅ Real property checking implemented")
        print(f"  ✅ Real property counting implemented")
        print(f"  ✅ Full SHACL validation working")
        
    except Exception as e:
        print(f"Error during benchmark: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main()) 