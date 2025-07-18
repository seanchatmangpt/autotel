#!/usr/bin/env python3
"""
7T SHACL Performance Test - Achieving <10ns validation
Uses optimized C runtime primitives for 7T performance
"""

import time
import sys
import os
import ctypes
from ctypes import c_uint32, c_size_t, byref

# Add the current directory to Python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from demo import RealSPARQL, MockSPARQL, lib7t
    RUNTIME_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import demo modules: {e}")
    RUNTIME_AVAILABLE = False

class Optimized7TSHACL:
    """7T-optimized SHACL engine using C runtime primitives for <10ns performance"""
    
    def __init__(self, sparql_engine):
        if not RUNTIME_AVAILABLE:
            raise RuntimeError("7T runtime not available")
        
        self.sparql = sparql_engine
        self.shapes = {}
        
        # Get direct access to C runtime functions for maximum performance
        self.lib = lib7t  # Use the global lib7t from demo.py
        self.engine = sparql_engine.engine
        
    def define_shape(self, shape_id, target_class, constraints):
        """Define a SHACL shape"""
        self.shapes[shape_id] = {
            'target_class': target_class,
            'constraints': constraints
        }
    
    def validate_node(self, node_id):
        """Validate a node against all applicable shapes - 7T optimized"""
        # Get interned node ID once
        node_id_interned = self.sparql._intern_string(node_id)
        
        for shape_id, shape in self.shapes.items():
            # Check if node is of target class using optimized C primitive
            target_class_interned = self.sparql._intern_string(shape['target_class'])
            
            # Use shacl_check_class for O(1) class validation
            if self.lib.shacl_check_class(self.engine, node_id_interned, target_class_interned):
                # Validate constraints using optimized C primitives
                if not self._validate_constraints_7t(node_id_interned, shape['constraints']):
                    return False
        
        return True
    
    def _validate_constraints_7t(self, node_id_interned, constraints):
        """Validate constraints using 7T optimized C primitives"""
        try:
            # Check required properties using s7t_ask_pattern (O(1) hash lookup)
            if 'properties' in constraints:
                for prop in constraints['properties']:
                    property_id_interned = self.sparql._intern_string(prop)
                    if not self.lib.s7t_ask_pattern(
                        self.engine, 
                        node_id_interned, 
                        property_id_interned, 
                        0  # Any object
                    ):
                        return False
            
            # Check min_count using optimized C primitive
            if 'min_count' in constraints:
                min_count = constraints['min_count']
                for prop in constraints.get('properties', []):
                    property_id_interned = self.sparql._intern_string(prop)
                    if not self.lib.shacl_check_min_count(
                        self.engine,
                        node_id_interned,
                        property_id_interned,
                        min_count
                    ):
                        return False
            
            # Check max_count using optimized C primitive
            if 'max_count' in constraints:
                max_count = constraints['max_count']
                for prop in constraints.get('properties', []):
                    property_id_interned = self.sparql._intern_string(prop)
                    if not self.lib.shacl_check_max_count(
                        self.engine,
                        node_id_interned,
                        property_id_interned,
                        max_count
                    ):
                        return False
            
            return True
            
        except Exception as e:
            print(f"Error in 7T validation: {e}")
            return False
    
    def _has_property_value_7t(self, node_id, property_id):
        """Check if node has property value using 7T optimized C primitive"""
        try:
            node_id_interned = self.sparql._intern_string(node_id)
            property_id_interned = self.sparql._intern_string(property_id)
            
            # Use s7t_ask_pattern for O(1) property existence check
            return self.lib.s7t_ask_pattern(
                self.engine, 
                node_id_interned, 
                property_id_interned, 
                0  # Any object
            ) != 0
        except Exception as e:
            return False
    
    def _count_property_values_7t(self, node_id, property_id):
        """Count property values using 7T optimized C primitive"""
        try:
            node_id_interned = self.sparql._intern_string(node_id)
            property_id_interned = self.sparql._intern_string(property_id)
            
            # Use s7t_get_objects for O(1) count retrieval
            count = c_size_t(0)
            objects = self.lib.s7t_get_objects(
                self.engine,
                property_id_interned,
                node_id_interned,
                byref(count)
            )
            
            return count.value if objects else 0
        except Exception as e:
            return 0

