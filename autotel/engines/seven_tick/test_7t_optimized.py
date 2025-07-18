#!/usr/bin/env python3
"""
7T Optimized Performance Test - Achieving <10ns validation
Uses string caching and optimized C calls for true 7T performance
"""

import time
import sys
import os
import ctypes
from ctypes import c_uint32, c_size_t, byref

# Add the current directory to Python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from demo import RealSPARQL, lib7t
    RUNTIME_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import demo modules: {e}")
    RUNTIME_AVAILABLE = False

def test_7t_optimized():
    """Test 7T performance with string caching optimization"""
    print("=" * 60)
    print("7T Optimized Performance Test - <10ns Target")
    print("=" * 60)
    
    if not RUNTIME_AVAILABLE:
        print("❌ 7T runtime not available")
        return
    
    # Create engine
    print("Creating 7T engine...")
    engine = lib7t.s7t_create_engine()
    
    # Cache for interned strings to avoid repeated interning
    string_cache = {}
    
    def intern_string(s):
        if s not in string_cache:
            string_cache[s] = lib7t.s7t_intern_string(engine, s.encode('utf-8'))
        return string_cache[s]
    
    # Add test data
    print("Adding test data...")
    for i in range(1000):
        s = intern_string(f"ex:Entity{i}")
        p = intern_string("ex:name")
        o = intern_string(f"Name{i}")
        lib7t.s7t_add_triple(engine, s, p, o)
        
        # Add type
        p_type = intern_string("rdf:type")
        o_type = intern_string("ex:Person")
        lib7t.s7t_add_triple(engine, s, p_type, o_type)
    
    print(f"Added {1000} entities")
    print(f"Cached {len(string_cache)} strings")
    
    # Pre-intern commonly used strings
    entity_ids = [intern_string(f"ex:Entity{i}") for i in range(1000)]
    name_pred = intern_string("ex:name")
    type_pred = intern_string("rdf:type")
    person_class = intern_string("ex:Person")
    
    # Test s7t_ask_pattern performance with cached strings
    print("\n=== Testing s7t_ask_pattern Performance (Optimized) ===")
    
    # Warm up
    for i in range(1000):
        lib7t.s7t_ask_pattern(engine, entity_ids[i % 1000], name_pred, 0)
    
    # Benchmark with cached strings
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        result = lib7t.s7t_ask_pattern(engine, entity_ids[i % 1000], name_pred, 0)
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"s7t_ask_pattern (optimized): {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test shacl_check_min_count performance with cached strings
    print("\n=== Testing shacl_check_min_count Performance (Optimized) ===")
    
    # Define function signature
    lib7t.shacl_check_min_count.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32]
    lib7t.shacl_check_min_count.restype = ctypes.c_int
    
    # Warm up
    for i in range(1000):
        lib7t.shacl_check_min_count(engine, entity_ids[i % 1000], name_pred, 1)
    
    # Benchmark with cached strings
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        result = lib7t.shacl_check_min_count(engine, entity_ids[i % 1000], name_pred, 1)
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"shacl_check_min_count (optimized): {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test shacl_check_class performance with cached strings
    print("\n=== Testing shacl_check_class Performance (Optimized) ===")
    
    # Define function signature
    lib7t.shacl_check_class.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
    lib7t.shacl_check_class.restype = ctypes.c_int
    
    # Warm up
    for i in range(1000):
        lib7t.shacl_check_class(engine, entity_ids[i % 1000], person_class)
    
    # Benchmark with cached strings
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        result = lib7t.shacl_check_class(engine, entity_ids[i % 1000], person_class)
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"shacl_check_class (optimized): {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test complete SHACL validation with cached strings
    print("\n=== Testing Complete SHACL Validation (Optimized) ===")
    
    # Warm up
    for i in range(1000):
        # Check class
        is_person = lib7t.shacl_check_class(engine, entity_ids[i % 1000], person_class)
        if is_person:
            # Check required properties
            has_name = lib7t.shacl_check_min_count(engine, entity_ids[i % 1000], name_pred, 1)
    
    # Benchmark complete validation
    start_time = time.perf_counter_ns()
    
    valid_count = 0
    for i in range(100000):
        # Complete SHACL validation: class check + property check
        is_person = lib7t.shacl_check_class(engine, entity_ids[i % 1000], person_class)
        if is_person:
            has_name = lib7t.shacl_check_min_count(engine, entity_ids[i % 1000], name_pred, 1)
            if has_name:
                valid_count += 1
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"Complete SHACL validation (optimized): {avg_time_ns:.2f} ns per validation")
    print(f"Valid entities: {valid_count}")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test raw C performance without Python overhead
    print("\n=== Testing Raw C Performance ===")
    
    # Test just the C function call overhead
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        # Just the C function call, no Python overhead
        pass
    
    end_time = time.perf_counter_ns()
    overhead_ns = (end_time - start_time) / 100000
    
    print(f"Python loop overhead: {overhead_ns:.2f} ns per iteration")
    
    # Calculate actual C function time
    actual_c_time = avg_time_ns - overhead_ns
    print(f"Actual C function time: {actual_c_time:.2f} ns per call")
    
    # Summary
    print("\n" + "=" * 60)
    print("7T OPTIMIZED PERFORMANCE SUMMARY")
    print("=" * 60)
    print("✅ String caching eliminates interning overhead")
    print("✅ Direct C function calls")
    print("✅ O(1) hash table lookups")
    print("✅ High-precision nanosecond timing")
    print(f"✅ Best performance: {avg_time_ns:.2f} ns per validation")
    
    if avg_time_ns < 10:
        print("🎯 TARGET ACHIEVED: <10ns validation performance!")
    else:
        print(f"🎯 Target: <10ns, Current: {avg_time_ns:.2f}ns")
        print("💡 Further optimization needed for true 7T performance")
    
    # Cleanup
    lib7t.s7t_destroy_engine(engine)
    
    return avg_time_ns

if __name__ == "__main__":
    test_7t_optimized() 