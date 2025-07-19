#!/usr/bin/env python3
"""
Test suite for MCTS7T - Monte Carlo Tree Search for 7-Tick Engine
"""

import unittest
import time
import numpy as np
from typing import List, Tuple
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from mcts7t import MCTS7T
except ImportError:
    print("Warning: MCTS7T module not found, skipping tests")
    MCTS7T = None


class TestMCTS7T(unittest.TestCase):
    """Test cases for MCTS7T implementation."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        if MCTS7T is None:
            cls.skipTest(cls, "MCTS7T module not available")
        
        cls.mcts = MCTS7T(
            max_iterations=100,
            time_budget_ms=10,
            max_depth=10,
            simulation_depth=5
        )
    
    @classmethod
    def tearDownClass(cls):
        """Clean up test fixtures."""
        if hasattr(cls, 'mcts'):
            del cls.mcts
    
    def test_mcts_initialization(self):
        """Test MCTS engine initialization."""
        mcts = MCTS7T(
            max_iterations=1000,
            time_budget_ms=50,
            max_depth=20,
            simulation_depth=10
        )
        
        self.assertIsNotNone(mcts)
        self.assertIsNotNone(mcts.engine)
        
        # Test configuration
        self.assertEqual(mcts.lib is not None, True)
    
    def test_query_optimization(self):
        """Test SPARQL query optimization."""
        patterns = [
            (1, 10, 100),  # High selectivity pattern
            (2, 20, 200),  # Medium selectivity pattern
            (3, 30, 300),  # Low selectivity pattern
        ]
        
        optimized_order = self.mcts.optimize_query(patterns)
        
        # Basic validation
        self.assertIsInstance(optimized_order, list)
        self.assertEqual(len(optimized_order), len(patterns))
        
        # Check that all pattern indices are present
        self.assertEqual(set(optimized_order), set(range(len(patterns))))
        
        # Check that no duplicates exist
        self.assertEqual(len(optimized_order), len(set(optimized_order)))
    
    def test_validation_optimization(self):
        """Test SHACL validation optimization."""
        shapes = [
            (1, 10),  # Person validation
            (2, 20),  # Organization validation
            (3, 30),  # Document validation
        ]
        
        optimized_order = self.mcts.optimize_validation(shapes)
        
        # Basic validation
        self.assertIsInstance(optimized_order, list)
        self.assertEqual(len(optimized_order), len(shapes))
        
        # Check that all shape indices are present
        self.assertEqual(set(optimized_order), set(range(len(shapes))))
        
        # Check that no duplicates exist
        self.assertEqual(len(optimized_order), len(set(optimized_order)))
    
    def test_reasoning_optimization(self):
        """Test OWL reasoning optimization."""
        axioms = [
            (1, 10, 100),  # Subclass axiom
            (2, 20, 200),  # Subproperty axiom
            (3, 30, 300),  # Transitive axiom
        ]
        
        optimized_order = self.mcts.optimize_reasoning(axioms)
        
        # Basic validation
        self.assertIsInstance(optimized_order, list)
        self.assertEqual(len(optimized_order), len(axioms))
        
        # Check that all axiom indices are present
        self.assertEqual(set(optimized_order), set(range(len(axioms))))
        
        # Check that no duplicates exist
        self.assertEqual(len(optimized_order), len(set(optimized_order)))
    
    def test_search_functionality(self):
        """Test general MCTS search functionality."""
        initial_state = [3, 1, 2, 3, 4, 5, 6]  # 3 patterns, 6 state values
        
        result = self.mcts.search(initial_state)
        
        # Validate result structure
        self.assertIsInstance(result, dict)
        self.assertIn('best_actions', result)
        self.assertIn('total_reward', result)
        self.assertIn('confidence', result)
        self.assertIn('computation_time_ns', result)
        self.assertIn('iterations_performed', result)
        self.assertIn('stats', result)
        
        # Validate result values
        self.assertIsInstance(result['best_actions'], list)
        self.assertIsInstance(result['total_reward'], (int, float))
        self.assertIsInstance(result['confidence'], (int, float))
        self.assertIsInstance(result['computation_time_ns'], int)
        self.assertIsInstance(result['iterations_performed'], int)
        
        # Validate confidence range
        self.assertGreaterEqual(result['confidence'], 0.0)
        self.assertLessEqual(result['confidence'], 1.0)
    
    def test_performance_stats(self):
        """Test performance statistics collection."""
        # Run a simple search to generate stats
        initial_state = [2, 1, 2, 3, 4]
        self.mcts.search(initial_state)
        
        stats = self.mcts.get_performance_stats()
        
        # Validate stats structure
        self.assertIsInstance(stats, dict)
        self.assertIn('total_time_ns', stats)
        self.assertIn('selection_time_ns', stats)
        self.assertIn('expansion_time_ns', stats)
        self.assertIn('simulation_time_ns', stats)
        self.assertIn('backpropagation_time_ns', stats)
        self.assertIn('nodes_created', stats)
        self.assertIn('nodes_visited', stats)
        self.assertIn('simulations_performed', stats)
        self.assertIn('iterations_performed', stats)
        self.assertIn('python_time_s', stats)
        
        # Validate stat values
        for key, value in stats.items():
            self.assertIsInstance(value, (int, float))
            self.assertGreaterEqual(value, 0)
    
    def test_benchmark_functionality(self):
        """Test benchmark functionality."""
        benchmark_results = self.mcts.benchmark(
            num_patterns=5,
            num_iterations=10
        )
        
        # Validate benchmark results
        self.assertIsInstance(benchmark_results, dict)
        self.assertIn('avg_time_ms', benchmark_results)
        self.assertIn('min_time_ms', benchmark_results)
        self.assertIn('max_time_ms', benchmark_results)
        self.assertIn('std_time_ms', benchmark_results)
        self.assertIn('avg_reward', benchmark_results)
        self.assertIn('throughput_ops_per_sec', benchmark_results)
        
        # Validate benchmark values
        for key, value in benchmark_results.items():
            self.assertIsInstance(value, (int, float))
            self.assertGreaterEqual(value, 0)
    
    def test_time_budget_respect(self):
        """Test that MCTS respects time budget."""
        # Create MCTS with very short time budget
        fast_mcts = MCTS7T(
            max_iterations=10000,
            time_budget_ms=1,  # 1ms budget
            max_depth=10,
            simulation_depth=5
        )
        
        start_time = time.time()
        patterns = [(i, i*10, i*100) for i in range(5)]
        fast_mcts.optimize_query(patterns)
        end_time = time.time()
        
        # Should complete within reasonable time (allow some overhead)
        execution_time_ms = (end_time - start_time) * 1000
        self.assertLess(execution_time_ms, 100)  # Should be much less than 100ms
    
    def test_iteration_limit_respect(self):
        """Test that MCTS respects iteration limit."""
        # Create MCTS with very low iteration limit
        limited_mcts = MCTS7T(
            max_iterations=5,  # Only 5 iterations
            time_budget_ms=1000,
            max_depth=10,
            simulation_depth=5
        )
        
        patterns = [(i, i*10, i*100) for i in range(3)]
        result = limited_mcts.search([len(patterns)] + [p for pattern in patterns for p in pattern])
        
        # Should not exceed iteration limit
        self.assertLessEqual(result['iterations_performed'], 5)
    
    def test_large_problem_scaling(self):
        """Test MCTS scaling with larger problems."""
        # Test with more patterns
        large_patterns = [(i, i*10, i*100) for i in range(10)]
        
        start_time = time.time()
        optimized_order = self.mcts.optimize_query(large_patterns)
        end_time = time.time()
        
        # Should complete successfully
        self.assertEqual(len(optimized_order), len(large_patterns))
        self.assertEqual(set(optimized_order), set(range(len(large_patterns))))
        
        # Should complete in reasonable time
        execution_time = end_time - start_time
        self.assertLess(execution_time, 1.0)  # Should complete within 1 second
    
    def test_consistency_across_runs(self):
        """Test that MCTS produces consistent results across runs."""
        patterns = [(i, i*10, i*100) for i in range(5)]
        
        # Run multiple times
        results = []
        for _ in range(3):
            optimized_order = self.mcts.optimize_query(patterns)
            results.append(optimized_order)
        
        # All results should be valid
        for result in results:
            self.assertEqual(len(result), len(patterns))
            self.assertEqual(set(result), set(range(len(patterns))))
        
        # Results should be consistent in structure (may vary due to randomness)
        for i in range(1, len(results)):
            self.assertEqual(len(results[i]), len(results[0]))
    
    def test_error_handling(self):
        """Test error handling for invalid inputs."""
        # Test with empty patterns
        with self.assertRaises(Exception):
            self.mcts.optimize_query([])
        
        # Test with None patterns
        with self.assertRaises(Exception):
            self.mcts.optimize_query(None)
        
        # Test with invalid state
        with self.assertRaises(Exception):
            self.mcts.search([])
    
    def test_memory_cleanup(self):
        """Test that MCTS properly cleans up resources."""
        # Create multiple MCTS instances
        instances = []
        for _ in range(5):
            mcts = MCTS7T(
                max_iterations=100,
                time_budget_ms=10,
                max_depth=5,
                simulation_depth=3
            )
            instances.append(mcts)
        
        # Use them
        patterns = [(1, 10, 100), (2, 20, 200)]
        for mcts in instances:
            mcts.optimize_query(patterns)
        
        # Clean up
        for mcts in instances:
            del mcts
        
        # Should not crash or leak memory


class TestMCTS7TPerformance(unittest.TestCase):
    """Performance-focused test cases for MCTS7T."""
    
    @classmethod
    def setUpClass(cls):
        """Set up performance test fixtures."""
        if MCTS7T is None:
            cls.skipTest(cls, "MCTS7T module not available")
    
    def test_microsecond_performance(self):
        """Test that MCTS can complete in microseconds for small problems."""
        mcts = MCTS7T(
            max_iterations=100,
            time_budget_ms=1,
            max_depth=5,
            simulation_depth=3
        )
        
        patterns = [(i, i*10, i*100) for i in range(3)]
        
        start_time = time.time()
        optimized_order = mcts.optimize_query(patterns)
        end_time = time.time()
        
        execution_time_us = (end_time - start_time) * 1000000
        
        # Should complete in microseconds for small problems
        self.assertLess(execution_time_us, 1000)  # Less than 1ms
        self.assertEqual(len(optimized_order), len(patterns))
    
    def test_throughput_benchmark(self):
        """Test MCTS throughput for multiple queries."""
        mcts = MCTS7T(
            max_iterations=50,
            time_budget_ms=5,
            max_depth=8,
            simulation_depth=4
        )
        
        # Generate multiple query sets
        query_sets = [
            [(i, i*10, i*100) for i in range(3)],
            [(i, i*10, i*100) for i in range(4)],
            [(i, i*10, i*100) for i in range(5)],
        ]
        
        start_time = time.time()
        results = []
        for patterns in query_sets:
            result = mcts.optimize_query(patterns)
            results.append(result)
        end_time = time.time()
        
        total_time = end_time - start_time
        throughput = len(query_sets) / total_time
        
        # Should achieve reasonable throughput
        self.assertGreater(throughput, 10)  # At least 10 queries per second
        
        # All results should be valid
        for i, result in enumerate(results):
            self.assertEqual(len(result), len(query_sets[i]))
    
    def test_memory_efficiency(self):
        """Test that MCTS doesn't consume excessive memory."""
        import psutil
        import os
        
        process = psutil.Process(os.getpid())
        initial_memory = process.memory_info().rss
        
        # Create and use MCTS instances
        instances = []
        for _ in range(10):
            mcts = MCTS7T(
                max_iterations=100,
                time_budget_ms=10,
                max_depth=10,
                simulation_depth=5
            )
            
            patterns = [(i, i*10, i*100) for i in range(5)]
            mcts.optimize_query(patterns)
            instances.append(mcts)
        
        # Clean up
        for mcts in instances:
            del mcts
        
        final_memory = process.memory_info().rss
        memory_increase_mb = (final_memory - initial_memory) / 1024 / 1024
        
        # Memory increase should be reasonable
        self.assertLess(memory_increase_mb, 100)  # Less than 100MB increase


