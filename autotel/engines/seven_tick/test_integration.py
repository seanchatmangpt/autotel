#!/usr/bin/env python3
"""
Integration tests for Seven-Tick Engine connectivity
Demonstrates cross-engine workflows and validates performance
"""

import unittest
import asyncio
import time
from typing import List, Dict
import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Import engines
from seven_tick import SevenTickEngine, SevenTickError, PerformanceError
from pm7t import PM7T

class TestSevenTickIntegration(unittest.TestCase):
    """Test cases for integrated 7-tick engine operations."""
    
    @classmethod
    def setUpClass(cls):
        """Set up test fixtures."""
        cls.engine = SevenTickEngine(enable_async=True, max_workers=4)
        
        # Sample event data for testing
        cls.sample_events = [
            # Case 1: Normal flow
            {"case_id": 1, "activity": "Order Received", "timestamp": 1000000000, "resource": "System", "cost": 0},
            {"case_id": 1, "activity": "Check Stock", "timestamp": 1000001000, "resource": "John", "cost": 10},
            {"case_id": 1, "activity": "Pack Items", "timestamp": 1000002000, "resource": "Mary", "cost": 20},
            {"case_id": 1, "activity": "Ship Order", "timestamp": 1000003000, "resource": "Logistics", "cost": 50},
            {"case_id": 1, "activity": "Order Complete", "timestamp": 1000004000, "resource": "System", "cost": 0},
            
            # Case 2: Express flow
            {"case_id": 2, "activity": "Order Received", "timestamp": 1000010000, "resource": "System", "cost": 0},
            {"case_id": 2, "activity": "Check Stock", "timestamp": 1000011000, "resource": "John", "cost": 10},
            {"case_id": 2, "activity": "Express Pack", "timestamp": 1000012000, "resource": "Express Team", "cost": 40},
            {"case_id": 2, "activity": "Express Ship", "timestamp": 1000013000, "resource": "Express Logistics", "cost": 100},
            {"case_id": 2, "activity": "Order Complete", "timestamp": 1000014000, "resource": "System", "cost": 0},
            
            # Case 3: Problem flow
            {"case_id": 3, "activity": "Order Received", "timestamp": 1000020000, "resource": "System", "cost": 0},
            {"case_id": 3, "activity": "Check Stock", "timestamp": 1000021000, "resource": "Mike", "cost": 10},
            {"case_id": 3, "activity": "Stock Issue", "timestamp": 1000022000, "resource": "System", "cost": 0},
            {"case_id": 3, "activity": "Reorder Stock", "timestamp": 1000023000, "resource": "Purchasing", "cost": 200},
            {"case_id": 3, "activity": "Pack Items", "timestamp": 1000024000, "resource": "Mary", "cost": 20},
            {"case_id": 3, "activity": "Ship Order", "timestamp": 1000025000, "resource": "Logistics", "cost": 50},
            {"case_id": 3, "activity": "Order Complete", "timestamp": 1000026000, "resource": "System", "cost": 0},
        ]
        
        # Sample SHACL validation rules
        cls.validation_rules = {
            "shapes": [
                {
                    "id": "OrderProcessShape",
                    "targetClass": "OrderProcess",
                    "properties": [
                        {
                            "path": "hasActivity",
                            "name": "Order must have required activities",
                            "minCount": 3,
                            "in": ["Order Received", "Check Stock", "Order Complete"]
                        },
                        {
                            "path": "avgDuration",
                            "name": "Average duration should be reasonable",
                            "maxValue": 10000000  # 10 seconds in nanoseconds
                        }
                    ]
                },
                {
                    "id": "ActivityShape",
                    "targetClass": "Activity",
                    "properties": [
                        {
                            "path": "hasFrequency",
                            "name": "Activity must occur at least once",
                            "minValue": 1
                        }
                    ]
                }
            ],
            "constraints": [
                {
                    "type": "sequence",
                    "activities": ["Order Received", "Order Complete"],
                    "message": "Order must start with receipt and end with completion"
                }
            ]
        }
    
    def setUp(self):
        """Reset engine state before each test."""
        self.engine.clear_all()
    
    def test_process_mining_basic(self):
        """Test basic process mining functionality."""
        # Mine process from events
        model = self.engine.mine_process(self.sample_events)
        
        # Verify model structure
        self.assertIn("transitions", model)
        self.assertIn("statistics", model)
        self.assertIn("performance", model)
        self.assertIn("event_count", model)
        
        # Check event count
        self.assertEqual(model["event_count"], len(self.sample_events))
        
        # Verify transitions exist
        self.assertGreater(len(model["transitions"]), 0)
        
        # Check statistics
        activities = model["statistics"]["activities"]
        self.assertGreater(len(activities), 0)
        
        # Verify required activities are present
        activity_names = [a["activity"] for a in activities]
        self.assertIn("Order Received", activity_names)
        self.assertIn("Order Complete", activity_names)
    
    def test_cross_engine_workflow(self):
        """Test cross-engine workflow integration."""
        # Run async workflow
        async def run_test():
            result = await self.engine.process_mining_to_validation(
                self.sample_events,
                self.validation_rules
            )
            return result
        
        # Execute workflow
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        result = loop.run_until_complete(run_test())
        loop.close()
        
        # Verify workflow results
        self.assertIn("model", result)
        self.assertIn("patterns", result)
        self.assertIn("validation", result)
        self.assertIn("workflow_stats", result)
        
        # Check model was created
        self.assertGreater(len(result["model"]["transitions"]), 0)
    
    def test_sparql_knowledge_management(self):
        """Test SPARQL knowledge storage and querying."""
        # Add knowledge about the process
        self.engine.add_knowledge("OrderProcess", "hasActivity", "Order Received")
        self.engine.add_knowledge("OrderProcess", "hasActivity", "Check Stock")
        self.engine.add_knowledge("OrderProcess", "hasActivity", "Ship Order")
        self.engine.add_knowledge("OrderProcess", "requiresResource", "System")
        self.engine.add_knowledge("OrderProcess", "requiresResource", "Logistics")
        
        # Query for activities (simplified test)
        query = """
        SELECT ?activity
        WHERE {
            OrderProcess hasActivity ?activity
        }
        """
        
        # Note: Actual SPARQL execution would require full implementation
        # For now, we test the interface
        results = self.engine.query(query)
        self.assertIsInstance(results, list)
    
    def test_mcts_optimization(self):
        """Test MCTS optimization for process improvement."""
        # Define optimization problem
        problem = {
            "type": "process_optimization",
            "initial": {
                "avg_duration": 5000000,  # 5ms
                "bottlenecks": ["Check Stock", "Pack Items"]
            },
            "goal": {
                "avg_duration": 2000000,  # 2ms target
                "bottlenecks": []
            },
            "actions": [
                {"name": "parallelize", "target": "Check Stock"},
                {"name": "automate", "target": "Pack Items"},
                {"name": "add_resource", "target": "any"}
            ]
        }
        
        constraints = [
            {"type": "budget", "max_cost": 1000},
            {"type": "quality", "min_accuracy": 0.95}
        ]
        
        # Run optimization
        result = self.engine.optimize_with_mcts(problem, constraints)
        
        # Verify optimization results
        self.assertIn("best_solution", result)
        self.assertIn("confidence", result)
        self.assertIn("iterations", result)
        
        # Check that optimization ran
        self.assertGreater(result["iterations"], 0)
    
    def test_parallel_execution(self):
        """Test parallel execution of multiple tasks."""
        # Define parallel tasks
        tasks = [
            (self.engine.mine_process, (self.sample_events[:5],), {}),
            (self.engine.mine_process, (self.sample_events[5:10],), {}),
            (self.engine.mine_process, (self.sample_events[10:],), {}),
        ]
        
        # Execute in parallel
        start_time = time.time()
        results = self.engine.execute_parallel(tasks)
        end_time = time.time()
        
        # Verify results
        self.assertEqual(len(results), 3)
        for result in results:
            self.assertIn("transitions", result)
            self.assertIn("event_count", result)
        
        # Parallel execution should be faster than sequential
        # (though this is hard to test reliably in unit tests)
        print(f"Parallel execution time: {end_time - start_time:.4f}s")
    
    def test_workflow_creation_and_execution(self):
        """Test workflow creation and execution."""
        # Create a workflow
        workflow_steps = [
            {
                "name": "mine_process",
                "function": "mine_process",
                "args": ["$events"]
            },
            {
                "name": "validate_model",
                "function": "validate",
                "args": ["$mine_process", "$rules"]
            }
        ]
        
        workflow_id = self.engine.create_workflow("test_workflow", workflow_steps)
        self.assertIsNotNone(workflow_id)
        
        # Execute workflow
        async def run_workflow():
            inputs = {
                "events": self.sample_events,
                "rules": self.validation_rules
            }
            return await self.engine.execute_workflow(workflow_id, inputs)
        
        # Run async workflow
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        results = loop.run_until_complete(run_workflow())
        loop.close()
        
        # Verify workflow execution
        self.assertIn("mine_process", results)
        self.assertIn("validate_model", results)
    
    def test_performance_tracking(self):
        """Test performance tracking and statistics."""
        # Perform several operations
        self.engine.mine_process(self.sample_events[:5])
        self.engine.mine_process(self.sample_events[5:10])
        self.engine.add_knowledge("Test", "hasValue", "123")
        
        # Get performance stats
        stats = self.engine.get_performance_stats()
        
        # Verify stats structure
        self.assertIn("total_operations", stats)
        self.assertIn("average_ticks", stats)
        self.assertIn("engines", stats)
        self.assertIn("memory_usage", stats)
        
        # Check operations were tracked
        self.assertGreater(stats["total_operations"], 0)
        
        # Verify engine-specific stats
        self.assertIn("pm7t", stats["engines"])
        self.assertGreater(stats["engines"]["pm7t"]["events_added"], 0)
    
    def test_error_handling(self):
        """Test error handling across engines."""
        # Test with invalid data
        with self.assertRaises(Exception):
            self.engine.mine_process([{"invalid": "data"}])
        
        # Test with empty events
        model = self.engine.mine_process([])
        self.assertEqual(model["event_count"], 0)
    
    def test_memory_management(self):
        """Test shared memory allocation and transfer."""
        # Allocate shared memory
        mem1 = self.engine._allocate_shared_memory("test_buffer", 1024)
        self.assertEqual(len(mem1), 1024)
        
        # Verify same memory is returned
        mem2 = self.engine._allocate_shared_memory("test_buffer", 1024)
        self.assertTrue((mem1 == mem2).all())
        
        # Test data transfer (simplified)
        self.engine._cache["pm7t:test_data"] = {"value": 42}
        self.engine._transfer_data("pm7t", "sparql7t", "test_data")
        
        # Verify transfer
        self.assertIn("sparql7t:test_data", self.engine._cache)
        self.assertEqual(self.engine._cache["sparql7t:test_data"]["value"], 42)
    
    def test_process_conformance_checking(self):
        """Test conformance checking between model and log."""
        # First, discover a model
        model_events = self.sample_events[:10]  # Use subset for model
        test_events = self.sample_events[10:]   # Use rest for conformance
        
        # Mine initial model
        initial_model = self.engine.mine_process(model_events)
        
        # Create PM7T instance to get actual model object
        pm = PM7T()
        for event in model_events:
            pm.add_event(**event)
        model_obj = pm.discover_process()
        
        # Check conformance
        conformance = self.engine.check_process_conformance(model_obj, test_events)
        
        # Verify conformance metrics
        self.assertIn("fitness", conformance)
        self.assertIn("precision", conformance)
        self.assertIn("generalization", conformance)
        self.assertIn("simplicity", conformance)
        
        # Cleanup
        pm.clear()
    
    @classmethod
    def tearDownClass(cls):
        """Clean up test fixtures."""
        cls.engine.clear_all()


