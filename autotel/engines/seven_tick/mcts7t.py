"""
Monte Carlo Tree Search (MCTS) for 7-Tick Engine
High-performance MCTS implementation using existing 7-tick engine components.
"""

import ctypes
import os
import time
from pathlib import Path
from typing import List, Tuple, Optional, Dict, Any
import numpy as np

class MCTS7T:
    """
    Monte Carlo Tree Search implementation for the 7-Tick engine.
    
    This class provides a high-level interface to the MCTS algorithm,
    integrating with SPARQL, SHACL, and OWL components for optimal
    query planning and execution.
    """
    
    def __init__(self, 
                 max_iterations: int = 10000,
                 time_budget_ms: int = 100,
                 max_depth: int = 50,
                 simulation_depth: int = 20,
                 exploration_constant: float = 1.414,
                 enable_parallel: bool = False,
                 num_threads: int = 1):
        """
        Initialize MCTS7T engine.
        
        Args:
            max_iterations: Maximum MCTS iterations
            time_budget_ms: Time budget in milliseconds
            max_depth: Maximum tree depth
            simulation_depth: Simulation rollout depth
            exploration_constant: UCB exploration constant
            enable_parallel: Enable parallel MCTS
            num_threads: Number of parallel threads
        """
        
        # Load C library
        lib_path = Path(__file__).parent / "lib" / "libmcts7t.so"
        if not lib_path.exists():
            self._build_library()
            
        self.lib = ctypes.CDLL(str(lib_path))
        
        # Define C structures
        self._define_structures()
        
        # Initialize configuration
        config = self.MCTSConfig(
            max_iterations=max_iterations,
            time_budget_ns=time_budget_ms * 1000000,
            max_depth=max_depth,
            simulation_depth=simulation_depth,
            exploration_constant=exploration_constant,
            rng_seed=int(time.time()),
            enable_parallel=enable_parallel,
            num_threads=num_threads
        )
        
        # Create MCTS engine
        self.engine = self.lib.mcts7t_create(None, ctypes.byref(config))
        if not self.engine:
            raise RuntimeError("Failed to create MCTS engine")
        
        # Performance tracking
        self.stats = {}
        self.last_search_time = 0.0
        
    def _build_library(self):
        """Build the MCTS7T C library."""
        makefile_path = Path(__file__).parent / "Makefile"
        if makefile_path.exists():
            os.system(f"cd {Path(__file__).parent} && make libmcts7t.so")
        else:
            # Create a simple Makefile for building
            self._create_makefile()
            os.system(f"cd {Path(__file__).parent} && make libmcts7t.so")
    
    def _create_makefile(self):
        """Create a Makefile for building the MCTS7T library."""
        makefile_content = """
CC = gcc
CFLAGS = -O3 -march=native -fPIC -Wall -Wextra
LDFLAGS = -shared

SOURCES = mcts7t.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = libmcts7t.so

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
"""
        with open(Path(__file__).parent / "Makefile", "w") as f:
            f.write(makefile_content)
    
    def _define_structures(self):
        """Define C structures for Python interface."""
        
        class MCTSConfig(ctypes.Structure):
            _fields_ = [
                ("max_iterations", ctypes.c_uint32),
                ("time_budget_ns", ctypes.c_uint64),
                ("max_depth", ctypes.c_uint32),
                ("simulation_depth", ctypes.c_uint32),
                ("exploration_constant", ctypes.c_double),
                ("rng_seed", ctypes.c_uint64),
                ("enable_parallel", ctypes.c_bool),
                ("num_threads", ctypes.c_uint32)
            ]
        
        class MCTSResult(ctypes.Structure):
            _fields_ = [
                ("best_actions", ctypes.POINTER(ctypes.c_uint32)),
                ("action_count", ctypes.c_size_t),
                ("total_reward", ctypes.c_double),
                ("confidence", ctypes.c_double),
                ("computation_time_ns", ctypes.c_uint64),
                ("iterations_performed", ctypes.c_uint64)
            ]
        
        class MCTSStats(ctypes.Structure):
            _fields_ = [
                ("selection_time_ns", ctypes.c_uint64),
                ("expansion_time_ns", ctypes.c_uint64),
                ("simulation_time_ns", ctypes.c_uint64),
                ("backpropagation_time_ns", ctypes.c_uint64),
                ("total_time_ns", ctypes.c_uint64),
                ("nodes_created", ctypes.c_uint64),
                ("nodes_visited", ctypes.c_uint64),
                ("simulations_performed", ctypes.c_uint64)
            ]
        
        self.MCTSConfig = MCTSConfig
        self.MCTSResult = MCTSResult
        self.MCTSStats = MCTSStats
        
        # Define function signatures
        self.lib.mcts7t_create.argtypes = [ctypes.c_void_p, ctypes.POINTER(MCTSConfig)]
        self.lib.mcts7t_create.restype = ctypes.c_void_p
        
        self.lib.mcts7t_search.argtypes = [
            ctypes.c_void_p, 
            ctypes.POINTER(ctypes.c_uint32), 
            ctypes.c_size_t
        ]
        self.lib.mcts7t_search.restype = ctypes.POINTER(MCTSResult)
        
        self.lib.mcts7t_get_stats.argtypes = [ctypes.c_void_p]
        self.lib.mcts7t_get_stats.restype = ctypes.POINTER(MCTSStats)
        
        self.lib.mcts7t_destroy.argtypes = [ctypes.c_void_p]
    
    def search(self, 
               initial_state: List[int],
               query_patterns: Optional[List[Tuple[int, int, int]]] = None,
               shacl_shapes: Optional[List[Tuple[int, int]]] = None,
               owl_axioms: Optional[List[Tuple[int, int, int]]] = None) -> Dict[str, Any]:
        """
        Perform MCTS search for optimal action sequence.
        
        Args:
            initial_state: Initial state vector
            query_patterns: List of (subject, predicate, object) patterns
            shacl_shapes: List of (node_id, shape_id) validations
            owl_axioms: List of (type, entity1, entity2) axioms
            
        Returns:
            Dictionary containing search results
        """
        
        start_time = time.time()
        
        # Convert state to C array
        state_array = (ctypes.c_uint32 * len(initial_state))(*initial_state)
        
        # Perform MCTS search
        result_ptr = self.lib.mcts7t_search(self.engine, state_array, len(initial_state))
        
        if not result_ptr:
            raise RuntimeError("MCTS search failed")
        
        result = result_ptr.contents
        
        # Extract results
        best_actions = [result.best_actions[i] for i in range(result.action_count)]
        
        # Get statistics
        stats_ptr = self.lib.mcts7t_get_stats(self.engine)
        stats = stats_ptr.contents if stats_ptr else None
        
        # Store performance metrics
        self.last_search_time = time.time() - start_time
        self.stats = {
            'total_time_ns': stats.total_time_ns if stats else 0,
            'selection_time_ns': stats.selection_time_ns if stats else 0,
            'expansion_time_ns': stats.expansion_time_ns if stats else 0,
            'simulation_time_ns': stats.simulation_time_ns if stats else 0,
            'backpropagation_time_ns': stats.backpropagation_time_ns if stats else 0,
            'nodes_created': stats.nodes_created if stats else 0,
            'nodes_visited': stats.nodes_visited if stats else 0,
            'simulations_performed': stats.simulations_performed if stats else 0,
            'iterations_performed': result.iterations_performed,
            'python_time_s': self.last_search_time
        }
        
        return {
            'best_actions': best_actions,
            'total_reward': result.total_reward,
            'confidence': result.confidence,
            'computation_time_ns': result.computation_time_ns,
            'iterations_performed': result.iterations_performed,
            'stats': self.stats
        }
    
    def optimize_query(self, patterns: List[Tuple[int, int, int]]) -> List[int]:
        """
        Optimize SPARQL query execution order using MCTS.
        
        Args:
            patterns: List of (subject, predicate, object) patterns
            
        Returns:
            Optimized pattern execution order
        """
        
        # Create initial state representing query patterns
        initial_state = [len(patterns)] + [p for pattern in patterns for p in pattern]
        
        # Perform MCTS search
        result = self.search(initial_state, query_patterns=patterns)
        
        # Extract optimized order
        action_sequence = result['best_actions']
        
        # Convert action sequence to pattern order
        pattern_order = []
        for action_id in action_sequence:
            if action_id < len(patterns):
                pattern_order.append(action_id)
        
        return pattern_order
    
    def optimize_validation(self, shapes: List[Tuple[int, int]]) -> List[int]:
        """
        Optimize SHACL validation order using MCTS.
        
        Args:
            shapes: List of (node_id, shape_id) validations
            
        Returns:
            Optimized validation order
        """
        
        # Create initial state representing validation shapes
        initial_state = [len(shapes)] + [s for shape in shapes for s in shape]
        
        # Perform MCTS search
        result = self.search(initial_state, shacl_shapes=shapes)
        
        # Extract optimized order
        action_sequence = result['best_actions']
        
        # Convert action sequence to shape order
        shape_order = []
        for action_id in action_sequence:
            if action_id < len(shapes):
                shape_order.append(action_id)
        
        return shape_order
    
    def optimize_reasoning(self, axioms: List[Tuple[int, int, int]]) -> List[int]:
        """
        Optimize OWL reasoning order using MCTS.
        
        Args:
            axioms: List of (type, entity1, entity2) axioms
            
        Returns:
            Optimized reasoning order
        """
        
        # Create initial state representing reasoning axioms
        initial_state = [len(axioms)] + [a for axiom in axioms for a in axiom]
        
        # Perform MCTS search
        result = self.search(initial_state, owl_axioms=axioms)
        
        # Extract optimized order
        action_sequence = result['best_actions']
        
        # Convert action sequence to axiom order
        axiom_order = []
        for action_id in action_sequence:
            if action_id < len(axioms):
                axiom_order.append(action_id)
        
        return axiom_order
    
    def get_performance_stats(self) -> Dict[str, Any]:
        """
        Get performance statistics from the last search.
        
        Returns:
            Dictionary containing performance metrics
        """
        return self.stats.copy()
    
    def benchmark(self, 
                  num_patterns: int = 10,
                  num_iterations: int = 100) -> Dict[str, float]:
        """
        Benchmark MCTS performance with synthetic data.
        
        Args:
            num_patterns: Number of patterns to optimize
            num_iterations: Number of benchmark iterations
            
        Returns:
            Dictionary containing benchmark results
        """
        
        # Generate synthetic patterns
        patterns = [(i % 100, i % 50, i % 200) for i in range(num_patterns)]
        
        times = []
        rewards = []
        
        for _ in range(num_iterations):
            start_time = time.time()
            result = self.optimize_query(patterns)
            end_time = time.time()
            
            times.append(end_time - start_time)
            rewards.append(len(result))  # Simple reward metric
        
        return {
            'avg_time_ms': np.mean(times) * 1000,
            'min_time_ms': np.min(times) * 1000,
            'max_time_ms': np.max(times) * 1000,
            'std_time_ms': np.std(times) * 1000,
            'avg_reward': np.mean(rewards),
            'throughput_ops_per_sec': num_iterations / np.sum(times)
        }
    
    def __del__(self):
        """Clean up C resources."""
        if hasattr(self, 'engine') and self.engine:
            self.lib.mcts7t_destroy(self.engine)