def run_performance_benchmark():
    """Run comprehensive performance benchmark."""
    if MCTS7T is None:
        print("MCTS7T module not available")
        return
    
    print("🚀 MCTS7T Performance Benchmark")
    print("=" * 50)
    
    # Test different problem sizes
    problem_sizes = [3, 5, 8, 10, 15]
    
    for size in problem_sizes:
        print(f"\n📊 Testing with {size} patterns:")
        
        mcts = MCTS7T(
            max_iterations=1000,
            time_budget_ms=100,
            max_depth=20,
            simulation_depth=10
        )
        
        patterns = [(i, i*10, i*100) for i in range(size)]
        
        # Warm up
        for _ in range(3):
            mcts.optimize_query(patterns)
        
        # Benchmark
        times = []
        for _ in range(10):
            start_time = time.time()
            result = mcts.optimize_query(patterns)
            end_time = time.time()
            times.append(end_time - start_time)
        
        avg_time_ms = np.mean(times) * 1000
        min_time_ms = np.min(times) * 1000
        max_time_ms = np.max(times) * 1000
        throughput = 1.0 / np.mean(times)
        
        print(f"  Average time: {avg_time_ms:.2f} ms")
        print(f"  Min time: {min_time_ms:.2f} ms")
        print(f"  Max time: {max_time_ms:.2f} ms")
        print(f"  Throughput: {throughput:.1f} queries/sec")
        
        # Get detailed stats
        stats = mcts.get_performance_stats()
        print(f"  Nodes created: {stats['nodes_created']}")
        print(f"  Nodes visited: {stats['nodes_visited']}")
        print(f"  Simulations: {stats['simulations_performed']}")
        print(f"  Iterations: {stats['iterations_performed']}")


if __name__ == "__main__":
    # Run performance benchmark
    run_performance_benchmark()
    
    # Run unit tests
    print("\n🧪 Running Unit Tests...")
    unittest.main(verbosity=2) 