# Performance benchmark tests
class TestSevenTickPerformance(unittest.TestCase):
    """Performance benchmarks for 7-tick operations."""
    
    def setUp(self):
        """Set up performance test."""
        self.engine = SevenTickEngine()
        
        # Generate larger dataset for performance testing
        self.large_events = []
        activities = ["Start", "Process A", "Process B", "Process C", "End"]
        resources = ["Resource1", "Resource2", "Resource3"]
        
        for case_id in range(100):
            timestamp = 1000000000
            for i, activity in enumerate(activities):
                self.large_events.append({
                    "case_id": case_id,
                    "activity": activity,
                    "timestamp": timestamp + (i * 1000),
                    "resource": resources[i % len(resources)],
                    "cost": (i + 1) * 10
                })
    
    def test_process_mining_performance(self):
        """Benchmark process mining performance."""
        start_time = time.perf_counter()
        
        # Mine process from large dataset
        model = self.engine.mine_process(self.large_events)
        
        end_time = time.perf_counter()
        duration_ms = (end_time - start_time) * 1000
        
        print(f"\nProcess mining performance:")
        print(f"  Events processed: {len(self.large_events)}")
        print(f"  Duration: {duration_ms:.2f}ms")
        print(f"  Events/second: {len(self.large_events) / (duration_ms / 1000):.0f}")
        print(f"  Transitions found: {len(model['transitions'])}")
        
        # Performance assertion - should process at least 1000 events/second
        self.assertLess(duration_ms, len(self.large_events))  # < 1ms per event
    
    def test_parallel_scalability(self):
        """Test scalability with parallel execution."""
        # Split events into chunks
        chunk_size = len(self.large_events) // 4
        chunks = [
            self.large_events[i:i+chunk_size]
            for i in range(0, len(self.large_events), chunk_size)
        ]
        
        # Sequential execution
        start_time = time.perf_counter()
        sequential_results = []
        for chunk in chunks:
            sequential_results.append(self.engine.mine_process(chunk))
        sequential_time = time.perf_counter() - start_time
        
        # Clear engine
        self.engine.clear_all()
        
        # Parallel execution
        start_time = time.perf_counter()
        tasks = [(self.engine.mine_process, (chunk,), {}) for chunk in chunks]
        parallel_results = self.engine.execute_parallel(tasks)
        parallel_time = time.perf_counter() - start_time
        
        # Calculate speedup
        speedup = sequential_time / parallel_time
        
        print(f"\nParallel scalability:")
        print(f"  Sequential time: {sequential_time:.3f}s")
        print(f"  Parallel time: {parallel_time:.3f}s")
        print(f"  Speedup: {speedup:.2f}x")
        print(f"  Efficiency: {speedup / 4 * 100:.1f}%")
        
        # Should achieve at least 50% efficiency with 4 cores
        self.assertGreater(speedup, 2.0)
    
    def tearDown(self):
        """Clean up after performance test."""
        self.engine.clear_all()


if __name__ == "__main__":
    # Run tests
    unittest.main(verbosity=2)