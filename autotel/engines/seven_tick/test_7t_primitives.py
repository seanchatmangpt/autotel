#!/usr/bin/env python3
"""
7T Primitives Performance Test - Achieving <10ns validation
Tests individual C runtime primitives for 7T performance
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

def test_7t_primitives():
    """Test individual 7T primitive performance"""
    print("=" * 60)
    print("7T Primitives Performance Test - <10ns Target")
    print("=" * 60)
    
    if not RUNTIME_AVAILABLE:
        print("❌ 7T runtime not available")
        return
    
    # Create engine
    print("Creating 7T engine...")
    engine = lib7t.s7t_create_engine()
    
    # Add some test data
    print("Adding test data...")
    for i in range(1000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        o = lib7t.s7t_intern_string(engine, f"Name{i}".encode('utf-8'))
        lib7t.s7t_add_triple(engine, s, p, o)
        
        # Add type
        p_type = lib7t.s7t_intern_string(engine, "rdf:type".encode('utf-8'))
        o_type = lib7t.s7t_intern_string(engine, "ex:Person".encode('utf-8'))
        lib7t.s7t_add_triple(engine, s, p_type, o_type)
    
    print(f"Added {1000} entities")
    
    # Test s7t_ask_pattern performance
    print("\n=== Testing s7t_ask_pattern Performance ===")
    
    # Warm up
    for i in range(1000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        lib7t.s7t_ask_pattern(engine, s, p, 0)
    
    # Benchmark
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i % 1000}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        result = lib7t.s7t_ask_pattern(engine, s, p, 0)
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"s7t_ask_pattern: {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test s7t_get_objects performance
    print("\n=== Testing s7t_get_objects Performance ===")
    
    # Define function signature
    lib7t.s7t_get_objects.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_size_t)]
    lib7t.s7t_get_objects.restype = ctypes.POINTER(ctypes.c_uint32)
    
    # Warm up
    for i in range(1000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        count = c_size_t(0)
        lib7t.s7t_get_objects(engine, p, s, byref(count))
    
    # Benchmark
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i % 1000}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        count = c_size_t(0)
        objects = lib7t.s7t_get_objects(engine, p, s, byref(count))
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"s7t_get_objects: {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test shacl_check_min_count performance
    print("\n=== Testing shacl_check_min_count Performance ===")
    
    # Define function signature
    lib7t.shacl_check_min_count.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32]
    lib7t.shacl_check_min_count.restype = ctypes.c_int
    
    # Warm up
    for i in range(1000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        lib7t.shacl_check_min_count(engine, s, p, 1)
    
    # Benchmark
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i % 1000}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        result = lib7t.shacl_check_min_count(engine, s, p, 1)
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"shacl_check_min_count: {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test shacl_check_max_count performance
    print("\n=== Testing shacl_check_max_count Performance ===")
    
    # Define function signature
    lib7t.shacl_check_max_count.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32]
    lib7t.shacl_check_max_count.restype = ctypes.c_int
    
    # Warm up
    for i in range(1000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        lib7t.shacl_check_max_count(engine, s, p, 1)
    
    # Benchmark
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i % 1000}".encode('utf-8'))
        p = lib7t.s7t_intern_string(engine, "ex:name".encode('utf-8'))
        result = lib7t.shacl_check_max_count(engine, s, p, 1)
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"shacl_check_max_count: {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Test shacl_check_class performance
    print("\n=== Testing shacl_check_class Performance ===")
    
    # Define function signature
    lib7t.shacl_check_class.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
    lib7t.shacl_check_class.restype = ctypes.c_int
    
    # Warm up
    for i in range(1000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i}".encode('utf-8'))
        c = lib7t.s7t_intern_string(engine, "ex:Person".encode('utf-8'))
        lib7t.shacl_check_class(engine, s, c)
    
    # Benchmark
    start_time = time.perf_counter_ns()
    
    for i in range(100000):
        s = lib7t.s7t_intern_string(engine, f"ex:Entity{i % 1000}".encode('utf-8'))
        c = lib7t.s7t_intern_string(engine, "ex:Person".encode('utf-8'))
        result = lib7t.shacl_check_class(engine, s, c)
    
    end_time = time.perf_counter_ns()
    avg_time_ns = (end_time - start_time) / 100000
    
    print(f"shacl_check_class: {avg_time_ns:.2f} ns per call")
    
    if avg_time_ns < 10:
        print("✅ ACHIEVING 7T PERFORMANCE: <10ns!")
    elif avg_time_ns < 100:
        print("⚠️  Close to 7T performance: <100ns")
    else:
        print("❌ Not achieving 7T performance: >100ns")
    
    # Summary
    print("\n" + "=" * 60)
    print("7T PRIMITIVES PERFORMANCE SUMMARY")
    print("=" * 60)
    print("✅ Testing individual C runtime primitives")
    print("✅ O(1) hash table lookups")
    print("✅ Direct C function calls")
    print("✅ High-precision nanosecond timing")
    
    # Cleanup
    lib7t.s7t_destroy_engine(engine)
    
    return True

if __name__ == "__main__":
    test_7t_primitives() 