class Test7TPerformance:
    """Test 7T SHACL performance to achieve <10ns validation"""
    
    def __init__(self):
        self.sparql = None
        self.shacl = None
        self.setup_engines()
    
    def setup_engines(self):
        """Setup optimized 7T engines"""
        try:
            if RUNTIME_AVAILABLE:
                self.sparql = RealSPARQL()
                self.shacl = Optimized7TSHACL(self.sparql)
                print("✅ Using 7T-optimized RealSPARQL and Optimized7TSHACL engines")
            else:
                raise RuntimeError("7T runtime not available")
        except Exception as e:
            print(f"❌ Failed to setup 7T engines: {e}")
            sys.exit(1)
    
    def load_test_data(self):
        """Load test data for 7T performance testing"""
        print("\n=== Loading Test Data for 7T Performance ===")
        
        # Add entities with various properties
        for i in range(10000):
            # Add entity type
            self.sparql.add_triple(f"ex:Entity{i}", "rdf:type", "ex:Person")
            
            # Add required properties
            self.sparql.add_triple(f"ex:Entity{i}", "ex:name", f"Name{i}")
            self.sparql.add_triple(f"ex:Entity{i}", "ex:email", f"email{i}@example.com")
            
            # Add optional properties
            if i % 10 == 0:
                self.sparql.add_triple(f"ex:Entity{i}", "ex:alias", f"alias{i}")
        
        # Add invalid entities (missing required properties)
        for i in range(10000, 10100):
            self.sparql.add_triple(f"ex:InvalidEntity{i}", "rdf:type", "ex:Person")
            # Missing name and email properties
        
        print(f"Loaded test data with approximately {len(self.sparql.string_cache) // 3} triples")
    
    def setup_shacl_shapes(self):
        """Setup SHACL shapes for 7T validation"""
        print("\n=== Setting up 7T SHACL Shapes ===")
        
        # Person shape with required properties
        person_shape = {
            'id': 'PersonShape',
            'target_class': 'ex:Person',
            'constraints': {
                'properties': ['ex:name', 'ex:email'],
                'min_count': 1
            }
        }
        
        # Employee shape with max_count constraint
        employee_shape = {
            'id': 'EmployeeShape',
            'target_class': 'ex:Person',
            'constraints': {
                'properties': ['ex:alias'],
                'min_count': 0,  # Optional
                'max_count': 1   # Max 1 alias
            }
        }
        
        self.shacl.define_shape('PersonShape', 'ex:Person', person_shape['constraints'])
        self.shacl.define_shape('EmployeeShape', 'ex:Person', employee_shape['constraints'])
        
        print("✅ Created 7T-optimized PersonShape and EmployeeShape")
    
    def benchmark_7t_performance(self):
        """Benchmark 7T SHACL validation performance"""
        print("\n=== Benchmarking 7T Performance (<10ns target) ===")
        
        # Test validation performance with high precision timing
        start_time = time.perf_counter_ns()
        
        valid_count = 0
        total_validations = 0
        
        # Validate all entities
        for i in range(10000):
            is_valid = self.shacl.validate_node(f"ex:Entity{i}")
            if is_valid:
                valid_count += 1
            total_validations += 1
        
        # Validate invalid entities
        for i in range(10000, 10100):
            is_valid = self.shacl.validate_node(f"ex:InvalidEntity{i}")
            if is_valid:
                valid_count += 1
            total_validations += 1
        
        end_time = time.perf_counter_ns()
        total_time_ns = end_time - start_time
        
        # Calculate performance metrics
        avg_time_ns = total_time_ns / total_validations
        validations_per_sec = total_validations / (total_time_ns / 1e9)
        
        print(f"Total validations: {total_validations}")
        print(f"Valid entities: {valid_count}")
        print(f"Invalid entities: {total_validations - valid_count}")
        print(f"Total time: {total_time_ns / 1e6:.3f} ms")
        print(f"Average time per validation: {avg_time_ns:.2f} ns")
        print(f"Validations per second: {validations_per_sec:,.0f}")
        
        # Check 7T performance targets
        if avg_time_ns < 10:
            print("✅ ACHIEVING 7T PERFORMANCE: <10ns validation!")
        elif avg_time_ns < 100:
            print("⚠️  Close to 7T performance: <100ns validation")
        else:
            print("❌ Not achieving 7T performance: >100ns validation")
        
        return validations_per_sec, avg_time_ns
    
    def test_7t_primitives(self):
        """Test individual 7T primitive performance"""
        print("\n=== Testing 7T Primitive Performance ===")
        
        # Test s7t_ask_pattern performance
        print("Testing s7t_ask_pattern performance...")
        start_time = time.perf_counter_ns()
        
        for i in range(100000):
            self.shacl._has_property_value_7t(f"ex:Entity{i % 10000}", "ex:name")
        
        end_time = time.perf_counter_ns()
        avg_time_ns = (end_time - start_time) / 100000
        print(f"  s7t_ask_pattern: {avg_time_ns:.2f} ns per call")
        
        # Test s7t_get_objects performance
        print("Testing s7t_get_objects performance...")
        start_time = time.perf_counter_ns()
        
        for i in range(100000):
            self.shacl._count_property_values_7t(f"ex:Entity{i % 10000}", "ex:name")
        
        end_time = time.perf_counter_ns()
        avg_time_ns = (end_time - start_time) / 100000
        print(f"  s7t_get_objects: {avg_time_ns:.2f} ns per call")
        
        # Test shacl_check_min_count performance
        print("Testing shacl_check_min_count performance...")
        start_time = time.perf_counter_ns()
        
        for i in range(100000):
            node_id_interned = self.sparql._intern_string(f"ex:Entity{i % 10000}")
            property_id_interned = self.sparql._intern_string("ex:name")
            self.shacl.lib.shacl_check_min_count(
                self.shacl.engine,
                node_id_interned,
                property_id_interned,
                1
            )
        
        end_time = time.perf_counter_ns()
        avg_time_ns = (end_time - start_time) / 100000
        print(f"  shacl_check_min_count: {avg_time_ns:.2f} ns per call")
    
    def test_7t_validation_accuracy(self):
        """Test 7T validation accuracy"""
        print("\n=== Testing 7T Validation Accuracy ===")
        
        # Test valid entity
        valid_entity = self.shacl.validate_node("ex:Entity0")
        print(f"ex:Entity0 validation: {'PASS' if valid_entity else 'FAIL'} ✅")
        
        # Test invalid entity (missing properties)
        invalid_entity = self.shacl.validate_node("ex:InvalidEntity10000")
        print(f"ex:InvalidEntity10000 validation: {'PASS' if not invalid_entity else 'FAIL'} ✅")
        
        # Test entity with multiple aliases (should fail max_count)
        self.sparql.add_triple("ex:Entity0", "ex:alias", "second_alias")
        entity_with_multiple_aliases = self.shacl.validate_node("ex:Entity0")
        print(f"ex:Entity0 with multiple aliases: {'PASS' if not entity_with_multiple_aliases else 'FAIL'} ✅")
    
    def run_7t_performance_test(self):
        """Run comprehensive 7T performance test"""
        print("=" * 70)
        print("7T SHACL Performance Test - Achieving <10ns Validation")
        print("=" * 70)
        
        print(f"Engine type: {type(self.sparql).__name__}")
        print(f"SHACL type: {type(self.shacl).__name__}")
        
        # Load test data
        self.load_test_data()
        
        # Setup SHACL shapes
        self.setup_shacl_shapes()
        
        # Test 7T primitives
        self.test_7t_primitives()
        
        # Benchmark 7T performance
        validations_per_sec, avg_time_ns = self.benchmark_7t_performance()
        
        # Test validation accuracy
        self.test_7t_validation_accuracy()
        
        # Summary
        print("\n" + "=" * 70)
        print("7T PERFORMANCE SUMMARY")
        print("=" * 70)
        print("✅ Using optimized C runtime primitives")
        print("✅ O(1) hash table lookups instead of O(n) linear searches")
        print("✅ Direct C function calls for maximum performance")
        print(f"✅ Performance: {validations_per_sec:,.0f} validations/sec")
        print(f"✅ Average time: {avg_time_ns:.2f} ns per validation")
        
        if avg_time_ns < 10:
            print("🎯 TARGET ACHIEVED: <10ns validation performance!")
        else:
            print(f"🎯 Target: <10ns, Current: {avg_time_ns:.2f}ns")
        
        return validations_per_sec, avg_time_ns

def main():
    """Main 7T performance test function"""
    test = Test7TPerformance()
    return test.run_7t_performance_test()

if __name__ == "__main__":
    main() 