# Example usage and testing
def demo_mcts7t():
    """Demonstrate MCTS7T functionality."""
    
    print("🚀 7-Tick MCTS Engine Demo")
    print("=" * 50)
    
    # Create MCTS engine
    mcts = MCTS7T(
        max_iterations=1000,
        time_budget_ms=50,
        max_depth=20,
        simulation_depth=10
    )
    
    # Test query optimization
    print("\n📊 Testing Query Optimization...")
    patterns = [
        (1, 10, 100),  # High selectivity
        (2, 20, 200),  # Medium selectivity  
        (3, 30, 300),  # Low selectivity
        (4, 40, 400),  # Very low selectivity
        (5, 50, 500)   # Medium selectivity
    ]
    
    optimized_order = mcts.optimize_query(patterns)
    print(f"Original order: {list(range(len(patterns)))}")
    print(f"Optimized order: {optimized_order}")
    
    # Test validation optimization
    print("\n✅ Testing Validation Optimization...")
    shapes = [
        (1, 10),  # Person validation
        (2, 20),  # Organization validation
        (3, 30),  # Document validation
        (4, 40),  # Event validation
        (5, 50)   # Location validation
    ]
    
    optimized_shapes = mcts.optimize_validation(shapes)
    print(f"Original order: {list(range(len(shapes)))}")
    print(f"Optimized order: {optimized_shapes}")
    
    # Performance benchmark
    print("\n⚡ Performance Benchmark...")
    benchmark_results = mcts.benchmark(num_patterns=8, num_iterations=50)
    
    for metric, value in benchmark_results.items():
        print(f"{metric}: {value:.3f}")
    
    # Get detailed stats
    print("\n📈 Detailed Statistics...")
    stats = mcts.get_performance_stats()
    for metric, value in stats.items():
        print(f"{metric}: {value}")
    
    print("\n✅ MCTS7T Demo Complete!")


if __name__ == "__main__":
    demo_mcts7t() 